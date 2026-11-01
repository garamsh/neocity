#include "net.hpp"
#include "roles.hpp"
#include "wire.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <unordered_map>

namespace neocity::lab {
namespace {

constexpr std::int64_t IDLE_WAIT_NS = 20'000'000;
constexpr std::size_t MAX_PACKET = 65'535;

struct Route {
  std::optional<Endpoint> host;
  std::optional<Endpoint> guest;
};

/// Forwards one room's packets between its two ends without reading the game.
void forward(Socket &socket, Route &route, std::span<const std::uint8_t> packet,
             const Endpoint &from) {
  if (is_host_hello(packet)) {
    route.host = from;
    return;
  }
  if (route.host && route.host->same_as(from)) {
    if (route.guest) {
      socket.send(*route.guest, packet);
    }
    return;
  }
  route.guest = from;
  if (route.host) {
    socket.send(*route.host, packet);
  }
}

} // namespace

void run_relay(std::uint16_t port) {
  Socket socket(port);
  std::unordered_map<std::uint32_t, Route> routes;
  std::array<std::uint8_t, MAX_PACKET> buffer{};
  const std::array descriptor{socket.descriptor()};
  while (!stop_requested()) {
    wait_readable(descriptor, IDLE_WAIT_NS);
    while (const auto received = socket.receive(buffer)) {
      const auto packet = std::span(buffer).first(received->first);
      if (const auto room = room_of(packet)) {
        forward(socket, routes[*room], packet, received->second);
      }
    }
  }
  Report out;
  out.text("role", "relay");
  out.number("rooms", routes.size());
  out.traffic(socket.traffic());
  out.print();
}

} // namespace neocity::lab
