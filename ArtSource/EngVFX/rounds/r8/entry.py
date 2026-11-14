#!/usr/bin/env python3
"""The entry from T1's Director's Cut camera (round 8). Play-In-Editor on 8017, Waves1to2, the flown Rift held at a
speed so the world-space wake streams. NS_TD_Entry attached to the ship root (as AtmosphericEntry plays it), User.Intensity
set; scene captures from the director camera (25 m ahead, 18 m to the side, 6 m below, looking back at the ship), from the
other side, and from the chase camera. Then NS_TD_Exit and NS_TD_Exit_Breakaway the same way.
usage: python entry.py <tag> [systems...]"""
import os, sys, time, subprocess
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'scripts'))
import rig
from rig import py, con, capture
tag_ = sys.argv[1] if len(sys.argv) > 1 else 'r8'
SYSTEMS = sys.argv[2:] or ['NS_TD_Entry']
OUT = os.path.join(rig.ROOT, 'Saved', 'EngVFX', 'r8')
CAMS = {'director': ((2500, 1800, -600), 35), 'director_far_side': ((2500, -1800, -600), 35), 'chase': ((-1700, 0, 420), 90)}


def cam(off, fov):
    c = py('''
xf = pawn.get_actor_transform()
target = ML.transform_location(xf, unreal.Vector(0, 0, 0))
loc = ML.transform_location(xf, unreal.Vector(%r, %r, %r))
r = ML.find_look_at_rotation(loc, target)
json.dumps({'loc': vec(loc), 'rot': {'pitch': r.pitch, 'yaw': r.yaw, 'roll': r.roll}})
''' % off)
    return c


rows = []
for sysname in SYSTEMS:
    for inten in ((1.0,) if 'Breakaway' not in sysname else (1.0,)):
        py('''
clear()
k = tag(NFL.spawn_system_attached(unreal.load_asset(SYS + %r), pawn.root_component, 'None', unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0),
        unreal.AttachLocation.KEEP_RELATIVE_OFFSET, True, True, unreal.NCPoolMethod.NONE, True))
k.set_variable_float('Intensity', %r)
"ok"''' % (sysname, inten))
        times = (0.25, 0.9) if 'Breakaway' in sysname else ((0.8, 2.2) if 'Exit' in sysname else (2.0,))
        t0 = time.time()
        row = []
        for t in times:
            while time.time() - t0 < t:
                time.sleep(0.05)
            for name, (off, fov) in CAMS.items():
                f = os.path.join(OUT, '%s_%s_%s_%.1f.png' % (tag_, sysname, name, t))
                capture(cam(off, fov), f, 800, 450, fov)
                row.append(f)
        o = os.path.join(OUT, '%s_%s_row.png' % (tag_, sysname))
        subprocess.check_call(['ffmpeg', '-y', '-loglevel', 'error'] + sum([['-i', f] for f in row], []) +
                              ['-filter_complex', 'xstack=inputs=%d:grid=3x%d' % (len(row), len(row) // 3) if len(row) > 3 else 'hstack=%d' % len(row), o])
        rows.append(o)
py('clear()\n"ok"')
print('\n'.join(rows))
