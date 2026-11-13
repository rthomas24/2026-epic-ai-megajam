#!/usr/bin/env python3
"""Triad Drift ENG-37: review-sheet rig. Drives the editor session (port 8017) while Play-In-Editor is running on
L_Galaxy: spawns effects on a clear "stage" above the player ship and renders them in PROFILE with the engine's
offscreen scene capture from a side camera (the chase camera only ever sees effects end-on).

usage: python rig.py <tag> [scenario ...]     scenarios: explosions impacts engines tracers   (default: all)
writes ArtSource/EngVFX/review/tier1_<tag>_<scenario>.png  (frames stacked top to bottom = time order)

Notes for whoever reads the sheets:
- the capture uses the world's post-process volumes but not the player camera's own overrides, and has no temporal
  history (no TAA accumulation), so it is slightly noisier than the real viewport;
- 'tracers' are attached to the player ship (about 6 000 cm/s), not to real projectiles (25 000-55 000 cm/s): real
  trails are several times longer.
"""
import base64, json, os, subprocess, sys, time
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..', '..'))
sys.path.insert(0, os.path.join(ROOT, 'Tools'))
import td_mcp

PORT = int(os.environ.get('TD_PORT', '8017'))
CAP = os.path.join(ROOT, 'Saved', 'EngVFX', 'cap')
OUT = os.path.join(ROOT, 'ArtSource', 'EngVFX', 'review')
os.makedirs(CAP, exist_ok=True)
os.makedirs(OUT, exist_ok=True)
c = td_mcp.McpClient('http://127.0.0.1:%d/mcp' % PORT, 300)
c.connect()
PY = 'editor_toolset.toolsets.python_scripting.PythonScriptingTools'
SC = 'SceneCaptureToolset.SceneCaptureToolset'

PRE = r'''
import json
SYS = '/Game/TriadDrift/Gameplay/VFX/Systems/'
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
NFL = unreal.NiagaraFunctionLibrary
ML = unreal.MathLibrary
def tag(comp):
    comp.component_tags = list(comp.component_tags) + ['EngVFXPreview']
    return comp
def clear():
    for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
        for k in a.get_components_by_tag(unreal.NiagaraComponent, 'EngVFXPreview'):
            k.destroy_component(k)
def vec(v):
    return {'x': v.x, 'y': v.y, 'z': v.z}
'''


def py(src):
    r = c.tool('call_tool', {'toolset_name': PY, 'tool_name': 'execute_python', 'arguments': {'script': {'content': PRE + src, 'language': 'Python'}}})
    txt = ''.join(b.get('text', '') for b in r.get('content', []) if b.get('type') == 'text')
    if r.get('isError'):
        raise RuntimeError(txt[:800])
    try:
        v = json.loads(txt)
        v = v.get('returnValue', v) if isinstance(v, dict) else v
        return json.loads(v) if isinstance(v, str) and v[:1] in '{[' else v
    except ValueError:
        return txt


def con(cmd):
    c.tool('call_tool', {'toolset_name': 'ConsoleToolset.ConsoleToolset', 'tool_name': 'RunConsoleCommand', 'arguments': {'command': cmd}})


_world = None
_mode = None


def capture(cam, path, w=1500, h=500, fov=50):
    global _world, _mode
    if _world is None:
        txt = json.dumps(td_mcp.unwrap(c.tool('call_tool', {'toolset_name': SC, 'tool_name': 'ListWorlds', 'arguments': {}})))
        import re
        paths = re.findall(r'"refPath": "([^"]+)"', txt)
        _world = ([p for p in paths if 'UEDPIE' in p] or paths)[0]
        modes = json.dumps(td_mcp.unwrap(c.tool('call_tool', {'toolset_name': SC, 'tool_name': 'ListModes', 'arguments': {'bIncludeUnavailable': False}})))
        _mode = next((m for m in ('Lit', 'FinalColor', 'Final', 'SceneColor') if '"%s"' % m in modes), None)
        if _mode is None:
            raise RuntimeError('no known capture mode in ' + modes[:600])
    r = c.tool('call_tool', {'toolset_name': SC, 'tool_name': 'Capture', 'arguments': {
        'world': {'refPath': _world}, 'mode': _mode, 'options': {'width': w, 'height': h},
        'camera': {'location': cam['loc'], 'rotation': cam['rot'], 'fOV': fov}}})
    for b in r.get('content', []):
        if b.get('type') == 'image':
            with open(path, 'wb') as f:
                f.write(base64.b64decode(b['data']))
            return True
    raise RuntimeError('capture returned no image: ' + json.dumps(r)[:500])


