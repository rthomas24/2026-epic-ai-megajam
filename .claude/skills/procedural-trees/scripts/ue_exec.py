#!/usr/bin/env python3
"""Run a Python file inside a running Unreal editor through its built-in MCP server.

    python ue_exec.py --port 8017 path/to/script.py [--args '{"key": "value"}'] [--timeout 600]

The file is sent to the tool `editor_toolset.toolsets.python_scripting.PythonScriptingTools.execute_python`.
Rules of that tool (see its read_me): `unreal` and `asyncio` are already imported, imports must be at
the top level, top-level `await` works, `time.sleep` and blocking network calls are rejected, and the
value of the script's LAST EXPRESSION is what comes back (print goes to the editor log, not here).

`--args` is a JSON object; it is made available to the script as the dict `ARGS`.
`--lib` prepends other files (helpers) to the script, in order, so one call carries its own library.

Exit code 0 = the tool call succeeded, 1 = transport or script error. Output: the returned value
(JSON is pretty-printed).
"""

import argparse
import json
import os
import sys


def find_repo_tools():
    here = os.path.dirname(os.path.abspath(__file__))
    probe = here
    for _ in range(8):
        candidate = os.path.join(probe, "Tools", "td_mcp.py")
        if os.path.exists(candidate):
            return os.path.join(probe, "Tools")
        probe = os.path.dirname(probe)
    raise SystemExit("ue_exec: Tools/td_mcp.py not found above %s" % here)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("script")
    parser.add_argument("--port", type=int, required=True)
    parser.add_argument("--args", default="{}", help="JSON object, or @file.json; exposed to the script as ARGS")
    parser.add_argument("--lib", action="append", default=[], help="helper file(s) prepended to the script")
    parser.add_argument("--timeout", type=int, default=600)
    options = parser.parse_args()

    sys.path.insert(0, find_repo_tools())
    import td_mcp  # noqa: E402  (the repo's MCP client)

    text = options.args
    if text.startswith("@"):
        with open(text[1:], encoding="utf-8") as handle:
            text = handle.read()
    arguments = json.loads(text)

    parts = ["import json as _ue_exec_json", "ARGS = _ue_exec_json.loads(%r)" % json.dumps(arguments)]
    for path in options.lib + [options.script]:
        with open(path, encoding="utf-8") as handle:
            parts.append("# ---- %s ----\n%s" % (os.path.basename(path), handle.read()))
    source = "\n".join(parts)

    client = td_mcp.McpClient("http://127.0.0.1:%d/mcp" % options.port, options.timeout)
    try:
        client.connect()
        result = client.tool("call_tool", {
            "toolset_name": "editor_toolset.toolsets.python_scripting.PythonScriptingTools",
            "tool_name": "execute_python",
            "arguments": {"script": {"content": source, "language": "Python"}},
        })
    except td_mcp.McpError as error:
        print(json.dumps({"error": str(error)}))
        return 1

    failed = bool(result.get("isError"))
    structured = result.get("structuredContent")
    if isinstance(structured, dict) and "returnValue" in structured:
        value = structured["returnValue"]
        try:
            print(json.dumps(json.loads(value), indent=1))
        except (TypeError, ValueError):
            print(value)
    else:
        for block in result.get("content", []) or []:
            if block.get("type") == "text":
                print(block.get("text", ""))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
