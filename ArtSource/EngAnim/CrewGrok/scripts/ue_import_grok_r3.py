# Triad Drift - import Grok Bot's crew AI clips r1 (PR #165: Trooper strafes / backpedal with aim, Grunt two-hit combo,
# Heavy guard-break stagger and shield walk, on SKEL_NullCrew) and Warden r2 (PR #166: stagger, enraged roar, on
# SKEL_Warden) into /Game/TriadDrift/Gameplay/Characters/Crew/Anims as A_Crew_<Type>_<Action>. Animation only.
#   .\Tools\td-python.ps1 -Script ArtSource\EngAnim\CrewGrok\scripts\ue_import_grok_r3.py
# Same normalisation as ue_import_grok_r2.py. Animation builder (eng/anim), Nov 14.
import math
import os
import unreal

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", ".."))
DST = "/Game/TriadDrift/Gameplay/Characters/Crew/Anims"
CREW = unreal.load_asset("/Game/TriadDrift/Gameplay/Characters/Crew/SKEL_NullCrew")
WARDEN = unreal.load_asset("/Game/TriadDrift/Gameplay/Characters/Warden/SKEL_Warden")
C = os.path.join(ROOT, "ArtSource", "EngAI", "Animations", "CrewAI", "r1")
W = os.path.join(ROOT, "ArtSource", "EngAI", "Animations", "Warden", "r2")
CLIPS = [(C, "A_CrewTrooper_StrafeAim_Left.fbx", "A_Crew_Trooper_StrafeLeft_Aim", CREW),
         (C, "A_CrewTrooper_StrafeAim_Right.fbx", "A_Crew_Trooper_StrafeRight_Aim", CREW),
         (C, "A_CrewTrooper_BackpedalAim.fbx", "A_Crew_Trooper_Backpedal_Aim", CREW),
         (C, "A_NullGrunt_MeleeCombo.fbx", "A_Crew_Grunt_MeleeCombo", CREW),
         (C, "A_NullHeavy_GuardBreakStagger.fbx", "A_Crew_Heavy_StaggerRM_Grok", CREW),
         (C, "A_NullHeavy_ShieldWalk.fbx", "A_Crew_Heavy_ShieldWalk", CREW),
         (W, "A_Warden_Stagger.fbx", "A_Crew_Warden_StaggerRM_Grok", WARDEN),
         (W, "A_Warden_EnragedRoar.fbx", "A_Crew_Warden_Roar", WARDEN)]
X = unreal.AnimPoseExtensions
tools = unreal.AssetToolsHelpers.get_asset_tools()


def log(m): unreal.log("TDPY: " + str(m))


def normalise(seq, skel):
    """Rewrite the clip in the crew set's own convention: every scale 1, bone offsets in centimetres at the skeleton's
    reference lengths. The trooper FBX comes in with root scale 100 and offsets in metres (UnitScaleFactor 100); the
    Warden FBX carries the 260 cm body's bone lengths, while SKEL_Warden's reference pose (and every clip the Warden
    plays) has the crew's, so its pelvis rode about 1.5x too high. Rotations are kept as they are."""
    ref = X.get_reference_pose(skel)
    names = [str(n) for n in X.get_bone_names(ref)]
    L = unreal.AnimPoseSpaces.LOCAL
    n = int(round(seq.get_play_length() * 30)) + 1
    opts = unreal.AnimPoseEvaluationOptions()
    p0 = X.get_anim_pose_at_time(seq, 0.0, opts)
    root_scale = X.get_bone_pose(p0, "root", L).scale3d.x
    thigh_ref = X.get_ref_bone_pose(ref, "calf_l", L).translation.x
    thigh_now = X.get_bone_pose(p0, "calf_l", L).translation.x * root_scale
    k = thigh_ref / thigh_now if abs(thigh_now) > 1e-6 else 1.0   # body size of the clip against the skeleton's
    keys = {b: ([], []) for b in names}
    for i in range(n):
        pose = X.get_anim_pose_at_time(seq, min(seq.get_play_length(), i / 30.0), opts)
        for b in names:
            t = X.get_bone_pose(pose, b, L)
            if b == "root":
                loc = unreal.Vector(t.translation.x * k, t.translation.y * k, t.translation.z * k)
            elif b == "pelvis":
                loc = unreal.Vector(t.translation.x * root_scale * k, t.translation.y * root_scale * k, t.translation.z * root_scale * k)
            else:
                loc = X.get_ref_bone_pose(ref, b, L).translation
            keys[b][0].append(loc)
            keys[b][1].append(t.rotation)
    c = seq.get_editor_property("controller")
    c.open_bracket(unreal.Text("Crew convention"), False)
    c.set_frame_rate(unreal.FrameRate(30, 1), False)
    c.set_number_of_frames(unreal.FrameNumber(n - 1), False)
    one = unreal.Vector(1, 1, 1)
    for b in names:
        try:
            c.add_bone_curve(b, False)
        except Exception:
            pass
        c.set_bone_track_keys(b, keys[b][0], keys[b][1], [one] * n, False)
    c.close_bracket(False)
    log("%s: normalised (root scale was %.0f, body size factor %.3f)" % (seq.get_name(), root_scale, k))


bad = 0
for folder, fbx, name, skel in CLIPS:
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", False); ui.set_editor_property("import_as_skeletal", True)
    ui.set_editor_property("import_animations", True); ui.set_editor_property("import_materials", False); ui.set_editor_property("import_textures", False)
    ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_ANIMATION)
    ui.set_editor_property("skeleton", skel)
    d = ui.get_editor_property("anim_sequence_import_data")
    d.set_editor_property("animation_length", unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
    d.set_editor_property("remove_redundant_keys", False); d.set_editor_property("convert_scene", True)
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", os.path.join(folder, fbx)); t.set_editor_property("destination_path", DST)
    t.set_editor_property("destination_name", name); t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True); t.set_editor_property("save", True); t.set_editor_property("options", ui)
    tools.import_asset_tasks([t])
    seq = unreal.load_asset(DST + "/" + name)
    if not seq or seq.get_editor_property("skeleton") != skel:
        log("FAILED %s (%s)" % (name, [str(p) for p in t.get_editor_property("imported_object_paths")])); bad += 1
        continue
    seq.set_editor_property("enable_root_motion", False)
    normalise(seq, skel)
    unreal.EditorAssetLibrary.save_loaded_asset(seq, False)
    L = seq.get_play_length()
    opts = unreal.AnimPoseEvaluationOptions()
    p0, p1 = X.get_anim_pose_at_time(seq, 0.0, opts), X.get_anim_pose_at_time(seq, L, opts)
    g = lambda p, b: X.get_bone_pose(p, b, unreal.AnimPoseSpaces.WORLD).translation
    r0, r1 = g(p0, "root"), g(p1, "root")
    low = min(g(p1, b).z for b in ("pelvis", "head", "spine_03", "hand_l", "hand_r", "foot_l", "foot_r"))
    fwd0 = g(p0, "head").y - g(p0, "pelvis").y
    log("%s: %.2f s on %s; root travel %.0f cm; start pelvis %.0f cm, head %.0f cm; end pelvis %.0f cm, head %.0f cm, lowest bone %.0f cm" % (
        name, L, skel.get_name(), math.hypot(r1.x - r0.x, r1.y - r0.y), g(p0, "pelvis").z, g(p0, "head").z, g(p1, "pelvis").z, g(p1, "head").z, low))
log("OK" if not bad else "%d clips failed" % bad)
