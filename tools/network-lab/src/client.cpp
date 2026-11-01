#include "bot.hpp"
#include "net.hpp"
#include "roles.hpp"
#include "wire.hpp"

#include <unistd.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace neocity::lab {
namespace {

using survival::Actor;

constexpr std::int64_t POLL_NS = 20'000'000;
constexpr std::int64_t JOIN_EVERY_NS = 200'000'000;
constexpr std::int64_t NS_PER_US = 1'000;
constexpr std::int64_t NS_PER_MS = 1'000'000;
constexpr useconds_t STEP_US = 100'000;
constexpr std::size_t MAX_PACKET = 65'535;
constexpr std::size_t SENT_WINDOW = 256;

Actor other(Actor actor) {
  return actor == Actor::Chef ? Actor::Appraiser : Actor::Chef;
}

std::size_t seat_index(Actor actor) { return actor == Actor::Chef ? 0U : 1U; }

enum class Ending : std::uint8_t { Playing, Bye, Silence };

const char *ending_name(Ending ending) {
  switch (ending) {
  case Ending::Playing:
    return "playing";
  case Ending::Bye:
    return "bye";
  case Ending::Silence:
    return "silence";
  }
  std::abort();
}

/// One seat of one room, with its own socket as a real client has.
struct Player {
  Player(std::uint32_t room_index, Actor seat)
      : socket(0), room(room_index), actor(seat) {}

  Socket socket;
  std::uint32_t room;
  Actor actor;
  std::optional<survival::GameView> latest;
  survival::EscapeSnapshot latest_escape{};
  StepLog log;
  std::uint64_t tick = 0;
  std::uint64_t seq = 0;
  std::uint64_t acked = 0;
  std::array<std::int64_t, SENT_WINDOW> sent_ns{};
  std::int64_t heard_ns = 0;
  std::int64_t join_ns = 0;
  std::int64_t max_gap_ns = 0;
  Ending ending = Ending::Playing;
  std::int64_t ended_ns = 0;
};

class Client {
public:
  explicit Client(const ClientOptions &settings)
      : options(settings), target(loopback(settings.port)),
        blank_room(settings.seed, 1), blank(blank_room.view()),
        brain(blank_room.companion_config()) {
    for (std::uint32_t index = 0; index < options.rooms; ++index) {
      for (const auto actor : options.actors) {
        players.emplace_back(options.first_room + index, actor);
        descriptors.push_back(players.back().socket.descriptor());
      }
    }
  }

  void run() {
    while (!stop_requested() && !all_ended()) {
      wait_readable(descriptors, POLL_NS);
      const auto now = now_ns();
      for (auto &player : players) {
        if (player.ending == Ending::Playing) {
          serve(player, now);
        }
      }
    }
    if (stop_requested()) {
      leave_all();
    }
    report();
  }

private:
  [[nodiscard]] bool all_ended() const {
    return std::ranges::all_of(players, [](const Player &player) {
      return player.ending != Ending::Playing;
    });
  }

  void serve(Player &player, std::int64_t now) {
    while (const auto packet = player.socket.receive(buffer)) {
      const auto message = parse(std::span(buffer).first(packet->first), blank);
      if (!message) {
        ++refused_packets;
      } else if (const auto *state = std::get_if<State>(&*message)) {
        receive(player, *state);
      } else if (std::holds_alternative<Bye>(*message)) {
        end(player, Ending::Bye);
        return;
      }
    }
    if (player.tick == 0 && now - player.join_ns >= JOIN_EVERY_NS) {
      player.join_ns = now;
      player.socket.send(
          target, encode(Join{.room = player.room, .actor = player.actor}));
    }
    if (player.tick > 0 &&
        now - player.heard_ns > options.timeout_ms * NS_PER_MS) {
      end(player, Ending::Silence);
    }
  }

  void receive(Player &player, const State &state) {
    const auto now = now_ns();
    if (state.tick <= player.tick) {
      return;
    }
    if (state.tick != player.tick + 1) {
      // A room's first states can be lost to the burst of joins at start.
      skipped_ticks += player.tick > 0 ? 1U : 0U;
    }
    if (player.tick > 0) {
      player.max_gap_ns = std::max(player.max_gap_ns, now - player.heard_ns);
    }
    player.heard_ns = now;
    player.tick = state.tick;
    player.log.record(state.tick, state.applied);
    state_latency.add((now - state.sent_ns) / NS_PER_US);
    const auto acked = state.acked.at(seat_index(player.actor));
    if (acked > player.acked) {
      player.acked = acked;
      input_round_trip.add((now - player.sent_ns.at(acked % SENT_WINDOW)) /
                           NS_PER_US);
    }
    player.latest = state.view;
    player.latest_escape = state.escape;
    ++player.seq;
    player.sent_ns.at(player.seq % SENT_WINDOW) = now_ns();
    player.socket.send(
        target,
        encode(Input{.room = player.room,
                     .actor = player.actor,
                     .seq = player.seq,
                     .keys = bot_input(brain, player.actor, state.view)}));
  }

