# Triad Drift - crew in a real fight (the AI attacks the pilot, who stands in god mode): a strip of frames from a fixed side camera: spawn a crew member, kill it with a real melee hit from the pilot
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
SPOTS = [(900, 900), (900, 3400), (3400, 900), (3400, 3400), (-1600, 900), (900, -1600), (-1600, 3400), (3400, -1600)]
PAT = re.compile(r"TD\.CrewAll (\S+) dead=(\d) loc=([-\d.,]+) x=([-\d.,]+) y=([-\d.,]+) pelvis=([-\d.,]+) head=([-\d.,]+) floor=([-\d.]+) low=([-\d.]+)(?: clip=(\S+) sim=(\d))?")


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
                 "floor": float(m.group(8)), "low": float(m.group(9)), "clip": m.group(10), "sim": m.group(11)}
            d = math.hypot(e["loc"][0] - near[0], e["loc"][1] - near[1])
            if best is None or d < best[0]:
                best = (d, e)
        return best[1] if best else None

    def playing():
        # which clip each crew mesh's single-node player holds (engine console: getall), newest lines only
        size = os.path.getsize(LOG)
        cmd("getall AnimSingleNodeInstance CurrentAsset")
        time.sleep(0.25)
        with open(LOG, "r", errors="replace") as f:
            f.seek(size)
            text = f.read()
        return sorted(set(re.findall(r"TDCrewCharacter\S*CurrentAsset = \S*/(\w+)\.", text)))

    world = None
    for b in td_mcp.unwrap(call("SceneCaptureToolset.SceneCaptureToolset", "ListWorlds"))["content"]:
        if isinstance(b, dict):
            for w in b.get("returnValue", []):
                if w.get("worldType") == "Game":
                    world = w["world"]["refPath"]
    secs = float(os.environ.get("TD_FIGHT_S", "12"))
    for i, item in enumerate(types):
        typ, _, dist = item.partition(":")
        spot = SPOTS[i % len(SPOTS)]
        cmd("TD.Ground.Teleport %d %d 300" % spot)
        time.sleep(1.0)
        cmd("TD.Ground.Spawn %s %s" % (typ, dist or "450"))
        e = None
        for _ in range(8):
            time.sleep(0.4)
            e = where((spot[0] + float(dist or 450), spot[1]))
            if e and e["type"].endswith(typ) and not e["dead"]:
                break
        if not e:
            print("no crew", typ)
            continue
        loc, fx, ry = e["loc"], e["x"], e["y"]
        mid = [(loc[0] + spot[0]) / 2.0, (loc[1] + spot[1]) / 2.0]
        side = float(os.environ.get("TD_CAM_SIDE", "750"))
        cam = [mid[0] + ry[0] * side, mid[1] + ry[1] * side, e["floor"] + 120]
        fwd = [mid[0] - cam[0], mid[1] - cam[1], (e["floor"] + 90) - cam[2]]
        camera = {"location": {"x": cam[0], "y": cam[1], "z": cam[2]},
                  "rotation": {"pitch": math.degrees(math.atan2(fwd[2], math.hypot(fwd[0], fwd[1]))), "yaw": math.degrees(math.atan2(fwd[1], fwd[0])), "roll": 0.0},
                  "projectionMode": "Perspective", "fOV": float(os.environ.get("TD_FOV", "50")), "orthoWidth": 512}
        t0 = time.time()
        k = 0
        # TD_CMDS_<Type> = "2.0:TD.Ground.CrewHit 5;4.0:TD.Ground.CrewHit 5 behind": console commands at those seconds
        todo = sorted((float(x.split(":", 1)[0]), x.split(":", 1)[1]) for x in os.environ.get("TD_CMDS_" + typ, "").split(";") if x.strip())
        while time.time() - t0 < secs:
            while todo and time.time() - t0 >= todo[0][0]:
                what = todo.pop(0)[1]
                if what.startswith("@"):   # "@" or "@<command>": first aim a close three-quarter camera at this crew member
                    w = where(loc)
                    if w and w["type"].endswith(typ):
                        l2, f2, r2 = w["loc"], w["x"], w["y"]
                        dist = float(os.environ.get("TD_CLOSE_CM", "560"))
                        cam = [l2[0] + r2[0] * dist * 0.75 + f2[0] * dist * 0.66, l2[1] + r2[1] * dist * 0.75 + f2[1] * dist * 0.66, w["floor"] + 150]
                        aim = [l2[0], l2[1], w["floor"] + 105]
                        fwd = [aim[j] - cam[j] for j in range(3)]
                        camera["location"] = {"x": cam[0], "y": cam[1], "z": cam[2]}
                        camera["rotation"] = {"pitch": math.degrees(math.atan2(fwd[2], math.hypot(fwd[0], fwd[1]))), "yaw": math.degrees(math.atan2(fwd[1], fwd[0])), "roll": 0.0}
                    what = what[1:]
                print("%s %.2f s: %s" % (typ, time.time() - t0, what or "(camera)"))
                if what:
                    cmd(what)
            r = call("SceneCaptureToolset.SceneCaptureToolset", "Capture", {"world": {"refPath": world}, "camera": camera, "mode": "Lit",
                     "options": {"width": 960, "height": 540, "screenPercentage": 1.0, "visualizationSubMode": ""}})
            for blk in r.get("content", []) or []:
                if blk.get("type") == "image" and blk.get("data"):
                    open(os.path.join(out, "fight_%s_%02d_%04d.png" % (typ, k, int((time.time() - t0) * 100))), "wb").write(base64.b64decode(blk["data"]))
                    k += 1
        print("%s: %d frames in %.1f s" % (typ, k, time.time() - t0))
        cmd("TD.Ground.CrewKill behind")
        time.sleep(0.3)
        cmd("TD.Dev.DestroySpawned")


if __name__ == "__main__":
    sys.exit(main())
