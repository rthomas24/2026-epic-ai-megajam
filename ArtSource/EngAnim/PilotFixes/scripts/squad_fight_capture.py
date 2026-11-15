# Triad Drift - a live three-person fight on foot (#209): the player and the two followers (ATDGroundAlly) against
# spawned crew, captured from 8 m to the side through a scene camera, plus the game viewport (chase camera).
# python squad_fight_capture.py <outdir> <tag>     (a running game session from Tools/td-play-mcp.ps1, on foot)
# env TD_PORT (21085), TD_SPAWN ("Grunt 900,Grunt 1000,Trooper 1100"), TD_SHOTS (frames, 10), TD_GAP (s between frames)
# Animation builder (eng/anim), Nov 14.
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
PORT = int(os.environ.get("TD_PORT", "21085"))
LOG = os.path.join(ROOT, "Saved", "Logs", "td-play-mcp_%d.log" % PORT)


def main():
    out, tag = sys.argv[1], sys.argv[2]
    os.makedirs(out, exist_ok=True)
    c = td_mcp.McpClient("http://127.0.0.1:%d/mcp" % PORT, 180)
    c.connect()

    def call(toolset, tool, args=None):
        return c.tool("call_tool", {"toolset_name": toolset, "tool_name": tool, "arguments": args or {}})

    def cmd(s):
        call("ConsoleToolset.ConsoleToolset", "RunConsoleCommand", {"command": s})

    def log_after(size, pat, wait=0.6):
        time.sleep(wait)
        with open(LOG, "r", errors="replace") as f:
            f.seek(size)
            return re.findall(pat, f.read())

    world = None
    for b in td_mcp.unwrap(call("SceneCaptureToolset.SceneCaptureToolset", "ListWorlds"))["content"]:
        if isinstance(b, dict):
            for w in b.get("returnValue", []):
                if w.get("worldType") == "Game":
                    world = w["world"]["refPath"]

    def save(r, name):
        for blk in r.get("content", []) or []:
            if blk.get("type") == "image" and blk.get("data"):
                p = os.path.join(out, name + ".png")
                open(p, "wb").write(base64.b64decode(blk["data"]))
                return p
        print("NO IMAGE", name, json.dumps(r)[:300])

    for s in os.environ.get("TD_PRE", "").split(";"):
        if s.strip():
            cmd(s.strip())
    time.sleep(2.0)
    cmd("TD.Dev.DestroySpawned")
    for s in os.environ.get("TD_SPAWN", "Grunt 900,Grunt 1000,Trooper 1100").split(","):
        cmd("TD.Ground.Spawn " + s.strip())
    time.sleep(0.3)
    cmd("TD.Input.Key W 1.2")   # run in, so the followers run in with the player and stop into the fight
    time.sleep(1.6)
    n, gap = int(os.environ.get("TD_SHOTS", "10")), float(os.environ.get("TD_GAP", "0.35"))
    # where the squad stands once it has run in: the log is flushed late, so read it once, then keep the camera fixed
    size = os.path.getsize(LOG)
    cmd("TD.Ground.AlliesDump")
    found = log_after(size, r"loc=V?\(?X=([-\d.]+) Y=([-\d.]+) Z=([-\d.]+)", 1.5)
    if not found and os.environ.get("TD_CENTER"):   # the game log can lag by seconds: a known fight centre instead
        found = [tuple(os.environ["TD_CENTER"].split(","))]
    print("squad at", found)
    for i in range(n):
        if i % 2 == 0:
            cmd("TD.Input.Key LeftMouseButton 0.05")
        save(call("SceneCaptureToolset.SceneCaptureToolset", "CaptureGameViewport", {"world": {"refPath": world}, "maxResolution": 1280}),
             "%s_chase_%02d" % (tag, i))
        if found:
            pts = [[float(v) for v in p] for p in found]
            cx = sum(p[0] for p in pts) / len(pts); cy = sum(p[1] for p in pts) / len(pts); cz = sum(p[2] for p in pts) / len(pts)
            yaw0 = float(os.environ.get("TD_SIDE_YAW", "90"))
            cam = [cx + 800.0 * math.cos(math.radians(yaw0)), cy + 800.0 * math.sin(math.radians(yaw0)), cz + 120.0]
            fwd = [cx - cam[0], cy - cam[1], cz - cam[2]]
            yaw = math.degrees(math.atan2(fwd[1], fwd[0])); pitch = math.degrees(math.atan2(fwd[2], math.hypot(fwd[0], fwd[1])))
            save(call("SceneCaptureToolset.SceneCaptureToolset", "Capture", {
                "world": {"refPath": world},
                "camera": {"location": {"x": cam[0], "y": cam[1], "z": cam[2]}, "rotation": {"pitch": pitch, "yaw": yaw, "roll": 0.0},
                           "projectionMode": "Perspective", "fOV": 50.0, "orthoWidth": 512},
                "mode": "Lit", "options": {"width": 1280, "height": 720, "screenPercentage": 1.0, "visualizationSubMode": ""}}),
                "%s_8m_%02d" % (tag, i))
        time.sleep(gap)
    return 0


if __name__ == "__main__":
    sys.exit(main())
