#!/usr/bin/env python3
"""Space reactor review, T4's primary route (PIE running on $TD_PORT, stage set by the caller): logs the ship and reactor
positions and bounds, spawns NS_TD_Reactor_Exposed at the reactor (the real cue may not play on the debug stage), fires at
the reactor through the real input for 6 s and captures, then spawns NS_TD_Reactor_Hit on the reactor's surface facing the
camera. usage: python reactor2.py <tag>"""
import os, sys, time, subprocess
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'scripts'))
import rig
from rig import py, con
tag_ = sys.argv[1]
OUT = os.path.join(rig.ROOT, 'Saved', 'EngVFX', 'r8')
cap = lambda n: subprocess.check_call([sys.executable, os.path.join(rig.HERE, 'cap.py'), str(rig.PORT), os.path.join(OUT, '%s_%s.png' % (tag_, n)), '1600'])
info = py('''
r = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.TDDominionReactor)
out = {'ship': vec(pawn.get_actor_location())}
if r:
    o, e = r[0].get_actor_bounds(True)
    out.update({'reactor': vec(r[0].get_actor_location()), 'bounds_centre': vec(o), 'extent': vec(e), 'dist_m': (r[0].get_actor_location() - pawn.get_actor_location()).length() / 100.0})
json.dumps(out)''')
print('positions', info)
con('TD.God'); con('TD.World.AimAt Reactor 2'); time.sleep(2.5)
cap('aimed')
con('TD.Input.Key LeftMouseButton 6'); time.sleep(1.5); cap('fire_1'); time.sleep(1.5); cap('fire_2'); time.sleep(3.2)
con('TD.World.DominionStatus')
# the effects at the reactor at real scale (VFXScale 1: what the cue plays)
print(py('''
r = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.TDDominionReactor)[0]
o, e = r.get_actor_bounds(True)
tag(NFL.spawn_system_at_location(world, unreal.load_asset(SYS + 'NS_TD_Reactor_Exposed'), o, r.get_actor_rotation(), unreal.Vector(1, 1, 1), True, True, unreal.NCPoolMethod.NONE, True))
"spawned exposed"'''))
time.sleep(0.5); cap('exposed_1'); time.sleep(1.2); cap('exposed_2')
print(py('''
r = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.TDDominionReactor)[0]
o, e = r.get_actor_bounds(True)
d = pawn.get_actor_location() - o
d = d / max(d.length(), 1.0)
for i in range(3):
    p = o + d * (min(e.x, e.y, e.z) * 0.9) + unreal.Vector(0, (i - 1) * 300, (i - 1) * 200)
    tag(NFL.spawn_system_at_location(world, unreal.load_asset(SYS + 'NS_TD_Reactor_Hit'), p, unreal.MathLibrary.make_rot_from_x(d), unreal.Vector(1, 1, 1), True, True, unreal.NCPoolMethod.NONE, True))
"spawned hits"'''))
time.sleep(0.1); cap('hit_spawned')
py('clear()\n"ok"')
