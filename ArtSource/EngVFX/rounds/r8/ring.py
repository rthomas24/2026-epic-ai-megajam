#!/usr/bin/env python3
"""Rift's ring review (round 8): NS_TD_Rift_RingIdle attached to the flown Rift's 'Portal' socket in Play-In-Editor
(editor on $TD_PORT (21031), Waves1to2, ship held), User.Power 0.5 then 1, User.Radius 44; scene captures from three cameras round
the ship. usage: python ring.py <tag> [radius cm]"""
import os, sys, time, subprocess
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'scripts'))
import rig
from rig import py, con, capture, CAP
tag_ = sys.argv[1] if len(sys.argv) > 1 else 'r8'
RADIUS = float(sys.argv[2]) if len(sys.argv) > 2 else 44.0   # cm: 44 = the ship builder's new disc; the disc on main measures about 65
OUT = os.path.join(rig.ROOT, 'Saved', 'EngVFX', 'r8')
# ship-local camera positions (cm), each looking at the portal socket
CAMS = {'rear34': (-560, 330, 260), 'near3m': (-470, 40, 90), 'chase': (-1700, 0, 420)}   # near3m: 3 m from the ring, as on foot
files = []
for power in (0.5, 1.0):
    py('''
clear()
mesh = [c for c in pawn.get_components_by_class(unreal.StaticMeshComponent) if 'Portal' in [str(s) for s in c.get_all_socket_names()]][0]
k = tag(NFL.spawn_system_attached(unreal.load_asset(SYS + 'NS_TD_Rift_RingIdle'), mesh, 'Portal', unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0),
        unreal.AttachLocation.SNAP_TO_TARGET, True, True, unreal.NCPoolMethod.NONE, True))
k.set_variable_float('Power', %r)
k.set_variable_float('Radius', %r)
"ok"
''' % (power, RADIUS))
    time.sleep(2.5)
    row = []
    for name, off in CAMS.items():
        cam = py('''
mesh = [c for c in pawn.get_components_by_class(unreal.StaticMeshComponent) if 'Portal' in [str(s) for s in c.get_all_socket_names()]][0]
target = mesh.get_socket_location('Portal')
loc = ML.transform_location(pawn.get_actor_transform(), unreal.Vector(%r, %r, %r))
r = ML.find_look_at_rotation(loc, target)
json.dumps({'loc': vec(loc), 'rot': {'pitch': r.pitch, 'yaw': r.yaw, 'roll': r.roll}})
''' % off)
        f = os.path.join(OUT, '%s_ring_p%s_%s.png' % (tag_, power, name))
        capture(cam, f, 800, 600, 40 if name != 'chase' else 60)
        row.append(f)
    files.append(row)
py('clear()\n"ok"')
rows = []
for i, row in enumerate(files):
    o = os.path.join(OUT, '%s_ring_row%d.png' % (tag_, i))
    subprocess.check_call(['ffmpeg', '-y', '-loglevel', 'error'] + sum([['-i', f] for f in row], []) + ['-filter_complex', 'hstack=%d' % len(row), o])
    rows.append(o)
sheet = os.path.join(OUT, '%s_ring_sheet.png' % tag_)
subprocess.check_call(['ffmpeg', '-y', '-loglevel', 'error'] + sum([['-i', f] for f in rows], []) + ['-filter_complex', 'vstack=%d' % len(rows), sheet])
print(sheet)
