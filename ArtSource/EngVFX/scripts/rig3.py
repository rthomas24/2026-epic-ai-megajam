#!/usr/bin/env python3
"""Triad Drift ENG-37 round 3: review sheets, built on rig.py (same editor Play-In-Editor session on port 8017).
usage: python rig3.py <tag> <scenario> [...]
  explosions impacts muzzles engines tracers            the tier-1 sheets (review/tier1_<tag>_<scenario>.png)
  row:<name>:<gap>:<dist>:<frames>:<slomo>:<Sys,Sys..>   an ad-hoc side-view row (review/tier1_<tag>_<name>.png)
Every sheet shows the effects from the SIDE and, where it matters in play, from BEHIND (the chase camera's view)."""
import os, sys, time
import rig
from rig import py, con, capture, stack, CAP, OUT


def cams(dist, back, up=4000.0):
    """Side camera and behind camera (looking along world +X, like the chase camera) for the stage above the player."""
    return py('''
o = pawn.get_actor_location() + unreal.Vector(0, 0, %r)
json.dumps({'side': {'loc': vec(o + unreal.Vector(0, -%r, 0)), 'rot': {'pitch': 0, 'yaw': 90, 'roll': 0}},
            'back': {'loc': vec(o + unreal.Vector(-%r, 0, %r)), 'rot': {'pitch': -12, 'yaw': 0, 'roll': 0}}})
''' % (up, dist, back, back * 0.22))


def spawn_row(names, gap, axis='x', rot=(0, 0, 0), params=None, up=4000.0):
    """rot = (pitch, yaw, roll). Row along world X (axis 'x': seen in profile by the side camera) or along world Y (axis 'y': for the behind camera)."""
    return py('''
clear()
names = %r; gap = %r; params = %r
o = pawn.get_actor_location() + unreal.Vector(0, 0, %r)
for i, n in enumerate(names):
    d = (i - (len(names) - 1) / 2.0) * gap
    k = tag(NFL.spawn_system_at_location(world, unreal.load_asset(SYS + n), o + (unreal.Vector(d, 0, 0) if %r == 'x' else unreal.Vector(0, d, 0)),
            unreal.Rotator(pitch=%r, yaw=%r, roll=%r), unreal.Vector(1, 1, 1), True, True, unreal.NCPoolMethod.NONE, True))
    for pn, pv in (params[i] if params else {}).items():
        k.set_variable_float(pn, float(pv))
"ok"
''' % (names, gap, params, up, axis, rot[0], rot[1], rot[2]))


def shoot(tag_, name, names, gap, dist, frames, h, slomo, view='side', rot=(0, 0, 0), params=None, wait=0.0, fov=50, back=None, w=1500):
    con('slomo ' + slomo)
    order = list(reversed(names)) if view == 'side' else list(names)
    pr = (list(reversed(params)) if view == 'side' else params) if params else None
    spawn_row(order, gap, 'x' if view == 'side' else 'y', rot, pr)
    cam = cams(dist, back or dist)[view]
    time.sleep(wait)
    files = []
    for i in range(frames):
        f = os.path.join(CAP, '%s_%s_%d.png' % (name, view, i))
        capture(cam, f, w, h, fov)
        files.append(f)
    out = os.path.join(CAP, '%s_%s_strip.png' % (name, view))
    if len(files) == 1:
        out = files[0]
    else:
        stack(files, out)
    py('clear()\n"ok"')
    return out


def sheet(tag_, name, strips):
    dst = os.path.join(OUT, 'tier1_%s_%s.png' % (tag_, name))
    if len(strips) == 1:
        import shutil
        shutil.copyfile(strips[0], dst)
    else:
        stack(strips, dst)
    print('sheet', name)


