# Pilot step 1 (Blender 5.2, headless): import the source GLB (read-only), measure it, extract the textures,
# make in-place versions of travelling locomotion, export FBX (mesh + one FBX per animation), save work/Pilot_v01.blend.
# Run: blender --background --factory-startup --python scripts/p10_build.py
import bpy, os, sys, json, math
import numpy as np
from mathutils import Matrix, Vector
from bpy_extras import anim_utils

HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE)
REPO = os.path.abspath(os.path.join(ROOT, "..", "..", ".."))
SRC = os.path.join(REPO, "ArtSource", "Characters", "Samples", "SentinelArmor",
                   "Meshy_AI_Sentinel_Armor_Turnaround_20261113_animated_ue5_mannequin_All_animation.glb")
EXPORT = os.path.join(ROOT, "export"); WORK = os.path.join(ROOT, "work")
NAMES = {"Running": "A_Pilot_Run", "Walking": "A_Pilot_Walk", "Casual_Walk": "A_Pilot_CasualWalk",
         "Idle_02": "A_Pilot_Idle02", "Idle_3": "A_Pilot_Idle3", "Idle_6": "A_Pilot_Idle6"}
TRAVEL_CM = 5.0     # net pelvis travel above this = a travelling animation
report = {}

scene = bpy.context.scene
for o in list(bpy.data.objects): bpy.data.objects.remove(o)
scene.render.fps = 30           # the glTF keys are on a 30 fps grid; at Blender's default 24 they land between frames
bpy.ops.import_scene.gltf(filepath=SRC)
arm = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
mesh = next(o for o in bpy.data.objects if o.type == 'MESH' and o.parent == arm)
for o in list(bpy.data.objects):
    if o not in (arm, mesh): bpy.data.objects.remove(o)      # the importer's bone-shape icosphere
arm.name = "root"               # Unreal turns this FBX node into a 'root' bone at the origin (the rig has none)
arm.data.name = "root"
mesh.name = "SK_Pilot"; mesh.data.name = "SK_Pilot"
mesh.data.materials[0].name = "MI_Pilot"
for pb in arm.pose.bones: pb.custom_shape = None
ad = arm.animation_data
for t in list(ad.nla_tracks): ad.nla_tracks.remove(t)
ad.action = None
for pb in arm.pose.bones: pb.matrix_basis.identity()
bpy.context.view_layer.update()

# ---- rest-pose measurements -------------------------------------------------------------------------------------
co = np.array([mesh.matrix_world @ v.co for v in mesh.data.vertices])
mesh.data.calc_loop_triangles()
used = {g.group for v in mesh.data.vertices for g in v.groups if g.weight > 1e-4}
weighted = {mesh.vertex_groups[i].name for i in used}
maxinf = max(sum(1 for g in v.groups if g.weight > 1e-4) for v in mesh.data.vertices)
report["mesh"] = {"verts": len(co), "tris": len(mesh.data.loop_triangles), "min_m": co.min(0).round(4).tolist(), "max_m": co.max(0).round(4).tolist(),
                  "size_cm": ((co.max(0) - co.min(0)) * 100).round(1).tolist(), "materials": [m.name for m in mesh.data.materials],
                  "bones": len(arm.data.bones), "bones_without_weights": sorted(b.name for b in arm.data.bones if b.name not in weighted),
                  "max_influences_per_vertex": maxinf,
                  "unweighted_verts": sum(1 for v in mesh.data.vertices if not any(g.weight > 1e-4 for g in v.groups))}
bones = {b.name: (arm.matrix_world @ b.head_local) for b in arm.data.bones}
report["rest_bone_heads_m"] = {k: [round(c, 3) for c in bones[k]] for k in ("pelvis", "head", "headfront", "head_end", "Bone_026", "Bone_028", "hand_l", "hand_r", "foot_l", "ball_l")}

# ---- textures ---------------------------------------------------------------------------------------------------
def raw(img, name):
    path = os.path.join(EXPORT, name)
    with open(path, "wb") as f: f.write(img.packed_file.data)
    return path
imgs = {i.name: i for i in bpy.data.images if i.packed_file}
raw(imgs["texture_0"], "T_Pilot_BC.png")
raw(imgs["texture_0_metallic_roughness"], "T_Pilot_ORM.png")       # glTF: G roughness, B metallic (R unused)
raw(imgs["normal"], "T_Pilot_N_source_opengl.png")
n = imgs["normal"]; w, h = n.size
px = np.array(n.pixels[:], dtype=np.float32).reshape(h, w, 4)       # row 0 = bottom (v up)
orm = imgs["texture_0_metallic_roughness"]; op = np.array(orm.pixels[:], dtype=np.float32).reshape(h, w, 4)
report["textures"] = {"size": [w, h], "normal_depth_bits": n.depth, "normal_mean_rgb": px[..., :3].mean((0, 1)).round(3).tolist(),
                      "normal_source_convention": "OpenGL (+Y): glTF specifies it and scripts/p12_normal_seam_test.py confirms it (seam agreement 0.82 vs 0.72)",
                      "T_Pilot_N": "green inverted here = DirectX (-Y), import into Unreal WITHOUT flip_green_channel",
                      "orm_mean_rgb": op[..., :3].mean((0, 1)).round(3).tolist(), "orm_min_rgb": op[..., :3].min((0, 1)).round(3).tolist(),
                      "orm_max_rgb": op[..., :3].max((0, 1)).round(3).tolist()}
