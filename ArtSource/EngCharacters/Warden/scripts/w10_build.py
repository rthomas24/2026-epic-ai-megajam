# Warden step 1 (Blender 5.2, headless): put Grok Bot's Rodin Warden (ArtSource/EngAI/Characters/Warden, read only) on the
# CREW skeleton's bone names and bone-axis conventions, so the crew clips (SKEL_NullCrew) play on it unchanged.
#   - crew armature (ArtSource/EngCharacters/Crew/export/NullHeavy/SK_NullHeavy.fbx, cm) is FITTED to the Rodin joints:
#     every mapped bone moves to the Rodin joint and its frame is turned by the smallest rotation that takes the crew
#     limb direction to the Warden limb direction (so a crew pose gives the Warden the same limb directions);
#   - body weights are renamed Rodin -> crew; fingers go to the hand; max 4 influences;
#   - the welded gun (right forearm) is cut off and replaced by the dedicated props_r1 gun arm, rigid on lowerarm_r;
#   - the soft-skinned shield is dropped and the props_r1 shield is rigid on hand_l, placed so that it stands upright in
#     front-left of the body in the crew's rifle idle (the pose the Warden stands in);
#   - sockets (SOCKET_ empties): Muzzle + GunArm @ lowerarm_r, Shield @ hand_l, HeadGlow @ head;
#   - exports export/SK_Warden.fbx (cm, armature object "root" = the root bone, as b_cm_export.py), a .blend and review renders.
# Run: blender --background --factory-startup --python w10_build.py -- <repo root (absolute)>
import bpy, sys, os, math, json
from mathutils import Matrix, Vector

REPO = sys.argv[sys.argv.index("--") + 1]
AI = os.path.join(REPO, "ArtSource", "EngAI", "Characters", "Warden")
OUTD = os.path.join(REPO, "ArtSource", "EngCharacters", "Warden")
CREW = os.path.join(REPO, "ArtSource", "EngCharacters", "Crew", "export", "NullHeavy", "SK_NullHeavy.fbx")
IDLE = os.path.join(REPO, "Saved", "EngAnim", "fbx", "MF_Rifle_Idle_ADS.fbx")
CFG = dict(shield_bone="spine_05", shield_yaw_deg=25.0, shield_off=(22.0, -4.0, -66.0), gun_off=(0.0, 0.0, 0.0), cut_from_elbow_cm=4.0)
if os.environ.get("WARDEN_CFG"):
    CFG.update(json.loads(os.environ["WARDEN_CFG"]))

scene = bpy.context.scene
for o in list(bpy.data.objects):
    bpy.data.objects.remove(o)
scene.unit_settings.scale_length = 0.01


def imp(path, anim=False):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=path, use_anim=anim, automatic_bone_orientation=False, ignore_leaf_bones=False)
    return [o for o in bpy.data.objects if o not in before]


def rot3(m):
    r = m.to_3x3()
    r.normalize()
    return r


# ---------------------------------------------------------------- crew poses (Unreal exports of the clips the Warden plays)
# For every bone: D = world rotation of the bone in the pose, relative to its rest (convention free). D_IDLE: the rifle idle.
POSES = []   # (label, {bone: D}, pelvis position, rest pelvis position)
CLIPS = [("MF_Rifle_Idle_ADS", (0.5,)), ("MF_Rifle_Walk_Fwd", (0.0, 0.25)), ("MF_Rifle_Jog_Fwd", (0.25,)), ("MM_ChargedAttack", (0.45, 0.62)),
         ("MM_Rifle_Fire", (0.2,)), ("MM_HitReact_Front_Lgt_01", (0.3,)), ("MM_Death_Front_01", (0.5, 1.0))]
for clip, fracs in CLIPS:
    path = os.path.join(REPO, "Saved", "EngAnim", "fbx", clip + ".fbx")
    if not os.path.isfile(path):
        print("INFO no pose source", clip); continue
    objs_ = imp(path, True)
    iarm = next(o for o in objs_ if o.type == 'ARMATURE')
    act = iarm.animation_data.action
    for fr in fracs:
        f = int(round(act.frame_range[0] + (act.frame_range[1] - act.frame_range[0]) * fr))
        scene.frame_set(f)
        bpy.context.view_layer.update()
        D = {pb.name: rot3(iarm.matrix_world @ pb.matrix) @ rot3(iarm.matrix_world @ pb.bone.matrix_local).inverted() for pb in iarm.pose.bones}
        POSES.append(("%s_%02d" % (clip, int(fr * 100)), D, iarm.matrix_world @ iarm.pose.bones["pelvis"].matrix.translation,
                      iarm.matrix_world @ iarm.pose.bones["pelvis"].bone.matrix_local.translation))
    for o in objs_:
        bpy.data.objects.remove(o)
    for a in list(bpy.data.actions):
        bpy.data.actions.remove(a)
