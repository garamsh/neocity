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

RunOptions run_options(const Arguments &arguments) {
  return {.seed = arguments.number("--seed", 7),
          .start_day = arguments.number("--start-day", 1)};
}

AuthorityOptions authority_options(const Arguments &arguments) {
  AuthorityOptions options{.run = run_options(arguments),
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
  ClientOptions options{.run = run_options(arguments),
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

/// Plays an AI-only run without sockets and reports the state packet's size by
/// day, the rule step's cost and how long the run takes to replay.
void run_sizes(const Arguments &arguments) {
  const auto run = run_options(arguments);
  const auto last_day = arguments.number("--last-day", 30);
  Room room(run.seed, 1);
  const StepInput ai_only{.inputs = {},
                          .control = {.chef = survival::Controller::Ai,
                                      .appraiser = survival::Controller::Ai}};
  std::map<std::uint32_t, std::size_t> largest;
  std::uint64_t ticks = 0;
  std::size_t last_size = 0;
  std::int64_t stepping_ns = 0;
  for (; room.view().progression.day < last_day; ++ticks) {
    const auto started = now_ns();
    room.step(ai_only);
    stepping_ns += now_ns() - started;
    const auto view = room.view();
    last_size =
        encode(
            State{.room = 0, .tick = ticks, .applied = ai_only, .view = view})
            .size();
    auto &day = largest[view.progression.day];
    day = std::max(day, last_size);
  }
  std::string days = "[";
  for (const auto &[day, bytes] : largest) {
    days += (days.size() > 1 ? ",[" : "[") + std::to_string(day) + "," +
            std::to_string(bytes) + "]";
  }
  Report out;
  out.text("role", "sizes");
  out.number("ticks_to_last_day", ticks);
  out.number("last_state_bytes", last_size);
  out.number("input_bytes", encode(Input{}).size());
  out.signed_number("mean_step_us",
                    stepping_ns / NS_PER_US / static_cast<std::int64_t>(ticks));
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
