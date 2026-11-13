# Crew anims step 0.5: for every ADDITIVE source among the Crew jobs, make a full-pose working copy in
# Crew/Anims/Tmp/<name> (duplicate, additive type -> none; an additive sequence's raw data is already the full pose,
# e.g. the rifle idle pose plus the fire kick). ue_c10_retarget.py then retargets from that copy, so the final asset
# is still created fresh by the batch retarget. Only assets in Crew/Anims/Tmp are created and saved.
import unreal

TMP = "/Game/TriadDrift/Gameplay/Characters/Crew/Anims/Tmp"
A = "/Game/Characters/Mannequins/Anims/"
SRCS = [A + "Rifle/MM_Rifle_Fire", A + "Rifle/HitReact/MM_HitReact_Front_Lgt_01"]
EAL = unreal.EditorAssetLibrary


def log(m):
    unreal.log("TDPY: " + str(m))


for src in SRCS:
    s = EAL.load_asset(src)
    name = src.split("/")[-1]
    at = s.get_editor_property("additive_anim_type")
    log("%s additive=%s refposetype=%s refseq=%s" % (name, at, s.get_editor_property("ref_pose_type"),
                                                     s.get_editor_property("ref_pose_seq")))
    if at == unreal.AdditiveAnimationType.AAT_NONE:
        continue
    dst = TMP + "/" + name
    if EAL.does_asset_exist(dst):
        EAL.delete_asset(dst)
    d = EAL.duplicate_asset(src, dst)
    d.set_editor_property("additive_anim_type", unreal.AdditiveAnimationType.AAT_NONE)
    d.set_editor_property("ref_pose_type", unreal.AdditiveBasePoseType.ABPT_NONE)
    d.set_editor_property("ref_pose_seq", None)
    log("tmp %s additive=%s save=%s" % (dst, d.get_editor_property("additive_anim_type"),
                                         EAL.save_asset(dst, only_if_is_dirty=False)))
dd = unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
log("dirty packages: %s" % [p.get_name() for p in dd])
log("OK")
