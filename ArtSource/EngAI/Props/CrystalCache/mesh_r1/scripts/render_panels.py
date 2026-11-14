import sys, bpy
from pathlib import Path
from mathutils import Vector
sys.path.insert(0, "/tmp/crystal-cache-build/src")
from cc_paths import RENDERS
from cc_scene import render_view, make_clay_material
from rerender_panels import feature_wire, socket_gizmo, vis_wire_box
FLOOR_Z=0.3005485534667969
O=bpy.data.objects
sc=bpy.context.scene; sc.view_settings.view_transform="Standard"
cr=O["SM_CrystalCacheCradle"]; cy=O["SM_CrystalCacheCrystal"]; pilot=O["FIG_Pilot_180"]
for n in ("BAKE_CrystalCacheCradle","SRC_CrystalCacheCradle","SRC_CrystalCacheCrystal","UCX_SM_CrystalCacheCradle_00","UCX_SM_CrystalCacheCrystal_00"):
    if n in O: O[n].hide_render=True
def show(*objs, hide=False):
    for o in objs: o.hide_render=hide
cy.location=(0,0,FLOOR_Z)
pilot.location=Vector((-1.7,-2.4,0.0))
show(cr,cy,pilot)
render_view(RENDERS/"assembled_hero.png", loc=Vector((4.2,-5.6,2.35)), target=Vector((-0.2,0.0,0.8)), lens=42)
show(cy,pilot,hide=True)
render_view(RENDERS/"cradle_alone.png", loc=Vector((4.2,-5.4,2.35)), target=Vector((0.0,0.0,0.85)), lens=45)
show(cr,hide=True); show(cy); cy.location=(0,0,0)
render_view(RENDERS/"crystal_alone.png", loc=Vector((2.15,-2.35,1.45)), target=Vector((0.0,0.0,0.65)), lens=50)
cy.location=(0,0,FLOOR_Z); show(cr,cy)
render_view(RENDERS/"closeup.png", loc=Vector((1.85,-2.55,1.70)), target=Vector((0.05,-0.05,1.05)), lens=45)
show(pilot); pilot.location=Vector((1.6,-1.2,0.0))
render_view(RENDERS/"front_ortho.png", loc=Vector((6.0,0.0,0.95)), target=Vector((0.0,0.0,0.9)), ortho=5.2)
pilot.location=Vector((-1.5,-1.0,0.0))
render_view(RENDERS/"end_ortho.png", loc=Vector((0.0,-6.0,0.95)), target=Vector((0.0,0.0,0.9)), ortho=3.6)
clay=make_clay_material()
for o in (cr,cy,pilot):
    o.data.materials.clear(); o.data.materials.append(clay)
pilot.location=Vector((-1.7,-2.4,0.0))
render_view(RENDERS/"clay.png", loc=Vector((4.2,-5.6,2.35)), target=Vector((-0.2,0.0,0.8)), lens=42)
w1=feature_wire(cr,"W_cradle",angle=28.0,radius=0.0028); w2=feature_wire(cy,"W_crystal",angle=20.0,radius=0.0022)
render_view(RENDERS/"clay_wire.png", loc=Vector((4.2,-5.6,2.35)), target=Vector((-0.2,0.0,0.8)), lens=42)
w1.hide_render=True; w2.hide_render=True
pilot.location=Vector((1.85,-2.40,0.0))
vis_wire_box(O["UCX_SM_CrystalCacheCradle_00"],"VIS_UCX_cradle",(0.25,0.95,0.30),0.020)
vis_wire_box(O["UCX_SM_CrystalCacheCrystal_00"],"VIS_UCX_crystal",(0.25,0.55,1.0),0.014)
socket_gizmo(O["SOCKET_Crystal"].matrix_world.translation.copy())
render_view(RENDERS/"collision_socket.png", loc=Vector((3.0,-4.6,4.2)), target=Vector((0.0,0.15,0.35)), lens=35)
print("[render] socket world", tuple(O["SOCKET_Crystal"].matrix_world.translation))
print("[render] DONE")
