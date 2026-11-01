#!/usr/bin/env python3
"""Calculates capacity and monthly cost from measured.json and pricing.json.

Everything this prints is calculated. Its measured inputs are printed first, and
each assumption below is a named constant, so a reader can change it and rerun.
"""

import argparse
import json
import math
from pathlib import Path

LAB_DIR = Path(__file__).resolve().parent
IP_UDP_HEADER_BYTES = 28
HOURS_PER_MONTH = 730
TICKS_PER_SECOND = 10
BYTES_PER_GB = 1e9
KIB_PER_GIB = 1024**2

# Assumptions, none measured.
ROOM_HOURS_PER_MONTH = [100, 1_000, 10_000, 100_000, 1_000_000]
AVERAGE_TO_PEAK_ROOMS = 0.1
COMPUTE_SHARE_OF_SERVER_PRICE = 0.5
USABLE_SHARE_OF_LAST_HELD_RUNG = [1.0, 0.25]


def wire_rate(report, seconds):
    """Bytes per second sent by one process, IP and UDP headers included."""
    return (report["sent_bytes"] + report["sent_packets"] * IP_UDP_HEADER_BYTES) / seconds


def session_seconds(authority):
    return authority["ticks"] / TICKS_PER_SECOND


def per_room(ladder):
    """Reads one process's cost per room from the highest rung that held."""
    held = [step for step in ladder if step["held"]]
    first, last = held[0], held[-1]
    grown = last["VmHWM"] - first["VmHWM"]
    return {
        "rooms_held_on_one_process": last["rooms"],
        "failed_rung": next((s["rooms"] for s in ladder if not s["held"]), None),
        "core_share_per_room": last["cpu_percent_of_one_core"] / 100 / last["rooms"],
        "memory_kib_per_room": grown / max(last["rooms"] - first["rooms"], 1),
        "process_base_kib": first["VmHWM"],
    }


def server_bill(room_hours, gb_per_room_hour, resources, server, usable_share):
    """Splits one month's bill into compute, memory, transfer and idle fixed."""
    usable_rooms = resources["rooms_held_on_one_process"] * usable_share
    average_rooms = room_hours / HOURS_PER_MONTH
    peak_rooms = average_rooms / AVERAGE_TO_PEAK_ROOMS
    servers = max(math.ceil(peak_rooms / usable_rooms), 1)
    servers_price = servers * server["monthly"]
    hourly = server["monthly"] / HOURS_PER_MONTH
    compute = (
        room_hours
        * resources["core_share_per_room"]
        / usable_share
        / server["vcpus"]
        * hourly
        * COMPUTE_SHARE_OF_SERVER_PRICE
    )
    memory = (
        room_hours
        * resources["memory_kib_per_room"]
        / KIB_PER_GIB
        / server["memory_gib"]
        * hourly
        * (1 - COMPUTE_SHARE_OF_SERVER_PRICE)
    )
    gigabytes = room_hours * gb_per_room_hour
    overage = max(gigabytes - servers * server["included_gb"], 0)
    return {
        "room_hours": room_hours,
        "peak_rooms": round(peak_rooms, 1),
        "servers": servers,
        "fixed": round(max(servers_price - compute - memory, 0), 2),
        "compute": round(compute, 2),
        "memory": round(memory, 2),
        "transfer": round(overage * server["overage_per_gb"], 2),
        "total": round(servers_price + overage * server["overage_per_gb"], 2),
        "per_room_hour": round(
            (servers_price + overage * server["overage_per_gb"]) / room_hours, 5
        ),
        "gigabytes": round(gigabytes, 1),
    }


def calculate(measured, pricing):
    dedicated = measured["normal_play"]["dedicated"]
    relayed = measured["normal_play"]["host-relay"]
    direct = measured["normal_play"]["host-direct"]
    dedicated_seconds = session_seconds(dedicated["authority"])
    relay_seconds = session_seconds(relayed["authority"])
    direct_seconds = session_seconds(direct["authority"])
    rates = {
        "dedicated_server_egress_bytes_per_second": wire_rate(
            dedicated["authority"], dedicated_seconds
        ),
        "relay_egress_bytes_per_second": wire_rate(relayed["relay"], relay_seconds),
        "player_host_uplink_bytes_per_second": wire_rate(
            direct["authority"], direct_seconds
        ),
        "guest_uplink_bytes_per_second": wire_rate(
            direct["clients"][0], direct_seconds
        ),
    }
    gb_per_hour = {
        key: rate * 3600 / BYTES_PER_GB
        for key, rate in rates.items()
    }
    resources = {
        "dedicated": per_room(measured["load"]["dedicated"]),
        "relay": per_room(measured["load"]["relay"]),
    }
    bills = {}
    for model, gb_key in (
        ("dedicated", "dedicated_server_egress_bytes_per_second"),
        ("relay", "relay_egress_bytes_per_second"),
    ):
        for server in pricing["servers"]:
            for usable_share in USABLE_SHARE_OF_LAST_HELD_RUNG:
                bills[f"{model} on {server['name']} at {usable_share}"] = {
                    "currency": server["currency"],
                    "rows": [
                        server_bill(
                            hours, gb_per_hour[gb_key], resources[model], server, usable_share
                        )
                        for hours in ROOM_HOURS_PER_MONTH
                    ],
                }
    metered = pricing["metered_relay"]
    turn_rows = [
        {
            "room_hours": hours,
            "gigabytes": round(
                hours * gb_per_hour["relay_egress_bytes_per_second"], 1
            ),
            "transfer": round(
                hours
                * gb_per_hour["relay_egress_bytes_per_second"]
                * metered["outbound_per_gb"],
                2,
            ),
        }
        for hours in ROOM_HOURS_PER_MONTH
    ]
    return {
        "measured_inputs": {"rates": rates, "resources": resources},
        "gb_per_room_hour": gb_per_hour,
        "bills": bills,
        "metered_relay": {
            "name": metered["name"],
            "currency": metered["currency"],
            "rows": turn_rows,
        },
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--measured", type=Path, default=LAB_DIR / "results" / "measured.json"
    )
    parser.add_argument("--pricing", type=Path, default=LAB_DIR / "pricing.json")
    arguments = parser.parse_args()
    result = calculate(
        json.loads(arguments.measured.read_text()),
        json.loads(arguments.pricing.read_text()),
    )
    print(json.dumps(result, indent=1))


if __name__ == "__main__":
    main()
