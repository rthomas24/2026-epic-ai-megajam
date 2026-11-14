# Triad Drift - import Grok Bot's crew trooper clips (on SKEL_NullCrew) and Warden fire / summon clips (on SKEL_Warden)
# into /Game/TriadDrift/Gameplay/Characters/Crew/Anims as A_Crew_<Type>_<Action>. Animation only, no retarget.
#   .\Tools\td-python.ps1 -Script ArtSource\EngAnim\CrewGrok\scripts\ue_import_grok_crew.py
# Logs each clip's length, root travel and, for the deaths, how high the body ends. Animation builder (eng/anim), Nov 13.
import math
import os
import unreal

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", ".."))
DST = "/Game/TriadDrift/Gameplay/Characters/Crew/Anims"
CREW = unreal.load_asset("/Game/TriadDrift/Gameplay/Characters/Crew/SKEL_NullCrew")
WARDEN = unreal.load_asset("/Game/TriadDrift/Gameplay/Characters/Warden/SKEL_Warden")
T = os.path.join(ROOT, "ArtSource", "EngAI", "Animations", "CrewTrooper", "r1")
W = os.path.join(ROOT, "ArtSource", "EngAI", "Animations", "Warden", "r1")
CLIPS = [(T, "A_CrewTrooper_%s.fbx" % n, "A_Crew_Trooper_%s" % d, CREW) for n, d in [
    ("Idle_Ready", "Idle_Ready"), ("Idle_Aim", "Idle_Aim"), ("Alert", "Alert"), ("Fire_Burst", "Fire_Burst"), ("Reload", "Reload_Rifle"),
    ("Cover_Enter", "Cover_Enter"), ("Cover_Idle", "Cover_Idle"), ("HitReact_Front", "HitReact_Front"), ("HitReact_Back", "HitReact_Back"),
    ("Death_Front", "Death_Front"), ("Death_Back", "Death_Back"), ("Death_Left", "Death_Left"), ("Death_Right", "Death_Right")]]
CLIPS += [(W, "A_Warden_%s.fbx" % n, "A_Crew_Warden_%s" % n, WARDEN) for n in ("Fire_01", "Fire_02", "Summon_01", "Summon_02")]
X = unreal.AnimPoseExtensions
tools = unreal.AssetToolsHelpers.get_asset_tools()


def log(m): unreal.log("TDPY: " + str(m))


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
