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
    elif kind == 'close':   # close:<name>[:dist cm[:side deg]]  a fixed camera looking at the pilot's right hand
        parts = arg.split(':')
        name, dist, side = parts[0], float(parts[1]) if len(parts) > 1 else 220.0, float(parts[2]) if len(parts) > 2 else 60.0
        info = py("w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world(); p = unreal.GameplayStatics.get_player_pawn(w, 0); l = p.get_actor_location(); r = p.get_actor_rotation(); '%f %f %f %f' % (l.x, l.y, l.z, r.yaw)")
        x, y, z, yaw = [float(v) for v in info.split()]
        import math
        a = math.radians(yaw + side)
        cam = (x + math.cos(a) * dist, y + math.sin(a) * dist, z + 40.0)
        tgt = (x, y, z + 10.0)
        f = [tgt[i] - cam[i] for i in range(3)]
        camera = {'location': {'x': cam[0], 'y': cam[1], 'z': cam[2]},
                  'rotation': {'pitch': math.degrees(math.atan2(f[2], math.hypot(f[0], f[1]))), 'yaw': math.degrees(math.atan2(f[1], f[0])), 'roll': 0.0},
                  'projectionMode': 'Perspective', 'fOV': 55.0, 'orthoWidth': 512}
        r = c.tool('call_tool', {'toolset_name': TS, 'tool_name': 'Capture', 'arguments': {'world': {'refPath': world()}, 'camera': camera, 'mode': 'Lit',
                   'options': {'width': 1280, 'height': 720, 'screenPercentage': 1.0, 'visualizationSubMode': ''}}})
        for b in r.get('content', []) or []:
            if b.get('type') == 'image' and b.get('data'):
                open(os.path.join(outdir, name + '.png'), 'wb').write(base64.b64decode(b['data']))
                break
        else:
            print('NO CLOSE IMAGE', json.dumps(r)[:300])
    elif kind == 'py':
        print(step, '->', py(open(arg, encoding='utf-8').read()))
print('done')
