# Test: would the CENTIMETRE crew mesh play the template SK_Mannequin sequences directly (no retarget)?
# Imports the cm SK_NullTrooper onto SK_Mannequin as a TEMPORARY mesh /Game/TriadDrift/Gameplay/Characters/Crew/TmpDirect/
# SK_NullTrooper_Manny (never saved; SK_Mannequin is never saved), evaluates template sequences with it as the optional
# mesh, reports head / foot heights, then discards everything. Saves NOTHING.
import os
import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
FBX = os.path.abspath(os.path.join(HERE, "..", "..", "export", "NullTrooper", "SK_NullTrooper.fbx")).replace("\\", "/")
TMP = "/Game/TriadDrift/Gameplay/Characters/Crew/TmpDirect"
EAL = unreal.EditorAssetLibrary
APE = unreal.AnimPoseExtensions
A = "/Game/Characters/Mannequins/Anims/"
SEQS = [A + "Unarmed/MM_Idle", A + "Rifle/Walk/MF_Rifle_Walk_Fwd", A + "Death/MM_Death_Front_01"]


def log(m):
    unreal.log("TDPY: " + str(m))


ui = unreal.FbxImportUI()
ui.set_editor_property("import_mesh", True); ui.set_editor_property("import_as_skeletal", True)
ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
ui.set_editor_property("skeleton", EAL.load_asset("/Game/Characters/Mannequins/Meshes/SK_Mannequin"))
for k in ("import_materials", "import_textures", "import_animations", "create_physics_asset"):
    ui.set_editor_property(k, False)
t = unreal.AssetImportTask()
t.set_editor_property("filename", FBX); t.set_editor_property("destination_path", TMP)
t.set_editor_property("destination_name", "SK_NullTrooper_Manny"); t.set_editor_property("automated", True)
t.set_editor_property("save", False); t.set_editor_property("options", ui)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
mesh = EAL.load_asset(TMP + "/SK_NullTrooper_Manny")
log("temp mesh %s skel %s" % (mesh, mesh.get_editor_property("skeleton").get_path_name() if mesh else None))
for p in SEQS:
    seq = EAL.load_asset(p)
    L = seq.get_play_length()
    for tm in (0.0, L * 0.5):
        o = unreal.AnimPoseEvaluationOptions()
        o.set_editor_property("evaluation_type", unreal.AnimDataEvalType.COMPRESSED)
        o.set_editor_property("optional_skeletal_mesh", mesh)
        pose = APE.get_anim_pose_at_time(seq, tm, o)
        h = APE.get_bone_pose(pose, "head", unreal.AnimPoseSpaces.WORLD).translation
        f = APE.get_bone_pose(pose, "foot_l", unreal.AnimPoseSpaces.WORLD).translation
        r = APE.get_bone_pose(pose, "root", unreal.AnimPoseSpaces.LOCAL).scale3d
        log("DIRECT %s t=%.2f bones=%d head.z=%.2f foot_l.z=%.2f head-foot=%.2f root scale %.3f" % (
            p.split("/")[-1], tm, len(APE.get_bone_names(pose)), h.z, f.z, (h - f).length(), r.x))
dirty = [str(x.get_name()) for x in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()]
log("dirty (NOT saved, discarded on exit): %s" % dirty)
log("OK")
