#!/usr/bin/env python3
"""#163: one process that drives the editor's PIE session through a list of steps and captures the game viewport.
usage: blade_cap.py <port> <outdir> step [step ...]
  act:<action>          blade_pie.py action (light, heavy, jump, slam, block, unblock, turn, info, pie_start, pie_stop)
  con:<console cmd>     console command in the PIE world (through blade_pie.py)
  sleep:<seconds>       cap:<name>[:maxres]     burst:<name>:<count>:<gap s>   (several captures in a row)
  py:<file>             any editor Python file
"""
import base64, json, os, re, sys, time
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, '..', '..', '..', '..'))
sys.path.insert(0, os.path.join(ROOT, 'Tools'))
import td_mcp

port, outdir = int(sys.argv[1]), sys.argv[2]
os.makedirs(outdir, exist_ok=True)
c = td_mcp.McpClient('http://127.0.0.1:%d/mcp' % port, 300); c.connect()
TS = 'SceneCaptureToolset.SceneCaptureToolset'
PY = 'editor_toolset.toolsets.python_scripting.PythonScriptingTools'
SRC = open(os.path.join(HERE, 'blade_pie.py'), encoding='utf-8').read()
_world = [None]


def py(src):
    r = c.tool('call_tool', {'toolset_name': PY, 'tool_name': 'execute_python', 'arguments': {'script': {'content': src, 'language': 'Python'}}})
    txt = ''
    for b in r.get('content', []):
        if b.get('type') == 'text':
            txt += b['text']
    try:
        j = json.loads(txt)
        txt = j.get('returnValue', txt) if isinstance(j, dict) else txt
    except ValueError:
        pass
    return str(txt)[:600]


def world():
    if not _world[0]:
        r = td_mcp.unwrap(c.tool('call_tool', {'toolset_name': TS, 'tool_name': 'ListWorlds', 'arguments': {}}))
        paths = re.findall(r'"refPath": "([^"]+)"', json.dumps(r))
        pie = [p for p in paths if 'UEDPIE' in p] or paths
        _world[0] = pie[0]
    return _world[0]


def cap(name, res):
    r = c.tool('call_tool', {'toolset_name': TS, 'tool_name': 'CaptureGameViewport', 'arguments': {'world': {'refPath': world()}, 'maxResolution': int(res or 1600)}})
    if r.get('isError'):
        _world[0] = None
        r = c.tool('call_tool', {'toolset_name': TS, 'tool_name': 'CaptureGameViewport', 'arguments': {'world': {'refPath': world()}, 'maxResolution': int(res or 1600)}})
    for b in r.get('content', []):
        if b.get('type') == 'image':
            open(os.path.join(outdir, name + '.png'), 'wb').write(base64.b64decode(b['data']))
            return
    print('NO IMAGE for', name, json.dumps(r)[:300])


for step in sys.argv[3:]:
    kind, _, arg = step.partition(':')
    if kind == 'act':
        print(step, '->', py("ACT = %r\n" % arg + SRC))
        if arg in ('pie_start', 'pie_stop'):
            _world[0] = None
    elif kind == 'con':
        print(step, '->', py("ACT = %r\n" % ('con:' + arg) + SRC))
    elif kind == 'sleep':
        time.sleep(float(arg))
    elif kind == 'cap':
        name, _, res = arg.partition(':')
        cap(name, res)
    elif kind == 'burst':
        name, n, gap = arg.split(':')
        for i in range(int(n)):
            cap('%s_%02d' % (name, i), 1600)
            time.sleep(float(gap))
    elif kind == 'py':
        print(step, '->', py(open(arg, encoding='utf-8').read()))
print('done')
