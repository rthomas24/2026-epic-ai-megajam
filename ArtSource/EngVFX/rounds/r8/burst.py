#!/usr/bin/env python3
"""Burst review (Lead's method, round 9n): a fixed world-space scene-capture camera (no HUD, nothing in front), slow
motion 0.1, 12 frames one every real 0.1 s, and the effect spawned at the subject by us (the SIZE judgement).
usage: python burst.py <tag> <mode> [args]
  reactor <dist_m>            NS_TD_Reactor_Hit x3 on the reactor surface, then NS_TD_Reactor_Exposed at its centre,
                              camera at the side of the ship->reactor line, dist_m from the reactor
  rocks <dist_m>              Break_S, _M, _L in a row 'dist_m' from a side camera (each 30 m apart)
  hyper                       camera 200 m to the side of the ship's nose line, then the real jump (TD.Input.Key H)
PIE must be running on $TD_PORT."""
import os, sys, time, subprocess
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'scripts'))
import rig
from rig import py, con, capture
tag_, mode = sys.argv[1], sys.argv[2]
arg = float(sys.argv[3]) if len(sys.argv) > 3 else 0.0
OUT = os.path.join(rig.ROOT, 'Saved', 'EngVFX', 'r8', 'burst')
os.makedirs(OUT, exist_ok=True)


def burst(name, cam, fov=40, n=12, w=960, h=540):
    files = []
    for i in range(n):
        f = os.path.join(OUT, '%s_%s_%02d.png' % (tag_, name, i))
        capture(cam, f, w, h, fov)
        files.append(f)
        time.sleep(0.1)
    o = os.path.join(OUT, '%s_%s.png' % (tag_, name))
    sel = files[0:12:2]   # 6 of the 12 in the strip, labelled by frame
    args = ['ffmpeg', '-y', '-loglevel', 'error'] + sum([['-i', x] for x in sel], [])
    fc = ''.join('[%d]' % k for k in range(len(sel))) + 'xstack=inputs=%d:grid=3x2' % len(sel)   # frames 0, 2, 4, 6, 8, 10, left to right
    subprocess.check_call(args + ['-filter_complex', fc, o])
    print(o)


SETUP = '''
r = (unreal.GameplayStatics.get_all_actors_of_class(world, unreal.TDDominionReactor) or [None])[0]
def look(loc, target):
    rr = ML.find_look_at_rotation(loc, target)
    return {'loc': vec(loc), 'rot': {'pitch': rr.pitch, 'yaw': rr.yaw, 'roll': rr.roll}}
'''
if mode == 'reactor':
    cam = py(SETUP + '''
o, e = r.get_actor_bounds(True)
d = pawn.get_actor_location() - o; d.z = 0; d = d / max(d.length(), 1.0)
side = unreal.Vector(d.y, -d.x, 0.0)
json.dumps(look(o + side * %r + unreal.Vector(0, 0, 1500), o))''' % (arg * 100.0))
    con('slomo 0.1')
    py(SETUP + '''
clear()
o, e = r.get_actor_bounds(True)
d = pawn.get_actor_location() - o; d = d / max(d.length(), 1.0)
for i in range(3):
    p = o + d * (min(e.x, e.y, e.z) * 0.9) + unreal.Vector(0, (i - 1) * 400, (i - 1) * 300)
    tag(NFL.spawn_system_at_location(world, unreal.load_asset(SYS + 'NS_TD_Reactor_Hit'), p, ML.make_rot_from_x(d), unreal.Vector(1, 1, 1), True, True, unreal.NCPoolMethod.NONE, True))
"hits"''')
    burst('hit_%dm' % arg, cam)
    con('slomo 1'); time.sleep(1.5); con('slomo 0.1')
    py(SETUP + '''
clear()
o, e = r.get_actor_bounds(True)
tag(NFL.spawn_system_at_location(world, unreal.load_asset(SYS + 'NS_TD_Reactor_Exposed'), o, r.get_actor_rotation(), unreal.Vector(1, 1, 1), True, True, unreal.NCPoolMethod.NONE, True))
"exposed"''')
    burst('exposed_%dm' % arg, cam, n=12)
    con('slomo 1'); py('clear()\n"ok"')
