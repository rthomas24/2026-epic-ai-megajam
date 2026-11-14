"""Renders for the contact sheet: opens blockout/DominionInterior_Blockout.blend (kit pieces at scale 1). Neutral studio
HDRI (Blender's bundled studio.exr) via render_common, Cycles CPU, AgX, plus the C++ point lights. Nothing is saved back.
blender -b --factory-startup --python render_views.py -- <plans_r1_dir> <out_dir> [samples]
ONLY_SHOTS=a.png,b.png to re-render a subset."""
import bpy, sys, os, math
argv = sys.argv[sys.argv.index('--') + 1:]
PLANS, OUT = argv[0], argv[1]; SAMPLES = int(argv[2]) if len(argv) > 2 else 40
sys.path.insert(0, os.path.join(PLANS, "source")); import render_common as R
os.makedirs(OUT, exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=os.path.join(PLANS, "blockout", "DominionInterior_Blockout.blend"))
def ue(x, y, z): return (x / 100.0, -y / 100.0, z / 100.0)
R.world(0.8, 0.8, bg=(0.02, 0.022, 0.03)); R.engine(SAMPLES, (1600, 900))
sc = bpy.context.scene; sc.cycles.light_sampling_threshold = 0.01
def cutaway(on, keep_y_walls_below=None):
    for o in bpy.data.objects:
        k = o.get("ue_xyz_yaw")
        if not k: continue
        if o.get("is_ceiling"): o.hide_render = on
        m = o.get("kit_mesh")
        if m in ("SM_DI_Room_Wall", "SM_DI_Walkway_Ring") and k[1] > 500 and abs(k[3]) in (90, 180): o.hide_render = on
        if m == "SM_DI_Corridor_Straight": o.hide_render = False
        if m == "SM_DI_Room_Wall" and k[0] == -1200 and k[2] >= 1200: o.hide_render = on   # pocket back wall above the hangar
def tp(on):
    for o in bpy.data.objects:
        if o.name.startswith("TP_") or o.name == "Figure_180cm_TrimPreview": o.hide_render = not on
tp(False)
cutaway(True)
c = R.camera("Cam_Cut_A", ue(2400, 3900, 3300), ue(2400, -500, 0), lens=16, clip_end=600)
R.shot(c, os.path.join(OUT, "bo_cutaway_hangar_corridor.png"))
c = R.camera("Cam_Cut_B", ue(10900, 3700, 3500), ue(11300, -400, 200), lens=16, clip_end=600)
R.shot(c, os.path.join(OUT, "bo_cutaway_warden_core.png"))
c = R.camera("Cam_Top", ue(3800, 0, 12000), ue(3800, 0, 0), ortho=232, clip_end=600); c.rotation_euler = (0, 0, 0)
R.shot(c, os.path.join(OUT, "bo_top.png"), (2400, 660))
cutaway(False)
sc.render.resolution_x, sc.render.resolution_y = 1600, 900
for (n, a, b, lens) in (("eye_hangar.png", (-1050, -900, 170), (1200, 200, 250), 20),
                        ("eye_corridor.png", (1350, 90, 170), (5200, -20, 140), 28),
                        ("eye_arena.png", (5450, -350, 170), (7600, 500, 220), 20),
                        ("eye_warden.png", (8520, 250, 170), (9900, 0, 180), 22),
                        ("eye_core.png", (11450, -600, 170), (13000, 100, 650), 18)):
    c = R.camera("Cam_" + n, ue(*a), ue(*b), lens=lens, clip_end=600)
    R.shot(c, os.path.join(OUT, n))
# trim preview: only its bay, brighter studio light
for o in bpy.data.objects:
    if o.type in ('MESH', 'LIGHT'): o.hide_render = True
tp(True)
R.world(1.0, 2.2, bg=(0.16, 0.165, 0.175))
from mathutils import Vector
for (n, loc, tgt, e, size) in (("TP_Key", ue(150, 5250, 520), ue(-50, 6200, 250), 2500, 3.0), ("TP_Fill", ue(-350, 5500, 200), ue(0, 6200, 150), 700, 2.0)):
    l = bpy.data.lights.new(n, 'AREA'); l.energy = e; l.size = size; lo = bpy.data.objects.new(n, l)
    lo.location = loc; lo.rotation_euler = (Vector(tgt) - Vector(loc)).to_track_quat('-Z', 'Y').to_euler(); sc.collection.objects.link(lo)
c = R.camera("Cam_TP", ue(250, 6000 - 950, 230), ue(-20, 6200, 260), lens=26, clip_end=200)
R.shot(c, os.path.join(OUT, "trim_preview.png"))
c = R.camera("Cam_TP_close", ue(-60, 6000 - 140, 120), ue(-150, 6200, 90), lens=35, clip_end=200)
R.shot(c, os.path.join(OUT, "trim_preview_close.png"))
print("RENDER_DONE")