def stack(frames, out):
    args = ['ffmpeg', '-y', '-loglevel', 'error']
    for f in frames:
        args += ['-i', f]
    args += ['-filter_complex', ''.join('[%d]' % i for i in range(len(frames))) + 'vstack=inputs=%d' % len(frames), out]
    subprocess.check_call(args)


def stage(names, gap, dist, boosts=None, up=4000.0):
    """Spawn the systems in a row along world X on a stage above the player; returns the side camera."""
    return py('''
clear()
names = %r; gap = %r; dist = %r; boosts = %r
o = pawn.get_actor_location() + unreal.Vector(0, 0, %r)
for i, n in enumerate(names):
    k = tag(NFL.spawn_system_at_location(world, unreal.load_asset(SYS + n), o + unreal.Vector((i - (len(names) - 1) / 2.0) * gap, 0, 0),
            unreal.Rotator(0, 0, 0), unreal.Vector(1, 1, 1), True, True, unreal.NCPoolMethod.NONE, True))
    if boosts:
        k.set_variable_float('Boost', float(boosts[i]))
json.dumps({'loc': vec(o + unreal.Vector(0, -dist, 0)), 'rot': {'pitch': 0, 'yaw': 90, 'roll': 0}})
''' % (names, gap, dist, boosts, up))


def scenario_row(tag_, name, names, gap, dist, frames, h, slomo='0.04', boosts=None, wait=0.0, fov=50):
    con('slomo ' + slomo)
    names = list(reversed(names))   # world +X is screen LEFT for this camera: reverse so the sheet reads left to right
    boosts = list(reversed(boosts)) if boosts else None
    cam = stage(names, gap, dist, boosts)
    time.sleep(wait)
    files = []
    for i in range(frames):
        f = os.path.join(CAP, '%s_%d.png' % (name, i))
        capture(cam, f, 1500, h, fov)
        files.append(f)
    stack(files, os.path.join(OUT, 'tier1_%s_%s.png' % (tag_, name)))
    py('clear()\n"ok"')


def scenario_tracers(tag_):
    con('slomo 0.05')
    names = ['NS_TD_Tracer_Rift', 'NS_TD_Tracer_Lancer', 'NS_TD_Tracer_Bastion', 'NS_TD_Tracer_Enemy']
    py('''
clear()
for i, n in enumerate(%r):
    tag(NFL.spawn_system_attached(unreal.load_asset(SYS + n), pawn.root_component, 'None', unreal.Vector(900, 0, 700 + i * 330), unreal.Rotator(0, 0, 0),
        unreal.AttachLocation.KEEP_RELATIVE_OFFSET, True, True, unreal.NCPoolMethod.NONE, True))
"ok"
''' % (names,))
    time.sleep(4.0)
    files = []
    for i in range(2):
        cam = py('''
xf = pawn.get_actor_transform()
target = ML.transform_location(xf, unreal.Vector(350, 0, 1195))
loc = ML.transform_location(xf, unreal.Vector(350, 2300, 1195))
r = ML.find_look_at_rotation(loc, target)
json.dumps({'loc': vec(loc), 'rot': {'pitch': r.pitch, 'yaw': r.yaw, 'roll': r.roll}})
''')
        f = os.path.join(CAP, 'tracers_%d.png' % i)
        capture(cam, f, 1500, 760, 50)
        files.append(f)
    stack(files, os.path.join(OUT, 'tier1_%s_tracers.png' % tag_))
    py('clear()\n"ok"')


def main():
    tag_ = sys.argv[1]
    which = sys.argv[2:] or ['explosions', 'impacts', 'engines', 'tracers']
    con('TD.God 1')
    eng = ['Rift', 'Lancer', 'Bastion', 'Enemy']
    for s in which:
        if s == 'explosions':
            scenario_row(tag_, s, ['NS_TD_Explosion_Small', 'NS_TD_Explosion_Medium', 'NS_TD_Explosion_Large'], 4200, 12500, 5, 520, slomo='0.22')
        elif s == 'impacts':
            scenario_row(tag_, s, ['NS_TD_Impact_Hull', 'NS_TD_Impact_Shield', 'NS_TD_Muzzle_Rift', 'NS_TD_Muzzle_Lancer', 'NS_TD_Muzzle_Bastion', 'NS_TD_Muzzle_Enemy'],
                         700, 4600, 3, 330, slomo='0.06')
        elif s == 'engines':
            scenario_row(tag_, s, ['NS_TD_Engine_' + e for e in eng] + ['NS_TD_Engine_' + e for e in eng], 560, 5200, 2, 330, slomo='0.3',
                         boosts=[0, 0, 0, 0, 1, 1, 1, 1], wait=2.0)
        elif s == 'tracers':
            scenario_tracers(tag_)
        print('sheet', s)
    con('slomo 1')


if __name__ == '__main__':
    main()
