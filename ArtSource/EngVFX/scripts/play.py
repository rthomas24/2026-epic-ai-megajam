#!/usr/bin/env python3
"""Triad Drift ENG-37: drive a game (or PIE) MCP session with a list of steps in one process.
usage: play.py <port> <outdir> step [step ...]
  con:<console command>     sleep:<seconds>     cap:<name>[:maxres]     py:<file>  (editor sessions only)
Captures go to <outdir>/<name>.png."""
import base64, json, os, re, sys, time
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'Tools'))
import td_mcp

port, outdir = int(sys.argv[1]), sys.argv[2]
os.makedirs(outdir, exist_ok=True)
c = td_mcp.McpClient('http://127.0.0.1:%d/mcp' % port, 300); c.connect()
TS = 'SceneCaptureToolset.SceneCaptureToolset'
_world = [None]


def world():
    if not _world[0]:
        r = td_mcp.unwrap(c.tool('call_tool', {'toolset_name': TS, 'tool_name': 'ListWorlds', 'arguments': {}}))
        paths = re.findall(r'"refPath": "([^"]+)"', json.dumps(r))
        pie = [p for p in paths if 'UEDPIE' in p] or [p for p in paths if 'L_Galaxy' in p] or paths
        _world[0] = pie[0]
    return _world[0]


for step in sys.argv[3:]:
    kind, _, arg = step.partition(':')
    if kind == 'con':
        c.tool('call_tool', {'toolset_name': 'ConsoleToolset.ConsoleToolset', 'tool_name': 'RunConsoleCommand', 'arguments': {'command': arg}})
    elif kind == 'sleep':
        time.sleep(float(arg))
    elif kind == 'cap':
        name, _, res = arg.partition(':')
        r = c.tool('call_tool', {'toolset_name': TS, 'tool_name': 'CaptureGameViewport', 'arguments': {'world': {'refPath': world()}, 'maxResolution': int(res or 1600)}})
        for b in r.get('content', []):
            if b.get('type') == 'image':
                open(os.path.join(outdir, name + '.png'), 'wb').write(base64.b64decode(b['data'])); break
        else:
            print('NO IMAGE for', name, json.dumps(r)[:300])
    elif kind == 'py':
        r = c.tool('call_tool', {'toolset_name': 'editor_toolset.toolsets.python_scripting.PythonScriptingTools', 'tool_name': 'execute_python',
                                 'arguments': {'script': {'content': open(arg, encoding='utf-8').read(), 'language': 'Python'}}})
        print(json.dumps(r)[:600])
print('done')
