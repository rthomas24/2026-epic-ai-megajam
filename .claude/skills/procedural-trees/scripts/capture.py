#!/usr/bin/env python3
"""Shell-side: render one PNG of the editor's current world from a given camera (Unreal MCP SceneCaptureToolset.Capture).

    python capture.py --port 8017 --out shot.png --location -1500 0 154 --look-at 0 3000 800 [--fov 70] [--mode Lit] [--size 1600 900]
    python capture.py --port 8017 --modes                      # list the capture modes this editor offers

Locations are centimetres. --look-at computes the rotation for you. The capture is an offscreen
render (no temporal history), so it shows materials, scale and silhouettes, not final image quality.
"""

import argparse
import base64
import json
import math
import os
import sys


def repo_tools():
    probe = os.path.dirname(os.path.abspath(__file__))
    for _ in range(8):
        if os.path.exists(os.path.join(probe, "Tools", "td_mcp.py")):
            return os.path.join(probe, "Tools")
        probe = os.path.dirname(probe)
    raise SystemExit("capture: Tools/td_mcp.py not found")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--port", type=int, required=True)
    parser.add_argument("--out")
    parser.add_argument("--location", nargs=3, type=float, default=[0, 0, 154])
    parser.add_argument("--look-at", nargs=3, type=float)
    parser.add_argument("--rotation", nargs=3, type=float, default=[0, 0, 0], help="pitch yaw roll")
    parser.add_argument("--fov", type=float, default=75.0)
    parser.add_argument("--mode", default="")
    parser.add_argument("--sub-mode", default="")
    parser.add_argument("--size", nargs=2, type=int, default=[1600, 900])
    parser.add_argument("--modes", action="store_true")
    options = parser.parse_args()

    sys.path.insert(0, repo_tools())
    import td_mcp  # noqa: E402

    toolset = "SceneCaptureToolset.SceneCaptureToolset"
    client = td_mcp.McpClient("http://127.0.0.1:%d/mcp" % options.port, 300)
    client.connect()

    def call(tool, arguments=None):
        return client.tool("call_tool", {"toolset_name": toolset, "tool_name": tool, "arguments": arguments or {}})

    def value(result):
        structured = result.get("structuredContent") or {}
        if "returnValue" in structured:
            return structured["returnValue"]
        for block in result.get("content", []):
            if block.get("type") == "text":
                try:
                    return json.loads(block["text"]).get("returnValue")
                except ValueError:
                    return block["text"]
        return None

    modes = value(call("ListModes", {"bIncludeUnavailable": False}))
    if not isinstance(modes, list):
        print("capture: could not list modes: %s" % modes)
        return 1
    if options.modes:
        print(json.dumps(modes, indent=1))
        return 0
    worlds = value(call("ListWorlds")) or []
    world = next((w for w in worlds if w.get("bCanCapture")), None)
    if not world:
        print("capture: no capturable world: %s" % worlds)
        return 1
    pitch, yaw, roll = options.rotation
    if options.look_at:
        dx, dy, dz = (options.look_at[i] - options.location[i] for i in range(3))
        yaw = math.degrees(math.atan2(dy, dx))
        pitch = math.degrees(math.atan2(dz, math.hypot(dx, dy)))
        roll = 0.0
    mode = options.mode
    if not mode:
        names = [m.get("name", m) if isinstance(m, dict) else m for m in (modes or [])]
        mode = next((n for n in ("FinalImage", "Lit", "SceneColor") if n in names), names[0] if names else "Lit")
    result = call("Capture", {
        "world": world["world"],
        "camera": {"location": dict(zip("xyz", options.location)), "rotation": {"pitch": pitch, "yaw": yaw, "roll": roll},
                   "projectionMode": "Perspective", "fOV": options.fov},
        "mode": mode,
        "options": {"width": options.size[0], "height": options.size[1], "screenPercentage": 1, "visualizationSubMode": options.sub_mode},
    })
    for block in result.get("content", []):
        if block.get("type") == "image" and block.get("data"):
            os.makedirs(os.path.dirname(os.path.abspath(options.out)), exist_ok=True)
            with open(options.out, "wb") as handle:
                handle.write(base64.b64decode(block["data"]))
            print("saved %s (mode %s, world %s)" % (os.path.abspath(options.out), mode, world.get("mapPath")))
            return 0
    print("capture failed: %s" % json.dumps(result)[:1500])
    return 1


if __name__ == "__main__":
    sys.exit(main())
