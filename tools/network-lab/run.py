#!/usr/bin/env python3
"""Runs every network-lab scenario on loopback in a private network namespace.

The namespace keeps other work off the interface, so the kernel's counters for
`lo` independently check the byte counts the lab reports about itself.
"""

import argparse
import json
import os
import platform
import re
import resource
import signal
import subprocess
import sys
import time
from pathlib import Path

LAB_DIR = Path(__file__).resolve().parent
LAB = LAB_DIR / "build" / "neocity-22-lab"
REPOSITORY = LAB_DIR.parent.parent

SERVER_PORT = 41000
RELAY_PORT = 41001
SEED = "7"
SESSION_SECONDS = 30
AFTER_DEPARTURE_SECONDS = 6
LOAD_SECONDS = 15
SUCCESSION_STEPS = "40"
ROOM_LADDER = [1, 10, 100, 500, 1000, 2000, 4000]
LOAD_PROCESSES = 8
HOST_PROCESSES = 4
LATENESS_LIMIT_US = 10_000
MINIMUM_TICK_RATE = 9.5
IP_UDP_HEADER_BYTES = 20 + 8
BYTE_COUNT_TOLERANCE = 0.01
TICKS_PER_SECOND = 10


def fail(message):
    raise SystemExit(f"run.py: {message}")


def cpu_seconds(pid):
    fields = Path(f"/proc/{pid}/stat").read_text().rsplit(")", 1)[1].split()
    return (int(fields[11]) + int(fields[12])) / os.sysconf("SC_CLK_TCK")


def memory_kib(pid):
    status = Path(f"/proc/{pid}/status").read_text()
    return {
        key: int(re.search(rf"{key}:\s+(\d+)", status).group(1))
        for key in ("VmRSS", "VmHWM")
    }


def loopback_counters():
    """Reads /proc, which follows the namespace; /sys shows the host's lo."""
    line = next(
        text
        for text in Path("/proc/net/dev").read_text().splitlines()
        if text.strip().startswith("lo:")
    )
    fields = line.split(":")[1].split()
    return {"tx_bytes": int(fields[8]), "tx_packets": int(fields[9])}


def udp_receive_errors():
    lines = [
        line.split()
        for line in Path("/proc/net/snmp").read_text().splitlines()
        if line.startswith("Udp:")
    ]
    return int(dict(zip(lines[0], lines[1]))["RcvbufErrors"])


