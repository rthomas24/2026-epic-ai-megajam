#!/usr/bin/env python3
"""Dome edge crossing the view (round 9f): with the dome up (dome.py run first), a scene capture from just inside the shell
(27 m from its centre) looking along the ship's forward axis, so the shell crosses the screen."""
import os, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'scripts'))
import rig
from rig import py, capture
tag_ = sys.argv[1]
cam = py('''
d = [a for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor) if 'ShieldDome' in a.get_class().get_name()]
c = d[0].get_actor_location() if d else pawn.get_actor_location()
fwd = pawn.get_actor_forward_vector(); right = pawn.get_actor_right_vector()
loc = c + right * 2700 + unreal.Vector(0, 0, 300)
r = unreal.MathLibrary.find_look_at_rotation(loc, loc + fwd * 1000 + right * 250)
json.dumps({'loc': vec(loc), 'rot': {'pitch': r.pitch, 'yaw': r.yaw, 'roll': r.roll}, 'found': len(d)})''')
print('dome actors', cam.get('found'))
capture(cam, os.path.join(rig.ROOT, 'Saved', 'EngVFX', 'r8', '%s_dome_edge.png' % tag_), 1600, 900, 90)
