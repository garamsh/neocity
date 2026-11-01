#include "wire.hpp"

#include <chrono>
#include <cstdlib>
#include <limits>

namespace neocity::lab {
namespace {

using namespace neocity::survival;

enum class Kind : std::uint8_t { Join, HostHello, Input, State, Bye };

constexpr std::uint8_t KIND_LIMIT = 4;
constexpr std::uint8_t OBJECT_STATE_MASK = 3;
constexpr std::uint8_t TRAVERSABLE_BIT = 4;
constexpr std::uint8_t MATERIAL_BIT = 8;
constexpr unsigned BYTE_BITS = 8;
constexpr unsigned VARINT_BITS = 7;
constexpr std::uint8_t VARINT_MORE = 0x80;
constexpr std::uint8_t DIRECTION_LIMIT = 4;
constexpr std::uint8_t REACTION_LIMIT = 3;
constexpr std::uint8_t OBJECT_STATE_LIMIT = 2;
constexpr std::uint8_t PHASE_LIMIT = 2;
constexpr std::uint8_t BINARY_LIMIT = 1;
constexpr std::uint8_t CONTROL_LIMIT = 3;

constexpr std::uint8_t TRAVEL_KEY = 1;
constexpr std::uint8_t INTERACT_KEY = 2;
constexpr std::uint8_t REPLACE_KEY = 4;
constexpr std::uint8_t FEED_KEY = 8;
constexpr std::uint8_t END_DAY_KEY = 16;
constexpr std::uint8_t RUNNING_KEY = 32;
constexpr std::uint8_t KEY_MASK = 63;

class Writer {
public:
  explicit Writer(std::vector<std::uint8_t> &target) : out(target) {}

  void number(std::uint64_t value) {
    while (value >= VARINT_MORE) {
      out.push_back(static_cast<std::uint8_t>(value | VARINT_MORE));
      value >>= VARINT_BITS;
    }
    out.push_back(static_cast<std::uint8_t>(value));
  }

  void fixed(std::uint64_t value, std::size_t bytes) {
    for (std::size_t index = 0; index < bytes; ++index) {
      out.push_back(static_cast<std::uint8_t>(value >> (index * BYTE_BITS)));
    }
  }

  void byte(std::uint8_t value) { out.push_back(value); }

private:
  std::vector<std::uint8_t> &out;
};

class Reader {
public:
  explicit Reader(std::span<const std::uint8_t> packet) : rest(packet) {}

  [[nodiscard]] bool usable() const { return !broken; }
  [[nodiscard]] bool exhausted() const { return rest.empty(); }
  [[nodiscard]] std::size_t remaining() const { return rest.size(); }
  void fail() { broken = true; }

  [[nodiscard]] std::uint8_t byte() {
    if (rest.empty()) {
      broken = true;
      return 0;
    }
    const auto value = rest.front();
    rest = rest.subspan(1);
    return value;
  }

  [[nodiscard]] std::uint8_t limited(std::uint8_t limit) {
    const auto value = byte();
    if (value > limit) {
      broken = true;
    }
    return value;
  }

  [[nodiscard]] std::uint64_t number() {
    std::uint64_t value = 0;
    for (unsigned shift = 0; shift < std::numeric_limits<std::uint64_t>::digits;
         shift += VARINT_BITS) {
      const auto next = byte();
      value |= static_cast<std::uint64_t>(next & ~VARINT_MORE) << shift;
      if ((next & VARINT_MORE) == 0) {
        return value;
      }
    }
    broken = true;
    return 0;
  }

  [[nodiscard]] std::uint64_t fixed(std::size_t bytes) {
    std::uint64_t value = 0;
    for (std::size_t index = 0; index < bytes; ++index) {
      value |= static_cast<std::uint64_t>(byte()) << (index * BYTE_BITS);
    }
    return value;
  }

private:
  std::span<const std::uint8_t> rest;
  bool broken = false;
};

std::uint64_t zigzag(std::int32_t value) {
  return static_cast<std::uint32_t>((value * 2) ^ (value >> 31));
}

std::int32_t unzigzag(std::uint32_t value) {
  return static_cast<std::int32_t>(value >> 1U) ^
         -static_cast<std::int32_t>(value & 1U);
}

/// Writes the fields `visit_view` names. It and `ReadPort` share that one field
/// list, so a field cannot travel in one direction only.
class WritePort {
public:
  explicit WritePort(Writer &target) : writer(target) {}

