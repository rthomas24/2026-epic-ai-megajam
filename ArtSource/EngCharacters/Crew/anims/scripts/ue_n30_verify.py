# Null crew own skeleton, step 3 (fresh editor): bone counts of SKEL_NullCrew and each mesh, data assets resolve,
# each of the 14 Crew/Anims sequences on SKEL_NullCrew, raw vs compressed poses at 3 times, FBX export per sequence
# (preview mesh = SK_NullTrooper) to Saved/NullCrewAnims, plus a mesh FBX of each crew mesh. Saves nothing in Content.
import os
import unreal

CH = "/Game/TriadDrift/Gameplay/Characters"
OUT = CH + "/Crew/Anims"
SKEL = CH + "/Crew/SKEL_NullCrew"
NAMES = ["MM_Idle", "MF_Unarmed_Walk_Fwd", "MF_Unarmed_Jog_Fwd", "MF_Rifle_Idle_ADS", "MF_Rifle_Walk_Fwd", "MF_Rifle_Jog_Fwd",
         "MM_Attack_01", "MM_ChargedAttack", "MM_Rifle_Fire", "MM_HitReact_Front_Lgt_01",
         "MM_Death_Front_01", "MM_Death_Back_01", "MM_Death_Left_01", "MM_Death_Right_01"]
MESHES = [("NullTrooper", "DA_Crew_Trooper_Visuals"), ("NullGrunt", "DA_Crew_Grunt_Visuals"), ("NullHeavy", "DA_Crew_Heavy_Visuals")]
SAVED = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "NullCrewAnims").replace("\\", "/")
EAL = unreal.EditorAssetLibrary
APE = unreal.AnimPoseExtensions
bad = []


def log(m):
    for line in str(m).splitlines():
        unreal.log("TDPY: " + line)


def pose(seq, t, et, mesh=None):
    o = unreal.AnimPoseEvaluationOptions()
    o.set_editor_property("evaluation_type", getattr(unreal.AnimDataEvalType, et))
    o.set_editor_property("extract_root_motion", False)
    if mesh is not None:
        try:
            o.set_editor_property("optional_skeletal_mesh", mesh)
        except Exception as e:
            log("note optional mesh %s" % e)
    p = APE.get_anim_pose_at_time(seq, t, o)
    return {str(n): APE.get_bone_pose(p, n, unreal.AnimPoseSpaces.WORLD).translation for n in APE.get_bone_names(p)}


def export(obj, out, exporter, preview=True):
    t = unreal.AssetExportTask()
    t.set_editor_property("object", obj); t.set_editor_property("filename", out)
    t.set_editor_property("automated", True); t.set_editor_property("prompt", False); t.set_editor_property("replace_identical", True)
    t.set_editor_property("exporter", exporter)
    opt = unreal.FbxExportOption()
    for k, v in (("ascii", False), ("export_preview_mesh", preview), ("force_front_x_axis", False),
                 ("level_of_detail", False), ("collision", False), ("export_morph_targets", False)):
        try:
            opt.set_editor_property(k, v)
        except Exception as e:
            log("note opt %s: %s" % (k, e))
    t.set_editor_property("options", opt)
    return unreal.Exporter.run_asset_export_task(t) and os.path.isfile(out)


def main():
    os.makedirs(SAVED, exist_ok=True)
    skel = EAL.load_asset(SKEL)
    sb = [str(n) for n in APE.get_bone_names(APE.get_reference_pose(skel))]
    log("SKEL_NullCrew bones %d (first %s)" % (len(sb), sb[:3]))
    ref = APE.get_reference_pose(skel)
    rh = APE.get_bone_pose(ref, "head", unreal.AnimPoseSpaces.WORLD).translation.z
    log("SKEL ref head.z %.2f" % rh)
    try:
        log("SKEL compatible skeletons %s" % skel.get_editor_property("compatible_skeletons"))
    except Exception as e:
        log("compatible n/a %s" % e)
    for a, da in MESHES:
        m = EAL.load_asset("%s/%s/SK_%s" % (CH, a, a))
        d = EAL.load_asset("%s/%s/%s" % (CH, a, da))
        dm = d.get_editor_property("skeletal_mesh")
        out = SAVED + "/SK_%s_mesh.fbx" % a
        ok = export(m, out, unreal.SkeletalMeshExporterFBX())
        socks = ["%s@%s" % (m.get_socket_by_index(i).get_editor_property("socket_name"), m.get_socket_by_index(i).get_editor_property("bone_name")) for i in range(m.num_sockets())]
        # mesh bone count: the pose evaluated with this mesh (bones the mesh actually has)
        p = pose(EAL.load_asset(OUT + "/MM_Idle"), 0.0, "RAW", m)
        log("MESH %s skel=%s pa=%s mats=%s sockets=%s posebones_with_mesh=%d | %s -> %s resolves=%s | fbx %s" % (
            a, m.get_editor_property("skeleton").get_path_name(), m.get_editor_property("physics_asset").get_path_name(),
            [x.get_editor_property("material_interface").get_path_name() for x in m.get_editor_property("materials")],
            socks, len(p), da, dm.get_path_name() if dm else None, dm == m, ok))
        if dm != m or m.get_editor_property("skeleton") != skel:
            bad.append(a)
    tm = EAL.load_asset(CH + "/NullTrooper/SK_NullTrooper")
    for name in NAMES:
        seq = EAL.load_asset(OUT + "/" + name)
        sk = seq.get_editor_property("skeleton").get_path_name().split(".")[0]
        log("SEQ %s skel=%s len=%.4f frames=%d rootmotion=%s codec=%s additive=%s" % (
            name, sk, seq.get_play_length(), unreal.AnimationLibrary.get_num_frames(seq), seq.get_editor_property("enable_root_motion"),
            unreal.AnimationLibrary.get_bone_compression_settings(seq).get_path_name(), seq.get_editor_property("additive_anim_type")))
        if sk != SKEL:
            bad.append(name)
        L = seq.get_play_length()
        for t in (0.0, L * 0.5, max(0.0, L - 0.034)):
            r = pose(seq, t, "RAW"); c = pose(seq, t, "COMPRESSED")
            diff = max((r[b] - c[b]).length() for b in r if b in c)
            feet = min(c["foot_l"].z, c["foot_r"].z)
            log("POSE %s t=%.3f head.z=%.2f pelvis.z=%.2f foot_l.z=%.2f foot_r.z=%.2f head-minfoot=%.2f root=(%.1f,%.1f,%.1f) raw-vs-compressed max=%.5f cm" % (
                name, t, c["head"].z, c["pelvis"].z, c["foot_l"].z, c["foot_r"].z, c["head"].z - feet,
                c["root"].x, c["root"].y, c["root"].z, diff))
            if (c["head"] - c["foot_l"]).length() < 50:
                bad.append("%s@%.2f collapsed" % (name, t))
        ok = export(seq, SAVED + "/" + name + ".fbx", unreal.AnimSequenceExporterFBX())
        log("export %s %s" % (name, ok))
        if not ok:
            bad.append(name + " export")
    d = unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
    log("dirty packages: %s" % [p.get_name() for p in d])
    log("bad %s" % bad)
    log("OK" if not bad else "FAILED")


main()
