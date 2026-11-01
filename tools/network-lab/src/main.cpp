#include "bot.hpp"
#include "net.hpp"
#include "roles.hpp"
#include "wire.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace neocity::lab {
namespace {

using survival::Actor;

constexpr std::int64_t NS_PER_US = 1'000;

/// Command-line `--key value` pairs after the role.
class Arguments {
public:
  Arguments(int count, char **values) {
    const std::vector<std::string> words(values + 2, values + count);
    if (words.size() % 2 != 0) {
      throw std::invalid_argument("expected --key value pairs");
    }
    for (std::size_t index = 0; index < words.size(); index += 2) {
      pairs[words.at(index)] = words.at(index + 1);
    }
  }

  [[nodiscard]] bool has(const std::string &key) const {
    return pairs.contains(key);
  }

  [[nodiscard]] std::string text(const std::string &key,
                                 const std::string &otherwise) const {
    const auto found = pairs.find(key);
    return found == pairs.end() ? otherwise : found->second;
  }

  [[nodiscard]] std::uint32_t number(const std::string &key,
                                     std::uint32_t otherwise) const {
    const auto found = pairs.find(key);
    return found == pairs.end()
               ? otherwise
               : static_cast<std::uint32_t>(std::stoul(found->second));
  }

  [[nodiscard]] std::uint16_t port(const std::string &key) const {
    const auto value = number(key, 0);
    if (value > UINT16_MAX) {
      throw std::invalid_argument(key + " is not a port");
    }
    return static_cast<std::uint16_t>(value);
  }

private:
  std::map<std::string, std::string> pairs;
};

Actor actor_named(const std::string &name) {
  if (name == "chef") {
    return Actor::Chef;
  }
  if (name == "appraiser") {
    return Actor::Appraiser;
  }
  throw std::invalid_argument("unknown actor " + name);
}

AuthorityOptions authority_options(const Arguments &arguments) {
  AuthorityOptions options{.seed = arguments.number("--seed", 7),
                           .port = arguments.port("--port"),
                           .first_room = arguments.number("--first-room", 0),
                           .rooms = arguments.number("--rooms", 1),
                           .local = std::nullopt,
                           .relay = std::nullopt,
                           .timeout_ms = arguments.number("--timeout-ms", 1000),
                           .progress_path = arguments.text("--progress", "")};
  if (arguments.has("--local")) {
    options.local = actor_named(arguments.text("--local", ""));
  }
  if (arguments.has("--relay")) {
    options.relay = arguments.port("--relay");
  }
  return options;
}

ClientOptions client_options(const Arguments &arguments) {
  ClientOptions options{.seed = arguments.number("--seed", 7),
                        .port = arguments.port("--port"),
                        .first_room = arguments.number("--first-room", 0),
                        .rooms = arguments.number("--rooms", 1),
                        .actors = {},
                        .timeout_ms = arguments.number("--timeout-ms", 1000),
                        .succeed_steps = std::nullopt};
  const auto actors = arguments.text("--actors", "chef,appraiser");
  if (actors == "chef,appraiser") {
    options.actors = {Actor::Chef, Actor::Appraiser};
  } else {
    options.actors = {actor_named(actors)};
  }
  if (arguments.has("--succeed-steps")) {
    options.succeed_steps = arguments.number("--succeed-steps", 0);
  }
  return options;
}

/// A step limit for runs a bot party cannot end; no seed in use reaches it.
constexpr std::uint64_t STEP_LIMIT = 200'000;

bool escaped(const Room &room) {
  return room.view().progression.phase == survival::Phase::Escaped;
}

std::int64_t mean_us(std::int64_t total_ns, std::uint64_t steps) {
  return total_ns / NS_PER_US / static_cast<std::int64_t>(steps);
}

/// Plays one seed's runs without sockets and reports the state packet's size
/// by day and the rule step's cost with AI seats and with key seats.
void run_sizes(const Arguments &arguments) {
  const auto seed = arguments.number("--seed", 7);
  Room ai_room(seed, 1);
  const StepInput ai_only{.inputs = {},
                          .control = {.chef = survival::Controller::Ai,
                                      .appraiser = survival::Controller::Ai}};
  std::map<std::uint32_t, std::size_t> largest;
  std::uint64_t ai_steps = 0;
  std::int64_t ai_ns = 0;
  for (; !escaped(ai_room) && ai_steps < STEP_LIMIT; ++ai_steps) {
    const auto started = now_ns();
    ai_room.step(ai_only);
    ai_ns += now_ns() - started;
    const auto view = ai_room.view();
    const auto size = encode(State{.tick = ai_steps,
                                   .applied = ai_only,
                                   .escape = ai_room.escape(),
                                   .view = view})
                          .size();
    auto &day = largest[view.progression.day];
    day = std::max(day, size);
  }

  Room key_room(seed, 1);
  const survival::Companion brain(key_room.companion_config());
  StepLog log;
  for (std::uint64_t tick = 1; !escaped(key_room) && tick < STEP_LIMIT;
       ++tick) {
    const auto view = key_room.view();
    const StepInput next{.inputs = {bot_input(brain, Actor::Chef, view),
                                    bot_input(brain, Actor::Appraiser, view)},
                         .control = survival::two_players()};
    key_room.step(next);
    log.record(tick, next);
  }
  const auto started = now_ns();
  const auto replayed = replay(log, seed);
  const auto key_ns = now_ns() - started;

  std::string days = "[";
  for (const auto &[day, bytes] : largest) {
    days += (days.size() > 1 ? ",[" : "[") + std::to_string(day) + "," +
            std::to_string(bytes) + "]";
  }
  Report out;
  out.text("role", "sizes");
  out.number("ai_steps_to_end", ai_steps);
  out.number("ai_end_day", ai_room.view().progression.day);
  out.flag("ai_run_escaped", escaped(ai_room));
  out.signed_number("mean_ai_seats_step_us", mean_us(ai_ns, ai_steps));
  out.number("key_steps_to_end", log.steps().size());
  out.number("key_end_day", key_room.view().progression.day);
  out.flag("key_run_escaped", escaped(key_room));
  out.signed_number("mean_key_seats_replay_step_us",
                    mean_us(key_ns, log.steps().size()));
  out.flag("key_replay_matches", same_view(replayed->view(), key_room.view()) &&
                                     replayed->escape() == key_room.escape());
  out.number("input_bytes_at_low_seq", encode(Input{}).size());
  out.object("largest_state_bytes_by_day", days + "]");
  out.print();
}

void run_role(int count, char **values) {
  if (count < 2) {
    throw std::invalid_argument("expected a role");
  }
  const std::string role = values[1];
  const Arguments arguments(count, values);
  watch_stop_signals();
  if (role == "server" || role == "host") {
    run_authority(authority_options(arguments));
  } else if (role == "client") {
    run_client(client_options(arguments));
  } else if (role == "relay") {
    run_relay(arguments.port("--port"));
  } else if (role == "sizes") {
    run_sizes(arguments);
  } else {
    throw std::invalid_argument("unknown role " + role);
  }
}

} // namespace
} // namespace neocity::lab

int main(int count, char **values) {
  try {
    neocity::lab::run_role(count, values);
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
