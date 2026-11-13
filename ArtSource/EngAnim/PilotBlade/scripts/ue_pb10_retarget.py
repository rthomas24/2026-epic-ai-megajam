# PilotBlade step 1: Grok Bot's HY-Motion clips (SMPL-H, ArtSource/EngAI/Animations/PilotBlade/r1, read only) -> SKEL_Pilot.
#  - imports the 14 clips onto the SMPL-H skeleton made by ue_pb00_probe.py (uncooked: /Game/TriadDrift/Dev/EngAnim/PilotBlade)
#  - IK_SMPLH (chains named as IK_Pilot's) + RTG_SMPLH_to_Pilot, same route as RTG_Manny_to_Pilot (ue_a10_retarget.py)
#  - retargets to /Game/TriadDrift/Gameplay/Characters/Pilot/Anims/A_Pilot_Blade_<Clip>  (NEW names; nothing overwritten
#    unless PB_REDO=1, which only deletes this script's own A_Pilot_Blade_* outputs)
#  - clean-up: root zeroed, pelvis horizontal travel removed (all in place: the capsule does the moving), ACL compression.
# Run: powershell -File Tools\td-python.ps1 -Script ArtSource\EngAnim\PilotBlade\scripts\ue_pb10_retarget.py
import json, math, os
import unreal
EAL = unreal.EditorAssetLibrary
AL = unreal.AnimationLibrary
APE = unreal.AnimPoseExtensions
LOCAL = unreal.AnimPoseSpaces.LOCAL; WORLD = unreal.AnimPoseSpaces.WORLD
ST = unreal.RetargetSourceOrTarget
HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", "..", ".."))
SRC = os.path.join(REPO, "ArtSource", "EngAI", "Animations", "PilotBlade", "r1").replace("\\", "/")
DEV = "/Game/TriadDrift/Dev/EngAnim/PilotBlade"
OUT = "/Game/TriadDrift/Gameplay/Characters/Pilot/Anims"
CLIPS = ["Light1", "Light2", "Light3", "HeavyLunge", "AirSlam", "BlockHold", "DeflectFlick", "Flourish", "Dash",
         "HitFront", "HitBack", "Death1", "Death2", "Idle"]
REDO = os.environ.get("PB_REDO") == "1"
NO_FOOTFIX = ("HeavyLunge", "Dash", "AirSlam", "Death1", "Death2")
LOOPS = ("Idle", "BlockHold")
tools = unreal.AssetToolsHelpers.get_asset_tools()
problems = []; report = []


def log(m):
    for line in str(m).splitlines():
        unreal.log("TDPY: " + line)


def import_anim(name, skel):
    dst = DEV + "/Src_" + name
    if EAL.does_asset_exist(dst):
        return EAL.load_asset(dst)
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", False); ui.set_editor_property("import_as_skeletal", True)
    ui.set_editor_property("import_animations", True); ui.set_editor_property("import_materials", False); ui.set_editor_property("import_textures", False)
    ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_ANIMATION)
    ui.set_editor_property("skeleton", skel)
    d = ui.get_editor_property("anim_sequence_import_data")
    d.set_editor_property("animation_length", unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
    d.set_editor_property("remove_redundant_keys", False); d.set_editor_property("convert_scene", True)
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", "%s/A_PilotBlade_%s.fbx" % (SRC, name)); t.set_editor_property("destination_path", DEV)
    t.set_editor_property("destination_name", "Src_" + name); t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True); t.set_editor_property("save", True); t.set_editor_property("options", ui)
    tools.import_asset_tasks([t])
    got = sorted(str(p) for p in t.get_editor_property("imported_object_paths"))
    a = EAL.load_asset(dst)
    if a is None and got:
        a = EAL.load_asset(got[0].split(".")[0])
    log("import %s -> %s len %s" % (name, got, a.get_play_length() if a else None))
    return a