D_IDLE = POSES[0][1]
print("INFO poses", [p_[0] for p_ in POSES])

# ---------------------------------------------------------------- crew armature (cm numbers, object scale 1)
crew_objs = imp(CREW)
arm = next(o for o in crew_objs if o.type == 'ARMATURE')
for o in crew_objs:
    if o is not arm:
        bpy.data.objects.remove(o)
arm.name = "root"
arm.scale = (1, 1, 1)
arm.location = (0, 0, 0)
arm.rotation_euler = (0, 0, 0)
bpy.context.view_layer.update()
print("INFO crew bones", len(arm.data.bones), "pelvis z", round(arm.data.bones["pelvis"].head_local.z, 2), "head z", round(arm.data.bones["head"].head_local.z, 2))

# ---------------------------------------------------------------- Rodin Warden: metres, facing +X -> cm, facing -Y
w_objs = imp(os.path.join(AI, "mesh_r1", "SK_Warden.fbx"))
warm = next(o for o in w_objs if o.type == 'ARMATURE')
body = next(o for o in w_objs if o.type == 'MESH' and o.name == "SK_Warden")
M = Matrix.Rotation(math.radians(-90.0), 4, 'Z')   # the importer already converts to cm (scene unit scale 0.01)
J = {b.name: M @ (warm.matrix_world @ b.head_local) for b in warm.data.bones}
JT = {b.name: M @ (warm.matrix_world @ b.tail_local) for b in warm.data.bones}
body.data.transform(M @ body.matrix_world)
body.parent = None
body.matrix_world = Matrix.Identity(4)
for m in list(body.modifiers):
    body.modifiers.remove(m)
for o in w_objs:
    if o is not body:
        bpy.data.objects.remove(o)
zs = [v.co.z for v in body.data.vertices]
print("INFO warden body height", round(max(zs) - min(zs), 1), "min z", round(min(zs), 2), "hips", tuple(round(v, 1) for v in J["Hips"]))

MAP = {"pelvis": "Hips", "spine_01": "Spine", "spine_03": "Spine1", "spine_05": "Spine2", "neck_01": "Neck", "head": "Head"}
for s, S in (("l", "Left"), ("r", "Right")):
    MAP.update({"clavicle_" + s: S + "Shoulder", "upperarm_" + s: S + "Arm", "lowerarm_" + s: S + "ForeArm", "hand_" + s: S + "Hand",
                "thigh_" + s: S + "UpLeg", "calf_" + s: S + "Leg", "foot_" + s: S + "Foot", "ball_" + s: S + "ToeBase"})

# ---------------------------------------------------------------- fit the crew armature
bpy.context.view_layer.objects.active = arm
arm.select_set(True)
bpy.ops.object.mode_set(mode='EDIT')
ebs = arm.data.edit_bones
for b in ebs:
    b.use_connect = False
O = {b.name: b.matrix.copy() for b in ebs}
PAR = {b.name: (b.parent.name if b.parent else None) for b in ebs}
P = {c: J[r].copy() for c, r in MAP.items()}


def between(a, mid, b):
    la = (O[mid].translation - O[a].translation).length
    lb = (O[b].translation - O[mid].translation).length
    return P[a].lerp(P[b], la / (la + lb))


P["spine_02"] = between("spine_01", "spine_02", "spine_03")
P["spine_04"] = between("spine_03", "spine_04", "spine_05")
P["neck_02"] = between("neck_01", "neck_02", "head")
CHAIN = [("spine_01", "spine_02"), ("spine_02", "spine_03"), ("spine_03", "spine_04"), ("spine_04", "spine_05"), ("spine_05", "neck_01"),
         ("neck_01", "neck_02"), ("neck_02", "head")]
for s in ("l", "r"):
    CHAIN += [("clavicle_" + s, "upperarm_" + s), ("upperarm_" + s, "lowerarm_" + s), ("lowerarm_" + s, "hand_" + s),
              ("thigh_" + s, "calf_" + s), ("calf_" + s, "foot_" + s), ("foot_" + s, "ball_" + s)]
