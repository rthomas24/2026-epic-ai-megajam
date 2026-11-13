# Pilot anims step 6: additive copies of the hit reacts (the sources are rifle hit reacts, so the full-body versions hold
# the arms in a rifle grip). The additive copies are local-space additive against their own key 0, so only the flinch is
# layered on top of whatever the pilot is doing. Only assets in Pilot/Anims are created and saved.
import unreal

OUT = "/Game/TriadDrift/Gameplay/Characters/Pilot/Anims"
EAL = unreal.EditorAssetLibrary


def log(m):
    unreal.log("TDPY: " + str(m))


for name in ("A_Pilot_HitReact_Front", "A_Pilot_HitReact_Back"):
    dst = OUT + "/" + name + "_Add"
    if EAL.does_asset_exist(dst):
        EAL.delete_asset(dst)
    s = EAL.duplicate_asset(OUT + "/" + name, dst)
    s.set_editor_property("additive_anim_type", unreal.AdditiveAnimationType.AAT_LOCAL_SPACE_BASE)
    s.set_editor_property("ref_pose_type", unreal.AdditiveBasePoseType.ABPT_ANIM_FRAME)
    s.set_editor_property("ref_pose_seq", EAL.load_asset(OUT + "/" + name))
    s.set_editor_property("ref_frame_index", 0)
    log("%s additive=%s ref=%s len=%.3f frames=%d save=%s" % (
        dst.split("/")[-1], s.get_editor_property("additive_anim_type"), s.get_editor_property("ref_pose_type"),
        s.get_play_length(), unreal.AnimationLibrary.get_num_frames(s), EAL.save_asset(dst, only_if_is_dirty=False)))
d = unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
log("dirty packages: %s" % [x.get_name() for x in d])
log("OK")
