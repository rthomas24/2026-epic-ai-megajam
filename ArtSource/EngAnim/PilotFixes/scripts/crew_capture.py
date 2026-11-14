# Triad Drift - captures of crew / Warden clips in a running game (Tools/td-play-mcp.ps1, pilot on foot).
# python crew_capture.py <outdir> <tag> <Type>:<Clip>:<t01>[,...]
# For each item: spawn that crew type 6 m in front of the pilot (once per type), hold the clip frozen at the fraction
# with TD.Ground.CrewHold, read where it stands (TD.CrewWhere in the game log) and render a front-quarter and a side
# view at 1600x900. Animation builder (eng/anim), Nov 13.
import base64
import json
import math
import os
import re
import sys
import time

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "..", "Tools"))
import td_mcp  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", "..", ".."))
PORT = int(os.environ.get("TD_PORT", "21081"))
LOG = os.path.join(ROOT, "Saved", "Logs", "td-play-mcp_%d.log" % PORT)


def main():
    out, tag, items = sys.argv[1], sys.argv[2], sys.argv[3].split(",")
    os.makedirs(out, exist_ok=True)
    c = td_mcp.McpClient("http://127.0.0.1:%d/mcp" % PORT, 180)
    c.connect()

    def call(toolset, tool, args=None):
        return c.tool("call_tool", {"toolset_name": toolset, "tool_name": tool, "arguments": args or {}})

    def cmd(s):
        call("ConsoleToolset.ConsoleToolset", "RunConsoleCommand", {"command": s})

    world = None
    for b in td_mcp.unwrap(call("SceneCaptureToolset.SceneCaptureToolset", "ListWorlds"))["content"]:
        if isinstance(b, dict):
            for w in b.get("returnValue", []):
                if w.get("worldType") == "Game":
                    world = w["world"]["refPath"]
    spawned = None
    for item in items:
        typ, clip, t01 = item.split(":")
        if typ != spawned:
            cmd("TD.Dev.DestroySpawned")
            cmd("TD.KillAll")
            time.sleep(0.5)
            cmd("TD.Ground.Spawn %s 600" % typ)
            spawned = typ
            time.sleep(1.5)
        size = os.path.getsize(LOG)
        cmd("TD.Ground.CrewHold %s %s" % (clip, t01))
        time.sleep(0.5)
        with open(LOG, "r", errors="replace") as f:
            f.seek(max(0, size - 200))
            text = f.read()
        m = re.search(r"TD\.CrewWhere (\S+) loc=([-\d.,]+) x=([-\d.,]+) y=([-\d.,]+) z?.*?h=([-\d.]+)", text)
        if not m:
            print("no crew for", item)
            continue
        loc = [float(v) for v in m.group(2).split(",")]
        fx = [float(v) for v in m.group(3).split(",")]
        ry = [float(v) for v in m.group(4).split(",")]
        h = float(m.group(5))
        centre = [loc[0], loc[1], loc[2]]
        dist = max(480.0, h * 5.5)
        for vname, d in (("front", [fx[k] * 0.85 + ry[k] * 0.5 for k in range(3)]), ("side", [ry[k] for k in range(3)])):
            n = math.sqrt(sum(v * v for v in d)); d = [v / n for v in d]
            cam = [centre[k] + d[k] * dist for k in range(3)]
            cam[2] += h * 0.15
            fwd = [centre[k] - cam[k] for k in range(3)]
            yaw = math.degrees(math.atan2(fwd[1], fwd[0])); pitch = math.degrees(math.atan2(fwd[2], math.hypot(fwd[0], fwd[1])))
            r = call("SceneCaptureToolset.SceneCaptureToolset", "Capture", {
                "world": {"refPath": world},
                "camera": {"location": {"x": cam[0], "y": cam[1], "z": cam[2]}, "rotation": {"pitch": pitch, "yaw": yaw, "roll": 0.0},
                           "projectionMode": "Perspective", "fOV": 40.0, "orthoWidth": 512},
                "mode": "Lit", "options": {"width": 1600, "height": 900, "screenPercentage": 1.0, "visualizationSubMode": ""}})
            for blk in r.get("content", []) or []:
                if blk.get("type") == "image" and blk.get("data"):
                    p = os.path.join(out, "%s_%s_%s_%s.png" % (tag, typ, clip, vname))
                    open(p, "wb").write(base64.b64decode(blk["data"]))
                    print(item, vname, p)
        cmd("TD.Ground.CrewHold off")
    cmd("TD.Dev.DestroySpawned")
    cmd("TD.KillAll")
    return 0


if __name__ == "__main__":
    sys.exit(main())