R = {"pelvis": Matrix.Identity(3)}
for b, c in CHAIN:
    od = O[c].translation - O[b].translation
    nd = P[c] - P[b]
    R[b] = od.rotation_difference(nd).to_matrix()
    print("INFO fit %-12s turn %5.1f deg  length x%.2f" % (b, math.degrees(od.angle(nd)), nd.length / od.length))
INHERIT = {"head": "neck_02", "hand_l": "lowerarm_l", "hand_r": "lowerarm_r", "ball_l": "foot_l", "ball_r": "foot_r"}
for b, src in INHERIT.items():
    R[b] = R[src]
order = []


def walk(n):
    order.append(n)
    for c in [k for k, v in PAR.items() if v == n]:
        walk(c)


for n in [k for k, v in PAR.items() if v is None]:
    walk(n)
N = {}
for n in order:
    p = PAR[n]
    if n in P:
        N[n] = Matrix.Translation(P[n]) @ (R[n] @ O[n].to_3x3()).to_4x4()
    elif p is None:
        N[n] = O[n].copy()
    else:
        N[n] = N[p] @ O[p].inverted() @ O[n]
for b in ebs:
    ln = b.length
    b.matrix = N[b.name]
    b.length = ln
bpy.ops.object.mode_set(mode='OBJECT')
bpy.context.view_layer.update()
err = max((arm.data.bones[c].head_local - J[r]).length for c, r in MAP.items())
print("INFO fit: mapped joints max error cm", round(err, 4))

NR = {b.name: b.matrix_local.copy() for b in arm.data.bones}


def fk(D, ppos, prest):
    """What a crew pose does to this rig: same bone frames as the crew (same local rotations), this rig's lengths, the
    pelvis translation scaled by the pelvis-height ratio (as Unreal's AnimationScaled)."""
    k = NR["pelvis"].translation.z / prest.z
    A = {}; POS = {}
    for n in order:
        p = PAR[n]
        if n in D:
            A[n] = D[n] @ O[n].to_3x3()
        elif p is None:
            A[n] = NR[n].to_3x3()
        else:
            A[n] = A[p] @ NR[p].to_3x3().inverted() @ NR[n].to_3x3()
        if n == "pelvis":
            POS[n] = Vector((ppos.x * k, ppos.y * k, ppos.z * k))
        elif p is None:
            POS[n] = NR[n].translation.copy()
        else:
            POS[n] = POS[p] + A[p] @ (NR[p].to_3x3().inverted() @ (NR[n].translation - NR[p].translation))
    return A, POS


A_IDLE, POS_IDLE = fk(POSES[0][1], POSES[0][2], POSES[0][3])

# ---------------------------------------------------------------- body weights Rodin -> crew
GMAP = {r: c for c, r in MAP.items()}
for S, s in (("Left", "l"), ("Right", "r")):
    for F in ("Thumb", "Index", "Middle", "Ring", "Pinky"):
        for i in (1, 2, 3):
            GMAP["%sHand%s%d" % (S, F, i)] = "hand_" + s
RIGHT_GUN = {"RightForeArm", "RightHand"} | {g for g in GMAP if g.startswith("RightHand")}
LEFT_HAND = {"LeftHand"} | {g for g in GMAP if g.startswith("LeftHand")}
gi = {g.index: g.name for g in body.vertex_groups}
elbow = J["RightForeArm"]
fdir = (J["RightHand"] - J["RightForeArm"]).normalized()
weights = {}
cut = set()
palm = Vector((0, 0, 0)); palm_n = 0
for v in body.data.vertices:
    w = {}
    gun = 0.0; lh = 0.0
    for ge in v.groups:
        name = gi[ge.group]
        if name in RIGHT_GUN:
            gun += ge.weight
        if name in LEFT_HAND:
            lh += ge.weight
        c = GMAP.get(name)
        if c and ge.weight > 0:
            w[c] = w.get(c, 0.0) + ge.weight
    if gun >= 0.5 and (v.co - elbow).dot(fdir) > CFG["cut_from_elbow_cm"]:
        cut.add(v.index)
    if lh >= 0.5:
        palm += v.co; palm_n += 1
    top = sorted(w.items(), key=lambda kv: -kv[1])[:4]
    tot = sum(x for _, x in top) or 1.0
    weights[v.index] = [(k, x / tot) for k, x in top]
