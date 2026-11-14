# SPDX-License-Identifier: MIT
# Triad Drift - import builder: the real pilot against the Cinder anchor tower's collision, in the running game
# (Tools/td-play-mcp.ps1 on L_Planet_Cinder first).  python anchor_lap.py <port>
#   1. probe: from 10.5 m out, walk straight at the tower from 16 directions (real W key): where does the plinth stop him?
#   2. lap: hugging the plinth (W, heading 30 deg inward of the tangent), all the way round.
import importlib.util, json, math, os, re, sys, time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
spec = importlib.util.spec_from_file_location("td_mcp", os.path.join(REPO, "Tools", "td_mcp.py"))
td = importlib.util.module_from_spec(spec); spec.loader.exec_module(td)
PORT = int(sys.argv[1])
LOG = os.path.join(REPO, "Saved", "Logs", "td-play-mcp_%d.log" % PORT)
client = td.McpClient("http://127.0.0.1:%d/mcp" % PORT, 180); client.connect()
TX, TY = 12600.0, -5000.0          # the tower (course-local = world on L_Planet_Cinder)


def cmd(c):
    client.tool("call_tool", {"toolset_name": "ConsoleToolset.ConsoleToolset", "tool_name": "RunConsoleCommand", "arguments": {"command": c}})


def where():
    def lines():
        with open(LOG, encoding="utf-8", errors="replace") as fh:
            return [l for l in fh if "TD.Where pos=" in l]
    n = len(lines())
    cmd("TD.Ground.Where")
    for _ in range(40):                 # wait for THIS answer (the log is flushed late): never a stale line
        time.sleep(0.1)
        got = lines()
        if len(got) > n:
            x, y, z = [float(v) for v in re.search(r"pos=\(([-\d.]+) ([-\d.]+) ([-\d.]+)\)", got[-1]).groups()]
            return x, y, z
    raise RuntimeError("no TD.Where answer")


def polar(x, y):
    return math.hypot(x - TX, y - TY), math.degrees(math.atan2(y - TY, x - TX))


if "--stage" in sys.argv:
    cmd("TD.Stage CinderOnFoot"); time.sleep(15)
out = {"probe": [], "lap": []}
for i in range(16):
    a = i * 22.5
    cmd("TD.Ground.Teleport %.0f %.0f 250" % (TX + 1050 * math.cos(math.radians(a)), TY + 1050 * math.sin(math.radians(a))))
    time.sleep(1.2)
    cmd("TD.Look %.1f 0" % (a + 180)); cmd("TD.Key W 3"); time.sleep(3.4)
    x, y, z = where(); r, ang = polar(x, y)
    out["probe"].append({"from_deg": a, "stopped_r_cm": round(r), "z": round(z)})
    print("probe %5.1f deg: stopped at r %.0f cm (z %.0f)" % (a, r, z)); sys.stdout.flush()
x, y, z = where(); r, ang = polar(x, y)
total, prev, stuck = 0.0, ang, 0
for step in range(140):
    cmd("TD.Look %.1f 0" % (ang + 90 + 30)); cmd("TD.Key W 0.6"); time.sleep(0.75)
    x, y, z = where(); r, ang = polar(x, y)
    d = (ang - prev + 180) % 360 - 180
    total += d; prev = ang
    stuck = stuck + 1 if abs(d) < 0.5 else 0
    out["lap"].append({"deg": round(ang, 1), "r_cm": round(r), "z": round(z)})
    if total >= 365 or stuck >= 6:
        break
rs = [p["r_cm"] for p in out["lap"]]
out["lap_total_deg"] = round(total, 1); out["lap_r_min_max_cm"] = [min(rs), max(rs)]; out["lap_steps"] = len(rs)
print("lap: %.0f deg in %d steps, r %d..%d cm, %s" % (total, len(rs), min(rs), max(rs), "COMPLETE" if total >= 360 else "STUCK at %.0f deg" % prev))
with open(os.path.join(REPO, "ArtSource", "EngImport", "CinderAnchor", "lap_report.json"), "w", encoding="utf-8") as fh:
    json.dump(out, fh, indent=1)
