# Editor script (run through ue_exec.py): put one mesh into the open level as the actor "Probe"
# (replacing the previous Probe), so an experiment's result can be captured. The level is not saved.
#
#   python ue_exec.py --port 8017 place_probe.py --args '{"mesh": "/Game/.../_Experiment/SM_Experiment", "x": -3000, "y": 0}'
#   python capture.py --port 8017 --out probe.png --location -5500 0 400 --look-at -3000 0 600
#   python ue_exec.py --port 8017 place_probe.py --args '{"remove": true}'          (take it out again)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for actor in actors.get_all_level_actors():
    if actor.get_actor_label() == "Probe":
        actors.destroy_actor(actor)
result = "removed"
if not ARGS.get("remove"):
    mesh = unreal.load_asset(ARGS["mesh"])
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(ARGS.get("x", 0.0), ARGS.get("y", 0.0), 0.0), unreal.Rotator(roll=0.0, pitch=0.0, yaw=ARGS.get("yaw", 0.0)))
    actor.set_actor_label("Probe")
    actor.static_mesh_component.set_static_mesh(mesh)
    bounds = mesh.get_bounds()
    result = "placed %s: %.2f m tall" % (ARGS["mesh"], (bounds.origin.z + bounds.box_extent.z) / 100.0)
result
