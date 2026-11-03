# Runs the command in argv[2:] and stops it once argv[1] seconds pass: prints the
# processes it still runs, sends SIGQUIT to each Godot among them so it dumps core
# where it is, kills the rest 20 seconds later, and fails with status 124.

import os
import signal
import subprocess
import sys

limit = int(sys.argv[1])
command = sys.argv[2:]
child = subprocess.Popen(command, start_new_session=True)
try:
    status = child.wait(limit)
except subprocess.TimeoutExpired:
    print(
        f"::error::{' '.join(command)} exceeded the {limit} s limit; "
        "the processes below were running, and nothing after them ran.",
        flush=True,
    )
    session = str(child.pid)
    subprocess.run(["ps", "-ww", "--forest", "-o", "pid,etime,args", "-s", session])
    subprocess.run(["pkill", "-QUIT", "-x", "-s", session, "godot"])
    try:
        child.wait(20)
    except subprocess.TimeoutExpired:
        pass
    try:
        os.killpg(child.pid, signal.SIGKILL)
    except ProcessLookupError:
        pass
    child.wait()
    sys.exit(124)
sys.exit(status if status >= 0 else 128 - status)