  void scalar(std::uint8_t value) { writer.byte(value); }
  void scalar(std::uint32_t value) { writer.number(value); }
  void scalar(std::uint64_t value) { writer.number(value); }
  void signed_scalar(std::int32_t value) { writer.number(zigzag(value)); }
  void time(std::chrono::microseconds value) {
    writer.number(static_cast<std::uint64_t>(value.count()));
  }
  template <class Enum> void choice(Enum value, std::uint8_t /*limit*/) {
    writer.byte(static_cast<std::uint8_t>(value));
  }
  template <class Id> void ids(const std::vector<Id> &values) {
    writer.number(values.size());
    for (const auto value : values) {
      writer.number(value.value);
    }
  }
  void outcome(const std::optional<DayOutcome> &value) {
    writer.byte(value ? 1 : 0);
    if (value) {
      writer.number(value->completed_day);
      writer.byte(static_cast<std::uint8_t>(value->reaction));
      writer.number(value->score);
      writer.number(value->acid_rise.value);
    }
  }
  void object(const ObjectSnapshot &value) {
    auto packed = static_cast<unsigned>(value.state);
    packed |= value.traversable ? TRAVERSABLE_BIT : 0U;
    packed |= value.material_available ? MATERIAL_BIT : 0U;
    writer.byte(static_cast<std::uint8_t>(packed));
  }

private:
  Writer &writer;
};

class ReadPort {
public:
  explicit ReadPort(Reader &source) : reader(source) {}

