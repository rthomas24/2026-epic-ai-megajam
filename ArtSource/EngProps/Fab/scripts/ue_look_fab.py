# SPDX-License-Identifier: MIT
# Triad Drift - one in-engine look at the three Fab props next to the pilot for scale. Run at editor start-up
# (Tools/td-play-mcp.ps1 -Editor -Map /Game/FirstPerson/Lvl_FirstPerson -ExtraArgs '-ExecutePythonScript=<this>'),
# then capture with SceneCaptureToolset. Spawns unsaved actors in the template level only: nothing is saved and no
# shipping map is touched. Logs "TDPY: LOOK <name> x y z" for the capture cameras.
import unreal

PROPS = [("CrateStack", 0.0), ("PowerCore", 700.0), ("OilBarrel", 1400.0)]
ROOT = "/Game/TriadDrift/Gameplay/Props/Fab"
PILOT = "/Game/TriadDrift/Gameplay/Characters/Pilot/SK_Pilot"

sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
start = next((a for a in sub.get_all_level_actors() if isinstance(a, unreal.PlayerStart)), None)
base = start.get_actor_location() if start else unreal.Vector(0, 0, 200)


def ground(x, y):
    hit = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, y, base.z + 500), unreal.Vector(x, y, base.z - 3000),
                                                 unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [], unreal.DrawDebugTrace.NONE, True)
    if hit:
        t = hit.to_tuple()
        for v in t:
            if isinstance(v, unreal.Vector) and v.z < base.z + 400:
                return v.z
    return base.z - 92.0


for name, dy in PROPS:
    x, y = base.x + 400.0, base.y + dy
    z = ground(x, y)
    mesh = unreal.load_asset("%s/%s/SM_Fab_%s" % (ROOT, name, name))
    a = sub.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y + 90.0, z), unreal.Rotator(0, 0, 200))
    a.static_mesh_component.set_static_mesh(mesh)
    p = sub.spawn_actor_from_class(unreal.SkeletalMeshActor, unreal.Vector(x, y - 90.0, z), unreal.Rotator(0, 0, 0))
    p.skeletal_mesh_component.set_skinned_asset_and_update(unreal.load_asset(PILOT))
    unreal.log("TDPY: LOOK %s %.1f %.1f %.1f" % (name, x, y, z))
unreal.log("TDPY: OK")
