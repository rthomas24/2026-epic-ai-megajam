#!/usr/bin/env python3
"""Round 10: M_TD_RegionHaze on a 600 m sphere in the PIE world. usage: python haze.py <tag> (PIE on $TD_PORT running).
Frames: the sphere 500 m ahead from the chase camera and from a side camera 500 m from its surface, then the ship INSIDE it
(sphere centred on the ship)."""
import os, sys, time, subprocess
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'scripts'))
import rig
from rig import py, con, capture
tag_ = sys.argv[1]
OUT = os.path.join(rig.ROOT, 'Saved', 'EngVFX', 'r8', 'spawn')
os.makedirs(OUT, exist_ok=True)
PLAY = [sys.executable, os.path.join(rig.ROOT, 'ArtSource', 'EngVFX', 'scripts', 'play.py'), os.environ.get('TD_PORT', '21031'), OUT]
SPAWN = '''
for a in unreal.GameplayStatics.get_all_actors_with_tag(world, 'EngVFXHaze'):
    a.destroy_actor()
xf = unreal.Transform(%s, unreal.Rotator(0, 0, 0), unreal.Vector(600, 600, 600))   # engine sphere is 100 cm across: 600 m
a = unreal.GameplayStatics.begin_deferred_actor_spawn_from_class(world, unreal.StaticMeshActor, xf)
a = unreal.GameplayStatics.finish_spawning_actor(a, xf)
a.tags = ['EngVFXHaze']
m = a.static_mesh_component
m.set_mobility(unreal.ComponentMobility.MOVABLE)
m.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Sphere'))
mi = m.create_dynamic_material_instance(0, unreal.load_asset('/Game/TriadDrift/Gameplay/VFX/Materials/M_TD_RegionHaze'))
mi.set_vector_parameter_value('Color', unreal.LinearColor(0.70, 0.85, 1.0, 1.0))
m.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
a.set_actor_scale3d(unreal.Vector(600, 600, 600))
"haze at %%s" %% a.get_actor_location()'''
# outside: centre 500 m + 300 m ahead (surface 500 m from the ship)
print(py(SPAWN % 'pawn.get_actor_location() + pawn.get_actor_forward_vector() * 80000'))
time.sleep(1.0)
subprocess.check_call(PLAY + ['cap:%s_haze_out_chase' % tag_], stdout=subprocess.DEVNULL)
cam = py('''
o = pawn.get_actor_location() + pawn.get_actor_forward_vector() * 80000
c = o + pawn.get_actor_right_vector() * 80000 + unreal.Vector(0, 0, 4000)
rr = ML.find_look_at_rotation(c, o)
json.dumps({'loc': vec(c), 'rot': {'pitch': rr.pitch, 'yaw': rr.yaw, 'roll': rr.roll}})''')
capture(cam, os.path.join(OUT, '%s_haze_out_side.png' % tag_), 960, 540, 60)
# inside: centred on the ship
print(py(SPAWN % 'pawn.get_actor_location()'))
time.sleep(1.0)
subprocess.check_call(PLAY + ['cap:%s_haze_in_chase' % tag_], stdout=subprocess.DEVNULL)
py('''
for a in unreal.GameplayStatics.get_all_actors_with_tag(world, 'EngVFXHaze'):
    a.destroy_actor()
"cleared"''')