# Unreal wants DirectX-style normals: T_Pilot_N.png = the source with green inverted
dx = px.copy()
dx[..., 1] = 1.0 - dx[..., 1]
dx[..., 3] = 1.0
out = bpy.data.images.new("T_Pilot_N", w, h, alpha=False, is_data=True)
out.pixels.foreach_set(dx.ravel()); out.file_format = 'PNG'; out.filepath_raw = os.path.join(EXPORT, "T_Pilot_N.png"); out.save()

# ---- animations -------------------------------------------------------------------------------------------------
pelvis = arm.pose.bones["pelvis"]
rest_rot_inv = arm.data.bones["pelvis"].matrix_local.to_3x3().inverted()

def use(action):
    ad.action = action
    ad.action_slot = action.slots[0]
    f0, f1 = (int(round(v)) for v in action.frame_range)
    scene.frame_start, scene.frame_end = f0, f1
    return f0, f1

def sample(action):
    f0, f1 = use(action)
    frames = []
    for f in range(f0, f1 + 1):
        scene.frame_set(f)
        frames.append({"pelvis": pelvis.matrix.translation.copy(), "loc": pelvis.location.copy(),
                       "heads": np.array([pb.head for pb in arm.pose.bones]),
                       "quats": [pb.matrix.to_quaternion() for pb in arm.pose.bones]})
    return f0, f1, frames

def pose_diff(a, b, relative=True):
    """max bone-head distance (cm, pelvis-relative unless told otherwise) and max bone rotation difference (degrees)"""
    ha = a["heads"] - (np.array(a["pelvis"]) if relative else 0); hb = b["heads"] - (np.array(b["pelvis"]) if relative else 0)
    d = float(np.linalg.norm(ha - hb, axis=1).max() * 100)
    r = max(math.degrees(qa.rotation_difference(qb).angle) for qa, qb in zip(a["quats"], b["quats"]))
    return round(d, 2), round(min(r, 360 - r), 2)

def describe(action):
    f0, f1, fr = sample(action)
    p = np.array([f["pelvis"] for f in fr]) * 100          # cm, Blender axes: X = character's left, -Y = forward, Z = up
    key_frames = sorted({kp.co[0] for fc in anim_utils.action_get_channelbag_for_slot(action, action.slots[0]).fcurves for kp in fc.keyframe_points})
    d_rel = pose_diff(fr[0], fr[-1]); d_abs = pose_diff(fr[0], fr[-1], relative=False)
    step = max(pose_diff(fr[i], fr[i + 1])[0] for i in range(len(fr) - 1))
    return {"frames": [f0, f1], "keys": len(key_frames), "keys_on_integer_frames": all(abs(k - round(k)) < 1e-3 for k in key_frames),
            "length_s": round((f1 - f0) / 30.0, 4),
            "pelvis_net_cm": {"left_x": round(float(p[-1, 0] - p[0, 0]), 2), "forward": round(float(-(p[-1, 1] - p[0, 1])), 2), "up_z": round(float(p[-1, 2] - p[0, 2]), 2)},
            "pelvis_range_cm": {"x": round(float(np.ptp(p[:, 0])), 2), "y": round(float(np.ptp(p[:, 1])), 2), "z": round(float(np.ptp(p[:, 2])), 2)},
            "pelvis_height_cm": [round(float(p[:, 2].min()), 1), round(float(p[:, 2].max()), 1)],
            "first_vs_last_pose": {"max_bone_offset_cm_pelvis_relative": d_rel[0], "max_rotation_deg": d_rel[1], "max_bone_offset_cm_absolute": d_abs[0]},
            "largest_frame_to_frame_offset_cm": step}, fr

anims = {}
final_actions = {}
for src_name, asset in NAMES.items():
    action = bpy.data.actions[src_name]
    info, fr = describe(action)
    f0, f1 = info["frames"]
    net = info["pelvis_net_cm"]
    info["travels"] = abs(net["forward"]) > TRAVEL_CM or abs(net["left_x"]) > TRAVEL_CM
    final = action
    if info["travels"]:
        # in place: remove the straight-line drift of the pelvis in the ground plane (X and Y), keep bob and sway
        final = action.copy(); final.name = src_name + "_InPlace"
        use(final)
        bag = anim_utils.action_get_channelbag_for_slot(final, final.slots[0])
        for fc in [fc for fc in bag.fcurves if fc.data_path == 'pose.bones["pelvis"].location']: bag.fcurves.remove(fc)
        p0 = fr[0]["pelvis"]; pn = fr[-1]["pelvis"]; n_ = max(f1 - f0, 1)
        for i, f in enumerate(range(f0, f1 + 1)):
            t = i / n_
            delta = Vector((-(pn.x - p0.x) * t, -(pn.y - p0.y) * t, 0.0))
            pelvis.location = fr[i]["loc"] + rest_rot_inv @ delta
            pelvis.keyframe_insert("location", frame=f)
        for fc in bag.fcurves:
            if fc.data_path == 'pose.bones["pelvis"].location':
                for kp in fc.keyframe_points: kp.interpolation = 'LINEAR'
        info["in_place"], _ = describe(final)
        info["source_speed_cm_s"] = round(math.hypot(net["forward"], net["left_x"]) / info["length_s"], 1)
    final.use_fake_user = True; action.use_fake_user = True
    info["exported_action"] = final.name
    anims[asset] = info
    final_actions[asset] = final