palm = palm / max(palm_n, 1)
print("INFO left-hand verts", palm_n, "palm centroid", tuple(round(v, 1) for v in palm), "welded-gun verts cut", len(cut), "of", len(body.data.vertices))
body.vertex_groups.clear()
groups = {}
for vi, ws in weights.items():
    for k, x in ws:
        g = groups.get(k) or groups.setdefault(k, body.vertex_groups.new(name=k))
        g.add([vi], x, 'REPLACE')
# cut the welded gun
import bmesh
bm = bmesh.new()
bm.from_mesh(body.data)
bm.verts.ensure_lookup_table()
bmesh.ops.delete(bm, geom=[bm.verts[i] for i in cut], context='VERTS')
bm.to_mesh(body.data)
bm.free()
body.data.materials.clear()
for m_ in list(bpy.data.materials):
    bpy.data.materials.remove(m_)
body.data.materials.append(bpy.data.materials.new("M_Warden"))


def load_prop(fname, matname, group):
    objs = imp(os.path.join(AI, "props_r1", fname))
    o = next(x for x in objs if x.type == 'MESH')
    o.data.transform(o.matrix_world)   # cm numbers, own axes kept (+X forward, +Z up)
    o.parent = None
    o.matrix_world = Matrix.Identity(4)
    bpy.context.view_layer.update()
    for x in objs:
        if x is not o:
            bpy.data.objects.remove(x)
    o.data.materials.clear()
    for m_ in list(bpy.data.materials):
        if m_.name.startswith(matname):
            bpy.data.materials.remove(m_)
    o.data.materials.append(bpy.data.materials.new(matname))
    cs = [v.co for v in o.data.vertices]
    print("INFO prop", fname, "tris", sum(len(p.vertices) - 2 for p in o.data.polygons), "bounds x %.1f..%.1f y %.1f..%.1f z %.1f..%.1f" % (
        min(c.x for c in cs), max(c.x for c in cs), min(c.y for c in cs), max(c.y for c in cs), min(c.z for c in cs), max(c.z for c in cs)))
    g = o.vertex_groups.new(name=group)
    g.add(list(range(len(o.data.vertices))), 1.0, 'REPLACE')
    return o


def frame(x, zhint):
    x = Vector(x).normalized()
    z = (Vector(zhint) - x * Vector(zhint).dot(x)).normalized()
    y = z.cross(x)
    return Matrix((x, y, z)).transposed()


# ---------------------------------------------------------------- gun arm: rigid on lowerarm_r, cuff at the elbow, barrels along the forearm
gun = load_prop("SM_Warden_GunArm.fbx", "M_Warden_GunArm", "lowerarm_r")
G = Matrix.Translation(elbow + Vector(CFG["gun_off"])) @ frame(fdir, J["RightArm"] - J["RightForeArm"]).to_4x4()
gun.data.transform(G)
muzzle = G @ Vector((124.0, 0.0, -18.0))
# ---------------------------------------------------------------- shield: rigid on hand_l; upright, face forward-left in the rifle idle
SB = CFG["shield_bone"]
shield = load_prop("SM_Warden_ShieldProp.fbx", "M_Warden_Shield", SB)
yaw = math.radians(CFG["shield_yaw_deg"])
S_des = frame((math.sin(yaw), -math.cos(yaw), 0.0), (0, 0, 1))          # +X (chevron face) forward (-Y), turned to the left (+X)
S_idle = Matrix.Translation(POS_IDLE["hand_l"] + Vector(CFG["shield_off"])) @ S_des.to_4x4()   # where it stands in the rifle idle
SH = NR[SB] @ (Matrix.Translation(POS_IDLE[SB]) @ A_IDLE[SB].to_4x4()).inverted() @ S_idle    # the same, carried back to the bind pose of its bone
shield.data.transform(SH)
print("INFO shield on", SB, "idle grip", tuple(round(v, 1) for v in S_idle.translation), "idle hand_l", tuple(round(v, 1) for v in POS_IDLE["hand_l"]))

# ---------------------------------------------------------------- one mesh, three material slots
for o in (body, gun, shield):
    for uv in o.data.uv_layers:
        uv.name = "UVMap"
    o.select_set(False)
for o in bpy.context.selected_objects:
    o.select_set(False)
for o in (gun, shield, body):
    o.select_set(True)