def make_src_rig(mesh):
    path = DEV + "/IK_SMPLH"
    if EAL.does_asset_exist(path):
        return EAL.load_asset(path)
    rig = unreal.IKRigDefinitionFactory.create_new_ik_rig_asset(DEV, "IK_SMPLH")
    c = unreal.IKRigController.get_controller(rig)
    c.set_skeletal_mesh(mesh)
    c.set_retarget_root("Pelvis")
    chains = [("Spine", "Spine1", "Spine3"), ("Neck", "Neck", "Neck"), ("Head", "Head", "Head")]
    for S, s in (("Left", "L"), ("Right", "R")):
        chains += [(S + "Leg", s + "_Hip", s + "_Ankle"), (S + "Foot", s + "_Foot", s + "_Foot"), (S + "Clavicle", s + "_Collar", s + "_Collar"),
                   (S + "Arm", s + "_Shoulder", s + "_Wrist")]
        for F in ("Thumb", "Index", "Middle", "Ring", "Pinky"):
            chains.append((S + F, "%s_%s1" % (s, F), "%s_%s3" % (s, F)))
    for n, a, b in chains:
        r = c.add_retarget_chain(n, a, b, "")
        if str(r) != n:
            r2 = c.add_retarget_chain(n + "C", a, b, "")   # "Head" is refused as a chain name here: HeadC, mapped by hand below
            log("chain %s -> %s, retried as %s" % (n, r, r2))
    log("IK_SMPLH chains %d root %s" % (len(c.get_retarget_chains()), c.get_retarget_root()))
    EAL.save_asset(path, only_if_is_dirty=False)
    return rig


def make_rtg(src_rig, tgt_rig, src_mesh, tgt_mesh):
    path = DEV + "/RTG_SMPLH_to_Pilot"
    if EAL.does_asset_exist(path):
        return EAL.load_asset(path)
    rtg = tools.create_asset("RTG_SMPLH_to_Pilot", DEV, unreal.IKRetargeter, unreal.IKRetargetFactory())
    rc = unreal.IKRetargeterController.get_controller(rtg)
    rc.set_ik_rig(ST.SOURCE, src_rig); rc.set_ik_rig(ST.TARGET, tgt_rig)
    rc.set_preview_mesh(ST.SOURCE, src_mesh); rc.set_preview_mesh(ST.TARGET, tgt_mesh)
    rc.add_default_ops()
    rc.assign_ik_rig_to_all_ops(ST.SOURCE, src_rig); rc.assign_ik_rig_to_all_ops(ST.TARGET, tgt_rig)
    rc.auto_map_chains(unreal.AutoMapChainType.EXACT, True)
    for i in range(rc.get_num_retarget_ops()):
        try:
            log("op %s set Head<-HeadC %s" % (rc.get_op_name(i), rc.set_source_chain("HeadC", "Head", rc.get_op_name(i))))
        except Exception as e:
            log("set_source_chain %s: %s" % (rc.get_op_name(i), e))
    # The SMPL-H rest pose has its origin in the chest (pelvis at z -19, soles at about -116): lift it so the pelvis is at
    # its standing height, or the pelvis-motion op scales every translation by a huge ratio.
    rc.set_root_offset_in_retarget_pose(unreal.Vector(0, 0, 116.3), ST.SOURCE)
    log("chain map %s" % rc.get_chain_map())
    rc.auto_align_all_bones(ST.TARGET, unreal.RetargetAutoAlignMethod.CHAIN_TO_CHAIN)
    try:
        log("validate %s" % rc.validate_retargeter())
    except Exception as e:
        log("validate err %s" % e)
    EAL.save_asset(path, only_if_is_dirty=False)
    return rtg