report["animations"] = anims

# ---- save the working file (metres), then convert to centimetres for the FBX -------------------------------------------
ad.action = None
for pb in arm.pose.bones: pb.matrix_basis.identity()
scene.frame_set(0); bpy.context.view_layer.update()
bpy.ops.file.pack_all()
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(WORK, "Pilot_v01.blend"))
# Blender's FBX exporter puts a 100x scale on the armature node when the scene is in metres, and Unreal keeps it as a
# root bone with scale 100 (seen in the first import). Baking the 100x into the data with a 0.01 unit scale gives scale 1.
scene.unit_settings.scale_length = 0.01
arm.scale = (100.0, 100.0, 100.0)
for o in bpy.context.selected_objects: o.select_set(False)
arm.select_set(True); bpy.context.view_layer.objects.active = arm
bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
# applying the armature's scale leaves a compensating 100x on its child: clear it and scale the vertices instead
mesh.matrix_parent_inverse = Matrix.Identity(4)
mesh.location = (0, 0, 0); mesh.rotation_euler = (0, 0, 0); mesh.rotation_quaternion = (1, 0, 0, 0); mesh.scale = (1, 1, 1)
mesh.data.transform(Matrix.Scale(100.0, 4))
for action in set(final_actions.values()):
    for fc in anim_utils.action_get_channelbag_for_slot(action, action.slots[0]).fcurves:
        if fc.data_path.endswith(".location"):
            for kp in fc.keyframe_points:
                kp.co[1] *= 100.0; kp.handle_left[1] *= 100.0; kp.handle_right[1] *= 100.0
            fc.update()
bpy.context.view_layer.update()
co2 = np.array([mesh.matrix_world @ v.co for v in mesh.data.vertices])
report["export_units"] = {"scene_unit_scale": 0.01, "mesh_height_units": round(float(co2[:, 2].max() - co2[:, 2].min()), 2), "armature_scale": list(arm.scale), "mesh_world_scale": list(mesh.matrix_world.to_scale())}

for asset, final in final_actions.items():
    f0, f1 = use(final); scene.frame_set(f0)
    # one FBX per animation: armature only
    for o in bpy.context.selected_objects: o.select_set(False)
    arm.select_set(True); bpy.context.view_layer.objects.active = arm
    bpy.ops.export_scene.fbx(filepath=os.path.join(EXPORT, asset + ".fbx"), use_selection=True, object_types={'ARMATURE'},
                             add_leaf_bones=False, bake_anim=True, bake_anim_use_all_bones=True, bake_anim_use_nla_strips=False,
                             bake_anim_use_all_actions=False, bake_anim_force_startend_keying=True, bake_anim_step=1.0,
                             bake_anim_simplify_factor=0.0, use_armature_deform_only=False, primary_bone_axis='Y', secondary_bone_axis='X',
                             axis_forward='-Z', axis_up='Y', apply_unit_scale=True, apply_scale_options='FBX_SCALE_NONE', global_scale=1.0)

# ---- mesh FBX (rest pose, no animation) ---------------------------------------------------------------------------
ad.action = None
for pb in arm.pose.bones: pb.matrix_basis.identity()
scene.frame_set(0); bpy.context.view_layer.update()
for o in bpy.context.selected_objects: o.select_set(False)
arm.select_set(True); mesh.select_set(True); bpy.context.view_layer.objects.active = arm
bpy.ops.export_scene.fbx(filepath=os.path.join(EXPORT, "SK_Pilot.fbx"), use_selection=True, object_types={'ARMATURE', 'MESH'},
                         add_leaf_bones=False, bake_anim=False, mesh_smooth_type='FACE', use_tspace=True,
                         use_armature_deform_only=False, primary_bone_axis='Y', secondary_bone_axis='X',
                         axis_forward='-Z', axis_up='Y', apply_unit_scale=True, apply_scale_options='FBX_SCALE_NONE',
                         global_scale=1.0, use_mesh_modifiers=True, use_triangles=True)
json.dump(report, open(os.path.join(EXPORT, "Pilot.manifest.json"), "w"), indent=1)
print("REPORT " + json.dumps(report))
