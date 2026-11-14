# SPDX-License-Identifier: MIT
# Triad Drift - capture the three Fab props spawned by ue_look_fab.py (editor session on TD_PORT), one lit frame each.
#   python look_capture.py <outdir>
import base64
import math
import os
import re
import sys
import time

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "..", "Tools"))
import td_mcp  # noqa: E402

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", ".."))
PORT = int(os.environ.get("TD_PORT", "21437"))
LOG = os.path.join(ROOT, "Saved", "Logs", "td-play-mcp_%d.log" % PORT)
LOOK = re.compile(r"TDPY: LOOK (\w+) ([-\d.]+) ([-\d.]+) ([-\d.]+)")


def main():
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    looks = {}
    for _ in range(120):
        with open(LOG, "r", errors="replace") as f:
            looks = {m.group(1): [float(m.group(i)) for i in (2, 3, 4)] for m in LOOK.finditer(f.read())}
        if len(looks) == 3:
            break
        time.sleep(2)
    if len(looks) < 3:
        print("spawn script did not report", looks)
        return 1
    c = td_mcp.McpClient("http://127.0.0.1:%d/mcp" % PORT, 180)
    c.connect()

    def call(toolset, tool, args=None):
        return c.tool("call_tool", {"toolset_name": toolset, "tool_name": tool, "arguments": args or {}})

    world = None
    for b in td_mcp.unwrap(call("SceneCaptureToolset.SceneCaptureToolset", "ListWorlds"))["content"]:
        if isinstance(b, dict):
            for w in b.get("returnValue", []):
                if w.get("worldType") == "Editor":
                    world = w["world"]["refPath"]
    print("world", world)
    time.sleep(8)          # let Nanite, textures and lighting settle
    for name, (x, y, z) in looks.items():
        loc = (x - 430.0, y - 120.0, z + 170.0)
        aim = (x, y, z + 75.0)
        d = [aim[i] - loc[i] for i in range(3)]
        cam = {"location": {"x": loc[0], "y": loc[1], "z": loc[2]},
               "rotation": {"pitch": math.degrees(math.atan2(d[2], math.hypot(d[0], d[1]))), "yaw": math.degrees(math.atan2(d[1], d[0])), "roll": 0.0},
               "projectionMode": "Perspective", "fOV": 50.0, "orthoWidth": 512}
        for attempt in range(2):
            r = call("SceneCaptureToolset.SceneCaptureToolset", "Capture", {"world": {"refPath": world}, "camera": cam, "mode": "Lit",
                     "options": {"width": 1280, "height": 720, "screenPercentage": 1.0, "visualizationSubMode": ""}})
            for blk in r.get("content", []) or []:
                if blk.get("type") == "image" and blk.get("data"):
                    open(os.path.join(out, "Fab_%s_ingame.png" % name), "wb").write(base64.b64decode(blk["data"]))
            time.sleep(2)
        print(name, "captured")
    return 0


if __name__ == "__main__":
    sys.exit(main())