def main():
    tag_ = sys.argv[1]
    con('TD.God 1')
    eng = ['Rift', 'Lancer', 'Bastion', 'Enemy']
    for s in sys.argv[2:]:
        if s == 'explosions':
            ex = ['NS_TD_Explosion_Small', 'NS_TD_Explosion_Medium', 'NS_TD_Explosion_Large']
            a = shoot(tag_, s, ex, 4600, 13000, 5, 520, '0.12')              # the first 0.3 s
            b = shoot(tag_, s + 'late', ex, 4600, 13000, 5, 520, '1.0')        # the next seconds: fire cooling, debris, smoke
            sheet(tag_, s, [a, b])
        elif s == 'impacts':
            im = ['NS_TD_Impact_Hull', 'NS_TD_Impact_Shield']
            a = shoot(tag_, s, im, 1600, 4200, 4, 380, '0.05', rot=(0, -60, 0))      # normal turned 60 deg towards the camera
            b = shoot(tag_, s + 'f', im, 1600, 4200, 4, 380, '0.05', rot=(0, -90, 0))  # normal straight at the camera
            sheet(tag_, s, [a, b])
        elif s == 'muzzles':
            mz = ['NS_TD_Muzzle_' + e for e in eng]
            a = shoot(tag_, s, mz, 900, 3600, 3, 300, '0.02')
            b = shoot(tag_, s, mz, 700, 3600, 3, 300, '0.02', view='back', back=2600)
            sheet(tag_, s, [a, b])
        elif s == 'engines':
            names = ['NS_TD_Engine_Lancer'] * 4 + ['NS_TD_Engine_Rift', 'NS_TD_Engine_Bastion', 'NS_TD_Engine_Enemy']
            params = [{'Throttle': 0.0}, {'Throttle': 0.6}, {'Throttle': 1.0}, {'Throttle': 1.0, 'Boost': 1.0}, {'Throttle': 0.6}, {'Throttle': 1.0, 'Boost': 1.0}, {'Throttle': 0.6}]
            a = shoot(tag_, s, names, 1000, 5600, 2, 420, '0.3', params=params, wait=2.0)
            b = shoot(tag_, s, names, 700, 7400, 1, 420, '0.3', view='back', params=params, wait=2.0, back=3200)
            sheet(tag_, s, [a, b])
        elif s == 'tracers':
            names = ['NS_TD_Tracer_Rift', 'NS_TD_Tracer_Lancer', 'NS_TD_Tracer_Bastion', 'NS_TD_Tracer_Enemy']
            con('slomo 0.05')
            py('''
clear()
for i, n in enumerate(%r):
    tag(NFL.spawn_system_attached(unreal.load_asset(SYS + n), pawn.root_component, 'None', unreal.Vector(900, (i - 1.5) * 420, 900), unreal.Rotator(0, 0, 0),
        unreal.AttachLocation.KEEP_RELATIVE_OFFSET, True, True, unreal.NCPoolMethod.NONE, True))
"ok"
''' % (names,))
            time.sleep(4.0)
            files = []
            for view, loc, h in (('top', (350, 0, 3600), 620), ('behind', (-1700, 0, 1250), 520)):
                cam = py('''
xf = pawn.get_actor_transform()
target = ML.transform_location(xf, unreal.Vector(%r, 0, 900))
loc = ML.transform_location(xf, unreal.Vector%r)
r = ML.find_look_at_rotation(loc, target)
json.dumps({'loc': vec(loc), 'rot': {'pitch': r.pitch, 'yaw': r.yaw, 'roll': r.roll}})
''' % (350 if view == 'top' else 1500, loc))
                f = os.path.join(CAP, 'tracers_%s.png' % view)
                capture(cam, f, 1500, h, 50)
                files.append(f)
            py('clear()\n"ok"')
            sheet(tag_, s, files)
        elif s.startswith('row:'):
            _, name, gap, dist, frames, slomo, systems = s.split(':')
            sheet(tag_, name, [shoot(tag_, name, systems.split(','), float(gap), float(dist), int(frames), 480, slomo)])
    con('slomo 1')


if __name__ == '__main__':
    main()
