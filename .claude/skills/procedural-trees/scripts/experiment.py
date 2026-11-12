# Editor script (run through ue_exec.py with --lib pve_lib.py): try grower overrides on one species and
# measure what each does. Use it when tuning an archetype or adding a new one, instead of guessing.
#
#   python ue_exec.py --port 8017 --lib pve_lib.py experiment.py --args @exp.json
#
# ARGS: {"biome": <biome file contents>, "species": "Cacao", "stage": 1.0, "seed": 61,
#        "overrides": {"name": "(TrunkGrowth=(BranchTargetLength=1.0))", ...},     FPVGrowerParams text applied on top
#        "foliage": {"name": "(SpacingSettings=(BranchDensity=3))", ...},          parametric foliage text applied on top
#        "params": {"branch_angle_deg": 30},                                       optional changes to the normalised params
#        "keep": "name"}                                                           optional: leave this one exported for a capture
# Everything is written to <content_root>/_Experiment and that folder is deleted at the end unless "keep" is set.
BIOME = ARGS["biome"]
species = next(s for s in BIOME["species"] if s["id"] == ARGS["species"])
folder = BIOME["content_root"] + "/_Experiment"
ensure_folder(folder)
params = tree_params(species, ARGS.get("stage", 1.0), ARGS.get("seed", 1))
params.update(ARGS.get("params") or {})
species_folder = "%s/%s" % (BIOME["content_root"], species["id"])
leaf_mesh = "%s/SM_TDVeg_%s_Leaf" % (species_folder, species["id"])
bark = "%s/MI_TDVeg_%s_Bark.MI_TDVeg_%s_Bark" % (species_folder, species["id"], species["id"])
graph, nodes = build_graph(folder, "PVE_Experiment", params, leaf_mesh, folder, "SM_Experiment", bark)
# Experiments run at the species' real size, so foliage and trunk read as they will in the game.
nodes["scale"].get_settings().set_editor_property("scale", params["mature_height_m"] / CANONICAL_HEIGHT_M)
rows = {}
overrides = dict(ARGS.get("overrides") or {"base": "()"})
keep = ARGS.get("keep")
if keep in overrides:
    overrides[keep] = overrides.pop(keep)  # run it last so its mesh is the one left on disk
for name, text in overrides.items():
    settings = nodes["grower"].get_settings()
    apply_grower(nodes, params, 1.0)
    set_struct(settings, "grower_params", text)
    try:
        await execute(graph)
        created, warnings = await export(graph)
        mesh = next(asset for asset in map(unreal.load_asset, created) if isinstance(asset, unreal.StaticMesh))
        measured = measure(mesh)
        rows[name] = "h %.2f  crown %.2f  tris %d" % (measured["height_m"], measured["crown_width_m"], measured["triangles"])
    except Exception as error:
        rows[name] = "FAILED: %s" % error
    await asyncio.sleep(0)
for name, text in (ARGS.get("foliage") or {}).items():
    apply_grower(nodes, params, 1.0)
    set_struct(nodes["foliage"].get_settings(), "parametric_settings", text)
    try:
        await execute(graph)
        created, warnings = await export(graph)
        mesh = next(asset for asset in map(unreal.load_asset, created) if isinstance(asset, unreal.StaticMesh))
        measured = measure(mesh)
        rows["foliage:" + name] = "h %.2f  crown %.2f  tris %d" % (measured["height_m"], measured["crown_width_m"], measured["triangles"])
    except Exception as error:
        rows["foliage:" + name] = "FAILED: %s" % error
    await asyncio.sleep(0)
if not keep:
    unreal.EditorAssetLibrary.delete_directory(folder)
{"target": "h %.2f crown %.2f" % (params["height_m"], params["crown_width_m"]), "results": rows, "kept": ("%s/SM_Experiment" % folder) if keep else None}
