# Triad Drift - crew hit timing at normal frame rate (no captures while it runs): spawn one crew type in front of the
# god-mode pilot, let it fight for N seconds, then print its td.Crew.LogHits lines (clip time of each damage event).
#   python crew_timing.py <Type> [seconds] [distance]      (game on TD_PORT, pilot on foot, td.Crew.LogHits 1)
# Animation builder (eng/anim), Nov 14.
import os
import re
import sys
import time

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "..", "Tools"))
import td_mcp  # noqa: E402

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", ".."))
PORT = int(os.environ.get("TD_PORT", "21085"))
LOG = os.path.join(ROOT, "Saved", "Logs", "td-play-mcp_%d.log" % PORT)
typ = sys.argv[1]
secs = float(sys.argv[2]) if len(sys.argv) > 2 else 12.0
dist = sys.argv[3] if len(sys.argv) > 3 else "350"
c = td_mcp.McpClient("http://127.0.0.1:%d/mcp" % PORT, 180)
c.connect()


def cmd(s):
    c.tool("call_tool", {"toolset_name": "ConsoleToolset.ConsoleToolset", "tool_name": "RunConsoleCommand", "arguments": {"command": s}})


cmd("TD.Dev.DestroySpawned")
cmd("td.Crew.LogHits 1")
size = os.path.getsize(LOG)
cmd("TD.Ground.Spawn %s %s" % (typ, dist))
time.sleep(secs)
cmd("TD.Dev.DestroySpawned")
time.sleep(0.5)
with open(LOG, "r", errors="replace") as f:
    f.seek(size)
    text = f.read()
for m in re.finditer(r"td\.Crew\.LogHits: (.*)", text):
    print(m.group(1)[:200])
