#!/usr/bin/env python3
"""Triad Drift - minimal client for Unreal Engine 5.9's built-in MCP server ("Unreal MCP"). Lead-owned.

Lets any agent (or a human) drive a running editor or game over HTTP from a shell, without the
server being registered in the agent's own MCP configuration. Each worktree can run its own game on
its own port, so several agents can play-test at once.

Start a game with the server on a private port (see Tools/td-play-mcp.ps1), then:

    python Tools/td_mcp.py --port 8011 tools                     # the server's own top-level tools
    python Tools/td_mcp.py --port 8011 toolsets                  # every toolset and its description
    python Tools/td_mcp.py --port 8011 describe <Toolset> [...]  # tools of one or more toolsets, with schemas
    python Tools/td_mcp.py --port 8011 call <Toolset> <Tool> '{"arg": 1}'
    python Tools/td_mcp.py --port 8011 call <Toolset> <Tool> @args.json --save-images Saved/TDShots/mcp

Output is JSON on stdout. Exit code 0 = the call succeeded, 1 = transport or tool error.
The server runs tool calls one at a time on the game thread: never send overlapping calls.
"""

import argparse
import base64
import json
import os
import sys
import urllib.error
import urllib.request

PROTOCOL = "2025-06-18"


class McpError(Exception):
    pass


class McpClient:
    def __init__(self, url, timeout=180):
        self.url = url
        self.timeout = timeout
        self.session = None
        self.next_id = 0
        self.protocol = PROTOCOL

    def _post(self, body):
        request = urllib.request.Request(
            self.url,
            data=json.dumps(body).encode("utf-8"),
            headers={
                "Content-Type": "application/json",
                "Accept": "application/json, text/event-stream",
                "Origin": "http://127.0.0.1",
            },
        )
        if self.session:
            request.add_header("Mcp-Session-Id", self.session)
            request.add_header("MCP-Protocol-Version", self.protocol)
        try:
            with urllib.request.urlopen(request, timeout=self.timeout) as response:
                session = response.headers.get("Mcp-Session-Id")
                if session:
                    self.session = session
                content_type = response.headers.get("Content-Type", "")
                raw = response.read().decode("utf-8", errors="replace")
        except urllib.error.HTTPError as error:
            raise McpError("HTTP %s: %s" % (error.code, error.read().decode("utf-8", errors="replace")[:600]))
        except (urllib.error.URLError, OSError) as error:
            raise McpError("cannot reach %s (%s). Is the game or editor running with the MCP server on this port?" % (self.url, error))
        return content_type, raw

    def request(self, method, params=None):
        self.next_id += 1
        body = {"jsonrpc": "2.0", "id": self.next_id, "method": method}
        if params is not None:
            body["params"] = params
        content_type, raw = self._post(body)
        message = None
        if "text/event-stream" in content_type:
            for line in raw.splitlines():
                if line.startswith("data:"):
                    try:
                        candidate = json.loads(line[5:].strip())
                    except ValueError:
                        continue
                    if candidate.get("id") == self.next_id:
                        message = candidate
        elif raw.strip():
            message = json.loads(raw)
        if message is None:
            raise McpError("no response to %s" % method)
        if "error" in message:
            raise McpError("%s failed: %s" % (method, json.dumps(message["error"])[:800]))
        return message.get("result", {})

    def notify(self, method, params=None):
        body = {"jsonrpc": "2.0", "method": method}
        if params is not None:
            body["params"] = params
        self._post(body)

    def connect(self):
        result = self.request("initialize", {
            "protocolVersion": PROTOCOL,
            "capabilities": {},
            "clientInfo": {"name": "td_mcp", "version": "1.0"},
        })
        self.protocol = result.get("protocolVersion", PROTOCOL)
        self.notify("notifications/initialized")
        return result

    def tool(self, name, arguments=None):
        return self.request("tools/call", {"name": name, "arguments": arguments or {}})


def save_images(result, directory):
    """Writes image blocks to disk and replaces their base64 payload with the file path."""
    os.makedirs(directory, exist_ok=True)
    saved = []
    for index, block in enumerate(result.get("content", []) or []):
        if block.get("type") == "image" and block.get("data"):
            extension = (block.get("mimeType", "image/png").split("/")[-1] or "png").replace("jpeg", "jpg")
            path = os.path.join(directory, "mcp_%02d_%d.%s" % (index, os.getpid(), extension))
            with open(path, "wb") as handle:
                handle.write(base64.b64decode(block["data"]))
            block["data"] = "<saved to %s>" % path
            saved.append(path)
    return saved


def strip_images(result):
    for block in result.get("content", []) or []:
        if block.get("type") in ("image", "audio") and len(block.get("data", "")) > 200:
            block["data"] = "<%d base64 chars omitted; pass --save-images DIR>" % len(block["data"])


def unwrap(result):
    """Tool results arrive as text blocks that usually hold JSON: decode them for readability."""
    out = dict(result)
    blocks = []
    for block in result.get("content", []) or []:
        if block.get("type") == "text":
            text = block.get("text", "")
            try:
                blocks.append(json.loads(text))
            except ValueError:
                blocks.append(text)
        else:
            blocks.append(block)
    out["content"] = blocks
    return out


def load_arguments(text):
    if not text:
        return {}
    if text.startswith("@"):
        with open(text[1:], encoding="utf-8") as handle:
            return json.load(handle)
    return json.loads(text)


def main():
    parser = argparse.ArgumentParser(description="Call Unreal Engine 5.9's built-in MCP server from a shell.")
    parser.add_argument("--port", type=int, default=8000)
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--path", default="/mcp")
    parser.add_argument("--timeout", type=int, default=180)
    parser.add_argument("--save-images", metavar="DIR", help="write returned images to DIR instead of printing base64")
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("tools", help="list the server's top-level tools (tools/list)")
    sub.add_parser("toolsets", help="list every toolset (list_toolsets)")
    describe = sub.add_parser("describe", help="describe one or more toolsets with schemas")
    describe.add_argument("toolsets", nargs="+")
    describe.add_argument("--brief", action="store_true", help="names and descriptions only, no schemas")
    call = sub.add_parser("call", help="call a toolset tool")
    call.add_argument("toolset")
    call.add_argument("tool")
    call.add_argument("arguments", nargs="?", default="", help="JSON object, or @file.json")
    raw = sub.add_parser("raw", help="call a top-level MCP tool by name with raw arguments")
    raw.add_argument("name")
    raw.add_argument("arguments", nargs="?", default="")
    args = parser.parse_args()

    client = McpClient("http://%s:%d%s" % (args.host, args.port, args.path), args.timeout)
    try:
        client.connect()
        if args.command == "tools":
            result = client.request("tools/list")
            print(json.dumps(result, indent=1))
            return 0
        if args.command == "toolsets":
            result = client.tool("list_toolsets")
        elif args.command == "describe":
            result = client.tool("describe_toolsets", {"toolset_names": args.toolsets, "include_schemas": not args.brief})
        elif args.command == "call":
            result = client.tool("call_tool", {"toolset_name": args.toolset, "tool_name": args.tool, "arguments": load_arguments(args.arguments)})
        else:
            result = client.tool(args.name, load_arguments(args.arguments))
    except McpError as error:
        print(json.dumps({"error": str(error)}))
        return 1

    if args.save_images:
        save_images(result, args.save_images)
    else:
        strip_images(result)
    print(json.dumps(unwrap(result), indent=1))
    return 1 if result.get("isError") else 0


if __name__ == "__main__":
    sys.exit(main())
