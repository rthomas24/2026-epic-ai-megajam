# Editor script (run through ue_exec.py): builds the verification map for a generated biome.
#
#   python ue_exec.py --port 8017 build_demo_map.py --args @demo_args.json
#
# ARGS: {"map": "/Game/TriadDrift/Gameplay/Vegetation/Maps/L_VegetationDemo",
#        "report": <contents of biomes/<name>.report.json>,
#        "mannequin": "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple",     (read-only reference)
#        "forest": {"Kapok": 6, ...}          instances per species in the forest patch (optional)
#        "material_folder": "/Game/TriadDrift/Gameplay/Vegetation/Materials"}
#
# Layout (cm, +X is "into the forest"):
#   y = 0 line   one mature tree of every species in a row along +Y, its sapling stages beside it,
#                the mannequin at the origin for scale (it is 1.8 m; the player's eye is at 1.54 m).
#   x > 150 m    a forest patch: every variant as hierarchical instanced static meshes.
# The level is NOT World Partition, so everything is saved inside the one .umap.
import math
import random

MAP = ARGS.get("map", "/Game/TriadDrift/Gameplay/Vegetation/Maps/L_VegetationDemo")
REPORT = ARGS["report"]
MANNEQUIN = ARGS.get("mannequin", "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple")
FOREST = ARGS.get("forest") or {}
MATERIAL_FOLDER = ARGS.get("material_folder", "/Game/TriadDrift/Gameplay/Vegetation/Materials")

level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
folder = MAP.rsplit("/", 1)[0]
if not unreal.EditorAssetLibrary.does_directory_exist(folder):
    unreal.EditorAssetLibrary.make_directory(folder)
if unreal.EditorAssetLibrary.does_asset_exist(MAP):
    current = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    if current is None or current.get_outermost().get_name() != MAP:
        # (Re-exporting a mesh the open map uses marks the map dirty, and load_level refuses to drop unsaved changes.)
        level_editor.load_level(MAP)
    for actor in actors.get_all_level_actors():
        if actor.get_class().get_name() not in ("WorldSettings", "Brush", "WorldDataLayers", "DefaultPhysicsVolume"):
            actors.destroy_actor(actor)
else:
    level_editor.new_level(MAP, False)


def spawn(cls, label, location=(0, 0, 0), rotation=(0, 0, 0)):
    actor = actors.spawn_actor_from_class(cls, unreal.Vector(*location), unreal.Rotator(roll=rotation[0], pitch=rotation[1], yaw=rotation[2]))
    actor.set_actor_label(label)
    return actor


def static_mesh_actor(label, mesh, location, yaw=0.0, scale=1.0, material=None):
    actor = spawn(unreal.StaticMeshActor, label, location, (0, 0, yaw))
    component = actor.static_mesh_component
    component.set_static_mesh(mesh)
    actor.set_actor_scale3d(unreal.Vector(scale, scale, scale) if not isinstance(scale, tuple) else unreal.Vector(*scale))
    if material:
        component.set_material(0, material)
    return actor


# ---- stage: ground, moonlight, sky, mist, fixed exposure ----
ground_path = "%s/MI_TDVeg_DemoGround" % MATERIAL_FOLDER
ground = unreal.load_asset(ground_path) if unreal.EditorAssetLibrary.does_asset_exist(ground_path) else None
if ground is None:
    ground = unreal.AssetToolsHelpers.get_asset_tools().create_asset("MI_TDVeg_DemoGround", MATERIAL_FOLDER, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    unreal.MaterialEditingLibrary.set_material_instance_parent(ground, unreal.load_asset(MATERIAL_FOLDER + "/M_TDVeg_Bark"))
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(ground, "BarkColor", unreal.LinearColor(0.012, 0.03, 0.035, 1.0))
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(ground, "RimGlow", 0.0)
    unreal.EditorAssetLibrary.save_loaded_asset(ground)
static_mesh_actor("Ground", unreal.load_asset("/Engine/BasicShapes/Plane"), (20000, 0, 0), 0.0, (900.0, 900.0, 1.0), ground)

moon = spawn(unreal.DirectionalLight, "Moonlight", (0, 0, 5000), (0, -38, 35))
moon.light_component.set_editor_property("intensity", ARGS.get("moon_lux", 9.0))
moon.light_component.set_editor_property("light_color", unreal.Color(r=150, g=190, b=255, a=255))
moon.light_component.set_editor_property("atmosphere_sun_light", True)
spawn(unreal.SkyAtmosphere, "SkyAtmosphere")
sky = spawn(unreal.SkyLight, "SkyLight")
sky.light_component.set_editor_property("real_time_capture", True)
sky.light_component.set_editor_property("intensity", 1.6)
fog = spawn(unreal.ExponentialHeightFog, "Mist")
fog.component.set_editor_property("fog_density", 0.012)
fog.component.set_editor_property("fog_height_falloff", 0.06)
fog.component.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(0.02, 0.09, 0.11, 1.0))
volume = spawn(unreal.PostProcessVolume, "Exposure")
volume.set_editor_property("unbound", True)
settings = volume.get_editor_property("settings")
# A fixed exposure (histogram clamped to one EV100 value), so captures compare from run to run and the
# glow stays an accent instead of the only thing the eye adapts to.
exposure = ARGS.get("exposure_ev100", 1.0)
for key, value in (("override_auto_exposure_method", True), ("auto_exposure_method", unreal.AutoExposureMethod.AEM_HISTOGRAM),
                   ("override_auto_exposure_min_brightness", True), ("auto_exposure_min_brightness", exposure),
                   ("override_auto_exposure_max_brightness", True), ("auto_exposure_max_brightness", exposure),
                   ("override_auto_exposure_bias", True), ("auto_exposure_bias", 0.0),
                   ("override_bloom_intensity", True), ("bloom_intensity", 0.9)):
    settings.set_editor_property(key, value)
