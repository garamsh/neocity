#pragma once

#include <survival/simulation.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace neocity::lab {

/// A dedicated server when `local` is empty; a player host of that actor
/// otherwise.
struct AuthorityOptions {
  std::uint32_t seed = 7;
  std::uint16_t port = 0;
  std::uint32_t first_room = 0;
  std::uint32_t rooms = 1;
  std::optional<survival::Actor> local;
  /// Reaches guests through this relay port instead of direct packets.
  std::optional<std::uint16_t> relay;
  std::int64_t timeout_ms = 1000;
  /// Holds room 0's last computed tick, which a SIGKILL cannot report.
  std::string progress_path;
};

/// Plays one seat per room in `actors`, starting at `first_room`.
struct ClientOptions {
  std::uint32_t seed = 7;
  std::uint16_t port = 0;
  std::uint32_t first_room = 0;
  std::uint32_t rooms = 1;
  std::vector<survival::Actor> actors;
  std::int64_t timeout_ms = 1000;
  /// Replays the log and plays on alone for this many steps when the
  /// authority is gone.
  std::optional<std::uint32_t> succeed_steps;
};

void run_authority(const AuthorityOptions &options);
void run_client(const ClientOptions &options);
void run_relay(std::uint16_t port);

} // namespace neocity::lab
