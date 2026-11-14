#!/usr/bin/env python3
"""Issue #31 before/after: NS_TD_Blade_AirSlam at the on-foot pilot's feet (Play-In-Editor, VerdanceOnFoot), slow motion
0.05, captured from a third-person camera (4.5 m behind, 2.2 m up, like the #31 strip) at 0.10 s and 0.30 s of effect time.
usage: python airslam.py <tag>"""
import os, sys, time, subprocess
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'scripts'))
import rig
from rig import py, con, capture
tag_ = sys.argv[1]
OUT = os.path.join(rig.ROOT, 'Saved', 'EngVFX', 'r8')
cam = py('''
xf = pawn.get_actor_transform()
feet = pawn.get_actor_location() - unreal.Vector(0, 0, 90)
loc = ML.transform_location(xf, unreal.Vector(-450, 60, 130))
r = ML.find_look_at_rotation(loc, feet + unreal.Vector(0, 0, 60))
json.dumps({'loc': vec(loc), 'rot': {'pitch': r.pitch, 'yaw': r.yaw, 'roll': r.roll}})
''')
con('slomo 0.05')
py('''
clear()
feet = pawn.get_actor_location() - unreal.Vector(0, 0, 90)
tag(NFL.spawn_system_at_location(world, unreal.load_asset(SYS + 'NS_TD_Blade_AirSlam'), feet, unreal.Rotator(0, 0, 0), unreal.Vector(1, 1, 1), True, True, unreal.NCPoolMethod.NONE, True))
"ok"''')
t0 = time.time()
files = []
for t in (0.10, 0.30):
    while time.time() - t0 < t / 0.05:
        time.sleep(0.05)
    f = os.path.join(OUT, '%s_airslam_%.2f.png' % (tag_, t))
    capture(cam, f, 960, 540, 70)
    files.append(f)
con('slomo 1')
py('clear()\n"ok"')
o = os.path.join(OUT, '%s_airslam.png' % tag_)
subprocess.check_call(['ffmpeg', '-y', '-loglevel', 'error', '-i', files[0], '-i', files[1], '-filter_complex', 'hstack=2', o])
print(o)