  void end(Player &player, Ending ending) {
    player.ending = ending;
    player.ended_ns = now_ns();
    if (options.succeed_steps) {
      succeed(player);
    }
  }

  /// Rebuilds the room from the seed and the log, then plays on with AI in the
  /// departed actor's seat.
  void succeed(const Player &player) {
    Report out;
    out.text("role", "succession");
    const auto started = now_ns();
    const auto room = replay(player.log, options.seed);
    if (!room || !player.latest) {
      out.flag("replayed", false);
      out.print();
      return;
    }
    const auto replayed = now_ns();
    const bool same = same_view(room->view(), *player.latest) &&
                      room->escape() == player.latest_escape;
    const auto control =
        survival::leave(player.log.steps().back().control, other(player.actor));
    auto before = room->view();
    std::uint32_t progress = 0;
    std::int64_t first_step_ns = 0;
    for (std::uint32_t step = 0; step < *options.succeed_steps; ++step) {
      StepInput next{.inputs = {}, .control = control};
      next.inputs.at(seat_index(player.actor)) =
          bot_input(brain, player.actor, before);
      room->step(next);
      if (step == 0) {
        first_step_ns = now_ns();
      }
      auto after = room->view();
      progress += actor_progressed(before, after, player.actor) ? 1U : 0U;
      before = std::move(after);
      (void)usleep(STEP_US);
    }
    out.flag("replayed", true);
    out.flag("state_matches_last_received", same);
    out.number("log_steps", player.log.steps().size());
    out.signed_number("replay_us", (replayed - started) / NS_PER_US);
    out.signed_number("detect_us",
                      (player.ended_ns - player.heard_ns) / NS_PER_US);
    out.signed_number("interruption_us",
                      (first_step_ns - player.heard_ns) / NS_PER_US);
    out.number("steps_after", *options.succeed_steps);
    out.number("steps_where_the_player_progressed", progress);
    out.number("day_after", before.progression.day);
    out.print();
  }

  void leave_all() {
    for (auto &player : players) {
      if (player.ending == Ending::Playing) {
        player.socket.send(
            target, encode(Bye{.room = player.room, .actor = player.actor}));
      }
    }
  }

  void report() {
    Report out;
    Traffic traffic;
    std::int64_t max_gap = 0;
    std::uint64_t last_tick = 0;
    std::int64_t detect = 0;
    for (const auto &player : players) {
      traffic.add(player.socket.traffic());
      max_gap = std::max(max_gap, player.max_gap_ns);
      last_tick = std::max(last_tick, player.tick);
      if (player.ending != Ending::Playing) {
        detect = std::max(detect, player.ended_ns - player.heard_ns);
      }
    }
    out.text("role", "client");
    out.text("ending", ending_name(players.front().ending));
    out.number("players", players.size());
    out.number("last_tick", last_tick);
    const auto &first = players.front();
    out.number("last_digest",
               first.latest ? digest(*first.latest, first.latest_escape) : 0);
    out.number("skipped_ticks", skipped_ticks);
    out.number("refused_packets", refused_packets);
    out.signed_number("max_gap_us", max_gap / NS_PER_US);
    out.signed_number("detect_us", detect / NS_PER_US);
    out.traffic(traffic);
    out.object("state_latency_us", state_latency.json());
    out.object("input_round_trip_us", input_round_trip.json());
    out.print();
  }

  ClientOptions options;
  Endpoint target;
  Room blank_room;
  survival::GameView blank;
  survival::Companion brain;
  std::vector<Player> players;
  std::vector<int> descriptors;
  std::vector<std::uint8_t> buffer = std::vector<std::uint8_t>(MAX_PACKET);
  std::uint64_t skipped_ticks = 0;
  std::uint64_t refused_packets = 0;
  Samples state_latency;
  Samples input_round_trip;
};

} // namespace

void run_client(const ClientOptions &options) {
  Client client(options);
  client.run();
}

} // namespace neocity::lab
