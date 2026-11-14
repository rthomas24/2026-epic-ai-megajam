# Triad Drift - crew deaths as the game plays them: spawn a crew member, kill it with a real melee hit from the pilot
# (TD.Ground.CrewKill), and capture the fall from a fixed side camera at about 0.3 / 0.8 / 1.4 / 2.4 s, with where
# the body ends up (TD.Ground.CrewWhere: capsule, pelvis, head, lowest bone above the floor).
# python death_capture.py <outdir> <Type>[,<Type>...]        (game running on TD_PORT, pilot on foot)
# Animation builder (eng/anim), Nov 13.
import base64
import json
import math
import os
import re
import sys
import time

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "..", "Tools"))
import td_mcp  # noqa: E402

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", ".."))
PORT = int(os.environ.get("TD_PORT", "21081"))
LOG = os.path.join(ROOT, "Saved", "Logs", "td-play-mcp_%d.log" % PORT)
SPOTS = [(900, 900), (900, 3400), (3400, 900), (3400, 3400)]
PAT = re.compile(r"TD\.CrewAll (\S+) dead=(\d) loc=([-\d.,]+) x=([-\d.,]+) y=([-\d.,]+) pelvis=([-\d.,]+) head=([-\d.,]+) floor=([-\d.]+) low=([-\d.]+)")


def main():
    out, types = sys.argv[1], sys.argv[2].split(",")
    os.makedirs(out, exist_ok=True)
    c = td_mcp.McpClient("http://127.0.0.1:%d/mcp" % PORT, 180)
    c.connect()

    def call(toolset, tool, args=None):
        return c.tool("call_tool", {"toolset_name": toolset, "tool_name": tool, "arguments": args or {}})

    def cmd(s):
        call("ConsoleToolset.ConsoleToolset", "RunConsoleCommand", {"command": s})

    def where(near):
        size = os.path.getsize(LOG)
        cmd("TD.Ground.CrewWhere")
        time.sleep(0.3)
        with open(LOG, "r", errors="replace") as f:
            f.seek(max(0, size - 100))
            text = f.read()
        best = None
        for m in PAT.finditer(text):
            v = lambda i: [float(x) for x in m.group(i).split(",")]
            e = {"type": m.group(1), "dead": int(m.group(2)), "loc": v(3), "x": v(4), "y": v(5), "pelvis": v(6), "head": v(7),
                 "floor": float(m.group(8)), "low": float(m.group(9))}
            d = math.hypot(e["loc"][0] - near[0], e["loc"][1] - near[1])
            if best is None or d < best[0]:
                best = (d, e)
        return best[1] if best else None

    world = None
    for b in td_mcp.unwrap(call("SceneCaptureToolset.SceneCaptureToolset", "ListWorlds"))["content"]:
        if isinstance(b, dict):
            for w in b.get("returnValue", []):
                if w.get("worldType") == "Game":
                    world = w["world"]["refPath"]
    report = {}
    for i, typ in enumerate(types):
        spot = SPOTS[i % len(SPOTS)]
        cmd("TD.Ground.Teleport %d %d 300" % spot)
        time.sleep(1.0)
        cmd("TD.Ground.Spawn %s 600" % typ)
        time.sleep(1.2)
        e = where((spot[0] + 600, spot[1]))
        if not e:
            print("no crew", typ)
            continue
        loc, fx, ry = e["loc"], e["x"], e["y"]
        centre = [loc[k] - fx[k] * 80 for k in range(3)]   # a front hit throws the body back
        cam = [centre[k] + ry[k] * 720 + fx[k] * 150 for k in range(3)]
        cam[2] = e["floor"] + 110
        fwd = [centre[0] - cam[0], centre[1] - cam[1], (e["floor"] + 60) - cam[2]]
        camera = {"location": {"x": cam[0], "y": cam[1], "z": cam[2]},
                  "rotation": {"pitch": math.degrees(math.atan2(fwd[2], math.hypot(fwd[0], fwd[1]))), "yaw": math.degrees(math.atan2(fwd[1], fwd[0])), "roll": 0.0},
                  "projectionMode": "Perspective", "fOV": 45.0, "orthoWidth": 512}
        cmd("TD.Ground.CrewKill")
        t0 = time.time()
        shots = []
        for want in (0.3, 0.8, 1.4, 2.4):
            time.sleep(max(0.0, want - (time.time() - t0)))
            t = time.time() - t0
            r = call("SceneCaptureToolset.SceneCaptureToolset", "Capture", {"world": {"refPath": world}, "camera": camera, "mode": "Lit",
                     "options": {"width": 1600, "height": 900, "screenPercentage": 1.0, "visualizationSubMode": ""}})
            for blk in r.get("content", []) or []:
                if blk.get("type") == "image" and blk.get("data"):
                    p = os.path.join(out, "death_%s_%d.png" % (typ, len(shots)))
                    open(p, "wb").write(base64.b64decode(blk["data"]))
                    shots.append([round(t, 2), p])
        end = where(loc)
        report[typ] = {"spawn": e, "end": end, "shots": shots}
        if end:
            off = math.hypot(end["pelvis"][0] - end["loc"][0], end["pelvis"][1] - end["loc"][1])
            print("%s dead=%d: pelvis %.0f cm from the capsule, pelvis %.0f cm / head %.0f cm above the floor, lowest bone %.0f cm, shots at %s" % (
                typ, end["dead"], off, end["pelvis"][2] - end["floor"], end["head"][2] - end["floor"], end["low"], [s[0] for s in shots]))
    json.dump(report, open(os.path.join(out, "deaths.json"), "w"), indent=1)


if __name__ == "__main__":
    sys.exit(main())