  void scalar(std::uint8_t &value) { value = reader.byte(); }
  void scalar(std::uint32_t &value) {
    const auto read = reader.number();
    if (read > std::numeric_limits<std::uint32_t>::max()) {
      reader.fail();
    }
    value = static_cast<std::uint32_t>(read);
  }
  void scalar(std::uint64_t &value) { value = reader.number(); }
  void signed_scalar(std::int32_t &value) {
    std::uint32_t read = 0;
    scalar(read);
    value = unzigzag(read);
  }
  void time(std::chrono::microseconds &value) {
    const auto read = reader.number();
    if (read >
        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
      reader.fail();
    }
    value = std::chrono::microseconds{static_cast<std::int64_t>(read)};
  }
  template <class Enum> void choice(Enum &value, std::uint8_t limit) {
    value = static_cast<Enum>(reader.limited(limit));
  }
  template <class Id> void ids(std::vector<Id> &values) {
    const auto count = reader.number();
    // Every id takes at least one byte, so this bounds the allocation.
    if (count > reader.remaining()) {
      reader.fail();
      return;
    }
    values.assign(static_cast<std::size_t>(count), Id{});
    for (auto &value : values) {
      scalar(value.value);
    }
  }
  void outcome(std::optional<DayOutcome> &value) {
    if (reader.limited(BINARY_LIMIT) == 0) {
      value = std::nullopt;
      return;
    }
    DayOutcome read;
    scalar(read.completed_day);
    choice(read.reaction, REACTION_LIMIT);
    scalar(read.score);
    scalar(read.acid_rise.value);
    value = read;
  }
  void object(ObjectSnapshot &value) {
    const auto read = reader.byte();
    constexpr unsigned KNOWN =
        OBJECT_STATE_MASK | TRAVERSABLE_BIT | MATERIAL_BIT;
    if ((read & OBJECT_STATE_MASK) > OBJECT_STATE_LIMIT ||
        (read & ~KNOWN) != 0U) {
      reader.fail();
    }
    value.state = static_cast<ObjectState>(read & OBJECT_STATE_MASK);
    value.traversable = (read & TRAVERSABLE_BIT) != 0;
    value.material_available = (read & MATERIAL_BIT) != 0;
  }

private:
  Reader &reader;
};

template <class Port, class Carried>
void visit_actor(Port &port, Carried &actor) {
  port.choice(actor.location, BINARY_LIMIT);
  port.scalar(actor.health);
  port.ids(actor.inventory);
  port.ids(actor.tools);
  port.scalar(actor.exhaustions);
}

template <class Port, class Explorer>
void visit_explorer(Port &port, Explorer &explorer) {
  port.signed_scalar(explorer.position.x);
  port.signed_scalar(explorer.position.y);
  port.scalar(explorer.stamina);
}

/// The mutable part of `GameView`; ids, areas and configuration stay in
/// `blank`.
template <class Port, class View> void visit_view(Port &port, View &view) {
  visit_actor(port, view.progression.chef);
  visit_actor(port, view.progression.appraiser);
  port.choice(view.progression.phase, PHASE_LIMIT);
  port.scalar(view.progression.day);
  port.time(view.progression.remaining_exploration);
  port.scalar(view.progression.acid.value);
  port.time(view.world.progression.elapsed);
  port.scalar(view.world.progression.acid.value);
  port.scalar(view.world.peak_acid.value);
  for (auto &object : view.world.objects) {
    port.object(object);
  }
  visit_explorer(port, view.exploration.chef);
  visit_explorer(port, view.exploration.appraiser);
  for (auto &source : view.exploration.sources) {
    port.scalar(source.remaining);
  }
  port.choice(view.workshop.magnifier, BINARY_LIMIT);
  port.choice(view.workshop.spatula, BINARY_LIMIT);
  port.ids(view.workshop.identified);
  port.ids(view.workshop.foods);
  port.ids(view.workshop.eaten_foods);
  port.ids(view.workshop.eaten_ingredients);
  port.outcome(view.workshop.last_day);
  port.scalar(view.workshop.rolls);
}

void write_keys(Writer &writer, const PlayerInput &keys) {
  unsigned flags = 0;
  flags |= keys.travel ? TRAVEL_KEY : 0U;
  flags |= keys.interact ? INTERACT_KEY : 0U;
  flags |= keys.replace_tool ? REPLACE_KEY : 0U;
  flags |= keys.feed ? FEED_KEY : 0U;
  flags |= keys.end_day ? END_DAY_KEY : 0U;
  flags |= keys.movement.running ? RUNNING_KEY : 0U;
  writer.byte(static_cast<std::uint8_t>(flags));
  writer.byte(static_cast<std::uint8_t>(keys.movement.direction));
}

PlayerInput read_keys(Reader &reader) {
  const auto flags = reader.limited(KEY_MASK);
  const auto direction = reader.limited(DIRECTION_LIMIT);
  return {.movement = {.direction = static_cast<Direction>(direction),
                       .running = (flags & RUNNING_KEY) != 0},
          .travel = (flags & TRAVEL_KEY) != 0,
          .interact = (flags & INTERACT_KEY) != 0,
          .replace_tool = (flags & REPLACE_KEY) != 0,
          .feed = (flags & FEED_KEY) != 0,
          .end_day = (flags & END_DAY_KEY) != 0,
          .switch_actor = false,
          .leave = false};
}

void write_header(Writer &writer, Kind kind, std::uint32_t room) {
  writer.byte(static_cast<std::uint8_t>(kind));
  writer.fixed(room, sizeof(room));
}

void write_body(Writer &writer, const Join &join) {
  write_header(writer, Kind::Join, join.room);
  writer.byte(static_cast<std::uint8_t>(join.actor));
}

void write_body(Writer &writer, const HostHello &hello) {
  write_header(writer, Kind::HostHello, hello.room);
}

void write_body(Writer &writer, const Input &input) {
  write_header(writer, Kind::Input, input.room);
  writer.byte(static_cast<std::uint8_t>(input.actor));
  writer.number(input.seq);
  write_keys(writer, input.keys);
}

void write_body(Writer &writer, const State &state) {
  write_header(writer, Kind::State, state.room);
  writer.number(state.tick);
  writer.fixed(static_cast<std::uint64_t>(state.sent_ns),
               sizeof(state.sent_ns));
  writer.number(state.acked.at(0));
  writer.number(state.acked.at(1));
  write_keys(writer, state.applied.inputs.at(0));
  write_keys(writer, state.applied.inputs.at(1));
  const auto chef = static_cast<unsigned>(state.applied.control.chef);
  const auto appraiser = static_cast<unsigned>(state.applied.control.appraiser);
  writer.byte(static_cast<std::uint8_t>(chef | (appraiser << 1U)));
  WritePort port(writer);
  port.scalar(state.escape.checked_day);
  port.scalar(state.escape.rolls);
  visit_view(port, state.view);
}

void write_body(Writer &writer, const Bye &bye) {
  write_header(writer, Kind::Bye, bye.room);
  writer.byte(static_cast<std::uint8_t>(bye.actor));
}

Actor read_actor(Reader &reader) {
  return static_cast<Actor>(reader.limited(BINARY_LIMIT));
}

State read_state(Reader &reader, std::uint32_t room, const GameView &blank) {
  State state{.room = room, .view = blank};
  state.tick = reader.number();
  state.sent_ns =
      static_cast<std::int64_t>(reader.fixed(sizeof(state.sent_ns)));
  state.acked.at(0) = reader.number();
  state.acked.at(1) = reader.number();
  state.applied.inputs.at(0) = read_keys(reader);
  state.applied.inputs.at(1) = read_keys(reader);
  const auto control = reader.limited(CONTROL_LIMIT);
  state.applied.control = {.chef = static_cast<Controller>(control & 1U),
                           .appraiser =
                               static_cast<Controller>((control >> 1U) & 1U)};
  ReadPort port(reader);
  port.scalar(state.escape.checked_day);
  port.scalar(state.escape.rolls);
  visit_view(port, state.view);
  return state;
}

Message read_body(Reader &reader, Kind kind, std::uint32_t room,
                  const GameView &blank) {
  switch (kind) {
  case Kind::Join:
    return Join{.room = room, .actor = read_actor(reader)};
  case Kind::HostHello:
    return HostHello{.room = room};
  case Kind::Input: {
    const auto actor = read_actor(reader);
    const auto seq = reader.number();
    return Input{
        .room = room, .actor = actor, .seq = seq, .keys = read_keys(reader)};
  }
  case Kind::State:
    return read_state(reader, room, blank);
  case Kind::Bye:
    return Bye{.room = room, .actor = read_actor(reader)};
  }
  std::abort();
}

} // namespace

std::vector<std::uint8_t> encode(const Message &message) {
  std::vector<std::uint8_t> out;
  Writer writer(out);
  std::visit([&writer](const auto &body) { write_body(writer, body); },
             message);
  return out;
}

std::optional<Message> parse(std::span<const std::uint8_t> packet,
                             const GameView &blank) {
  Reader reader(packet);
  const auto kind = reader.limited(KIND_LIMIT);
  const auto room =
      static_cast<std::uint32_t>(reader.fixed(sizeof(std::uint32_t)));
  if (!reader.usable()) {
    return std::nullopt;
  }
  auto message = read_body(reader, static_cast<Kind>(kind), room, blank);
  if (!reader.usable() || !reader.exhausted()) {
    return std::nullopt;
  }
  return message;
}

std::uint64_t digest(const GameView &view, EscapeSnapshot escape) {
  constexpr std::uint64_t FNV_OFFSET = 14695981039346656037U;
  constexpr std::uint64_t FNV_PRIME = 1099511628211U;
  auto hash = FNV_OFFSET;
  for (const auto byte : encode(State{.escape = escape, .view = view})) {
    hash = (hash ^ byte) * FNV_PRIME;
  }
  return hash;
}

std::optional<std::uint32_t> room_of(std::span<const std::uint8_t> packet) {
  if (packet.size() < HEADER_SIZE) {
    return std::nullopt;
  }
  Reader reader(packet.subspan(1));
  return static_cast<std::uint32_t>(reader.fixed(sizeof(std::uint32_t)));
}

bool is_host_hello(std::span<const std::uint8_t> packet) {
  return !packet.empty() &&
         packet.front() == static_cast<std::uint8_t>(Kind::HostHello);
}

} // namespace neocity::lab