bpy.context.view_layer.objects.active = body
bpy.ops.object.join()
body.name = "SK_Warden"
body.data.name = "SK_Warden"
body.parent = arm
mod = body.modifiers.new("Armature", 'ARMATURE')
mod.object = arm
tris = sum(len(p.vertices) - 2 for p in body.data.polygons)
zs = [v.co.z for v in body.data.vertices]
unw = sum(1 for v in body.data.vertices if not v.groups)
print("INFO SK_Warden tris", tris, "verts", len(body.data.vertices), "height", round(max(zs) - min(zs), 1), "min z", round(min(zs), 2),
      "unweighted", unw, "slots", [m.name for m in body.data.materials], "groups", len(body.vertex_groups))

# ---------------------------------------------------------------- sockets
hb = arm.data.bones
SOCKETS = [("Muzzle", "lowerarm_r", Matrix.Translation(muzzle) @ frame(fdir, (0, 0, 1)).to_4x4()),
           ("GunArm", "lowerarm_r", G), ("Shield", SB, SH),
           ("HeadGlow", "head", Matrix.Translation(hb["head"].head_local + Vector((0, -22.0, 12.0))) @ frame((0, -1, 0), (0, 0, 1)).to_4x4())]
empties = []
for name, bone, target in SOCKETS:
    e = bpy.data.objects.new("SOCKET_" + name, None)
    scene.collection.objects.link(e)
    e.parent = arm; e.parent_type = 'BONE'; e.parent_bone = bone
    bpy.context.view_layer.update()
    e.matrix_world = target
    empties.append(e)
bpy.context.view_layer.update()

os.makedirs(os.path.join(OUTD, "export"), exist_ok=True)
os.makedirs(os.path.join(OUTD, "review"), exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUTD, "Warden_v01.blend"))
for o in bpy.context.selected_objects:
    o.select_set(False)
arm.select_set(True); body.select_set(True)
for e in empties:
    e.select_set(True)
bpy.context.view_layer.objects.active = arm
OUT = os.path.join(OUTD, "export", "SK_Warden.fbx")
bpy.ops.export_scene.fbx(filepath=OUT, use_selection=True, object_types={'ARMATURE', 'MESH', 'EMPTY'},
                         add_leaf_bones=False, bake_anim=False, mesh_smooth_type='FACE', use_tspace=True,
                         use_armature_deform_only=False, primary_bone_axis='Y', secondary_bone_axis='X',
                         axis_forward='-Z', axis_up='Y', apply_unit_scale=True, apply_scale_options='FBX_SCALE_NONE',
                         global_scale=1.0, use_mesh_modifiers=True, use_triangles=True)
print("EXPORT_OK", OUT)

# ---------------------------------------------------------------- review renders (bind pose): front, left side, 3/4, back
cols = {"M_Warden": (0.25, 0.25, 0.28, 1), "M_Warden_GunArm": (0.8, 0.35, 0.1, 1), "M_Warden_Shield": (0.75, 0.1, 0.1, 1)}
for m in body.data.materials:
    m.diffuse_color = cols[m.name]
scene.render.engine = 'BLENDER_WORKBENCH'
scene.display.shading.light = 'STUDIO'
scene.display.shading.color_type = 'MATERIAL'
scene.render.resolution_x = 700; scene.render.resolution_y = 900
cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam"))
scene.collection.objects.link(cam)
scene.camera = cam
cam.data.type = 'ORTHO'; cam.data.ortho_scale = 330.0; cam.data.clip_end = 5000
def shots(prefix, views):
    for tag, d in views:
        d = Vector(d).normalized()
        cam.location = Vector((0, 0, 135)) + d * 900
        cam.rotation_euler = (-d).to_track_quat('-Z', 'Y').to_euler()
        scene.render.filepath = os.path.join(OUTD, "review", "%s_%s.png" % (prefix, tag))
        bpy.ops.render.render(write_still=True)


shots("bind", (("front", (0, -1, 0)), ("left", (1, 0, 0)), ("threeq", (0.7, -0.7, 0.25)), ("back", (0, 1, 0)), ("right", (-1, 0, 0))))
# Pose previews: what a crew clip does to this rig (same local rotations = same bone frames; lengths from this rig; the
# pelvis translation scaled by the pelvis-height ratio, as Unreal's AnimationScaled does).
bpy.context.view_layer.objects.active = arm
for label, D, ppos, prest in POSES:
    A, POS = fk(D, ppos, prest)
    for n in order:
        arm.pose.bones[n].matrix = Matrix.Translation(POS[n]) @ A[n].to_4x4()
        bpy.context.view_layer.update()
    shots("pose_" + label, (("front", (0, -1, 0)), ("threeq", (0.7, -0.7, 0.25)), ("left", (1, 0, 0))))
print("RENDER_OK")
