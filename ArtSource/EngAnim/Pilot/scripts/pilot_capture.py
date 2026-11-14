# Triad Drift - pilot on-foot move strips in third person (game running on TD_PORT, pilot on foot, TD.God on).
# Slows the world (slomo) so each move gets many frames, triggers the move with real key presses (TD.Key, game
# seconds) and captures from the gameplay chase position (behind-right, 320 cm arm, FOV 85) and a side camera.
#   python pilot_capture.py <outdir> <move>[,<move>...]        moves: see MOVES below; "all" = every move
# Env: TD_PORT (default 21085), TD_VIEWS (default "chase,side"), TD_W / TD_H (default 640x360).
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

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", ".."))
PORT = int(os.environ.get("TD_PORT", "21085"))
LOG = os.path.join(ROOT, "Saved", "Logs", "td-play-mcp_%d.log" % PORT)
WHERE = re.compile(r"TD\.Where pos=\(([-\d.]+) ([-\d.]+) ([-\d.]+)\) yaw=([-\d.]+)")

# move: (slomo, real seconds to capture, [(real s, console command)], pre-commands at full speed)
MOVES = {
    "idle":    (1.0, 3.0, [], []),
    "walk":    (0.35, 3.0, [(0.0, "TD.Key W 1.2 0.45")], []),
    "run":     (0.35, 3.0, [(0.0, "TD.Key W 1.2")], []),
    "strafe":  (0.35, 3.0, [(0.0, "TD.Key D 1.2")], []),
    "turn":    (0.35, 3.0, [(0.0, "TD.Key S 0.25"), (0.9, "TD.Key W 0.6")], []),
    "stop":    (0.35, 3.0, [(0.0, "TD.Key W 0.25")], [("TD.Key W 1.0", 1.0)]),
    "light123": (0.08, 9.5, [(0.0, "TD.Key LeftMouseButton"), (2.0, "TD.Key LeftMouseButton"), (5.0, "TD.Key LeftMouseButton")], []),
    "light_run": (0.1, 7.0, [(0.0, "TD.Key W 0.7"), (0.4, "TD.Key LeftMouseButton"), (2.6, "TD.Key LeftMouseButton")], []),
    "heavy":   (0.1, 8.0, [(0.0, "TD.Key LeftMouseButton 0.45")], []),
    "airslam": (0.15, 8.0, [(0.0, "TD.Key SpaceBar"), (0.9, "TD.Key LeftMouseButton")], []),
    "block":   (0.2, 4.0, [(0.0, "TD.Key RightMouseButton 0.7")], []),
    "block_move": (0.3, 4.0, [(0.0, "TD.Key RightMouseButton 1.1"), (0.1, "TD.Key A 1.0")], []),
    "hitreact": (0.3, 8.0, [], [("TD.Ground.Spawn Trooper 900", 2.0)]),
}


def main():
    out, names = sys.argv[1], sys.argv[2]
    names = list(MOVES) if names == "all" else names.split(",")
    views = os.environ.get("TD_VIEWS", "chase,side").split(",")
    W, H = int(os.environ.get("TD_W", "640")), int(os.environ.get("TD_H", "360"))
    os.makedirs(out, exist_ok=True)
    c = td_mcp.McpClient("http://127.0.0.1:%d/mcp" % PORT, 180)
    c.connect()

    def call(toolset, tool, args=None):
        return c.tool("call_tool", {"toolset_name": toolset, "tool_name": tool, "arguments": args or {}})

    def cmd(s):
        call("ConsoleToolset.ConsoleToolset", "RunConsoleCommand", {"command": s})

    def where():
        size = os.path.getsize(LOG)
        cmd("TD.Ground.Where")
        time.sleep(0.3)
        with open(LOG, "r", errors="replace") as f:
            f.seek(size)
            m = None
            for m in WHERE.finditer(f.read()):
                pass
        return [float(m.group(i)) for i in range(1, 5)] if m else None

    world = None
    for b in td_mcp.unwrap(call("SceneCaptureToolset.SceneCaptureToolset", "ListWorlds"))["content"]:
        if isinstance(b, dict):
            for w in b.get("returnValue", []):
                if w.get("worldType") == "Game":
                    world = w["world"]["refPath"]

    def camera(p, yaw, view):
        f = (math.cos(math.radians(yaw)), math.sin(math.radians(yaw)))
        r = (-f[1], f[0])
        if view == "chase":   # the gameplay lens: 320 cm behind the shoulder pivot, 55 right, 65 up (+ eye 60)
            loc = (p[0] - f[0] * 320 + r[0] * 55, p[1] - f[1] * 320 + r[1] * 55, p[2] + 125)
            aim, fov = (p[0] + f[0] * 300, p[1] + f[1] * 300, p[2] + 60), 85.0
        elif view == "side":  # the pilot's right side, chest height
            loc = (p[0] + r[0] * 380 + f[0] * 40, p[1] + r[1] * 380 + f[1] * 40, p[2] + 30)
            aim, fov = (p[0] + f[0] * 40, p[1] + f[1] * 40, p[2]), 55.0
        else:                 # front three-quarter
            loc = (p[0] + f[0] * 330 + r[0] * 200, p[1] + f[1] * 330 + r[1] * 200, p[2] + 40)
            aim, fov = (p[0], p[1], p[2]), 55.0
        d = [aim[i] - loc[i] for i in range(3)]
        return {"location": {"x": loc[0], "y": loc[1], "z": loc[2]},
                "rotation": {"pitch": math.degrees(math.atan2(d[2], math.hypot(d[0], d[1]))), "yaw": math.degrees(math.atan2(d[1], d[0])), "roll": 0.0},
                "projectionMode": "Perspective", "fOV": fov, "orthoWidth": 512}

    if os.environ.get("TD_PASS") is not None:  # A/B: TD.Pilot.MeleePass 0 (before) or 1 (after)
        cmd("TD.Pilot.MeleePass %s" % os.environ["TD_PASS"])
    report = {}
    for name in names:
        slomo, secs, todo, pre = MOVES[name]
        cmd("slomo 1")
        cmd("TD.Look 0 -10")
        time.sleep(0.6)
        for pc, wait in pre:
            cmd(pc)
            time.sleep(wait * 0.6)
        p = where()
        if not p:
            print("no pilot")
            return 1
        cams = {v: camera(p, 0.0, v) for v in views}
        cmd("slomo %.3f" % slomo)
        todo = sorted(todo)
        t0 = time.time()
        k = 0
        while time.time() - t0 < secs:
            while todo and time.time() - t0 >= todo[0][0]:
                cmd(todo.pop(0)[1])
            for v in views:
                r = call("SceneCaptureToolset.SceneCaptureToolset", "Capture", {"world": {"refPath": world}, "camera": cams[v], "mode": "Lit",
                         "options": {"width": W, "height": H, "screenPercentage": 1.0, "visualizationSubMode": ""}})
                for blk in r.get("content", []) or []:
                    if blk.get("type") == "image" and blk.get("data"):
                        open(os.path.join(out, "%s_%s_%03d.png" % (name, v, k)), "wb").write(base64.b64decode(blk["data"]))
            k += 1
        cmd("slomo 1")
        report[name] = {"frames": k, "slomo": slomo, "real_s": secs, "pos": p}
        print("%s: %d frames" % (name, k))
        time.sleep(1.0)
    json.dump(report, open(os.path.join(out, "capture.json"), "w"), indent=1)
    return 0


if __name__ == "__main__":
    sys.exit(main())
