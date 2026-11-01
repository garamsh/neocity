#include "bot.hpp"
#include "wire.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

namespace neocity::lab {
namespace {

using survival::Actor;

constexpr std::uint32_t SEED = 7;
constexpr std::uint32_t LATE_DAY = 30;

State late_state() {
  const Room room(SEED, LATE_DAY);
  return {
      .room = 3,
      .tick = 900,
      .sent_ns = -5,
      .acked = {4, 9},
      .applied = {.inputs = {survival::PlayerInput{
                                 .movement = {survival::Direction::Up, true},
                                 .interact = true},
                             survival::PlayerInput{.end_day = true}},
                  .control = {.chef = survival::Controller::Human,
                              .appraiser = survival::Controller::Ai}},
      .escape = {.checked_day = 31, .rolls = 2},
      .view = room.view()};
}

TEST(Wire, StateRoundTripRestoresThePlayedView) {
  const auto state = late_state();
  const auto blank = Room(SEED, 1).view();
  // Each packed object field must differ from `blank` somewhere, or restoring
  // it from `blank` would pass unnoticed.
  bool state_changed = false;
  bool access_changed = false;
  bool supply_changed = false;
  for (std::size_t index = 0; index < blank.world.objects.size(); ++index) {
    const auto &played = state.view.world.objects.at(index);
    const auto &initial = blank.world.objects.at(index);
    state_changed = state_changed || played.state != initial.state;
    access_changed =
        access_changed || played.traversable != initial.traversable;
    supply_changed = supply_changed ||
                     played.material_available != initial.material_available;
  }
  ASSERT_TRUE(state_changed);
  ASSERT_TRUE(access_changed);
  ASSERT_TRUE(supply_changed);

  const auto parsed = parse(encode(state), blank);

  ASSERT_TRUE(parsed.has_value());
  const auto &read = std::get<State>(*parsed);
  EXPECT_TRUE(same_view(read.view, state.view));
  EXPECT_EQ(read.room, state.room);
  EXPECT_EQ(read.tick, state.tick);
  EXPECT_EQ(read.sent_ns, state.sent_ns);
  EXPECT_EQ(read.acked, state.acked);
  EXPECT_EQ(read.escape, state.escape);
  EXPECT_EQ(read.applied.control, state.applied.control);
  EXPECT_TRUE(read.applied.inputs.at(0).interact);
  EXPECT_TRUE(read.applied.inputs.at(0).movement.running);
  EXPECT_EQ(read.applied.inputs.at(0).movement.direction,
            survival::Direction::Up);
  EXPECT_TRUE(read.applied.inputs.at(1).end_day);
}

TEST(Wire, InputRoundTripKeepsEveryKey) {
  const Input input{.room = 2,
                    .actor = Actor::Appraiser,
                    .seq = 300,
                    .keys = {.movement = {survival::Direction::Left, false},
                             .travel = true,
                             .replace_tool = true,
                             .feed = true}};

  const auto parsed = parse(encode(input), Room(SEED, 1).view());

  ASSERT_TRUE(parsed.has_value());
  const auto &read = std::get<Input>(*parsed);
  EXPECT_EQ(read.room, input.room);
  EXPECT_EQ(read.actor, input.actor);
  EXPECT_EQ(read.seq, input.seq);
  EXPECT_TRUE(read.keys.travel);
  EXPECT_TRUE(read.keys.replace_tool);
  EXPECT_TRUE(read.keys.feed);
  EXPECT_FALSE(read.keys.interact);
  EXPECT_EQ(read.keys.movement.direction, survival::Direction::Left);
}

TEST(Wire, TruncatedStateIsRefusedWhileTheWholePacketParses) {
  const auto blank = Room(SEED, 1).view();
  auto packet = encode(late_state());
  ASSERT_TRUE(parse(packet, blank).has_value());

  packet.pop_back();

  EXPECT_FALSE(parse(packet, blank).has_value());
}

TEST(Wire, TrailingBytesAreRefusedWhileTheExactPacketParses) {
  const auto blank = Room(SEED, 1).view();
  const auto exact = encode(Bye{.room = 1, .actor = Actor::Chef});
  ASSERT_TRUE(parse(exact, blank).has_value());

  std::vector<std::uint8_t> longer(exact.size() + 1);
  std::ranges::copy(exact, longer.begin());

  EXPECT_FALSE(parse(longer, blank).has_value());
}

TEST(Wire, UnknownKindAndActorAreRefusedWhileKnownOnesParse) {
  const auto blank = Room(SEED, 1).view();
  auto packet = encode(Join{.room = 1, .actor = Actor::Appraiser});
  ASSERT_TRUE(parse(packet, blank).has_value());

  auto bad_actor = packet;
  bad_actor.at(HEADER_SIZE) = 2;
  auto bad_kind = packet;
  bad_kind.at(0) = 200;

  EXPECT_FALSE(parse(bad_actor, blank).has_value());
  EXPECT_FALSE(parse(bad_kind, blank).has_value());
}

TEST(Wire, ListCountBeyondThePacketIsRefused) {
  const auto blank = Room(SEED, 1).view();
  auto packet = encode(State{.view = blank});
  ASSERT_TRUE(parse(packet, blank).has_value());
  // Header, tick, time, two acks, two key pairs, control, two escape values,
  // location and health come before the chef's inventory count.
  constexpr std::size_t INVENTORY_COUNT = 5 + 1 + 8 + 2 + 4 + 1 + 2 + 1 + 1;

  // Claims 2^63 ids; building that list would throw, not refuse.
  const std::vector<std::uint8_t> huge{0x80, 0x80, 0x80, 0x80, 0x80,
                                       0x80, 0x80, 0x80, 0x80, 0x01};
  packet.erase(packet.begin() + INVENTORY_COUNT);
  packet.insert(packet.begin() + INVENTORY_COUNT, huge.begin(), huge.end());

  EXPECT_FALSE(parse(packet, blank).has_value());
}

TEST(Wire, DigestFollowsTheViewAndTheEscapeWhileEqualStatesAgree) {
  const auto view = Room(SEED, 1).view();
  auto moved = view;
  ++moved.exploration.chef.position.x;
  const survival::EscapeSnapshot rolled{.checked_day = 30, .rolls = 1};

  EXPECT_EQ(digest(view, {}), digest(view, {}));
  EXPECT_NE(digest(view, {}), digest(moved, {}));
  EXPECT_NE(digest(view, {}), digest(view, rolled));
}

TEST(Wire, RelayReadsTheRoomFromTheHeader) {
  EXPECT_EQ(room_of(encode(HostHello{.room = 70000})), 70000U);
  EXPECT_TRUE(is_host_hello(encode(HostHello{.room = 1})));
  EXPECT_FALSE(is_host_hello(encode(Join{.room = 1})));
  EXPECT_FALSE(room_of(std::vector<std::uint8_t>{1, 2}).has_value());
}

} // namespace
} // namespace neocity::lab
