#!/usr/bin/env python3
"""Capture the PIE (or game) viewport of an MCP session to a PNG.  usage: cap.py <port> <out.png> [maxres]"""
import base64, json, os, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'Tools'))
import td_mcp
port, out = int(sys.argv[1]), sys.argv[2]
res = int(sys.argv[3]) if len(sys.argv) > 3 else 1600
c = td_mcp.McpClient('http://127.0.0.1:%d/mcp' % port, 180); c.connect()
TS = 'SceneCaptureToolset.SceneCaptureToolset'
r = td_mcp.unwrap(c.tool('call_tool', {'toolset_name': TS, 'tool_name': 'ListWorlds', 'arguments': {}}))
txt = json.dumps(r)
import re
paths = re.findall(r'"refPath": "([^"]+)"', txt)
pie = [p for p in paths if 'UEDPIE' in p] or [p for p in paths if 'L_Galaxy' in p] or paths
r = c.tool('call_tool', {'toolset_name': TS, 'tool_name': 'CaptureGameViewport', 'arguments': {'world': {'refPath': pie[0]}, 'maxResolution': res}})
for b in r.get('content', []):
    if b.get('type') == 'image':
        open(out, 'wb').write(base64.b64decode(b['data'])); print('cap', os.path.basename(out)); break
else:
    print('NO IMAGE', json.dumps(r)[:400], 'worlds', paths[:6])
