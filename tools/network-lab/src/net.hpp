#pragma once

#include <netinet/in.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace neocity::lab {

/// An IPv4 peer. The lab binds and sends only on loopback.
struct Endpoint {
  sockaddr_in address{};
  [[nodiscard]] bool same_as(const Endpoint &other) const;
};

[[nodiscard]] Endpoint loopback(std::uint16_t port);

/// Application bytes and datagrams that crossed one socket.
struct Traffic {
  std::uint64_t sent_packets = 0;
  std::uint64_t sent_bytes = 0;
  std::uint64_t received_packets = 0;
  std::uint64_t received_bytes = 0;
  std::uint64_t send_failures = 0;
  void add(const Traffic &other);
};

/// A nonblocking UDP socket that counts what it carries.
class Socket {
public:
  /// Binds to loopback; port 0 lets the system choose. Throws on failure.
  explicit Socket(std::uint16_t port);
  ~Socket();
  Socket(const Socket &) = delete;
  Socket &operator=(const Socket &) = delete;
  Socket(Socket &&other) noexcept;
  Socket &operator=(Socket &&) = delete;

  void send(const Endpoint &to, std::span<const std::uint8_t> packet);
  /// Reads one waiting datagram into `buffer`, or nothing when none is waiting.
  [[nodiscard]] std::optional<std::pair<std::size_t, Endpoint>>
  receive(std::span<std::uint8_t> buffer);
  [[nodiscard]] int descriptor() const { return fd; }
  [[nodiscard]] const Traffic &traffic() const { return counted; }

private:
  int fd;
  Traffic counted;
};

/// Monotonic nanoseconds, comparable across processes on this machine.
[[nodiscard]] std::int64_t now_ns();

/// Sleeps until a descriptor is readable or `timeout_ns` passes.
void wait_readable(std::span<const int> descriptors, std::int64_t timeout_ns);

/// Makes SIGTERM and SIGINT set `stop_requested`.
void watch_stop_signals();
[[nodiscard]] bool stop_requested();

/// Latency samples in microseconds.
class Samples {
public:
  void add(std::int64_t microseconds) { values.push_back(microseconds); }
  /// Formats the count, percentiles and maximum as a JSON object.
  [[nodiscard]] std::string json();

private:
  std::vector<std::int64_t> values;
};

/// One JSON object, printed as one line for `run.py`.
class Report {
public:
  void number(const std::string &key, std::uint64_t value);
  void signed_number(const std::string &key, std::int64_t value);
  void flag(const std::string &key, bool value);
  void text(const std::string &key, const std::string &value);
  void object(const std::string &key, const std::string &json);
  void traffic(const Traffic &counted);
  void print() const;

private:
  std::string body;
};

} // namespace neocity::lab
