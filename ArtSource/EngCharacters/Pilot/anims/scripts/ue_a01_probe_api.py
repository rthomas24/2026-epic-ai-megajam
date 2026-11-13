# Pilot anims step 0b: print docstrings of the retarget API. Writes nothing.
import unreal


def log(m):
    for line in str(m).splitlines():
        unreal.log("TDPY: " + line)


for cname, meths in [
    ("IKRetargetBatchOperation", ["duplicate_and_retarget", "run_batch_retarget", "run_batch_retarget_with_results"]),
    ("IKRigController", ["get_controller", "set_skeletal_mesh", "apply_auto_generated_retarget_definition", "apply_auto_fbik", "get_characterization_report", "set_retarget_root"]),
    ("IKRetargeterController", ["get_controller", "set_ik_rig", "add_default_ops", "auto_map_chains", "auto_align_all_bones", "set_preview_mesh", "get_retarget_op_types", "run_op_initial_setup", "set_root_offset_in_retarget_pose", "snap_bone_to_ground", "get_chain_map", "assign_ik_rig_to_all_ops", "create_retarget_pose", "set_current_retarget_pose"]),
    ("IKRigDefinitionFactory", ["create_new_ik_rig_asset"]),
]:
    c = getattr(unreal, cname)
    for m in meths:
        f = getattr(c, m, None)
        log("=== %s.%s" % (cname, m))
        log(f.__doc__ if f else "absent")
for sname in dir(unreal):
    if "Retarget" in sname or "IKRig" in sname:
        log("TYPE " + sname)
for sname in ["IKRetargetBatchOperationContext", "BatchRetargetSettings"]:
    s = getattr(unreal, sname, None)
    if s:
        log("=== " + sname)
        log(s.__doc__)
log("OK")
