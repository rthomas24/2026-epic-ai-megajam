# SPDX-License-Identifier: MIT
# Environment builder's capture driver: runs console commands and viewport captures on a game started with
#   powershell -File Tools\td-play-mcp.ps1 -Port 8016
# Usage:
#   python ArtSource/EngEnvironment/scripts/td_env_drive.py --port 8016 cmd "TD.Stage VerdanceOnFoot"
#   python ArtSource/EngEnvironment/scripts/td_env_drive.py --port 8016 shot <out.png> [--world <refPath>]
#   python ArtSource/EngEnvironment/scripts/td_env_drive.py --port 8016 seq <steps.json>
# steps.json: [["cmd", "TD.Stage VerdanceOnFoot"], ["wait", 8], ["shot", "out/v_pad.png"], ...]
import argparse
import base64
import json
import os
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(REPO, "Tools"))
from td_mcp import McpClient, McpError  # noqa: E402


class Drive:
    def __init__(self, port):
        self.c = McpClient("http://127.0.0.1:%d/mcp" % port, 240)
        self.c.connect()

    def call(self, toolset, tool, args):
        r = self.c.tool("call_tool", {"toolset_name": toolset, "tool_name": tool, "arguments": args})
        if r.get("isError"):
            raise McpError(json.dumps(r)[:800])
        return r

    def cmd(self, command):
        r = self.call("ConsoleToolset.ConsoleToolset", "RunConsoleCommand", {"command": command})
        return [b.get("text", "") for b in r.get("content", []) if b.get("type") == "text"]

    def worlds(self):
        r = self.call("SceneCaptureToolset.SceneCaptureToolset", "ListWorlds", {})
        return [b.get("text", "") for b in r.get("content", []) if b.get("type") == "text"]

    def world_ref(self):
        txt = " ".join(self.worlds())
        try:
            data = json.loads(txt)
        except ValueError:
            return None
        items = data if isinstance(data, list) else data.get("returnValue", data.get("worlds", []))
        for w in items:
            ref = w.get("refPath") or w.get("world", {}).get("refPath") if isinstance(w, dict) else None
            if ref and "Transition" not in ref:
                return ref
        return None

    def shot(self, path, world=None, res=1600):
        world = world or self.world_ref()
        r = self.call("SceneCaptureToolset.SceneCaptureToolset", "CaptureGameViewport",
                      {"world": {"refPath": world}, "maxResolution": res})
        for b in r.get("content", []):
            if b.get("type") == "image" and b.get("data"):
                os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
                with open(path, "wb") as f:
                    f.write(base64.b64decode(b["data"]))
                return path
        raise McpError("no image: " + json.dumps(r)[:400])


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--port", type=int, default=8016)
    sub = p.add_subparsers(dest="op", required=True)
    a = sub.add_parser("cmd")
    a.add_argument("command")
    s = sub.add_parser("shot")
    s.add_argument("out")
    s.add_argument("--world")
    sub.add_parser("worlds")
    q = sub.add_parser("seq")
    q.add_argument("steps")
    args = p.parse_args()
    d = Drive(args.port)
    if args.op == "cmd":
        print("\n".join(d.cmd(args.command)))
    elif args.op == "worlds":
        print("\n".join(d.worlds()))
    elif args.op == "shot":
        print(d.shot(args.out, args.world))
    else:
        with open(args.steps) as f:
            steps = json.load(f)
        for st in steps:
            kind = st[0]
            try:
                if kind == "cmd":
                    print("cmd", st[1], "->", " ".join(d.cmd(st[1]))[:200], flush=True)
                elif kind == "wait":
                    time.sleep(float(st[1]))
                elif kind == "shot":
                    print("shot", d.shot(st[1]), flush=True)
                elif kind == "reconnect":
                    d = Drive(args.port)
            except McpError as e:
                print("ERROR", st, str(e)[:300], flush=True)
                if kind == "shot":
                    time.sleep(3)
                    try:
                        d = Drive(args.port)
                        print("shot(retry)", d.shot(st[1]), flush=True)
                    except McpError as e2:
                        print("ERROR retry", str(e2)[:300], flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
