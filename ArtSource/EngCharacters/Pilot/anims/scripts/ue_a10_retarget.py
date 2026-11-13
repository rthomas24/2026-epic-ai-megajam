# Pilot anims step 1 (route 1): IK Rigs + IK Retargeter (saved only in Pilot/Anims) and batch retarget of the
# mannequin source sequences onto SK_Pilot. Saves ONLY assets inside /Game/TriadDrift/Gameplay/Characters/Pilot/Anims.
# Run: powershell -File Tools\td-python.ps1 -Script ArtSource\EngCharacters\Pilot\anims\scripts\ue_a10_retarget.py
import unreal

OUT = "/Game/TriadDrift/Gameplay/Characters/Pilot/Anims"
SRC_MESH = "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"
TGT_MESH = "/Game/TriadDrift/Gameplay/Characters/Pilot/SK_Pilot"
A = "/Game/Characters/Mannequins/Anims/"
JOBS = [
    ("A_Pilot_Light1", A + "Unarmed/Attack/MM_Attack_01"),
    ("A_Pilot_Light2", A + "Unarmed/Attack/MM_Attack_02"),
    ("A_Pilot_Light3", A + "Unarmed/Attack/MM_Attack_03"),
    ("A_Pilot_Heavy", A + "Unarmed/Attack/MM_ChargedAttack"),
    ("A_Pilot_Dash", A + "Unarmed/Jump/MM_Dash"),
    ("A_Pilot_JumpStart", A + "Unarmed/Jump/MM_Jump"),
    ("A_Pilot_FallLoop", A + "Unarmed/Jump/MM_Fall_Loop"),
    ("A_Pilot_Land", A + "Unarmed/Jump/MM_Land"),
    ("A_Pilot_HitReact_Front", A + "Rifle/HitReact/MM_HitReact_Front_Med_01"),
    ("A_Pilot_HitReact_Back", A + "Rifle/HitReact/MM_HitReact_Back_Med_01"),
    ("A_Pilot_Death_Front", A + "Death/MM_Death_Front_01"),
    ("A_Pilot_Death_Back", A + "Death/MM_Death_Back_01"),
]
EAL = unreal.EditorAssetLibrary
SRC_OR_TGT = unreal.RetargetSourceOrTarget


def log(m):
    for line in str(m).splitlines():
        unreal.log("TDPY: " + line)


def make_rig(name, mesh):
    path = OUT + "/" + name
    if EAL.does_asset_exist(path):
        rig = EAL.load_asset(path)
        log("reusing " + path)
    else:
        rig = unreal.IKRigDefinitionFactory.create_new_ik_rig_asset(OUT, name)
    c = unreal.IKRigController.get_controller(rig)
    log("%s set mesh %s" % (name, c.set_skeletal_mesh(mesh)))
    rep = c.get_characterization_report()
    log("%s characterization: %s" % (name, rep))
    ok_def = c.apply_auto_generated_retarget_definition()
    log("%s auto retarget def %s" % (name, ok_def))
    if not ok_def:
        return rig, c, False
    try:
        r = c.apply_auto_fbik()
        log("%s auto fbik %s" % (name, r))
    except Exception as e:
        log("%s auto fbik error %s" % (name, e))
    for ch in c.get_retarget_chains():
        log("  chain %s" % ch)
    log("%s retarget root %s" % (name, c.get_retarget_root()))
    return rig, c, True


def dirty_report(tag):
    try:
        d = unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
        log("%s dirty packages: %s" % (tag, [p.get_name() for p in d]))
    except Exception as e:
        log("dirty report err %s" % e)


def main():
    src_mesh = EAL.load_asset(SRC_MESH)
    tgt_mesh = EAL.load_asset(TGT_MESH)
    log("meshes %s %s" % (src_mesh, tgt_mesh))
    src_rig, sc, ok1 = make_rig("IK_Pilot_SrcManny", src_mesh)
    tgt_rig, tc, ok2 = make_rig("IK_Pilot", tgt_mesh)
    if not (ok1 and ok2):
        log("FAILED: auto retarget definition not applied (src %s tgt %s)" % (ok1, ok2))
        dirty_report("fail")
        return
    rpath = OUT + "/RTG_Manny_to_Pilot"
    if EAL.does_asset_exist(rpath):
        rtg = EAL.load_asset(rpath)
    else:
        rtg = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "RTG_Manny_to_Pilot", OUT, unreal.IKRetargeter, unreal.IKRetargetFactory())
    rc = unreal.IKRetargeterController.get_controller(rtg)
    rc.set_ik_rig(SRC_OR_TGT.SOURCE, src_rig)
    rc.set_ik_rig(SRC_OR_TGT.TARGET, tgt_rig)
    rc.set_preview_mesh(SRC_OR_TGT.SOURCE, src_mesh)
    rc.set_preview_mesh(SRC_OR_TGT.TARGET, tgt_mesh)
    rc.add_default_ops()
    rc.assign_ik_rig_to_all_ops(SRC_OR_TGT.SOURCE, src_rig)
    rc.assign_ik_rig_to_all_ops(SRC_OR_TGT.TARGET, tgt_rig)
    rc.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
    log("chain map %s" % rc.get_chain_map())
    for i in range(rc.get_num_retarget_ops()):
        log("op %d %s enabled=%s" % (i, rc.get_op_name(i), rc.get_retarget_op_enabled(i)))
    try:
        rc.auto_align_all_bones(SRC_OR_TGT.TARGET, unreal.RetargetAutoAlignMethod.CHAIN_TO_CHAIN)
        log("auto align target done")
    except Exception as e:
        log("auto align error %s" % e)
    try:
        log("validate %s" % rc.validate_retargeter())
    except Exception as e:
        log("validate err %s" % e)
    for p in (OUT + "/IK_Pilot_SrcManny", OUT + "/IK_Pilot", rpath):
        log("save %s %s" % (p, EAL.save_asset(p, only_if_is_dirty=False)))

    created = []
    for name, src in JOBS:
        dst = OUT + "/" + name
        if EAL.does_asset_exist(dst):
            log("exists, deleting old %s %s" % (dst, EAL.delete_asset(dst)))
        ad = unreal.EditorAssetLibrary.find_asset_data(src)
        srcname = src.split("/")[-1]
        try:
            res = unreal.IKRetargetBatchOperation.duplicate_and_retarget(
                [ad], src_mesh, tgt_mesh, rtg, srcname, name, "", "", OUT, False, False, True)
        except Exception as e:
            log("retarget error %s: %s" % (name, e))
            continue
        paths = [str(r.package_name) for r in res]
        log("retargeted %s -> %s" % (srcname, paths))
        if EAL.does_asset_exist(dst):
            seq = EAL.load_asset(dst)
            log("NEW %s len=%.3f frames=%d skel=%s" % (name, seq.get_play_length(),
                unreal.AnimationLibrary.get_num_frames(seq), seq.get_editor_property("skeleton").get_name()))
            created.append(dst)
        else:
            log("MISSING after retarget: " + dst)
    for p in created:
        log("save %s %s" % (p, EAL.save_asset(p, only_if_is_dirty=False)))
    dirty_report("end")
    log("created %d of %d" % (len(created), len(JOBS)))
    log("OK")


main()
