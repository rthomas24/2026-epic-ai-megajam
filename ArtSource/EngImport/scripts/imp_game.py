# SPDX-License-Identifier: MIT
# Triad Drift - import builder: tiny in-game driver over Unreal's MCP server (Tools/td_mcp.py's client), after
# ArtSource/EngProps/Outpost/scripts/op_game.py. Start the game first with Tools/td-play-mcp.ps1 -Port <port>.
#   python imp_game.py <port> cmd "<console command>" ["sleep 2"] ...   console commands in order
#   python imp_game.py <port> shot <out dir> <name>                     viewport capture -> <out dir>/<name>.png
#   python imp_game.py <port> log <regex> [n]                           last n matching lines of the game log
#   python imp_game.py <port> seq <json file>                           [["cmd", "..."], ["sleep", 2], ["shot", "dir", "name"], ["log", "re", 3]]
import importlib.util
import json
import os
import re
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
spec = importlib.util.spec_from_file_location("td_mcp", os.path.join(REPO, "Tools", "td_mcp.py"))
td = importlib.util.module_from_spec(spec)
spec.loader.exec_module(td)
PORT = int(sys.argv[1])
LOG = os.path.join(REPO, "Saved", "Logs", "td-play-mcp_%d.log" % PORT)
client = td.McpClient("http://127.0.0.1:%d/mcp" % PORT, 180)
client.connect()


def cmd(c):
    return client.tool("call_tool", {"toolset_name": "ConsoleToolset.ConsoleToolset", "tool_name": "RunConsoleCommand",
                                     "arguments": {"command": c}})


def log_tail(pattern, n=1):
    with open(LOG, encoding="utf-8", errors="replace") as fh:
        lines = [l.rstrip() for l in fh if re.search(pattern, l)]
    return lines[-n:]


def shot(out, name, max_res=1600):
    os.makedirs(out, exist_ok=True)
    w = td.unwrap(client.tool("call_tool", {"toolset_name": "SceneCaptureToolset.SceneCaptureToolset", "tool_name": "ListWorlds", "arguments": {}}))
    txt = json.dumps(w)
    m = re.search(r'"refPath": "([^"]+)"', txt) or re.search(r'(/Game/[A-Za-z0-9_/]+\.[A-Za-z0-9_]+)', txt)
    r = client.tool("call_tool", {"toolset_name": "SceneCaptureToolset.SceneCaptureToolset", "tool_name": "CaptureGameViewport",
                                  "arguments": {"world": {"refPath": m.group(1)}, "maxResolution": max_res}})
    saved = td.save_images(r, out)
    if not saved:
        return {"error": json.dumps(td.unwrap(r))[:600]}
    dst = os.path.join(out, name + os.path.splitext(saved[0])[1])
    if os.path.exists(dst):
        os.remove(dst)
    os.rename(saved[0], dst)
    return dst


def run(steps):
    for s in steps:
        kind = s[0]
        if kind == "cmd":
            cmd(s[1])
        elif kind == "sleep":
            time.sleep(float(s[1]))
        elif kind == "shot":
            print(shot(s[1], s[2]))
        elif kind == "log":
            for line in log_tail(s[1], int(s[2]) if len(s) > 2 else 1):
                print(line[:400])
        sys.stdout.flush()


a = sys.argv[2:]
if a[0] == "cmd":
    run([["sleep", c.split()[1]] if c.startswith("sleep ") else ["cmd", c] for c in a[1:]])
    print("ok")
elif a[0] == "shot":
    print(shot(a[1], a[2]))
elif a[0] == "log":
    for line in log_tail(a[1], int(a[2]) if len(a) > 2 else 1):
        print(line[:400])
elif a[0] == "seq":
    with open(a[1], encoding="utf-8") as fh:
        run(json.load(fh))
