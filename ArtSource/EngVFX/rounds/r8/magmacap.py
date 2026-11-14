#!/usr/bin/env python3
"""Round 10: NS_TD_Magma_Heat on the TDMagmaVolume nearest the ship (PIE on Cinder running). usage: magmacap.py <tag>
Fixed cameras at eye height (1.7 m) 5 m and 20 m from the volume's edge, 3 frames each."""
import os, sys, time, subprocess, json
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'scripts'))
import rig
from rig import py, capture
tag_ = sys.argv[1]
OUT = os.path.join(rig.ROOT, 'Saved', 'EngVFX', 'r8', 'spawn')
cam = py('''
clear()
vs = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.TDMagmaVolume)
p = pawn.get_actor_location()
v = sorted(vs, key=lambda a: (a.get_actor_location() - p).length())[0]
o, e = v.get_actor_bounds(False)
c = tag(NFL.spawn_system_attached(unreal.load_asset(SYS + 'NS_TD_Magma_Heat'), v.root_component, 'None', unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0),
        unreal.AttachLocation.KEEP_RELATIVE_OFFSET, True, True, unreal.NCPoolMethod.NONE, True))
c.set_variable_float('SizeX', e.x * 2.0); c.set_variable_float('SizeY', e.y * 2.0); c.set_variable_float('Intensity', 1.0)
top = v.get_actor_location()
cams = []
for d in (500.0, 2000.0):
    cl = top + unreal.Vector(e.x + d, 0, 170.0)
    rr = ML.find_look_at_rotation(cl, top + unreal.Vector(e.x * 0.5, 0, 0))
    cams.append({'loc': vec(cl), 'rot': {'pitch': rr.pitch, 'yaw': rr.yaw, 'roll': rr.roll}})
json.dumps({'cams': cams, 'size': [e.x * 2, e.y * 2]})''')
print(cam)
time.sleep(2.5)
files = []
for j, cm in enumerate((cam if isinstance(cam, dict) else json.loads(cam))['cams']):
    for i in range(3):
        f = os.path.join(OUT, '%s_magma_%dm_%02d.png' % (tag_, (5, 20)[j], i)); capture(cm, f, 960, 540, 70); files.append(f); time.sleep(0.25)
subprocess.check_call(['ffmpeg', '-y', '-loglevel', 'error'] + sum([['-i', x] for x in files], []) +
                      ['-filter_complex', ''.join('[%d]' % k for k in range(6)) + 'xstack=inputs=6:grid=3x2', os.path.join(OUT, tag_ + '_magma.png')])
py('clear()\n"ok"')