volume.set_editor_property("settings", settings)

# ---- scale reference ----
mannequin_mesh = unreal.load_asset(MANNEQUIN)
mannequin = spawn(unreal.SkeletalMeshActor, "ScaleReference_Mannequin", (0, -300, 0), (0, 0, 90))
mannequin.skeletal_mesh_component.set_skeletal_mesh_asset(mannequin_mesh)
bounds = mannequin_mesh.get_bounds()
mannequin_height_cm = bounds.origin.z + bounds.box_extent.z

# ---- line-up: mature trees along +Y, younger stages in front of them ----
layout = {"mannequin_height_cm": round(mannequin_height_cm, 1), "lineup": [], "forest": {}}
cursor = 0.0
for species in REPORT["species"]:
    variants = sorted(species["variants"], key=lambda v: -v["stage"])
    widest = max(v["crown_width_m"] for v in variants) * 100.0
    cursor += widest * 0.5 + 300.0
    for index, variant in enumerate(variants):
        mesh = unreal.load_asset(variant["mesh"])
        x = 0.0 if index == 0 else -(variants[0]["crown_width_m"] * 50.0 + 400.0 * index + sum(v["crown_width_m"] * 100.0 for v in variants[1:index + 1]) * 0.6)
        location = (x, cursor, 0.0)
        static_mesh_actor("%s_%s" % (species["id"], variant["mesh"].rsplit("_", 2)[-2] + "_" + variant["mesh"].rsplit("_", 1)[-1]), mesh, location)
        layout["lineup"].append({"species": species["id"], "mesh": variant["mesh"], "stage": variant["stage"], "location_cm": list(location),
                                 "height_m": variant["height_m"], "crown_width_m": variant["crown_width_m"]})
    cursor += widest * 0.5
layout["lineup_length_cm"] = round(cursor)

# ---- forest patch: hierarchical instanced static meshes ----
if FOREST:
    rng = random.Random(606)
    holder = spawn(unreal.Actor, "Forest", (0, 0, 0))
    root_added = await call_tool("EditorToolset.ActorToolset.AddComponent", owner=holder, componentType=unreal.SceneComponent, name="Root")
    centre = (30000.0, cursor * 0.5)
    radius = ARGS.get("forest_radius_cm", 14000.0)
    for species in REPORT["species"]:
        total = int(FOREST.get(species["id"], 0))
        if total <= 0:
            continue
        variants = species["variants"]
        placed = 0
        for index, variant in enumerate(variants):
            share = total // len(variants) + (1 if index < total % len(variants) else 0)
            if share <= 0:
                continue
            name = "HISM_%s_%d" % (species["id"], index)
            await call_tool("EditorToolset.ActorToolset.AddComponent", owner=holder, componentType=unreal.HierarchicalInstancedStaticMeshComponent, name=name)
            component = next(c for c in holder.get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent) if c.get_name() == name)
            component.set_static_mesh(unreal.load_asset(variant["mesh"]))
            transforms = []
            for _ in range(share):
                distance = radius * math.sqrt(rng.random())
                angle = rng.uniform(0.0, 2.0 * math.pi)
                scale = rng.uniform(0.88, 1.12)
                transforms.append(unreal.Transform(unreal.Vector(centre[0] + distance * math.cos(angle), centre[1] + distance * math.sin(angle), 0.0),
                                                   unreal.Rotator(roll=0.0, pitch=0.0, yaw=rng.uniform(0.0, 360.0)), unreal.Vector(scale, scale, scale)))
            component.add_instances(transforms, False, True)
            placed += share
            await asyncio.sleep(0)
        layout["forest"][species["id"]] = placed
    layout["forest_centre_cm"] = [centre[0], centre[1]]
    layout["forest_radius_cm"] = radius
    layout["forest_instances"] = sum(layout["forest"].values())

level_editor.save_current_level()
layout["map"] = MAP
layout
