# Triad Drift - close-up captures of the pilot's right hand on the blade hilt in a running game.
# Start the game first:  .\Tools\td-play-mcp.ps1 -Map /Game/TriadDrift/Maps/L_Planet_Verdance -Port 21081 -MaxSeconds 900
# Then:  python ArtSource\EngAnim\PilotGrip\scripts\grip_capture.py <outdir> <tag> [--before]
#   --before: the hand as it was (grip layer off, hilt back on hand_r with the old by-eye offset)
# For each pose (idle, walk, light swing at mid-arc, block, ...) it holds the clip with TD.Ground.GripHold, reads the hand
# with TD.Ground.GripWhere from the game log, and renders two offscreen close-ups aimed at the hilt (back of the hand,
# palm side) at 1600x900 plus the chase camera view. Animation builder (eng/anim), Nov 13.
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
W, H = 1600, 900

POSES = [
    # name, clip, time01, upper
    ("idle", None, 0.0, False),
    ("walk", "A_Pilot_Walk", 0.3, False),
    ("run", "A_Pilot_Run", 0.3, False),
    ("light1_mid", "A_Pilot_Light1", 0.5, False),
    ("light2_mid", "A_Pilot_Light2", 0.5, False),
    ("light3_mid", "A_Pilot_Light3", 0.5, False),
    ("heavy_mid", "A_Pilot_Heavy", 0.5, False),
    ("block", "A_Pilot_Block", 0.5, True),
    ("dash", "A_Pilot_Dash", 0.5, False),
    ("jump", "A_Pilot_JumpStart", 0.6, False),
]


def v(s):
    return tuple(float(x) for x in s.split(","))


def sub(a, b): return (a[0] - b[0], a[1] - b[1], a[2] - b[2])
def add(a, b): return (a[0] + b[0], a[1] + b[1], a[2] + b[2])
def mul(a, k): return (a[0] * k, a[1] * k, a[2] * k)
def dot(a, b): return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]
def cross(a, b): return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def norm(a):
    n = math.sqrt(dot(a, a))
    return mul(a, 1.0 / n) if n > 1e-9 else a


def look_rot(fwd):
    """Rotator (pitch, yaw, roll) looking along fwd."""
    f = norm(fwd)
    yaw = math.degrees(math.atan2(f[1], f[0]))
    pitch = math.degrees(math.asin(max(-1.0, min(1.0, f[2]))))
    return {"pitch": pitch, "yaw": yaw, "roll": 0.0}


