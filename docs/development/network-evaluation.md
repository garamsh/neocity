# Network evaluation

The comparison of a dedicated server and a player host for two-player online play, for [#22](https://github.com/garamsh/neocity/issues/22): what was measured, what was calculated from it, and a proposal awaiting the user's decision.

Measured figures come from one recorded run, [`tools/network-lab/results/measured.json`](../../tools/network-lab/results/measured.json), at commit `28322995356e19fc3738f6035065bf0fc6529fd8`. Calculated figures come from [`tools/network-lab/cost.py`](../../tools/network-lab/cost.py) over that file and [`tools/network-lab/pricing.json`](../../tools/network-lab/pricing.json). Nothing here is adopted: the network model stays open in [survival](../architecture/survival.md) until the user confirms a choice, which is recorded separately.

## Contents

- Proposal
- Comparison
- What was compared
- Reproduction
- Environment
- Measured: normal play
- Measured: departures
- Measured: rooms on one process
- Calculated: capacity and transfer
- Calculated: monthly cost
- Host succession
- Unverified

## Proposal

Proposed, not decided: run the rules on a small dedicated server built from the Godot-free `survival` library, with both players as clients. It is the only compared model in which either player can leave, gracefully or by crashing, with no recovery step and no measured pause, and a single small server covers the calculated range below.

Player host stays the alternative if the release ships through a platform whose relay needs no running service of our own, or if the user prefers to run nothing. It needs a relay and the succession work below to match that continuity, and neither is proven beyond this machine.

## Comparison

| Criterion | Dedicated server | Player host, direct | Player host, relayed |
|---|---|---|---|
| Connecting | Outbound to one known address; unverified across networks | Needs a reachable host; unverified across networks | Both ends outbound to the relay; unverified across networks and with any real relay |
| Worked on this machine | yes | yes | yes |
| A player leaves | Room continues; that actor goes to AI | Room continues if the guest leaves | Room continues if the guest leaves |
| The authority leaves | Both players' session ends | Session ends, unless succession runs | Not run; the guest would need the relay to outlive the host |
| Build and operation | Network client plus a public service to run, watch and pay for | Network client, address discovery, succession | The same, plus a relay |
| Running cost | From one server's price a month | None for us | One relay server, or metered transfer |

## What was compared

- `tools/network-lab/src/room.cpp` steps the real `survival` rules in `SurvivalRun::step`'s order, at the game's 100ms step. It does not link Godot.
- All models send the same packets at 10 Hz: one 9-byte input per client and the whole mutable game state after every step, 152 to 160 bytes in the sessions below. Immutable map data is rebuilt from the seed.
- Dedicated server: a process with no player, and two client processes.
- Player host: a process that also plays one actor, and one guest process. The relayed form sends everything through `tools/network-lab/src/relay.cpp`, which forwards by room number without reading the game.
- Each client is a bot. It reads the received state, asks `Companion` what an AI-only party would do and sends that as keys, so input travels the same path as a person's.
- Room joining is the minimum: a room number and a seat. There is no lobby, account or matchmaking.
- The transport is plain UDP on loopback, not Godot's ENet, and no Godot client took part. The rules and CMake toolchain are the project's; this is an unnamed case the issue's "existing Godot/C++ tools" does not settle.

## Reproduction

From the repository root, with `GODOT` naming Godot 4.7.2. The first command builds the extension the Godot baseline loads and installs the pinned tools. The `run.py` step takes several minutes and leaves no process running. Nothing here is part of `make ci`.

```sh
make build GODOT="$GODOT"
.tools/cpp/bin/cmake -S tools/network-lab -B tools/network-lab/build -G Ninja -DCMAKE_MAKE_PROGRAM="$PWD/.tools/cpp/bin/ninja" -DCMAKE_BUILD_TYPE=Release
.tools/cpp/bin/cmake --build tools/network-lab/build
.tools/cpp/bin/ctest --test-dir tools/network-lab/build --output-on-failure --no-tests=error
python3 tools/network-lab/run.py --godot "$GODOT" --out tools/network-lab/results/measured.json
python3 tools/network-lab/cost.py
```

`run.py` runs inside a private network namespace, so nothing else shares its loopback interface, and it fails if the kernel's interface counters disagree with the lab's own byte counts by more than 1%. In the recorded normal-play sessions they matched exactly.

## Environment

| Field | Record |
|---|---|
| Machine | Intel i5-11500, 12 logical CPUs, 24 GB memory, Linux 6.12.113 |
| Build | GCC 14.2.0, CMake Release, `-O3 -DNDEBUG` |
| Network | Loopback in a private namespace, on one machine |
| Run seed and rate | Seed 7; 100ms step; state and input each sent 10 times a second |
| Rooms | 1 in the sessions; 1 to 4000 on the room ladder |
| Available resources | Each measured process is single-threaded and unpinned; other lab processes and other work shared the 12 CPUs |
| Other load | The machine was not idle: another worktree's `make ci` was running when the run ended, and the 1-minute load average before each ladder rung was 2.3 to 7.7 |

## Measured: normal play

One room for 30 seconds, from a state played to day 30. Rates include IP and UDP headers.

| Measure | Dedicated | Host, direct | Host, relayed |
|---|---|---|---|
| Authority CPU, % of one core | 0.33 | 0.63 | 0.67 |
| Authority memory (peak RSS) | 3.6 MiB | 3.6 MiB | 3.6 MiB |
| Client CPU, % of one core | 0.50 | 0.70 | 0.70 |
| Relay CPU, % of one core | not used | not used | 0.30 |
| Relay memory (peak RSS) | not used | not used | 3.4 MiB |
| Authority sends | 3.66 KB/s | 1.83 KB/s | 1.83 KB/s |
| Authority receives | 0.76 KB/s | 0.38 KB/s | 0.38 KB/s |
| Relay sends | not used | not used | 2.20 KB/s |
| State to client, median | 149 and 161 µs | 151 µs | 258 µs |
| State to client, p99 | 188 and 203 µs | 176 µs | 294 µs |
| Largest gap between states | 100.3 ms | 100.7 ms | 100.7 ms |
| Ticks lost | 0 | 0 | 0 |

- State-to-client latency is send to receipt on one machine, so it shows this code path and says nothing about a real network.
- An input's round trip measured one step, about 100ms, in every model: the bot answers straight after a state, so its input waits for the next step. That is the step length, not network delay.
- Host CPU includes its bot's route search, which a person's keys would replace; the host's tick took about 600 µs against the server's 113 µs at the median. It leaves out everything a real game draws.
- Without any player or network, a headless Godot of the real project sat at its menu at 117 MiB peak RSS and 3.1% of one core.

State size over a played run, with no sockets: the largest state on any day up to day 30 was 157 bytes, and day 1's was 146. The rule step averaged 162 µs under AI. The bots reached day 30 in 770 steps, because an AI party with nothing reachable ends its day at once, so these runs spend far less time exploring than a person would.

## Measured: departures

Each row is one run. "Pause" is the longest gap the remaining client saw; for a host the room's own ticks stayed on time, so no gap applies.

| Event | Remaining player continues | Pause | State lost |
|---|---|---|---|
| Dedicated: player leaves | yes | 100.3 ms, one normal step | None; AI takes the actor |
| Dedicated: player process killed | yes | 100.3 ms; AI takes over after the 1 s timeout | None; AI takes the actor |
| Host, direct or relayed: guest leaves or is killed | yes | None measured | None; AI takes the actor |
| Dedicated: server stops | no, for both players | Session over; noticed by a goodbye packet | The whole run: no save exists |
| Dedicated: server killed | no, for both players | Session over; noticed after 1.0 s of silence | The whole run: no save exists |
| Host: stops, no succession | no | Session over; noticed by a goodbye packet | The whole run: no save exists |
| Host: killed, no succession | no | Session over; noticed after 1.0 s of silence | The whole run: no save exists |
| Host: stops, with succession | yes, alone with AI | 0.20 s | None: replayed state equalled the last received |
| Host: killed, with succession | yes, alone with AI | 1.15 s | None: replayed state equalled the last received |

- In every authority-loss run, the last tick the authority computed had reached the client. A killed host can lose a state in flight, and nothing here shows how often.
- The 1 s silence timeout is this lab's choice. A real network needs its own value, which sets the forced-exit pause.
- Succession ran on the direct path only. In those runs the replay took 0.13 to 0.14 s for 809 steps, and all 40 steps played afterwards changed the state.

## Measured: rooms on one process

Each rung ran 15 settled seconds on one single-threaded process, with the load generators as separate processes. A rung held if no tick was skipped, no receive buffer dropped a packet while settled and rooms kept at least 9.5 ticks a second; for the server, 99% of ticks also had to start within 10 ms.

| Rooms | Server CPU, % of a core | Server peak RSS | Held | Relay CPU, % of a core | Relay peak RSS | Held |
|---|---|---|---|---|---|---|
| 1 | 0.27 | 3.6 MiB | yes | 0.07 | 3.4 MiB | yes |
| 10 | 0.80 | 3.8 MiB | yes | 0.33 | 3.4 MiB | yes |
| 100 | 4.2 | 6.1 MiB | yes | 3.27 | 3.4 MiB | yes |
| 500 | 9.4 | 15.5 MiB | yes | 8.93 | 3.5 MiB | yes |
| 1000 | 18.1 | 27.3 MiB | yes | 12.4 | 3.5 MiB | yes |
| 2000 | 34.1 | 52.4 MiB | yes | 20.5 | 3.6 MiB | no |
| 4000 | 84.2 | 101.0 MiB | no | not run | not run | not run |

- These rungs start at day 1, not day 30, which changes the state by a few bytes.
- CPU per room falls as rooms rise, because a busy process sleeps less often.
- Neither limit is located. At 4000 rooms the load generators were using about seven cores and clients saw seconds of delay. At 2000 relayed rooms the emulated hosts were late and the relay sat at 20.5% of a core. Packets were dropped in both, and which socket dropped them was not recorded.
- Client latency on these rungs reflects the load generators and is left out as a server figure.

## Calculated: capacity and transfer

Calculated from the measurements above. Nothing in this section was measured.

| Quantity | Value | Basis |
|---|---|---|
| Dedicated server egress | 0.0132 GB per room-hour | Measured send rate |
| Relay egress, every room relayed | 0.0079 GB per room-hour | Measured send rate |
| Player host uplink | 0.0066 GB per room-hour | Measured send rate |
| Rooms on one server process | 500 to 2000 | The highest held rung, and a quarter of it |
| Rooms on one relay process | 250 to 1000 | The highest held rung, and a quarter of it |
| Memory per added server room | 25 KiB | The rise in peak RSS from 1 to 2000 rooms |
| Rooms per 4 GiB server with one Godot process each | 34 | 117 MiB each, by memory alone |

The quarter is an assumption standing in for a slower cloud core and a margin; the true factor is unmeasured. Transfer scales with the state: a state twice the size nearly doubles it.

## Calculated: monthly cost

Prices were checked on 2026-11-02, excluding VAT, from [Hetzner cost-optimized cloud](https://www.hetzner.com/cloud/cost-optimized/), [AWS Lightsail pricing](https://aws.amazon.com/lightsail/pricing/) and [Cloudflare Realtime TURN](https://developers.cloudflare.com/realtime/turn/). Euro and dollar figures are not converted.

| Service | Price used |
|---|---|
| Hetzner CX23 | EUR 5.99 a month; 2 vCPU, 4 GB; 20 TB included, then EUR 1.00 per TB |
| AWS Lightsail | USD 12 a month; 2 vCPU, 2 GB; 3 TB included, then USD 0.09 per GB in the page's US East (Ohio) example |
| Cloudflare Realtime TURN | USD 0.05 per GB outbound to clients; no server to run |

Assumptions, named in `cost.py`: peak rooms are ten times the average, one process runs per server, and a server's price divides evenly between compute and memory. Compute and memory below are what rooms actually use of that price; fixed is the rest, paid whether used or not. Transfer stayed inside the included allowance in every row.

Dedicated server on Hetzner CX23, EUR a month, at the higher and lower capacity:

| Room-hours | Peak rooms | Servers | Fixed | Compute | Memory | Transfer | Total | GB sent |
|---|---|---|---|---|---|---|---|---|
| 100 | 1.4 | 1 | 5.99 | 0.00 | 0.00 | 0.00 | 5.99 | 1.3 |
| 1,000 | 13.7 | 1 | 5.99 | 0.00 | 0.00 | 0.00 | 5.99 | 13.2 |
| 10,000 | 137 | 1 | 5.98 to 5.99 | 0.00 to 0.01 | 0.00 | 0.00 | 5.99 | 131.9 |
| 100,000 | 1,370 | 1 to 3 | 5.95 to 17.83 | 0.04 to 0.14 | 0.00 | 0.00 | 5.99 to 17.97 | 1,318.8 |
| 1,000,000 | 13,699 | 7 to 28 | 41.56 to 166.30 | 0.35 to 1.40 | 0.02 | 0.00 | 41.93 to 167.72 | 13,188 |

Totals for the other options a month, at the higher and lower capacity:

| Room-hours | Dedicated, Lightsail (USD) | Own relay, Hetzner (EUR) | Own relay, Lightsail (USD) | TURN, every room relayed (USD) |
|---|---|---|---|---|
| 100 | 12 | 5.99 | 12 | 0.04 |
| 1,000 | 12 | 5.99 | 12 | 0.40 |
| 10,000 | 12 | 5.99 | 12 | 3.97 |
| 100,000 | 12 to 36 | 11.98 to 35.94 | 24 to 72 | 39.67 |
| 1,000,000 | 84 to 336 | 83.86 to 329.45 | 168 to 660 | 396.66 |

- A direct player-host room costs us nothing to run, and each relayed room costs as in the last three columns. Any model still needs some way for players to find each other, which is not priced here.
- Each extra region repeats the server line; regions were not compared.
- The Hetzner page showed these plans as unavailable when fetched, and rendered its traffic overage as "per hour", so confirm both before relying on them. No row reaches the overage.
- The Lightsail page names no region for its bundle prices.
- Cloudflare's page bills outbound traffic. The TURN column applies that to the relay's measured send rate and leaves out TURN's own framing.
- The AWS EC2 pages fetched showed no instance price, so EC2 is not priced.
- Steam Datagram Relay's page states no fee and requires a version shipping on Steam for players on other stores. No official Epic Online Services price was found. Neither is priced here.

## Host succession

Implemented in this lab only, for a guest left alone, and no release design is implied. Every state carries the two inputs and the controllers of its step, 5 bytes that all models send so the comparison stays equal. The guest keeps them, and when the host goes it rebuilds the room from the seed, replays the log, compares the result with the last state received, and plays on with AI in the host's seat.

Replay is needed because the public snapshots cannot restore a run: they omit movement remainders, world repair history and `Workshop`'s random state, and the rules have no restore command.

Calculated: a run of 30 full 45-second days is at least 8,100 steps, so a 40.5 KB log and about 1.4 s of replay at the rate measured above, added to the detection time.

A release would still need:

- Delivery of the log over a lossy network, since one missing step refuses the replay. The lab has no retransmission, and detects gaps only.
- A silence timeout chosen from real network data.
- A way for the departed host to return, and for a guest to rejoin at all.
- Replay fitted into the Godot client, with both players' builds on identical rules.
- A save or restore in `survival`, if a run should outlive both players.

## Unverified

- Direct connection between different networks, and any NAT traversal.
- Any real relay. Only the lab's own relay on loopback ran.
- Latency, loss and jitter on a real network, and how the unreliable state stream behaves under them.
- Godot's ENet or any transport other than plain UDP, its overhead, and a Godot client.
- State size in human play. Bots hold what the AI collects, and a person may carry more.
- Cloud CPU speed, and more than one process or core.
- Where the room limits actually lie.
- Repeatability: the tables record one run, on a machine doing other work.
