# Network evaluation

The comparison of a dedicated server and a player host for two-player online play, for [#22](https://github.com/garamsh/neocity/issues/22): what was measured, what was calculated from it, and a proposal awaiting the user's decision.

Every measured figure comes from one recorded run, [`tools/network-lab/results/measured.json`](../../tools/network-lab/results/measured.json), made from a clean lab build at commit `c0ded52581d08d6b4969db6dc9d8d209644a8519`, which includes `Escape` from #48. Calculated figures come from [`tools/network-lab/cost.py`](../../tools/network-lab/cost.py) over that file and [`tools/network-lab/pricing.json`](../../tools/network-lab/pricing.json). Nothing here is adopted: the network model stays open in [survival](../architecture/survival.md) until the user confirms a choice, which is recorded separately.

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

Proposed, not decided: a player host, with log-replay succession, and a relay for rooms that cannot connect directly. This moved from the first draft of this document, which proposed a dedicated server, once the loss of each authority was compared.

- **Worst measured loss.** Losing a dedicated server ended both players' whole run, and it would do so in every room on that server: 500 to 2000 rooms by the calculation below. Losing a player host touched one room, and with succession it lost no state: the guest played on after 0.08 s on a clean stop and 1.01 s on a kill, measured on a 40-step log. A full run adds about 0.07 to 2.3 s of replay, calculated.
- **Cost.** A direct room costs nothing to run. A relayed room costs about the same as a dedicated one.
- **What a dedicated server keeps.** Either player can leave with no recovery step and no pause beyond a normal step, with no address discovery and no succession code. Each host that leaves first costs its guest a pause, and that cost is paid in ordinary play.

This rests on things not measured, and any of these changes it:

- There is no failure-rate data for either side: not how often a server is lost, and not how often a host quits or crashes.
- If most rooms need a relay and no platform relay is free to use, the cost advantage goes and a dedicated server becomes the simpler build.
- Succession is proven only on loopback with no packet loss, and one lost step refuses the replay.
- A server's loss could also be softened by saves or by replaying a client's log. Neither was built, so the comparison is between what ran.

## Comparison

| Criterion | Dedicated server | Player host, direct | Player host, relayed |
|---|---|---|---|
| Connecting | Outbound to one known address; unverified across networks | Needs a reachable host; unverified across networks | Both ends outbound to the relay; unverified across networks and with any real relay |
| Worked on this machine | yes | yes | yes |
| A player leaves | Room continues; that actor goes to AI | Room continues if the guest leaves | Room continues if the guest leaves |
| The authority leaves | Both players' session ends | Session ends; with lab succession the guest plays on | Session ends; with lab succession the guest plays on, while the relay stays up |
| What one failure reaches | Every room on that server | One room | One room for a host; losing the relay itself was not run |
| Build and operation | Network client plus a public service to run, watch and pay for | Network client, address discovery, succession | The same, plus a relay |
| Running cost | From one server's price a month | None for us | One relay server, or metered transfer |

## What was compared

- `tools/network-lab/src/room.cpp` steps the real `survival` rules in `SurvivalRun::step`'s order, including `Escape` and the early return after it, at the game's 100ms step. It does not link Godot.
- All models send the same packets at 10 Hz. One input per client: 9 or 10 bytes, 9.58 on average in the sessions below. The whole mutable game state after every step: 111 to 158 bytes, 147 on average. Immutable map data is rebuilt from the seed.
- Dedicated server: a process with no player, and two client processes.
- Player host: a process that also plays one actor, and one guest process. The relayed form sends everything through `tools/network-lab/src/relay.cpp`, which forwards by room number without reading the game.
- Each client is a bot. It reads the received state, asks `Companion` what an AI-only party would do and sends that as keys, so input travels the same path as a person's.
- Sessions start on day 1. A room started late in the run would escape within the measured time, and seed 7 ends after day 32.
- Room joining is the minimum: a room number and a seat. There is no lobby, account or matchmaking.
- The transport is plain UDP on loopback, not Godot's ENet, and no Godot client took part. The rules and CMake toolchain are the project's; this is an unnamed case the issue's "existing Godot/C++ tools" does not settle.

## Reproduction

From the repository root, with `GODOT` naming Godot 4.7.2. The first command builds the extension the Godot baseline loads and installs the pinned tools. The `run.py` step takes several minutes and leaves no process running, including when it receives SIGTERM or SIGINT. Nothing here is part of `make ci`.

```sh
make build GODOT="$GODOT"
.tools/cpp/bin/cmake -S tools/network-lab -B tools/network-lab/build -G Ninja -DCMAKE_MAKE_PROGRAM="$PWD/.tools/cpp/bin/ninja" -DCMAKE_BUILD_TYPE=Release
.tools/cpp/bin/cmake --build tools/network-lab/build
.tools/cpp/bin/ctest --test-dir tools/network-lab/build --output-on-failure --no-tests=error
python3 tools/network-lab/run.py --godot "$GODOT" --out tools/network-lab/results/measured.json
python3 tools/network-lab/cost.py
```

`run.py` runs inside a private network namespace, so nothing else shares its loopback interface, and it fails if the kernel's interface counters disagree with the lab's own byte counts by more than 1%. In the recorded normal-play sessions they matched exactly. It also records the commit and whether the lab or rules differed from it; for this run they did not.

## Environment

| Field | Record |
|---|---|
| Machine | Intel i5-11500, 12 logical CPUs, 24 GB memory, Linux 6.12.113 |
| Build | GCC 14.2.0, CMake Release, `-O3 -DNDEBUG` |
| Network | Loopback in a private namespace, on one machine |
| Run seed and rate | Seed 7; 100ms step; state and input each sent 10 times a second |
| Rooms | 1 in the sessions; 1 to 4000 on the room ladder |
| Available resources | Each measured process is single-threaded and unpinned; other lab processes and any other work shared the 12 CPUs |
| Other load | The machine is shared, and the 1-minute load average before each ladder rung was 0.6 to 2.8 |

## Measured: normal play

One room for 30 seconds from day 1. No run ended during a session. Rates include IP and UDP headers.

| Measure | Dedicated | Host, direct | Host, relayed |
|---|---|---|---|
| Authority CPU, % of one core | 0.37 | 0.77 | 0.73 |
| Authority memory (peak RSS) | 3.6 MiB | 3.6 MiB | 3.6 MiB |
| Client CPU, % of one core | 0.63 | 0.60 | 0.50 |
| Relay CPU, % of one core | not used | not used | 0.27 |
| Relay memory (peak RSS) | not used | not used | 3.4 MiB |
| Authority sends | 3.50 KB/s | 1.74 KB/s | 1.74 KB/s |
| Authority receives | 0.76 KB/s | 0.38 KB/s | 0.38 KB/s |
| Relay sends | not used | not used | 2.12 KB/s |
| State to client, median | 138 and 147 µs | 143 µs | 223 µs |
| State to client, p99 | 173 and 184 µs | 200 µs | 276 µs |
| Largest gap between states | 100.3 ms | 101.0 ms | 101.2 ms |
| Ticks lost | 0 | 0 | 0 |

- State-to-client latency is send to receipt on one machine, so it shows this code path and says nothing about a real network.
- An input's round trip measured one step, about 100ms, in every model: the bot answers straight after a state, so its input waits for the next step. That is the step length, not network delay.
- The host's tick took 494 µs at the median against the server's 134 µs. The cause was not isolated. It leaves out everything a real game draws.
- Without any player or network, a headless Godot of the real project sat at its menu at 117.5 MiB peak RSS and 1.1% of one core.

Over whole runs with no sockets, seed 7 ended after day 32, in 861 steps with AI seats and 1379 with key seats. These bots end a day as soon as nothing is reachable, so they spend far less time exploring than a person would. The largest state on any day was 159 bytes, and day 1's was 148. A step averaged 168 µs with two AI seats, whose route search is the difference, and 5 µs replaying key seats.

## Measured: departures

Each row is one run. "Pause" is the longest gap the remaining client saw between states. For a host left alone no client exists, and its room kept ticking on time.

A player's departure was checked from the room: the remaining actor moved or changed what it carried in every step after the departure, 60 of 60 steps after a clean exit and 50 of 50 after a kill, so a day's rollover alone would not count. On a dedicated server the remaining client's last tick and state hash equalled the room's at tick 90. The departed actor lost no items on the next step and exploration time did not rewind.

| Event | Remaining player continues | Pause | State lost |
|---|---|---|---|
| Dedicated: player leaves | yes | 100.4 ms, one normal step | None measured; AI takes the actor |
| Dedicated: player process killed | yes | 100.5 ms; AI takes over after the 1 s timeout | None measured; AI takes the actor |
| Host, direct or relayed: guest leaves or is killed | yes | None measured | None measured; AI takes the actor |
| Dedicated: server stops | no, for both players | Session over; noticed by a goodbye packet | The whole run: no save exists |
| Dedicated: server killed | no, for both players | Session over; noticed after 1.0 s of silence | The whole run: no save exists |
| Host, direct: stops, no succession | no | Session over; noticed by a goodbye packet | The whole run: no save exists |
| Host, direct: killed, no succession | no | Session over; noticed after 1.0 s of silence | The whole run: no save exists |
| Host, direct or relayed: stops, with succession | yes, alone with AI | 0.08 s | None: replayed state equalled the last received |
| Host, direct or relayed: killed, with succession | yes, alone with AI | 1.01 s | None: replayed state equalled the last received |

- A server's loss reaches every room on it. The runs above had one room; that reach follows from every room sharing the process.
- In every authority-loss run, the last tick the authority computed had reached the client. A killed host can lose a state in flight, and nothing here shows how often.
- The 1 s silence timeout is this lab's choice. A real network needs its own value, which sets the forced-exit pause.
- The clean-stop pause of 0.08 s is mostly where in the step the stop fell.
- Succession replayed a 40-step log in about 1.6 ms, and the player progressed in all 40 steps played afterwards.
- With succession, the relay stayed up. Its own loss was not run.

## Measured: rooms on one process

Each rung ran 15 settled seconds on one single-threaded process, with the load generators as separate processes. Rooms have two connected seats. A rung held if no tick was skipped, no receive buffer dropped a packet while settled and rooms kept at least 9.5 ticks a second; for the server, 99% of ticks also had to start within 10 ms. Drops during startup are excluded from that test, because every room joins at once, and are counted here.

| Rooms | Server CPU, % of a core | Server peak RSS | Startup drops | Held | Relay CPU, % of a core | Relay peak RSS | Startup drops | Held |
|---|---|---|---|---|---|---|---|---|
| 1 | 0.40 | 3.6 MiB | 0 | yes | 0.27 | 3.4 MiB | 0 | yes |
| 10 | 2.13 | 3.8 MiB | 0 | yes | 1.33 | 3.4 MiB | 0 | yes |
| 100 | 10.7 | 6.6 MiB | 0 | yes | 8.07 | 3.5 MiB | 0 | yes |
| 500 | 10.1 | 17.9 MiB | 0 | yes | 4.00 | 3.4 MiB | 0 | yes |
| 1000 | 19.1 | 32.2 MiB | 0 | yes | 8.20 | 3.5 MiB | 1327 | yes |
| 2000 | 36.9 | 61.4 MiB | 4210 | yes | 16.7 | 3.6 MiB | 2979 | yes |
| 4000 | 75.5 | 121.8 MiB | 7556 | no | 23.7 | 3.7 MiB | 49825 | no |

- These rungs start at day 1, and their largest state was 157 or 158 bytes.
- CPU per room falls as rooms rise, because a busy process sleeps less often.
- Neither limit is located. At 4000 rooms the other lab processes used about seven cores and either clients or emulated hosts fell seconds behind, with 234,894 packets dropped while settled on the server rung and 214,315 on the relay rung. Which socket dropped them was not recorded.
- The relay's top rung is soft: a discarded run at commit `23b30e589cf46791240d0e45cfd88091967555d4` failed at 2000 rooms, with the relay at 19.7% of a core. That run's results were not kept, because it recorded its tree as changed.
- Client latency under load is left out as a server figure: the load generators' polling was not separated from it.

## Calculated: capacity and transfer

Calculated from the measurements above. Nothing in this section was measured.

| Quantity | Value | Basis |
|---|---|---|
| Dedicated server egress | 0.0126 GB per room-hour | Measured send rate |
| Dedicated server ingress | 0.0027 GB per room-hour | Measured receive rate |
| Relay egress, every room relayed | 0.0076 GB per room-hour | Measured send rate |
| Relay ingress, every room relayed | 0.0076 GB per room-hour | Measured receive rate |
| Player host uplink | 0.0063 GB per room-hour | Measured send rate |
| Rooms on one server process | 500 to 2000 | The highest rung held under the test above, which leaves out startup drops, and a quarter of it |
| Rooms on one relay process | 500 to 2000 | The same |
| Memory per added server room | 30 KiB | The rise in peak RSS from 1 to 2000 rooms |
| CPU of one AI seat | 0.08% of a core | Half the difference between the AI-seat and key-seat steps, at 10 steps a second |
| Server rooms on a full core if each has one AI seat | 1000 | That seat added to the measured CPU per room |
| Rooms per 4 GiB server at one Godot process each | At most 34 | The menu baseline alone, by memory alone; a Godot process running a room would use more, and running one was not measured |

The quarter is an assumption standing in for a slower cloud core and a margin; the true factor is unmeasured. Server capacity assumes two connected seats. An AI seat searches routes on the server, so a room left with one costs several times the CPU of a two-player room, and the bills below do not include that. Transfer scales with the state: a state twice the size nearly doubles it.

## Calculated: monthly cost

Prices were checked on 2026-11-02, excluding VAT, from [Hetzner cost-optimized cloud](https://www.hetzner.com/cloud/cost-optimized/), [AWS Lightsail pricing](https://aws.amazon.com/lightsail/pricing/) and [Cloudflare Realtime TURN](https://developers.cloudflare.com/realtime/turn/). Euro and dollar figures are not converted.

| Service | Price used |
|---|---|
| Hetzner CX23 | EUR 5.99 a month; 2 vCPU, 4 GB; 20 TB included for servers in the EU, then EUR 1.00 per TB at EU-Central locations. The page gives no traffic terms for its other locations |
| AWS Lightsail | USD 12 a month; 2 vCPU, 2 GB; 3 TB included, counting inbound and outbound, and half that in six listed regions; outbound beyond it at USD 0.09 per GB in the page's US East (Ohio) example |
| Cloudflare Realtime TURN | USD 0.05 per GB outbound to clients; no server to run |

Assumptions, named in `cost.py`: peak rooms are ten times the average, one process runs per server, a server's price divides evenly between compute and memory, and every room has two connected seats. Compute and memory below are what rooms actually use of that price; fixed is the rest, paid whether used or not. Transfer stayed inside the included allowance in every row, with Lightsail's inbound counted.

One small server covers the calculated range only up to 10,000 room-hours a month, or 100,000 at the higher capacity. At 100,000 the lower capacity needs 3 servers, and at 1,000,000 it takes 7 to 28.

Dedicated server on Hetzner CX23, EUR a month, at the higher and lower capacity:

| Room-hours | Peak rooms | Servers | Fixed | Compute | Memory | Transfer | Total | GB sent |
|---|---|---|---|---|---|---|---|---|
| 100 | 1.4 | 1 | 5.99 | 0.00 | 0.00 | 0.00 | 5.99 | 1.3 |
| 1,000 | 13.7 | 1 | 5.99 | 0.00 | 0.00 | 0.00 | 5.99 | 12.6 |
| 10,000 | 137 | 1 | 5.97 to 5.99 | 0.00 to 0.02 | 0.00 | 0.00 | 5.99 | 125.9 |
| 100,000 | 1,370 | 1 to 3 | 5.95 to 17.82 | 0.04 to 0.15 | 0.00 | 0.00 | 5.99 to 17.97 | 1,259.4 |
| 1,000,000 | 13,699 | 7 to 28 | 41.52 to 166.18 | 0.38 to 1.52 | 0.03 | 0.00 | 41.93 to 167.72 | 12,594 |

Totals for the other options a month, at the higher and lower capacity:

| Room-hours | Dedicated, Lightsail (USD) | Own relay, Hetzner (EUR) | Own relay, Lightsail (USD) | TURN, every room relayed (USD) |
|---|---|---|---|---|
| 100 | 12 | 5.99 | 12 | 0.04 |
| 1,000 | 12 | 5.99 | 12 | 0.38 |
| 10,000 | 12 | 5.99 | 12 | 3.82 |
| 100,000 | 12 to 36 | 5.99 to 17.97 | 12 to 36 | 38.19 |
| 1,000,000 | 84 to 336 | 41.93 to 167.72 | 84 to 336 | 381.88 |

- A direct player-host room costs us nothing to run, and each relayed room costs as in the last three columns. Any model still needs some way for players to find each other, which is not priced here.
- Each extra region repeats the server line; regions were not compared.
- The Hetzner page showed these plans as unavailable when fetched, and renders its traffic overage as "per hour", so confirm both before relying on them. No row reaches the overage.
- The Lightsail page names no region for its bundle prices.
- Cloudflare's page bills outbound traffic. The TURN column applies that to the relay's measured send rate and leaves out TURN's own framing.
- The AWS EC2 pages fetched showed no instance price, so EC2 is not priced.
- Steam Datagram Relay's page states no fee and requires a version shipping on Steam for players on other stores. No official Epic Online Services price was found. Neither is priced here.

## Host succession

Implemented in this lab only, for a guest left alone, and no release design is implied. Every state carries the two inputs and the controllers of its step, 5 bytes that all models send so the comparison stays equal. The guest keeps them, and when the host goes it rebuilds the room from the seed, replays the log, compares the result with the last state received, and plays on with AI in the host's seat.

Replay is needed because the public snapshots cannot restore a run: they omit movement remainders, world repair history, and `Workshop`'s and `Escape`'s random states, and the rules have no restore command.

Calculated: a run of 30 full 45-second days is 450 steps a day and 13,500 steps, so a 67.5 KB log. Replaying it takes 0.07 s at the measured key-seat rate and 2.3 s at the measured AI-seat rate, and a real log lies between them according to how many steps had an AI seat. That is added to the detection time.

A release would still need:

- Delivery of the log over a lossy network, since one missing step refuses the replay. The lab has no retransmission, and detects gaps only.
- A silence timeout chosen from real network data.
- A way for the departed host to return, and for a guest to rejoin at all.
- Replay fitted into the Godot client, with both players' builds on identical rules.
- A save or restore in `survival`, if a run should outlive both players.

## Unverified

- Direct connection between different networks, and any NAT traversal.
- Any real relay. Only the lab's own relay on loopback ran, and its own loss was not run.
- Latency, loss and jitter on a real network, and how the unreliable state stream behaves under them.
- How often a server is lost, and how often a host quits or crashes.
- Godot's ENet or any transport other than plain UDP, its overhead, and a Godot client.
- State size in human play. Bots hold what the AI collects, and a person may carry more.
- Cloud CPU speed, and more than one process or core.
- Where the room limits actually lie.
- A server's capacity with AI seats, which was calculated and not run.
- Repeatability: the tables record one run, on a shared machine.
