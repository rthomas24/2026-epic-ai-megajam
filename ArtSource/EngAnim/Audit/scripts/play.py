# Animation audit: drive a running game (Tools/td-play-mcp.ps1 -Port 8016) from a step list and save viewport captures.
# Usage: python play.py <outdir> <step> <step> ...
#   a step is a console command, or:  wait:<seconds>   shot:<name>   burst:<name>:<count>:<gap seconds>   world:<refPath>
# Captures are written as <outdir>/<name>[_NN].png. One call at a time (the server is single-threaded).
import base64, json, os, sys, time
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "..", "Tools"))
import td_mcp

PORT = int(os.environ.get("TD_PORT", "8016"))
RES = int(os.environ.get("TD_RES", "960"))


def main():
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    c = td_mcp.McpClient("http://127.0.0.1:%d/mcp" % PORT, 180)
    c.connect()

    def call(toolset, tool, args=None):
        return c.tool("call_tool", {"toolset_name": toolset, "tool_name": tool, "arguments": args or {}})

    worlds = td_mcp.unwrap(call("SceneCaptureToolset.SceneCaptureToolset", "ListWorlds"))
    world = None
    for b in worlds["content"]:
        if isinstance(b, dict):
            for w in b.get("returnValue", []):
                if w.get("worldType") == "Game":
                    world = w["world"]["refPath"]
    print("world", world)

    def shot(name):
        r = call("SceneCaptureToolset.SceneCaptureToolset", "CaptureGameViewport", {"world": {"refPath": world}, "maxResolution": RES})
        for b in r.get("content", []) or []:
            if b.get("type") == "image" and b.get("data"):
                p = os.path.join(out, name + ".png")
                open(p, "wb").write(base64.b64decode(b["data"]))
                return p
        print("no image for", name, json.dumps(td_mcp.unwrap(r))[:300])

    for step in sys.argv[2:]:
        if step.startswith("wait:"):
            time.sleep(float(step[5:]))
        elif step.startswith("world:"):
            world = step[6:]
        elif step.startswith("shot:"):
            print("shot", shot(step[5:]))
        elif step.startswith("burst:"):
            _, name, count, gap = step.split(":")
            for i in range(int(count)):
                t = time.time()
                shot("%s_%02d" % (name, i))
                time.sleep(max(0.0, float(gap) - (time.time() - t)))
            print("burst", name, count)
        else:
            r = call("ConsoleToolset.ConsoleToolset", "RunConsoleCommand", {"command": step})
            if r.get("isError"):
                print("ERR", step, json.dumps(td_mcp.unwrap(r))[:300])
    return 0


if __name__ == "__main__":
    sys.exit(main())
