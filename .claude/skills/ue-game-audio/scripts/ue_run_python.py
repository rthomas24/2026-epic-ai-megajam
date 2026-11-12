#!/usr/bin/env python3
"""Triad Drift - run a Python file inside a running Unreal editor through its built-in MCP server.

    python ue_run_python.py --port 8021 ue_audio_build.py --spec spec.json
    python ue_run_python.py --port 8021 my_script.py

The file is sent to `execute_python` (toolset editor_toolset.toolsets.python_scripting.PythonScriptingTools).
With --spec, the JSON file is loaded and handed to the script as the dict SPEC; relative "file" entries
in it are resolved against the spec's own folder. Prints the script's result as JSON.

Exit code 0 = the script ran and (if it returned a dict) its "errors" list is empty; 1 otherwise.
Needs an editor started with -ModelContextProtocolStartServer -ModelContextProtocolPort=<port>
(Tools/td-play-mcp.ps1 -Editor does that) and this repo's Tools/td_mcp.py.
"""
import argparse
import json
import os
import sys

TOOLSET = "editor_toolset.toolsets.python_scripting.PythonScriptingTools"


def find_td_mcp(start):
    """Walks up from this file (or the current folder) to the checkout that holds Tools/td_mcp.py."""
    for origin in (start, os.getcwd()):
        folder = os.path.abspath(origin)
        while True:
            candidate = os.path.join(folder, "Tools", "td_mcp.py")
            if os.path.isfile(candidate):
                return os.path.dirname(candidate)
            parent = os.path.dirname(folder)
            if parent == folder:
                break
            folder = parent
    return None


def absolutise(value, base):
    """Makes every relative "file" entry of the spec absolute, with forward slashes."""
    if isinstance(value, dict):
        out = {}
        for key, item in value.items():
            if key == "file" and isinstance(item, str) and not os.path.isabs(item):
                item = os.path.normpath(os.path.join(base, item))
            if key == "file" and isinstance(item, str):
                item = item.replace("\\", "/")
            out[key] = absolutise(item, base)
        return out
    if isinstance(value, list):
        return [absolutise(item, base) for item in value]
    return value


def main():
    parser = argparse.ArgumentParser(description="Run a Python file inside a running Unreal editor (MCP execute_python).")
    parser.add_argument("script", help="Python file to run in the editor")
    parser.add_argument("--port", type=int, required=True, help="the editor's -ModelContextProtocolPort")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--spec", help="JSON file handed to the script as SPEC")
    parser.add_argument("--timeout", type=int, default=600)
    parser.add_argument("--tools-dir", help="folder holding td_mcp.py (default: found by walking up)")
    args = parser.parse_args()

    tools_dir = args.tools_dir or find_td_mcp(os.path.dirname(os.path.abspath(__file__)))
    if not tools_dir:
        print(json.dumps({"error": "Tools/td_mcp.py not found; pass --tools-dir"}))
        return 1
    sys.path.insert(0, tools_dir)
    import td_mcp  # noqa: E402  (the repo's MCP client)

    with open(args.script, encoding="utf-8") as handle:
        source = handle.read()
    if args.spec:
        with open(args.spec, encoding="utf-8") as handle:
            spec = absolutise(json.load(handle), os.path.dirname(os.path.abspath(args.spec)))
        source = "import json\nSPEC = json.loads(%r)\n" % json.dumps(spec) + source

    client = td_mcp.McpClient("http://%s:%d/mcp" % (args.host, args.port), args.timeout)
    try:
        client.connect()
        result = client.tool("call_tool", {
            "toolset_name": TOOLSET,
            "tool_name": "execute_python",
            "arguments": {"script": {"content": source, "language": "Python"}},
        })
    except td_mcp.McpError as error:
        print(json.dumps({"error": str(error)}))
        return 1

    content = td_mcp.unwrap(result).get("content") or []
    value = content[0] if content else None
    if isinstance(value, dict) and "returnValue" in value:
        value = value["returnValue"]
    if isinstance(value, str):
        try:
            value = json.loads(value)
        except ValueError:
            pass
    print(json.dumps(value, indent=1))
    failed = bool(result.get("isError")) or (isinstance(value, dict) and bool(value.get("errors")))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
