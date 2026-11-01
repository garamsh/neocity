#include "net.hpp"

#include <arpa/inet.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <csignal>
#include <ctime>
#include <iostream>
#include <system_error>

namespace neocity::lab {
namespace {

constexpr std::int64_t NS_PER_SECOND = 1'000'000'000;
constexpr int SOCKET_BUFFER = 8 * 1024 * 1024;

volatile std::sig_atomic_t stop_flag = 0;

extern "C" void note_stop(int /*signal*/) { stop_flag = 1; }

[[noreturn]] void fail(const char *what) {
  throw std::system_error(errno, std::generic_category(), what);
}

sockaddr *as_generic(sockaddr_in &address) {
  return reinterpret_cast<sockaddr *>(&address);
}

const sockaddr *as_generic(const sockaddr_in &address) {
  return reinterpret_cast<const sockaddr *>(&address);
}

} // namespace

bool Endpoint::same_as(const Endpoint &other) const {
  return address.sin_port == other.address.sin_port &&
         address.sin_addr.s_addr == other.address.sin_addr.s_addr;
}

Endpoint loopback(std::uint16_t port) {
  Endpoint endpoint;
  endpoint.address.sin_family = AF_INET;
  endpoint.address.sin_port = htons(port);
  endpoint.address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  return endpoint;
}

void Traffic::add(const Traffic &other) {
  sent_packets += other.sent_packets;
  sent_bytes += other.sent_bytes;
  received_packets += other.received_packets;
  received_bytes += other.received_bytes;
  send_failures += other.send_failures;
}

Socket::Socket(std::uint16_t port)
    : fd(socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0)) {
  if (fd < 0) {
    fail("socket");
  }
  // The kernel caps this at rmem_max; run.py reads drops from /proc/net/snmp.
  (void)setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &SOCKET_BUFFER,
                   sizeof(SOCKET_BUFFER));
  const auto local = loopback(port);
  if (bind(fd, as_generic(local.address), sizeof(local.address)) != 0) {
    fail("bind");
  }
}

Socket::~Socket() {
  if (fd >= 0) {
    (void)close(fd);
  }
}

Socket::Socket(Socket &&other) noexcept
    : fd(std::exchange(other.fd, -1)), counted(other.counted) {}

void Socket::send(const Endpoint &to, std::span<const std::uint8_t> packet) {
  const auto written = sendto(fd, packet.data(), packet.size(), 0,
                              as_generic(to.address), sizeof(to.address));
  if (written < 0) {
    ++counted.send_failures;
    return;
  }
  ++counted.sent_packets;
  counted.sent_bytes += packet.size();
}

std::optional<std::pair<std::size_t, Endpoint>>
Socket::receive(std::span<std::uint8_t> buffer) {
  Endpoint from;
  socklen_t length = sizeof(from.address);
  const auto read = recvfrom(fd, buffer.data(), buffer.size(), 0,
                             as_generic(from.address), &length);
  if (read < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
      return std::nullopt;
    }
    fail("recvfrom");
  }
  const auto size = static_cast<std::size_t>(read);
  ++counted.received_packets;
  counted.received_bytes += size;
  return std::pair{size, from};
}

std::int64_t now_ns() {
  timespec time{};
  if (clock_gettime(CLOCK_MONOTONIC, &time) != 0) {
    fail("clock_gettime");
  }
  return (static_cast<std::int64_t>(time.tv_sec) * NS_PER_SECOND) +
         time.tv_nsec;
}

void wait_readable(std::span<const int> descriptors, std::int64_t timeout_ns) {
  std::vector<pollfd> waiting;
  waiting.reserve(descriptors.size());
  for (const auto descriptor : descriptors) {
    waiting.push_back({.fd = descriptor, .events = POLLIN, .revents = 0});
  }
  const auto bounded = std::max<std::int64_t>(timeout_ns, 0);
  const timespec timeout{.tv_sec = bounded / NS_PER_SECOND,
                         .tv_nsec = bounded % NS_PER_SECOND};
  (void)ppoll(waiting.data(), waiting.size(), &timeout, nullptr);
}

void watch_stop_signals() {
  (void)std::signal(SIGTERM, note_stop);
  (void)std::signal(SIGINT, note_stop);
}

bool stop_requested() { return stop_flag != 0; }

std::string Samples::json() {
  std::ranges::sort(values);
  const auto at = [this](double share) -> std::int64_t {
    if (values.empty()) {
      return 0;
    }
    const auto last = static_cast<double>(values.size() - 1);
    return values.at(static_cast<std::size_t>(share * last));
  };
  return R"({"count":)" + std::to_string(values.size()) + R"(,"p50":)" +
         std::to_string(at(0.50)) + R"(,"p95":)" + std::to_string(at(0.95)) +
         R"(,"p99":)" + std::to_string(at(0.99)) + R"(,"max":)" +
         std::to_string(at(1.0)) + "}";
}

void Report::object(const std::string &key, const std::string &json) {
  body += (body.empty() ? "{\"" : ",\"") + key + "\":" + json;
}

void Report::number(const std::string &key, std::uint64_t value) {
  object(key, std::to_string(value));
}

void Report::signed_number(const std::string &key, std::int64_t value) {
  object(key, std::to_string(value));
}

void Report::flag(const std::string &key, bool value) {
  object(key, value ? "true" : "false");
}

void Report::text(const std::string &key, const std::string &value) {
  object(key, "\"" + value + "\"");
}

void Report::traffic(const Traffic &counted) {
  number("sent_packets", counted.sent_packets);
  number("sent_bytes", counted.sent_bytes);
  number("received_packets", counted.received_packets);
  number("received_bytes", counted.received_bytes);
  number("send_failures", counted.send_failures);
}

void Report::print() const { std::cout << body << "}" << std::endl; }

} // namespace neocity::lab
