# Animation audit: what the engineering visuals assets point at (mesh, AnimBPClass, clip slots). Reads only.
import unreal
EAL = unreal.EditorAssetLibrary
CH = "/Game/TriadDrift/Gameplay/Characters/"
for p in ("Pilot/DA_Pilot_Visuals", "NullTrooper/DA_Crew_Trooper_Visuals", "NullGrunt/DA_Crew_Grunt_Visuals", "NullHeavy/DA_Crew_Heavy_Visuals", "NullDrone/DA_Crew_Drone_Visuals"):
    a = EAL.load_asset(CH + p)
    out = []
    for k in ("skeletal_mesh", "static_mesh", "anim_bp_class", "idle", "walk", "run", "mesh_yaw", "mesh_scale", "head_bone"):
        try:
            v = a.get_editor_property(k)
            out.append("%s=%s" % (k, v.to_soft_object_path() if hasattr(v, "to_soft_object_path") else v))
        except Exception as e:
            pass
    unreal.log("TDPY: %s (%s): %s" % (p, a.get_class().get_name(), "; ".join(out)))
unreal.log("TDPY: OK")
