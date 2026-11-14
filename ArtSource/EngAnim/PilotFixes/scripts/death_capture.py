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
    report = {}
    for i, item in enumerate(types):
        typ, _, how = item.partition(":")   # Type or Type:behind
        if not how and os.environ.get("TD_KILL_BEHIND"):
            how = "behind"
        behind = how
        spot = SPOTS[i % len(SPOTS)]
        cmd("TD.Ground.Teleport %d %d 300" % spot)
        time.sleep(1.0)
        cmd("TD.Ground.Spawn %s 600" % typ)
        time.sleep(2.5)
        e = where((spot[0] + 600, spot[1]))
        if not e:
            print("no crew", typ)
            continue
        loc, fx, ry = e["loc"], e["x"], e["y"]
        centre = [loc[k] - fx[k] * 80 for k in range(3)]   # a front hit throws the body back
        cam = [centre[k] + ry[k] * 720 + fx[k] * 150 for k in range(3)]
        cam[2] = e["floor"] + float(os.environ.get("TD_CAM_H", "110"))
        fwd = [centre[0] - cam[0], centre[1] - cam[1], (e["floor"] + 60) - cam[2]]
        camera = {"location": {"x": cam[0], "y": cam[1], "z": cam[2]},
                  "rotation": {"pitch": math.degrees(math.atan2(fwd[2], math.hypot(fwd[0], fwd[1]))), "yaw": math.degrees(math.atan2(fwd[1], fwd[0])), "roll": 0.0},
                  "projectionMode": "Perspective", "fOV": 45.0, "orthoWidth": 512}
        if how == "front" and typ == "Heavy":
            cmd("TD.Ground.BreakGuard")   # T5's dev command: a Heavy dies from the front only with its guard broken
            time.sleep(0.1)
        for attempt in range(10):   # a shielded Heavy takes several hits (the first ones break its guard)
            cmd("TD.Ground.CrewKill" + (" " + how if how else ""))
            time.sleep(0.15)
            chk = where(loc)
            if chk and chk["dead"]:
                break
            time.sleep(0.4)
        t0 = time.time() - 0.3
        shots = []
        trace = []
        for want in [float(x) for x in os.environ.get("TD_TIMES", "0.6,1.6,2.6,3.5").split(",")]:
            time.sleep(max(0.0, want - (time.time() - t0)))
            t = time.time() - t0
            w = where(loc)
            if w:
                trace.append([round(t, 2), round(w["pelvis"][2] - e["floor"], 1), round(w["head"][2] - e["floor"], 1), w["clip"], w["sim"], playing()])
            r = call("SceneCaptureToolset.SceneCaptureToolset", "Capture", {"world": {"refPath": world}, "camera": camera, "mode": "Lit",
                     "options": {"width": 1600, "height": 900, "screenPercentage": 1.0, "visualizationSubMode": ""}})
            for blk in r.get("content", []) or []:
                if blk.get("type") == "image" and blk.get("data"):
                    p = os.path.join(out, "death_%s_%s_%d.png" % (typ, how or "pilot", len(shots)))
                    open(p, "wb").write(base64.b64decode(blk["data"]))
                    shots.append([round(t, 2), p])
        end = where(loc) or where(loc)
        if end:
            end["floor"] = e["floor"]   # the capsule changes at death: keep the floor measured while it stood
        key = "%s_%s" % (typ, how or "pilot")
        report[key] = {"spawn": e, "end": end, "shots": shots, "trace": trace}
        print("%s: t, pelvis cm, head cm, clip, ragdoll: %s" % (key, trace))
        cmd("TD.Dev.DestroySpawned")
        if end:
            off = math.hypot(end["pelvis"][0] - end["loc"][0], end["pelvis"][1] - end["loc"][1])
            print("%s clip=%s ragdoll=%s" % (key, end.get("clip"), end.get("sim")))
            print("  %s dead=%d: pelvis %.0f cm from the capsule, pelvis %.0f cm / head %.0f cm above the floor, lowest bone %.0f cm, shots at %s" % (
                typ, end["dead"], off, end["pelvis"][2] - end["floor"], end["head"][2] - end["floor"], end["low"], [s[0] for s in shots]))
    json.dump(report, open(os.path.join(out, "deaths.json"), "w"), indent=1)


if __name__ == "__main__":
    sys.exit(main())