def main():
    out = sys.argv[1]
    tag = sys.argv[2]
    before = "--before" in sys.argv
    only = [a[7:] for a in sys.argv if a.startswith("--only=")]
    os.makedirs(out, exist_ok=True)
    c = td_mcp.McpClient("http://127.0.0.1:%d/mcp" % PORT, 180)
    c.connect()

    def call(toolset, tool, args=None):
        return c.tool("call_tool", {"toolset_name": toolset, "tool_name": tool, "arguments": args or {}})

    def cmd(s):
        r = call("ConsoleToolset.ConsoleToolset", "RunConsoleCommand", {"command": s})
        if r.get("isError"):
            print("ERR", s, json.dumps(td_mcp.unwrap(r))[:300])

    worlds = td_mcp.unwrap(call("SceneCaptureToolset.SceneCaptureToolset", "ListWorlds"))
    world = None
    for b in worlds["content"]:
        if isinstance(b, dict):
            for w in b.get("returnValue", []):
                if w.get("worldType") == "Game":
                    world = w["world"]["refPath"]
    print("world", world)

    def save(r, name):
        for b in r.get("content", []) or []:
            if b.get("type") == "image" and b.get("data"):
                p = os.path.join(out, name + ".png")
                open(p, "wb").write(base64.b64decode(b["data"]))
                return p
        print("no image for", name, json.dumps(td_mcp.unwrap(r))[:400])

    def where():
        size = os.path.getsize(LOG)
        cmd("TD.Ground.GripWhere")
        time.sleep(0.3)
        with open(LOG, "r", errors="replace") as f:
            f.seek(max(0, size - 200))
            text = f.read()
        bones = {}
        for m in re.finditer(r"TD\.GripWhere (\w+) loc=([-\d.,]+) x=([-\d.,]+) y=([-\d.,]+) z=([-\d.,]+)", text):
            bones[m.group(1)] = {"loc": v(m.group(2)), "x": v(m.group(3)), "y": v(m.group(4)), "z": v(m.group(5))}
        return bones

    def capture(name, loc, fwd, fov):
        cam = {"location": {"x": loc[0], "y": loc[1], "z": loc[2]}, "rotation": look_rot(fwd), "projectionMode": "Perspective",
               "fOV": fov, "orthoWidth": 512}
        r = call("SceneCaptureToolset.SceneCaptureToolset", "Capture", {
            "world": {"refPath": world}, "camera": cam, "mode": os.environ.get("TD_MODE", "Lit"),
            "options": {"width": W, "height": H, "screenPercentage": 1.0, "visualizationSubMode": ""}})
        return save(r, name)

    cmd("TD.Ground.ThirdPerson 1")
    cmd("TD.God")
    if before:
        cmd("TD.Ground.Grip 0")
        cmd("TD.Ground.GripSocket -9 -3 0 -90 0 0")  # the old HandBladeOffset on hand_r
    else:
        cmd("TD.Ground.Grip 1")
        cmd(os.environ.get("TD_SOCKET_CMD", "TD.Ground.GripSocket reset"))
    time.sleep(0.5)
    report = {}
    for name, clip, t01, upper in POSES:
        if only and name not in only:
            continue
        if clip:
            cmd("TD.Ground.GripHold %s %.2f%s" % (clip, t01, " upper" if upper else ""))
        else:
            cmd("TD.Ground.GripHold off")
        time.sleep(0.6)
        b = where()
        if "hilt" not in b or "hand_r" not in b:
            print("no hand data for", name, list(b))
            continue
        hilt = b["hilt"]["loc"]
        axis = b["hilt"]["x"]
        # palm normal: across the knuckles x along the hand, pointing to the palm side (where the finger tips curl)
        across = norm(sub(b["index_01_r"]["loc"], b["pinky_01_r"]["loc"]))
        along = norm(sub(b["middle_01_r"]["loc"], b["hand_r"]["loc"]))
        n = norm(cross(along, across))
        palmside = norm(sub(b["middle_03_r"]["loc"], b["middle_01_r"]["loc"]))
        if dot(n, palmside) < 0:
            n = mul(n, -1.0)
        side = norm(cross(axis, n))
        centre = add(hilt, mul(axis, 1.0))
        # views from the body's own axes (what a player sees): outside of the arm, front quarter, over the shoulder
        F = b["actor"]["x"]
        R = b["actor"]["y"]
        U = b["actor"]["z"]
        views = {
            "side": norm(add(add(R, mul(F, 0.25)), mul(U, 0.25))),
            "front": norm(add(add(F, mul(R, 0.6)), mul(U, 0.15))),
            "behind": norm(add(add(mul(F, -1.0), mul(R, 0.7)), mul(U, 0.6))),
        }
        report[name] = {"bones": b}
        for vname, d in views.items():
            loc = add(centre, mul(d, float(os.environ.get("TD_CAM_CM", "75"))))
            p = capture("%s_%s_%s" % (tag, name, vname), loc, mul(d, -1.0), float(os.environ.get("TD_FOV", "32")))
            print(name, vname, p)
        r = call("SceneCaptureToolset.SceneCaptureToolset", "CaptureGameViewport", {"world": {"refPath": world}, "maxResolution": W})
        save(r, "%s_%s_chase" % (tag, name))
    cmd("TD.Ground.GripHold off")
    with open(os.path.join(out, "%s_hands.json" % tag), "w") as f:
        json.dump(report, f, indent=1)
    return 0


if __name__ == "__main__":
    sys.exit(main())