def clean(seq, name):
    """Root zero; pelvis XY travel removed (kept: the sway about a 0.5 s moving average; the rest is the capsule job)."""
    n = AL.get_num_frames(seq) + 1
    pel = [AL.get_bone_poses_for_frame(seq, ["pelvis"], f, False)[0] for f in range(n)]
    travel = max((p.translation - pel[0].translation).length() for p in pel)
    x0, y0 = pel[0].translation.x, pel[0].translation.y
    xs = [p.translation.x - x0 for p in pel]; ys = [p.translation.y - y0 for p in pel]
    W = 7

    def smooth(v):
        out = []
        for i in range(n):
            w = v[max(0, i - W):min(n, i + W + 1)]
            out.append(sum(w) / len(w))
        return out
    sx, sy = smooth(xs), smooth(ys)
    newt = [unreal.Vector(x0 + xs[i] - sx[i], y0 + ys[i] - sy[i], pel[i].translation.z) for i in range(n)]
    # Foot anchoring (not for the travelling clips): Grok Bot's in-place bake locked the pelvis, so a weight shift moves the
    # feet under a fixed pelvis. Put it back: move the body against the velocity of whichever foot is on the ground
    # (weighted by how low it is), limited to LIMIT cm from the capsule centre; loops are closed (no drift at the seam).
    if name not in NO_FOOTFIX:
        opts = unreal.AnimPoseEvaluationOptions()
        F = {b: [] for b in ("foot_l", "foot_r")}
        for f in range(n):
            pose = APE.get_anim_pose_at_frame(seq, f, opts)
            for b in F:
                t = APE.get_bone_pose(pose, b, WORLD).translation
                d = newt[f] - pel[f].translation
                F[b].append((t.x + d.x, t.y + d.y, t.z))
        zmin = {b: min(p[2] for p in F[b]) for b in F}
        ox = [0.0] * n; oy = [0.0] * n
        for f in range(1, n):
            sw = cx = cy = 0.0
            for b in F:
                w = max(0.0, min(1.0, 1.0 - (F[b][f][2] - zmin[b]) / 6.0)) ** 2
                sw += w; cx -= w * (F[b][f][0] - F[b][f - 1][0]); cy -= w * (F[b][f][1] - F[b][f - 1][1])
            if sw > 0.05:
                cx /= sw; cy /= sw
            else:
                cx = cy = 0.0
            ox[f] = ox[f - 1] + cx; oy[f] = oy[f - 1] + cy
        if name in LOOPS:
            ex, ey = ox[-1], oy[-1]
            ox = [ox[f] - ex * f / (n - 1) for f in range(n)]; oy = [oy[f] - ey * f / (n - 1) for f in range(n)]
        LIMIT = 18.0
        for f in range(n):
            r = (ox[f] ** 2 + oy[f] ** 2) ** 0.5
            if r > 1e-6:
                k = LIMIT * math.tanh(r / LIMIT) / r
                ox[f] *= k; oy[f] *= k
        newt = [unreal.Vector(newt[f].x + ox[f], newt[f].y + oy[f], newt[f].z) for f in range(n)]
    c = seq.controller
    c.set_bone_track_keys("pelvis", newt, [p.rotation for p in pel], [p.scale3d for p in pel], True)
    c.set_bone_track_keys("root", [unreal.Vector(0, 0, 0)] * n, [unreal.Quat(0, 0, 0, 1)] * n, [unreal.Vector(1, 1, 1)] * n, True)
    return travel, max(max(abs(v) for v in sx), max(abs(v) for v in sy))


