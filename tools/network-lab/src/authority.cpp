#include "bot.hpp"
#include "net.hpp"
#include "roles.hpp"
#include "wire.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <queue>
#include <span>
#include <utility>
#include <variant>
#include <vector>

namespace neocity::lab {
namespace {

using survival::Actor;
using survival::ControlMap;
using survival::PlayerInput;

constexpr std::int64_t STEP_NS = 100'000'000;
constexpr std::int64_t IDLE_WAIT_NS = 20'000'000;
constexpr std::int64_t HELLO_EVERY_NS = 200'000'000;
constexpr std::int64_t NS_PER_US = 1'000;
constexpr std::int64_t NS_PER_MS = 1'000'000;
constexpr std::size_t MAX_PACKET = 65'535;
constexpr std::array ACTORS{Actor::Chef, Actor::Appraiser};
constexpr ControlMap AI_ONLY{.chef = survival::Controller::Ai,
                             .appraiser = survival::Controller::Ai};

std::size_t seat_index(Actor actor) { return actor == Actor::Chef ? 0U : 1U; }

struct Seat {
  bool local = false;
  bool joined = false;
  bool gone = false;
  Endpoint peer{};
  PlayerInput keys{};
  std::uint64_t seq = 0;
  std::int64_t heard_ns = 0;
};

struct HostedRoom {
  std::unique_ptr<Room> rules;
  std::array<Seat, 2> seats{};
  ControlMap control = survival::two_players();
  std::uint64_t tick = 0;
  bool running = false;
};

/// When a room next steps, and which room; earliest first.
using DueRoom = std::pair<std::int64_t, std::size_t>;

class Authority {
public:
  explicit Authority(const AuthorityOptions &settings)
      : options(settings), socket(settings.port),
        blank(Room(settings.seed, 1).view()) {
    rooms.reserve(options.rooms);
    for (std::uint32_t index = 0; index < options.rooms; ++index) {
      HostedRoom room;
      room.rules = std::make_unique<Room>(options.seed, 1);
      if (options.local) {
        room.seats.at(seat_index(*options.local)).local = true;
      }
      rooms.push_back(std::move(room));
    }
    brain = std::make_unique<survival::Companion>(
        rooms.at(0).rules->companion_config());
    if (!options.progress_path.empty()) {
      progress = open(options.progress_path.c_str(),
                      O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
    }
  }

  void run() {
    while (!stop_requested()) {
      const auto now = now_ns();
      const std::array descriptor{socket.descriptor()};
      wait_readable(descriptor, next_due(now) - now);
      drain();
      say_hello();
      tick_due_rooms();
    }
    say_goodbye();
    report();
  }

private:
  [[nodiscard]] std::int64_t next_due(std::int64_t now) const {
    const auto idle = now + IDLE_WAIT_NS;
    return due.empty() ? idle : std::min(idle, due.top().first);
  }

  void drain() {
    while (const auto packet = socket.receive(buffer)) {
      const auto message = parse(std::span(buffer).first(packet->first), blank);
      if (!message) {
        ++refused_packets;
        continue;
      }
      std::visit(
          [this, packet](const auto &body) { handle(body, packet->second); },
          *message);
    }
  }

  [[nodiscard]] HostedRoom *room_at(std::uint32_t id) {
    if (id < options.first_room || id - options.first_room >= rooms.size()) {
      return nullptr;
    }
    return &rooms.at(id - options.first_room);
  }

  void handle(const Join &join, const Endpoint &from) {
    auto *room = room_at(join.room);
    if (room == nullptr) {
      return;
    }
    auto &seat = room->seats.at(seat_index(join.actor));
    if (seat.local || seat.gone) {
      return;
    }
    seat.joined = true;
    seat.peer = from;
    seat.heard_ns = now_ns();
    start_when_seated(*room, join.room - options.first_room);
  }

  void handle(const Input &input, const Endpoint &from) {
    auto *room = room_at(input.room);
    if (room == nullptr) {
      return;
    }
    auto &seat = room->seats.at(seat_index(input.actor));
    if (!seat.joined || seat.gone || input.seq <= seat.seq) {
      return;
    }
    seat.keys = input.keys;
    seat.seq = input.seq;
    seat.peer = from;
    seat.heard_ns = now_ns();
  }

  void handle(const Bye &bye, const Endpoint & /*from*/) {
    if (auto *room = room_at(bye.room)) {
      depart(*room, bye.actor);
    }
  }

  void handle(const HostHello & /*hello*/, const Endpoint & /*from*/) {}
  void handle(const State & /*state*/, const Endpoint & /*from*/) {}

  void start_when_seated(HostedRoom &room, std::uint32_t index) {
    const bool seated = std::ranges::all_of(
        room.seats, [](const Seat &seat) { return seat.local || seat.joined; });
    if (seated && !room.running) {
      room.running = true;
      // Spreads the rooms across one step so their inputs do not arrive at
      // once.
      due.emplace(now_ns() + ((STEP_NS * index) / options.rooms), index);
    }
  }

  void depart(HostedRoom &room, Actor actor) {
    auto &seat = room.seats.at(seat_index(actor));
    if (seat.gone || seat.local) {
      return;
    }
    seat.gone = true;
    ++departures;
    departure_tick = room.tick;
    const auto view = room.rules->view();
    departed_actor = actor;
    carried_at_departure = carried(view, actor);
    elapsed_at_departure = view.world.progression.elapsed.count();
    if (survival::leave(room.control, actor) == AI_ONLY) {
      room.running = false;
      return;
    }
    room.control = survival::leave(room.control, actor);
  }

  /// Steps what is due now and no more, so an overloaded loop still drains its
  /// socket between batches.
  void tick_due_rooms() {
    const auto batch = now_ns();
    while (!due.empty() && due.top().first <= batch) {
      const auto [scheduled, index] = due.top();
      due.pop();
      auto &room = rooms.at(index);
      if (!room.running) {
        continue;
      }
      const auto started = now_ns();
      lateness.add((started - scheduled) / NS_PER_US);
      tick(room, static_cast<std::uint32_t>(index));
      work.add((now_ns() - started) / NS_PER_US);
      if (room.running) {
        due.emplace(scheduled + STEP_NS, index);
      }
    }
  }

  void tick(HostedRoom &room, std::uint32_t index) {
    for (const auto actor : ACTORS) {
      auto &seat = room.seats.at(seat_index(actor));
      if (seat.local) {
        seat.keys = bot_input(*brain, actor, room.rules->view());
      } else if (seat.joined && !seat.gone &&
                 now_ns() - seat.heard_ns > options.timeout_ms * NS_PER_MS) {
        depart(room, actor);
      }
    }
    if (!room.running) {
      return;
    }
    const StepInput applied{
        .inputs = {room.seats.at(0).keys, room.seats.at(1).keys},
        .control = room.control};
    const auto before = room.rules->view();
    room.rules->step(applied);
    ++room.tick;
    ++ticks;
    const auto after = room.rules->view();
    note_departure_effects(before, after);
    final_tick = room.tick;
    final_digest = digest(after, room.rules->escape());
    final_escaped = after.progression.phase == survival::Phase::Escaped;
    room.seats.at(0).keys = {};
    room.seats.at(1).keys = {};
    const auto packet =
        encode(State{.room = options.first_room + index,
                     .tick = room.tick,
                     .sent_ns = now_ns(),
                     .acked = {room.seats.at(0).seq, room.seats.at(1).seq},
                     .applied = applied,
                     .escape = room.rules->escape(),
                     .view = after});
    state_min = std::min(state_min, packet.size());
    state_max = std::max(state_max, packet.size());
    if (progress >= 0 && index == 0) {
      (void)pwrite(progress, &room.tick, sizeof(room.tick), 0);
    }
    for (const auto &seat : room.seats) {
      if (!seat.local && !seat.gone) {
        socket.send(seat.peer, packet);
      }
    }
  }

  /// Records, for a one-room scenario, what the remaining side did next and
  /// whether the step after a departure kept what the departed actor held.
  void note_departure_effects(const survival::GameView &before,
                              const survival::GameView &after) {
    if (!departed_actor) {
      return;
    }
    if (!carried_next_step) {
      carried_next_step = carried(after, *departed_actor);
      elapsed_next_step = after.world.progression.elapsed.count();
    }
    const auto remaining =
        *departed_actor == Actor::Chef ? Actor::Appraiser : Actor::Chef;
    remaining_progress_steps += actor_progressed(before, after, remaining);
  }

  void say_hello() {
    if (!options.relay || now_ns() - hello_ns < HELLO_EVERY_NS) {
      return;
    }
    hello_ns = now_ns();
    const auto relay = loopback(*options.relay);
    for (std::uint32_t index = 0; index < rooms.size(); ++index) {
      if (!rooms.at(index).running) {
        socket.send(relay,
                    encode(HostHello{.room = options.first_room + index}));
      }
    }
  }

  void say_goodbye() {
    for (std::uint32_t index = 0; index < rooms.size(); ++index) {
      const auto packet = encode(Bye{.room = options.first_room + index});
      for (const auto &seat : rooms.at(index).seats) {
        if (seat.joined && !seat.gone) {
          socket.send(seat.peer, packet);
        }
      }
    }
  }

  void report() {
    Report out;
    out.text("role", options.local ? "host" : "server");
    out.number("rooms", options.rooms);
    out.number("ticks", ticks);
    out.number("departures", departures);
    out.number("departure_tick", departure_tick);
    out.number("carried_at_departure", carried_at_departure);
    out.number("carried_next_step", carried_next_step.value_or(0));
    out.signed_number("elapsed_at_departure_us", elapsed_at_departure);
    out.signed_number("elapsed_next_step_us", elapsed_next_step);
    out.number("remaining_progress_steps", remaining_progress_steps);
    out.number("final_tick", final_tick);
    out.number("final_digest", final_digest);
    out.flag("final_escaped", final_escaped);
    out.number("refused_packets", refused_packets);
    out.number("state_min_bytes", state_min == SIZE_MAX ? 0 : state_min);
    out.number("state_max_bytes", state_max);
    out.traffic(socket.traffic());
    out.object("tick_lateness_us", lateness.json());
    out.object("tick_work_us", work.json());
    out.print();
  }

  AuthorityOptions options;
  Socket socket;
  survival::GameView blank;
  std::vector<HostedRoom> rooms;
  std::priority_queue<DueRoom, std::vector<DueRoom>, std::greater<>> due;
  std::vector<std::uint8_t> buffer = std::vector<std::uint8_t>(MAX_PACKET);
  std::unique_ptr<survival::Companion> brain;
  int progress = -1;
  std::int64_t hello_ns = 0;
  std::uint64_t ticks = 0;
  std::uint64_t departures = 0;
  std::uint64_t departure_tick = 0;
  std::optional<Actor> departed_actor;
  std::size_t carried_at_departure = 0;
  std::optional<std::size_t> carried_next_step;
  std::int64_t elapsed_at_departure = 0;
  std::int64_t elapsed_next_step = 0;
  std::uint64_t remaining_progress_steps = 0;
  std::uint64_t final_tick = 0;
  std::uint64_t final_digest = 0;
  bool final_escaped = false;
  std::uint64_t refused_packets = 0;
  std::size_t state_min = SIZE_MAX;
  std::size_t state_max = 0;
  Samples lateness;
  Samples work;
};

} // namespace

void run_authority(const AuthorityOptions &options) {
  Authority authority(options);
  authority.run();
}

} // namespace neocity::lab