class Processes:
    """Everything a scenario started, stopped on every way out."""

    def __init__(self):
        self.running = []

    def __enter__(self):
        return self

    def __exit__(self, *_):
        for process in self.running:
            if process.poll() is None:
                process.kill()
            process.communicate()

    def start(self, role, *arguments):
        process = subprocess.Popen(
            [LAB, role, *map(str, arguments)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        self.running.append(process)
        return process


def finish(process, stop_signal=signal.SIGTERM):
    """Stops a process and returns the JSON lines it printed."""
    if stop_signal is not None and process.poll() is None:
        process.send_signal(stop_signal)
    out, err = process.communicate(timeout=60)
    if stop_signal != signal.SIGKILL and process.returncode != 0:
        fail(f"{process.args[1]} exited {process.returncode}: {err}")
    return [json.loads(line) for line in out.splitlines() if line.startswith("{")]


def usage(process, cpu_before, seconds):
    return {
        "cpu_percent_of_one_core": round(
            100 * (cpu_seconds(process.pid) - cpu_before) / seconds, 2
        ),
        **memory_kib(process.pid),
    }


def run_args():
    return ("--seed", SEED)


def start_authority(processes, model, *extra, local="chef"):
    if model == "dedicated":
        return processes.start("server", "--port", SERVER_PORT, *run_args(), *extra)
    arguments = ["--port", SERVER_PORT, "--local", local, *run_args(), *extra]
    if model == "host-relay":
        arguments += ["--relay", RELAY_PORT]
    return processes.start("host", *arguments)


def client_port(model):
    return RELAY_PORT if model == "host-relay" else SERVER_PORT


def start_client(processes, model, actor, *extra):
    return processes.start(
        "client",
        "--port",
        client_port(model),
        "--actors",
        actor,
        *run_args(),
        *extra,
    )


def seats(model, local="chef"):
    if model == "dedicated":
        return ["chef", "appraiser"]
    return ["appraiser" if local == "chef" else "chef"]


def start_relay(processes, model):
    if model == "host-relay":
        return processes.start("relay", "--port", RELAY_PORT)
    return None


def normal_play(model):
    """Two players for a fixed time, with every process measured."""
    with Processes() as processes:
        relay = start_relay(processes, model)
        before = loopback_counters()
        authority = start_authority(processes, model)
        clients = [start_client(processes, model, seat) for seat in seats(model)]
        time.sleep(2)
        watched = {"authority": authority, "client_0": clients[0]}
        if relay:
            watched["relay"] = relay
        cpu_before = {name: cpu_seconds(p.pid) for name, p in watched.items()}
        time.sleep(SESSION_SECONDS)
        measured = {
            name: usage(process, cpu_before[name], SESSION_SECONDS)
            for name, process in watched.items()
        }
        client_reports = [finish(client)[0] for client in clients]
        authority_report = finish(authority)[0]
        relay_report = finish(relay)[0] if relay else None
        time.sleep(0.2)
        after = loopback_counters()

    reports = [authority_report, *client_reports]
    if relay_report:
        reports.append(relay_report)
    counted_packets = sum(r["sent_packets"] for r in reports)
    counted_bytes = sum(r["sent_bytes"] for r in reports)
    kernel_packets = after["tx_packets"] - before["tx_packets"]
    kernel_bytes = after["tx_bytes"] - before["tx_bytes"]
    framed_bytes = counted_bytes + counted_packets * IP_UDP_HEADER_BYTES
    if abs(kernel_bytes - framed_bytes) > BYTE_COUNT_TOLERANCE * kernel_bytes:
        fail(f"{model}: lo saw {kernel_bytes}, lab counted {framed_bytes}")
    if any(r["skipped_ticks"] or r["send_failures"] for r in client_reports):
        fail(f"{model}: a client lost ticks: {client_reports}")
    if authority_report["ticks"] < SESSION_SECONDS * TICKS_PER_SECOND:
        fail(f"{model}: only {authority_report['ticks']} ticks ran")
    if authority_report["final_escaped"]:
        fail(f"{model}: the run ended during the session")
    return {
        "seconds": SESSION_SECONDS,
        "usage": measured,
        "authority": authority_report,
        "clients": client_reports,
        "relay": relay_report,
        "kernel_lo": {
            "packets": kernel_packets,
            "bytes": kernel_bytes,
            "lab_counted_bytes_with_headers": framed_bytes,
        },
    }


def participant_leaves(model, stop_signal):
    """One remote player goes; the result is read from the room, not assumed."""
    with Processes() as processes:
        relay = start_relay(processes, model)
        authority = start_authority(processes, model)
        clients = [start_client(processes, model, seat) for seat in seats(model)]
        time.sleep(3)
        finish(clients[-1], stop_signal)
        time.sleep(AFTER_DEPARTURE_SECONDS)
        room = finish(authority)[0]
        remaining = finish(clients[0], None)[0] if len(clients) > 1 else None
        if relay:
            finish(relay)
    if room["departures"] != 1:
        fail(f"{model}: departure not seen: {room}")
    if room["final_escaped"]:
        fail(f"{model}: the run ended during the scenario")
    client_matches_room = remaining is None or (
        remaining["last_tick"] == room["final_tick"]
        and remaining["last_digest"] == room["final_digest"]
    )
    items_lost = max(room["carried_at_departure"] - room["carried_next_step"], 0)
    time_rewound = room["elapsed_next_step_us"] < room["elapsed_at_departure_us"]
    continues = (
        room["ticks"] >= (3 + AFTER_DEPARTURE_SECONDS - 1) * TICKS_PER_SECOND
        and room["remaining_progress_steps"] > 0
        and client_matches_room
    )
    if not continues or items_lost or time_rewound:
        fail(f"{model}: the remaining player did not continue intact: {room}")
    return {
        "remaining_player_continues": continues,
        "room_ticks": room["ticks"],
        "departure_tick": room["departure_tick"],
        "steps_where_the_remaining_player_progressed": room[
            "remaining_progress_steps"
        ],
        "client_matches_room_at_tick": remaining and room["final_tick"],
        "max_gap_seen_by_remaining_client_us": remaining and remaining["max_gap_us"],
        "tick_lateness_us": room["tick_lateness_us"],
        "items_lost_by_the_departed_actor": items_lost,
        "exploration_time_rewound": time_rewound,
    }


def authority_goes(model, stop_signal, succeed):
    """The authority goes; records what each remaining client could do."""
    progress = LAB_DIR / "build" / "progress.bin"
    extra = ("--succeed-steps", SUCCESSION_STEPS) if succeed else ()
    with Processes() as processes:
        relay = start_relay(processes, model)
        authority = start_authority(
            processes, model, "--progress", progress, local="appraiser"
        )
        clients = [
            start_client(processes, model, seat, *extra)
            for seat in seats(model, local="appraiser")
        ]
        time.sleep(4)
        authority_report = finish(authority, stop_signal)
        reports = [finish(client, None) for client in clients]
        if relay:
            finish(relay)
    computed_tick = int.from_bytes(progress.read_bytes(), "little")
    client_reports = [r for lines in reports for r in lines if r["role"] == "client"]
    successions = [r for lines in reports for r in lines if r["role"] == "succession"]
    last_tick = max(r["last_tick"] for r in client_reports)
    if succeed and not all(
        r.get("state_matches_last_received")
        and r["steps_where_the_player_progressed"] > 0
        for r in successions
    ):
        fail(f"{model}: succession failed: {successions}")
    return {
        "authority_reported": bool(authority_report),
        "computed_tick": computed_tick,
        "last_tick_a_client_received": last_tick,
        "ticks_computed_but_undelivered": computed_tick - last_tick,
        "client_endings": [r["ending"] for r in client_reports],
        "detect_us": [r["detect_us"] for r in client_reports],
        "succession": successions,
    }


def shares(rooms, parts):
    """Yields (first room, room count) for an even split over `parts`."""
    share, extra = divmod(rooms, parts)
    first = 0
    for index in range(parts):
        count = share + (1 if index < extra else 0)
        if count:
            yield first, count
        first += count


def start_load(processes, rooms, port, actors):
    return [
        processes.start(
            "client",
            "--port",
            port,
            "--first-room",
            first,
            "--rooms",
            count,
            "--actors",
            actors,
            "--timeout-ms",
            "5000",
            *run_args(),
        )
        for first, count in shares(rooms, LOAD_PROCESSES)
    ]


def start_relayed_hosts(processes, rooms):
    """Player hosts are separate machines, so they get their own CPU here too."""
    return [
        processes.start(
            "host",
            "--port",
            SERVER_PORT + 10 + index,
            "--local",
            "chef",
            "--relay",
            RELAY_PORT,
            "--first-room",
            first,
            "--rooms",
            count,
            "--timeout-ms",
            "5000",
            *run_args(),
        )
        for index, (first, count) in enumerate(shares(rooms, HOST_PROCESSES))
    ]


def merged_lateness(reports):
    return {
        key: max(r["tick_lateness_us"][key] for r in reports)
        for key in ("p50", "p95", "p99", "max")
    }


def load_step(model, rooms):
    """One rung of a room-count ladder, with the measured process isolated."""
    startup_drops_before = udp_receive_errors()
    load_average = os.getloadavg()[0]
    settle_seconds = 3 + rooms / 500
    with Processes() as processes:
        if model == "relay":
            measured = processes.start("relay", "--port", RELAY_PORT)
            authorities = start_relayed_hosts(processes, rooms)
            generators = start_load(processes, rooms, RELAY_PORT, "appraiser")
            others = [*authorities, *generators]
        else:
            measured = processes.start(
                "server",
                "--port",
                SERVER_PORT,
                "--rooms",
                rooms,
                "--timeout-ms",
                "5000",
                *run_args(),
            )
            authorities = [measured]
            generators = start_load(processes, rooms, SERVER_PORT, "chef,appraiser")
            others = generators
        time.sleep(settle_seconds)
        drops_before = udp_receive_errors()
        cpu_before = cpu_seconds(measured.pid)
        others_before = sum(cpu_seconds(p.pid) for p in others)
        time.sleep(LOAD_SECONDS)
        drops = udp_receive_errors() - drops_before
        result = usage(measured, cpu_before, LOAD_SECONDS)
        others_cpu = sum(cpu_seconds(p.pid) for p in others) - others_before
        client_reports = [finish(g)[0] for g in generators]
        authority_reports = [finish(a)[0] for a in authorities]
        relay_report = finish(measured)[0] if model == "relay" else None
    rate = sum(r["ticks"] for r in authority_reports) / rooms / (
        LOAD_SECONDS + settle_seconds
    )
    lateness = merged_lateness(authority_reports)
    skipped = sum(r["skipped_ticks"] for r in client_reports)
    counted = relay_report or authority_reports[0]
    return {
        "rooms": rooms,
        "machine_load_average_before": round(load_average, 2),
        **result,
        "other_lab_processes_cpu_percent_of_one_core": round(
            100 * others_cpu / LOAD_SECONDS, 1
        ),
        "ticks_per_room_second": round(rate, 2),
        "worst_tick_lateness_us": lateness,
        "worst_client_state_latency_p99_us": max(
            r["state_latency_us"]["p99"] for r in client_reports
        ),
        "udp_receive_buffer_drops_while_settled": drops,
        "udp_receive_buffer_drops_at_startup": drops_before - startup_drops_before,
        "skipped_ticks": skipped,
        "state_max_bytes": max(r["state_max_bytes"] for r in authority_reports),
        "sent_bytes": counted["sent_bytes"],
        "sent_packets": counted["sent_packets"],
        "received_bytes": counted["received_bytes"],
        "received_packets": counted["received_packets"],
        "held": (
            drops == 0
            and skipped == 0
            and rate >= MINIMUM_TICK_RATE
            and (model == "relay" or lateness["p99"] <= LATENESS_LIMIT_US)
        ),
    }


def load_ladder(model):
    steps = []
    for rooms in ROOM_LADDER:
        steps.append(load_step(model, rooms))
        print(f"  {model} {rooms} rooms: {steps[-1]['held']}", flush=True)
        if not steps[-1]["held"]:
            break
    return steps


def godot_baseline(godot):
    """A headless Godot of the real project at its menu, without any network."""
    if godot is None:
        return "not run: no --godot given"
    with subprocess.Popen(
        [godot, "--headless", "--path", REPOSITORY / "project"],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    ) as process:
        try:
            time.sleep(4)
            cpu_before = cpu_seconds(process.pid)
            time.sleep(10)
            return usage(process, cpu_before, 10)
        finally:
            process.kill()


def environment():
    compiler = subprocess.run(
        ["c++", "--version"], capture_output=True, text=True, check=True
    )
    head = subprocess.run(
        ["git", "-C", LAB_DIR, "rev-parse", "HEAD"],
        capture_output=True,
        text=True,
        check=True,
    )
    changes = subprocess.run(
        [
            "git",
            "-C",
            REPOSITORY,
            "status",
            "--porcelain",
            "--",
            "src",
            "tools",
            ":!tools/network-lab/results",
        ],
        capture_output=True,
        text=True,
        check=True,
    )
    cpu = re.search(r"model name\s+: (.+)", Path("/proc/cpuinfo").read_text())
    return {
        "date": time.strftime("%Y-%m-%d"),
        "kernel": platform.release(),
        "cpu": cpu.group(1),
        "logical_cpus": os.cpu_count(),
        "memory_kib": int(
            re.search(r"MemTotal:\s+(\d+)", Path("/proc/meminfo").read_text()).group(1)
        ),
        "compiler": compiler.stdout.splitlines()[0],
        "build": "CMake Release (-O3 -DNDEBUG)",
        "commit": head.stdout.strip(),
        "tree_differs_from_commit": bool(changes.stdout.strip()),
        "network": "loopback in a private network namespace; one machine",
        "tick_and_send_rate_hz": TICKS_PER_SECOND,
    }


def stop_on_signal(number, _frame):
    """Unwinds through `Processes`, which stops everything a scenario started."""
    sys.exit(128 + number)


def measure(godot, out):
    signal.signal(signal.SIGTERM, stop_on_signal)
    signal.signal(signal.SIGINT, stop_on_signal)
    resource.setrlimit(
        resource.RLIMIT_NOFILE, (resource.getrlimit(resource.RLIMIT_NOFILE)[1],) * 2
    )
    subprocess.run(["ip", "link", "set", "lo", "up"], check=True)
    sizes = subprocess.run(
        [LAB, "sizes", "--seed", SEED],
        capture_output=True,
        text=True,
        check=True,
    )
    results = {"environment": environment(), "sizes": json.loads(sizes.stdout)}
    models = ["dedicated", "host-direct", "host-relay"]
    results["normal_play"] = {model: normal_play(model) for model in models}
    print("normal play done", flush=True)
    results["participant_leaves"] = {
        model: {
            "graceful": participant_leaves(model, signal.SIGTERM),
            "forced": participant_leaves(model, signal.SIGKILL),
        }
        for model in models
    }
    print("participant departures done", flush=True)
    results["authority_goes"] = {
        "dedicated": {
            "graceful": authority_goes("dedicated", signal.SIGTERM, False),
            "forced": authority_goes("dedicated", signal.SIGKILL, False),
        },
        "host-direct-no-succession": {
            "graceful": authority_goes("host-direct", signal.SIGTERM, False),
            "forced": authority_goes("host-direct", signal.SIGKILL, False),
        },
        "host-direct-succession": {
            "graceful": authority_goes("host-direct", signal.SIGTERM, True),
            "forced": authority_goes("host-direct", signal.SIGKILL, True),
        },
        "host-relay-succession": {
            "graceful": authority_goes("host-relay", signal.SIGTERM, True),
            "forced": authority_goes("host-relay", signal.SIGKILL, True),
        },
    }
    print("authority departures done", flush=True)
    results["load"] = {
        "dedicated": load_ladder("dedicated"),
        "relay": load_ladder("relay"),
    }
    results["godot_headless_menu"] = godot_baseline(godot)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(results, indent=1) + "\n")
    print(f"wrote {out}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--godot", help="Godot binary for the headless baseline")
    parser.add_argument(
        "--out", type=Path, default=LAB_DIR / "build" / "measured.json"
    )
    parser.add_argument("--inside-namespace", action="store_true", help=argparse.SUPPRESS)
    arguments = parser.parse_args()
    if not LAB.exists():
        fail(f"{LAB} is missing; build the lab first")
    if arguments.inside_namespace:
        measure(arguments.godot, arguments.out.resolve())
        return
    # Replaces this process with `unshare`, which replaces itself with the inner
    # run, so a signal sent to this pid reaches the inner handler.
    os.execvp(
        "unshare",
        [
            "unshare",
            "-Urn",
            sys.executable,
            __file__,
            "--inside-namespace",
            *sys.argv[1:],
        ],
    )


if __name__ == "__main__":
    main()
