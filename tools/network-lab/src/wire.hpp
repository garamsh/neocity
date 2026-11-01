#pragma once

#include "room.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <variant>
#include <vector>

namespace neocity::lab {

/// Asks an authority for an actor's seat; repeated until the first state.
struct Join {
  std::uint32_t room = 0;
  survival::Actor actor = survival::Actor::Chef;
};

/// Registers a player host with a relay; repeated until a guest joins.
struct HostHello {
  std::uint32_t room = 0;
};

/// One step's key state. `seq` returns in `State::acked`.
struct Input {
  std::uint32_t room = 0;
  survival::Actor actor = survival::Actor::Chef;
  std::uint64_t seq = 0;
  survival::PlayerInput keys{};
};

/// The whole mutable game state after one step, so a lost packet needs no
/// retransmission. `applied` is what a guest replays to succeed a host.
struct State {
  std::uint32_t room = 0;
  std::uint64_t tick = 0;
  std::int64_t sent_ns = 0;
  std::array<std::uint64_t, 2> acked{};
  StepInput applied{};
  survival::EscapeSnapshot escape{};
  survival::GameView view{};
};

/// A deliberate departure: of the authority when it sends this, else of
/// `actor`.
struct Bye {
  std::uint32_t room = 0;
  survival::Actor actor = survival::Actor::Chef;
};

using Message = std::variant<Join, HostHello, Input, State, Bye>;

/// Bytes of the kind and room header, which a relay reads to route a packet.
inline constexpr std::size_t HEADER_SIZE = 5;

[[nodiscard]] std::vector<std::uint8_t> encode(const Message &message);

/// Reads a packet from the network. Immutable state comes from `blank`, a view
/// of the same content, so only what the rules can change travels. Returns
/// nothing for anything malformed.
[[nodiscard]] std::optional<Message> parse(std::span<const std::uint8_t> packet,
                                           const survival::GameView &blank);

/// A hash of everything a state carries about the game, for comparing what two
/// processes hold.
[[nodiscard]] std::uint64_t digest(const survival::GameView &view,
                                   survival::EscapeSnapshot escape);

/// The room of a packet long enough to carry one.
[[nodiscard]] std::optional<std::uint32_t>
room_of(std::span<const std::uint8_t> packet);

/// Whether a relay should bind the sender as this room's host.
[[nodiscard]] bool is_host_hello(std::span<const std::uint8_t> packet);

} // namespace neocity::lab