def metrics(seq):
    """Lowest ball height, planted-foot slide (cm travelled by a foot while within 3 cm of its lowest), right-hand peak speed."""
    n = AL.get_num_frames(seq) + 1
    opts = unreal.AnimPoseEvaluationOptions()
    P = {b: [] for b in ("foot_l", "foot_r", "ball_l", "ball_r", "hand_r", "pelvis", "head")}
    for f in range(n):
        pose = APE.get_anim_pose_at_frame(seq, f, opts)
        for b in P:
            P[b].append(APE.get_bone_pose(pose, b, WORLD).translation)
    out = {}
    out["min_ball_z"] = round(min(min(p.z for p in P["ball_l"]), min(p.z for p in P["ball_r"])), 1)
    out["pelvis_z0"] = round(P["pelvis"][0].z, 1); out["min_head_z"] = round(min(p.z for p in P["head"]), 1)
    slide = 0.0
    for b in ("foot_l", "foot_r"):
        zmin = min(p.z for p in P[b])
        for f in range(1, n):
            if P[b][f].z < zmin + 3.0 and P[b][f - 1].z < zmin + 3.0:
                d = P[b][f] - P[b][f - 1]
                slide += (d.x * d.x + d.y * d.y) ** 0.5
    out["planted_slide_cm"] = round(slide, 1)
    sp = [(P["hand_r"][f] - P["hand_r"][f - 1]).length() * 30.0 for f in range(1, n)]
    k = max(range(len(sp)), key=lambda i: sp[i])
    out["hand_r_peak_frame"] = k + 1; out["hand_r_peak_s"] = round((k + 1) / 30.0, 3); out["hand_r_peak_cms"] = round(sp[k])
    return out


def main():
    src_mesh = EAL.load_asset(DEV + "/SK_SMPLH")
    tgt_mesh = EAL.load_asset("/Game/TriadDrift/Gameplay/Characters/Pilot/SK_Pilot")
    if src_mesh is None:
        log("FAILED: run ue_pb00_probe.py first"); return
    skel = src_mesh.get_editor_property("skeleton")
    srcs = {n: import_anim(n, skel) for n in CLIPS}
    src_rig = make_src_rig(src_mesh)
    tgt_rig = EAL.load_asset(OUT + "/IK_Pilot")
    rtg = make_rtg(src_rig, tgt_rig, src_mesh, tgt_mesh)
    acl = EAL.load_asset("/ACLPlugin/ACLAnimBoneCompressionSettings")
    for n in CLIPS:
        s = srcs[n]
        if s is None:
            problems.append("no source " + n); continue
        name = "A_Pilot_Blade_" + n
        dst = OUT + "/" + name
        if EAL.does_asset_exist(dst):
            if not REDO:
                log("SKIP exists " + dst); continue
            EAL.delete_asset(dst)
        sp = s.get_path_name().split(".")[0]
        try:
            unreal.IKRetargetBatchOperation.duplicate_and_retarget([EAL.find_asset_data(sp)], src_mesh, tgt_mesh, rtg, "Src_" + n, name, "", "", OUT, False, False, True)
        except Exception as e:
            problems.append("retarget %s: %s" % (n, e)); continue
        if not EAL.does_asset_exist(dst):
            problems.append("missing after retarget " + name); continue
        seq = EAL.load_asset(dst)
        seq.set_editor_property("additive_anim_type", unreal.AdditiveAnimationType.AAT_NONE)
        travel, removed = clean(seq, n)
        seq.set_editor_property("enable_root_motion", False)
        AL.set_bone_compression_settings(seq, acl)
        m = metrics(seq)
        ok = EAL.save_asset(dst, only_if_is_dirty=False)
        row = dict(name=name, path=dst, source="ArtSource/EngAI/Animations/PilotBlade/r1/A_PilotBlade_%s.fbx" % n, len=round(seq.get_play_length(), 4),
                   frames=AL.get_num_frames(seq), src_len=round(s.get_play_length(), 4), skel=seq.get_editor_property("skeleton").get_name(),
                   pelvis_travel_before_cm=round(travel, 1), travel_removed_cm=round(removed, 1), saved=ok, status="new", **m)
        report.append(row)
        log("NEW %s" % json.dumps(row))
        if row["skel"] != "SKEL_Pilot":
            problems.append(name + " wrong skeleton")
    outdir = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "EngAnim")
    os.makedirs(outdir, exist_ok=True)
    json.dump(report, open(os.path.join(outdir, "pilotblade_report.json"), "w"), indent=1)
    log("still dirty: %s" % [str(p.get_name()) for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()])
    log("FAILED: %s" % problems if problems else "OK")


main()
