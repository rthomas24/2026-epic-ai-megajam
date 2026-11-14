#!/usr/bin/env python3
"""Bastion's shield dome (round 9): Play-In-Editor on $TD_PORT (Waves1to2). DA_Bastion_Visuals.Power2Active is set IN MEMORY
only (Characters/Squad is not ours to save), the Bastion raises the dome; captures: the chase camera (inside the dome)
and a scene capture from 90 m outside. usage: python dome.py <tag> [inmem 0|1]"""
import os, sys, time, subprocess
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'scripts'))
import rig
from rig import py, con, capture
tag_ = sys.argv[1]
inmem = len(sys.argv) < 3 or sys.argv[2] == '1'
OUT = os.path.join(rig.ROOT, 'Saved', 'EngVFX', 'r8')
if inmem:
    print(py('''
da = unreal.load_asset('/Game/TriadDrift/Gameplay/Characters/Squad/DA_Bastion_Visuals')
c = da.get_editor_property('power2_active')
c.set_editor_property('vfx', unreal.load_asset(SYS + 'NS_TD_ShieldDome'))
da.set_editor_property('power2_active', c)
"in memory: " + str(da.get_editor_property('power2_active').get_editor_property('vfx'))[:60]'''))
con('TD.Squad.Switch Bastion'); time.sleep(1.5)
con('TD.World.Hold 900 0'); con('TD.Power.Ready'); con('TD.Power.Use Signature2'); time.sleep(1.5)
files = []
for name, off, fov in (('inside_chase', None, 0), ('outside_90m', (-2500, 8600, 1800), 60), ('inside_side', (-900, 400, 250), 90)):
    f = os.path.join(OUT, '%s_dome_%s.png' % (tag_, name))
    if off is None:
        subprocess.check_call([sys.executable, os.path.join(rig.HERE, 'cap.py'), str(rig.PORT), f, '1600'])
    else:
        cam = py('''
xf = pawn.get_actor_transform()
loc = ML.transform_location(xf, unreal.Vector(%r, %r, %r))
r = ML.find_look_at_rotation(loc, pawn.get_actor_location())
json.dumps({'loc': vec(loc), 'rot': {'pitch': r.pitch, 'yaw': r.yaw, 'roll': r.roll}})''' % off)
        capture(cam, f, 1600, 900, fov)
    files.append(f)
o = os.path.join(OUT, '%s_dome.png' % tag_)
subprocess.check_call(['ffmpeg', '-y', '-loglevel', 'error'] + sum([['-i', x] for x in files], []) +
                      ['-filter_complex', '[0]scale=1600:900[a];[1]scale=1600:900[b];[2]scale=1600:900[c];[a][b][c]hstack=3,scale=2400:-1', o])
print(o)