elif mode == 'reactor_up':   # 9p: above the trench, up and back along the ship->reactor line, looking down at the reactor
    cam = py(SETUP + '''
o, e = r.get_actor_bounds(True)
d = pawn.get_actor_location() - o; d.z = 0; d = d / max(d.length(), 1.0)
json.dumps(look(o + d * (%r * 0.6) + unreal.Vector(0, 0, %r * 0.8), o))''' % (arg * 100.0, arg * 100.0))
    con('slomo 0.1')
    py(SETUP + '''
clear()
o, e = r.get_actor_bounds(True)
d = pawn.get_actor_location() - o; d = d / max(d.length(), 1.0)
up = unreal.Vector(0, 0, 1)
for i in range(3):
    p = o + (d * 0.4 + up * 0.9) * max(e.x, e.y, e.z) * 0.8 + unreal.Vector(0, (i - 1) * 400, 0)
    tag(NFL.spawn_system_at_location(world, unreal.load_asset(SYS + 'NS_TD_Reactor_Hit'), p, ML.make_rot_from_x(up), unreal.Vector(1, 1, 1), True, True, unreal.NCPoolMethod.NONE, True))
"hits"''')
    burst('hitup_%dm' % arg, cam)
    con('slomo 1'); time.sleep(1.5); con('slomo 0.1')
    py(SETUP + '''
clear()
o, e = r.get_actor_bounds(True)
tag(NFL.spawn_system_at_location(world, unreal.load_asset(SYS + 'NS_TD_Reactor_Exposed'), o, r.get_actor_rotation(), unreal.Vector(1, 1, 1), True, True, unreal.NCPoolMethod.NONE, True))
"exposed"''')
    burst('exposedup_%dm' % arg, cam)
    con('slomo 1'); py('clear()\n"ok"')
elif mode == 'rocks':
    cam = py(SETUP + '''
o = pawn.get_actor_location() + pawn.get_actor_forward_vector() * 30000
right = pawn.get_actor_right_vector()
json.dumps(look(o + right * %r, o))''' % (arg * 100.0))
    con('slomo 0.1')
    py(SETUP + '''
clear()
o = pawn.get_actor_location() + pawn.get_actor_forward_vector() * 30000
fwd = pawn.get_actor_forward_vector()
for i, n in enumerate(['NS_TD_Asteroid_Break_S', 'NS_TD_Asteroid_Break_M', 'NS_TD_Asteroid_Break_L']):
    tag(NFL.spawn_system_at_location(world, unreal.load_asset(SYS + n), o + fwd * ((i - 1) * 3000), unreal.Rotator(0, 0, 0), unreal.Vector(1, 1, 1), True, True, unreal.NCPoolMethod.NONE, True))
"rocks"''')
    burst('rocks_%dm' % arg, cam, fov=50)
    con('slomo 1'); py('clear()\n"ok"')
elif mode == 'hyper':   # 9p: two fixed cameras: 200 m to the side of the spool point, and 300 m ahead on the jump line looking back
    cams = py(SETUP + '''
o = pawn.get_actor_location(); f = pawn.get_actor_forward_vector(); rt = pawn.get_actor_right_vector()
json.dumps({'side': look(o + rt * 20000 + unreal.Vector(0, 0, 2000), o + f * 3000), 'ahead': look(o + f * 30000 + rt * 2500 + unreal.Vector(0, 0, 1500), o)})''')
    con('TD.Input.Key H 1.6'); time.sleep(0.3)
    burst('hyper_side', cams['side'], fov=60, n=8)
    burst('hyper_ahead', cams['ahead'], fov=60, n=8)
