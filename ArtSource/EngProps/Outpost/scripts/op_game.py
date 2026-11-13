# SPDX-License-Identifier: MIT
# Triad Drift - Outpost: tiny driver for the in-game walk test (system Python; uses Tools/td_mcp.py's client).
#   python op_game.py <port> cmd "<console command>" [...more commands]
#   python op_game.py <port> where            -> the pilot's position (TD.Ground.Where, read back from the log)
#   python op_game.py <port> shot <name>      -> Outpost/review/game/<name>.png (CaptureGameViewport)
#   python op_game.py <port> walk <yaw> <seconds> [key=W]   -> TD.Look, hold the key, wait, then where
#   python op_game.py <port> log <pattern> [n]
import importlib.util, json, os, re, sys, time

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
REPO = os.path.dirname(os.path.dirname(os.path.dirname(KIT)))
spec = importlib.util.spec_from_file_location("td_mcp", os.path.join(REPO, "Tools", "td_mcp.py"))
td = importlib.util.module_from_spec(spec); spec.loader.exec_module(td)
PORT = int(sys.argv[1])
LOG = os.path.join(REPO, "Saved", "Logs", "td-play-mcp_%d.log" % PORT)
OUT = os.environ.get("TD_SHOT_DIR") or os.path.join(KIT, "review", "game")   # Relay / Wreck set their own review/game
client = td.McpClient("http://127.0.0.1:%d/mcp" % PORT, 180)
client.connect()


def cmd(c):
    return client.tool("call_tool", {"toolset_name": "ConsoleToolset.ConsoleToolset", "tool_name": "RunConsoleCommand", "arguments": {"command": c}})


def log_tail(pattern, n=1):
    with open(LOG, encoding="utf-8", errors="replace") as fh:
        lines = [l.rstrip() for l in fh if re.search(pattern, l)]
    return lines[-n:]


def where():
    cmd("TD.Ground.Where")
    time.sleep(0.4)
    line = (log_tail(r"TD\.Where pos=") or ["?"])[-1]
    m = re.search(r"pos=\(([-\d.]+) ([-\d.]+) ([-\d.]+)\) yaw=([-\d.]+)", line)
    return [float(x) for x in m.groups()] if m else line


def world():
    r = td.unwrap(client.tool("call_tool", {"toolset_name": "SceneCaptureToolset.SceneCaptureToolset", "tool_name": "ListWorlds", "arguments": {}}))
    return r


def shot(name, max_res=1280):
    os.makedirs(OUT, exist_ok=True)
    w = world()
    txt = json.dumps(w)
    m = re.search(r'"refPath": "([^"]+)"', txt) or re.search(r'(/Game/[A-Za-z0-9_/]+\.[A-Za-z0-9_]+)', txt)
    r = client.tool("call_tool", {"toolset_name": "SceneCaptureToolset.SceneCaptureToolset", "tool_name": "CaptureGameViewport",
                                  "arguments": {"world": {"refPath": m.group(1)}, "maxResolution": max_res}})
    saved = td.save_images(r, OUT)
    if not saved:
        return {"error": json.dumps(td.unwrap(r))[:600]}
    dst = os.path.join(OUT, name + os.path.splitext(saved[0])[1])
    if os.path.exists(dst):
        os.remove(dst)
    os.rename(saved[0], dst)
    return dst


a = sys.argv[2:]
if a[0] == "cmd":
    for c in a[1:]:
        if c.startswith("sleep "):
            time.sleep(float(c.split()[1]))
        else:
            cmd(c)
    print("ok")
elif a[0] == "where":
    print(where())
elif a[0] == "shot":
    print(shot(a[1]))
elif a[0] == "worlds":
    print(json.dumps(world())[:1500])
elif a[0] == "walk":
    yaw, secs = float(a[1]), float(a[2])
    key = a[3] if len(a) > 3 else "W"
    before = where()
    cmd("TD.Look %.1f 0" % yaw)
    cmd("TD.Key %s %.2f" % (key, secs))
    time.sleep(secs + 0.6)
    after = where()
    print(json.dumps({"before": before, "after": after}))
elif a[0] == "log":
    for l in log_tail(a[1], int(a[2]) if len(a) > 2 else 10):
        print(l[:400])
