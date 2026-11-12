# Editor-side library for the procedural-trees skill. Prepend it to a script with ue_exec.py --lib.
# Everything here runs inside the Unreal editor (the `unreal` and `asyncio` modules are already there).
#
# Flow for one tree:   species JSON -> unreal.TDTreeSpeciesData -> TDVegetationLibrary.build_tree_params
#                      (C++, deterministic) -> grower_text() -> PVE grower preset + PVE graph ->
#                      execute -> export a static mesh -> measure -> correct the scale -> export again.
import time

PVE_CYCLES_MATURE = 40          # growth cycles of a mature tree; a sapling gets PVE_CYCLES_SAPLING
PVE_CYCLES_SAPLING = 10
CANONICAL_INCREMENTAL_RADIUS_M = 0.0075   # trunk radius gained per cycle while growing (0.6 m across after 40 cycles)
CANONICAL_HEIGHT_M = 12.0       # every tree is GROWN at this mature height and then scaled to its real size by the
                                # Scale node. The grower's physics (weight, light, breaking) is not scale-free: a tree
                                # grown directly at 60 m loses nearly all its branches, one grown at 12 m does not.
                                # One internode per cycle whatever the size, so a 60 m giant and an 8 m
                                # understorey tree have about the same number of segments (and triangles).


def pve():
    return unreal.get_editor_subsystem(unreal.ProceduralVegetationEditorSubsystem)


# ---------------------------------------------------------------- species data -> C++ matcher
_ENUMS = {
    "crown_shape": ("TDCrownShape", {"Unknown": "UNKNOWN", "Conical": "CONICAL", "Columnar": "COLUMNAR", "Spreading": "SPREADING", "Round": "ROUND", "Oval": "OVAL", "Weeping": "WEEPING", "Rosette": "ROSETTE", "Irregular": "IRREGULAR"}),
    "branching": ("TDBranchingHabit", {"Unknown": "UNKNOWN", "Excurrent": "EXCURRENT", "Decurrent": "DECURRENT", "Unbranched": "UNBRANCHED", "MultiStem": "MULTI_STEM", "Pendulous": "PENDULOUS"}),
    "leaf_type": ("TDLeafType", {"Unknown": "UNKNOWN", "Broadleaf": "BROADLEAF", "CompoundBroadleaf": "COMPOUND_BROADLEAF", "Needle": "NEEDLE", "Scale": "SCALE", "Frond": "FROND"}),
    "persistence": ("TDLeafPersistence", {"Unknown": "UNKNOWN", "Evergreen": "EVERGREEN", "Deciduous": "DECIDUOUS"}),
}
_NUMBERS = ("height_m", "crown_width_m", "trunk_diameter_m", "clear_bole_fraction", "leaf_length_m", "crown_leaf_count")


def species_struct(species):
    """Biome-file species entry -> unreal.TDTreeSpeciesData. A field that is absent stays 'unknown'."""
    data = species.get("data", {})
    out = unreal.TDTreeSpeciesData()
    out.set_editor_property("common_name", species.get("common_name", ""))
    out.set_editor_property("latin_name", species.get("latin_name", ""))
    for key in _NUMBERS:
        if key in data and data[key].get("value") is not None:
            value = data[key]["value"]
            out.set_editor_property(key, int(value) if key == "crown_leaf_count" else float(value))
    for key, (enum_name, names) in _ENUMS.items():
        if key in data and data[key].get("value"):
            out.set_editor_property(key, getattr(getattr(unreal, enum_name), names[data[key]["value"]]))
    return out


_PARAM_FIELDS = ("height_m", "crown_width_m", "trunk_diameter_m", "mature_height_m", "crown_base_fraction", "apical_dominance",
                 "branch_angle_deg", "branch_droop", "branch_density", "branch_orders", "crown_flatness", "stem_count",
                 "leaf_length_m", "leaf_density", "crown_leaf_count", "evergreen", "growth_stage", "seed")


def tree_params(species, stage, seed):
    """Runs the C++ matcher and returns its result as a plain dict (the normalised parameter format)."""
    result = unreal.TDVegetationLibrary.build_tree_params(species_struct(species), float(stage), int(seed))
    out = {name: result.get_editor_property(name) for name in _PARAM_FIELDS}
    out["archetype"] = str(result.get_editor_property("archetype")).split(".")[-1].split(":")[0]
    out["leaf_type"] = str(result.get_editor_property("leaf_type")).split(".")[-1].split(":")[0]
    out["archetype_reason"] = str(result.get_editor_property("archetype_reason"))
    out["defaulted"] = [str(name) for name in result.get_editor_property("defaulted_fields")]
    out["notes"] = [str(note) for note in result.get_editor_property("notes")]
    out["common_name"] = species.get("common_name", "")
    out["latin_name"] = species.get("latin_name", "")
    return out


# ---------------------------------------------------------------- normalised params -> PVE grower
def ramp(points):
    """[(x, y), ...] -> the text form of an FPVFloatRamp."""
    keys = ",".join("(Time=%.4f,Value=%.4f)" % (x, y) for x, y in points)
    return "(EditorCurveData=(Keys=(%s)))" % keys


# What each archetype needs from PVE that the normalised numbers alone do not say. Tuned by eye
# against the engine's starter presets; the numbers that differ per species come from tree_params.
PVE_ARCHETYPE = {
    #                  phyllotaxy of the trunk            bifurcation (trunk, branch)   light pruning   whorl
    "CONICAL_CONIFER": {"type": "Whorled", "bifurcate": (False, False), "senescence": 0.10, "whorl": (4, 6), "branch_formation": "Disticious", "flatten": 0.6},
    "COLUMNAR":        {"type": "Spiral",  "bifurcate": (False, False), "senescence": 0.10, "whorl": (3, 6), "branch_formation": "Octasticious", "flatten": 0.0},
    "BROAD_SPREADING": {"type": "Spiral",  "bifurcate": (True, True),   "senescence": 0.25, "whorl": (3, 6), "branch_formation": "Octasticious", "flatten": 0.2},
    "ROUND_OVAL":      {"type": "Spiral",  "bifurcate": (False, True),   "senescence": 0.20, "whorl": (3, 6), "branch_formation": "Octasticious", "flatten": 0.0},
    "WEEPING":         {"type": "Spiral",  "bifurcate": (True, False),  "senescence": 0.15, "whorl": (3, 6), "branch_formation": "Octasticious", "flatten": 0.0},
    "PALM":            {"type": "Spiral",  "bifurcate": (False, False), "senescence": 0.00, "whorl": (3, 6), "branch_formation": "Octasticious", "flatten": 0.0},
    "MULTI_STEM_SHRUB": {"type": "Spiral", "bifurcate": (True, True),   "senescence": 0.10, "whorl": (3, 6), "branch_formation": "Octasticious", "flatten": 0.0},
}


def trunk_radius_scale(params):
    """Mesh Builder radius multiplier that turns the canonical girth into the species' trunk diameter."""
    to_canonical = CANONICAL_HEIGHT_M / max(0.3, params["mature_height_m"])
    grown = 2.0 * growth_cycles(params) * CANONICAL_INCREMENTAL_RADIUS_M
    return min(4.0, max(0.15, params["trunk_diameter_m"] * to_canonical / grown))


def growth_cycles(params):
    return int(round(PVE_CYCLES_SAPLING + (PVE_CYCLES_MATURE - PVE_CYCLES_SAPLING) * params["growth_stage"]))


def bole_cycles(params):
    """Cycles the trunk grows bare before the crown starts (0 = branch from the ground up, or a rosette form).
    Measured on UE 5.9.0: the grower's axillary ramps do not keep the lower trunk clear (a ramp of zeros still
    branches), and Carve trims from the top. What does work is growing in two phases, as a real forest tree
    does: a first Grower raises a bare trunk to the crown base, a second one resumes from it and branches."""
    if int(params["branch_orders"]) <= 1:
        return 0
    cycles = growth_cycles(params)
    bare = int(round(params["crown_base_fraction"] * cycles))
    return bare if bare >= 3 else 0


def grower_text(params, crown_scale=1.0, phase="whole"):
    """The FPVGrowerParams text (Unreal import_text form) for one tree. Only the fields listed are changed.
    crown_scale multiplies the side-branch length; generate_tree() uses it to correct the crown width."""
    kind = PVE_ARCHETYPE[params["archetype"]]
    to_canonical = CANONICAL_HEIGHT_M / max(0.3, params["mature_height_m"])
    height = max(0.3, params["height_m"] * to_canonical)
    cycles = growth_cycles(params)
    total_cycles = cycles
    if phase == "bole":
        cycles = bole_cycles(params)
    elif phase == "crown":
        cycles = cycles - bole_cycles(params)
    # Girth is grown the same for every species (a thin trunk bends and collapses in the grower's physics);
    # the species' real trunk diameter is applied afterwards by the Mesh Builder, see trunk_radius_scale().
    incremental_radius = CANONICAL_INCREMENTAL_RADIUS_M
    # The trunk adds one internode per cycle and must reach the target height by the last one.
    segment = height / (total_cycles * 0.92)
    crown_base = min(0.95, max(0.0, params["crown_base_fraction"]))
    crown_radius = max(0.2, params["crown_width_m"] * to_canonical * 0.5) * crown_scale
    angle = params["branch_angle_deg"]
    orders = int(params["branch_orders"])
    apical = params["apical_dominance"]
    droop = params["branch_droop"]
    density = params["branch_density"]
    flat = params["crown_flatness"]

    # Side branches: none below the crown base, then rising to full density.
    axillary = ramp([(0.0, 0.0), (max(0.01, crown_base - 0.02), 0.0), (min(0.99, crown_base + 0.08), 0.35 + 0.65 * density), (1.0, 1.0)])
    # The leader: with high apical dominance it keeps going to the top; with low it gives up early.
    apical_ramp = ramp([(0.0, 1.0), (0.45, 0.35 + 0.6 * apical), (1.0, 0.15 * apical)])
    # Side branches stop near the crown radius.
    child_ramp = ramp([(0.0, 1.0), (0.6, 0.55), (1.0, 0.05)])
    trunk_bifurcate = "True" if (kind["bifurcate"][0] and apical < 0.6 and orders > 1) else "False"
    branch_bifurcate = "True" if (kind["bifurcate"][1] and orders > 2) else "False"
    gravity = 0.4 + 9.0 * droop
    # Side branches only have the crown phase to grow in: lengthen their internodes so they can reach the crown radius.
    branch_scale = min(3.0, max(1.0, crown_radius / (max(1, cycles) * segment * 0.55)))
    # Longer internodes are longer levers: without this a wide, thin-limbed crown sags to the ground.
    gravity = gravity / (branch_scale * branch_scale)
    cell_weight = 0.08 + 1.6 * droop

    if phase == "bole":
        orders = 1
    if orders <= 1:
        # Rosette forms and the bare-trunk phase: a stem with no branches. The fronds are placed by the foliage distributor, not grown.
        return ("(GrowthCycles=%d,RandomSeed=%d,"
                "Phyllotaxy=(AxilAngle=60.0,Type=Spiral,Formation=Octasticious),"
                "TrunkGrowth=(MaxGeneration=1,IncrementalRadius=%.5f,PlantTargetLength=%.3f,BranchTargetLength=0.0,SegmentLength=%.4f,AxillaryParentGrowth=0.0,AxillaryChildGrowth=0.0,"
                "AxillaryPriorityGradient=%s,ApicalPriorityGradient=%s,Corymb=0.0),"
                "Phototropism=(Apical=0.05,ApicalBias=0.2,Axillary=0.0,AxillaryBias=0.5,LightRequirement=0.0),"
                "GravityParams=(GravitationalForce=0.3,CellWeight=0.05,TrunkReinforcement=0.9),bSenescence=False,"
                "Bifurcation=(bEnableBifurcation=False),BranchBifurcation=(bEnableBifurcation=False),"
                "Directional=(ApicalRandomAngle=%.2f,AxillaryRandomAngle=0.0),Auxin=(ApicalDominance=1.0,RadicalAuxin=1.0),"
                "Foliage=(Density=0))") % (cycles, params["seed"], incremental_radius, height, segment, ramp([(0.0, 0.0), (1.0, 0.0)]), ramp([(0.0, 1.0), (1.0, 1.0)]), 2.5)

    return ("(GrowthCycles=%d,RandomSeed=%d,"
            "Phyllotaxy=(AxilAngle=%.2f,Type=%s,Formation=Octasticious,Min=%d,Max=%d,Stagger=0.12),"
            "bBranchPhyllotaxySameAsTrunk=False,BranchPhyllotaxy=(AxilAngle=%.2f,Type=Spiral,Formation=%s,Flatten=%.2f,Stagger=0.3),"
            "TrunkGrowth=(MaxGeneration=%d,IncrementalRadius=%.5f,PlantTargetLength=%.3f,BranchTargetLength=%.3f,SegmentLength=%.4f,BranchScale=%.3f,"
            "AxillaryParentGrowth=%.3f,AxillaryChildGrowth=%.3f,bAxillaryRetry=True,AxillaryRampBasis=PlantTargetLength,AxillaryPriorityGradient=%s,"
            "ApicalParentGrowth=1.0,ApicalChildGrowth=1.0,ApicalRampBasis=PlantTargetLength,ApicalPriorityGradient=%s,"
            "bApicalUseChildGradient=True,ApicalChildRampBasis=BranchTargetLength,ApicalPriorityChildGradient=%s,Corymb=%.3f),"
            "Phototropism=(Apical=%.3f,ApicalBias=0.4,Axillary=0.15,AxillaryBias=0.7,LightRequirement=0.0),bBranchPhototropismSameAsTrunk=True,"
            "LightSenescence=(SenescenceThreshold=%.3f,SenescenceMin=2,SenescenceMax=6),"
            "GravityParams=(GravitationalForce=%.3f,CellWeight=%.3f,FoliageWeight=0.04,BranchWeightBreakThreshold=0.95,HitGroundBehaviour=Kill,MinGravitationalDot=0.0,TrunkReinforcement=%.2f),"
            "bSenescence=True,AgeSenescence=(Mode=Degrade,SenescenceAge=60),"
            "Bifurcation=(bEnableBifurcation=%s,SplitAngle=%.1f,SplitBias=0.2,SplitMin=2,SplitMax=3,SplitThreshold=0.33,CytokininBuildup=0.5,CytokininRandomness=0.4),"
            "BranchBifurcation=(bEnableBifurcation=%s,SplitAngle=55.0,SplitBias=0.1,SplitMin=2,SplitMax=2,SplitThreshold=0.4,CytokininBuildup=0.45,CytokininRandomness=0.4),"
            "Directional=(ApicalRandomAngle=%.2f,AxillaryRandomAngle=6.0,CodominentRandomAngle=6.0),bBranchDirectionalSameAsTrunk=False,BranchDirectional=(ApicalRandomAngle=7.0,AxillaryRandomAngle=10.0,CodominentRandomAngle=6.0),"
            "Auxin=(AuxinFalloff=0.2,ApicalDominance=%.3f,RadicalAuxin=%.3f),"
            "Foliage=(StartScale=0.5,EndScale=1.0,Density=%d,EthyleneBuildup=0.1,EthyleneThreshold=0.8))") % (
        cycles, params["seed"],
        angle, kind["type"], kind["whorl"][0], kind["whorl"][1],
        max(20.0, angle * 0.75), kind["branch_formation"], kind["flatten"],
        orders, incremental_radius, height, crown_radius, segment, branch_scale,
        0.6 + 1.0 * density, 0.6 + 1.0 * density, axillary,
        apical_ramp, child_ramp, flat,
        0.1 + 0.3 * apical,
        kind["senescence"],
        gravity, cell_weight, 0.5 + 0.5 * apical,
        trunk_bifurcate, min(85.0, angle * 1.1),
        branch_bifurcate,
        1.0 + 2.5 * (1.0 - apical),
        min(0.6, apical * 0.6), min(1.0, crown_base * 1.2),
        max(1, int(round(1 + 3 * params["leaf_density"]))))


# ---------------------------------------------------------------- assets
def ensure_folder(folder):
    if not unreal.EditorAssetLibrary.does_directory_exist(folder):
        unreal.EditorAssetLibrary.make_directory(folder)


def delete_asset(path):
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)


def save(asset):
    unreal.EditorAssetLibrary.save_loaded_asset(asset, False)


def write_preset(folder, name, text):
    """Creates or updates a ProceduralVegetationGrowerPreset data asset holding the grower parameters."""
    path = "%s/%s" % (folder, name)
    preset = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if preset is None:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.ProceduralVegetationGrowerPreset)
        preset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.ProceduralVegetationGrowerPreset, factory)
    growth = unreal.PVGrowerParams()
    growth.import_text(text)
    preset.set_editor_property("growth_params", growth)
    save(preset)
    return preset


def material_instance(folder, name, parent_path, vectors, scalars):
    path = "%s/%s" % (folder, name)
    instance = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if instance is None:
        instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel = unreal.MaterialEditingLibrary
    mel.set_material_instance_parent(instance, unreal.load_asset(parent_path))
    for key, rgb in vectors.items():
        mel.set_material_instance_vector_parameter_value(instance, key, unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    for key, value in scalars.items():
        mel.set_material_instance_scalar_parameter_value(instance, key, float(value))
    mel.update_material_instance(instance)
    save(instance)
    return instance


# ---------------------------------------------------------------- the PVE graph
def touch(settings):
    """Makes PCG notice a settings change.
    Measured on UE 5.9.0: assigning a STRUCT property from Python stores the value but does not mark the
    graph out of date, so the next execute_graph returns the cached result. Changing a plain property does.
    Flip the node's (unused) PCG seed and put it back: the settings checksum is rebuilt with the new struct."""
    seed = settings.get_editor_property("seed")
    settings.set_editor_property("seed", seed + 1)
    settings.set_editor_property("seed", seed)


def apply_grower(nodes, params, crown_scale):
    """Writes the grower parameters: one Grower, or the bare-trunk Grower followed by the crown Grower."""
    if "bole" in nodes:
        set_struct(nodes["bole"].get_settings(), "grower_params", grower_text(params, crown_scale, "bole"))
        set_struct(nodes["grower"].get_settings(), "grower_params", grower_text(params, crown_scale, "crown"))
    else:
        set_struct(nodes["grower"].get_settings(), "grower_params", grower_text(params, crown_scale))


def set_struct(settings, prop, text):
    value = settings.get_editor_property(prop)
    value.import_text(text)
    settings.set_editor_property(prop, value)
    touch(settings)


def build_graph(folder, name, params, leaf_mesh_path, mesh_folder, mesh_name, bark_material_path):
    """A fresh PVE asset: Grower -> Scale -> Mesh Builder -> Foliage Distributor (<- Foliage Palette) -> Export."""
    subsystem = pve()
    delete_asset("%s/%s" % (folder, name))
    graph = subsystem.create_graph(name, folder)
    nodes = {}
    for index, (key, type_name) in enumerate((("grower", "ProceduralVegetationGrower"), ("scale", "ProceduralVegetationScale"),
                                             ("mesher", "ProceduralVegetationMesher"), ("palette", "ProceduralVegetationFoliage"),
                                             ("foliage", "ProceduralVegetationFoliageDistributor"), ("export", "Export"))):
        nodes[key] = subsystem.add_node(graph, type_name, key.capitalize(), "", "", index * 320, 200 if key == "palette" else 0).node
    for source, source_pin, target, target_pin in (("grower", "Out", "scale", "In"), ("scale", "Out", "mesher", "In"),
                                                   ("mesher", "Out", "foliage", "In"), ("palette", "Out", "foliage", "Foliage"),
                                                   ("foliage", "Out", "export", "In")):
        if graph.add_edge(nodes[source], source_pin, nodes[target], target_pin) is None:
            raise RuntimeError("PVE: could not connect %s.%s -> %s.%s" % (source, source_pin, target, target_pin))

    if bole_cycles(params) > 0:
        nodes["bole"] = subsystem.add_node(graph, "ProceduralVegetationGrower", "Bole", "", "", -320, 0).node
        if graph.add_edge(nodes["bole"], "Out", nodes["grower"], "In") is None:
            raise RuntimeError("PVE: could not connect the bole grower to the crown grower")
    apply_grower(nodes, params, 1.0)

    # Cheap trunk and limbs: few sides, long segments, no caps on twigs that foliage hides.
    mesher = nodes["mesher"].get_settings()
    mesher_params = mesher.get_editor_property("mesher_settings")
    details = mesher_params.get_editor_property("mesh_details")
    details.import_text("(SkeletonResolution=14,PointRemoval=0.0,SegmentReduction=0.55,MinDivisions=4,MaxDivisions=9,AddEndCaps=False)")
    mesher_params.set_editor_property("mesh_details", details)
    radius = mesher_params.get_editor_property("branch_radius")
    if int(params["branch_orders"]) <= 1:
        # Palm and tree-fern stems barely taper: hold the tip at 80% of the base.
        radius.import_text("(DaVinciRuleStrength=0.0,MinRadius=0.8,GenerationScales=((Scale=%.4f)))" % trunk_radius_scale(params))
    else:
        radius.import_text("(DaVinciRuleStrength=0.7,MinRadius=0.02,GenerationScales=((Scale=%.4f)))" % trunk_radius_scale(params))
    mesher_params.set_editor_property("branch_radius", radius)
    # One bark material for every branch generation (slot 0 of the exported mesh).
    material_details = mesher_params.get_editor_property("material_details")
    material_details.import_text('(MaterialSetups=((bDeriveFromTrunkTextureSetup=False,Material="%s")))' % bark_material_path)
    mesher_params.set_editor_property("material_details", material_details)
    mesher.set_editor_property("mesher_settings", mesher_params)

    palette = nodes["palette"].get_settings()
    info = unreal.PVFoliageInfo()
    info.import_text('(bUseAsMask=False,Mesh="%s",Attributes=(Height=1.0))' % leaf_mesh_path)
    infos = [info]
    if "bole" in nodes:
        # A mask entry that wins in the lower half of the tree: no foliage on a stray twig along the clear trunk.
        mask = unreal.PVFoliageInfo()
        mask.import_text("(bUseAsMask=True,Attributes=(Height=0.0))")
        infos.append(mask)
        set_struct(nodes["foliage"].get_settings(), "condition_settings", "(bActivateHeight=True)")
    palette.set_editor_property("foliage_infos", infos)
    touch(palette)

    configure_foliage(nodes["foliage"].get_settings(), params, leaf_mesh_path)

    export = nodes["export"].get_settings()
    export_params = export.get_editor_property("export_settings")
    export_params.set_editor_property("mesh_name", mesh_name)
    export_params.set_editor_property("content_browser_folder", unreal.DirectoryPath(path=mesh_folder))
    export_params.set_editor_property("export_mesh_type", unreal.PVExportMeshType.STATIC_MESH)
    export_params.set_editor_property("replace_policy", unreal.PVAssetReplacePolicy.REPLACE)
    export_params.set_editor_property("create_nanite_foliage", False)
    export_params.set_editor_property("nanite_shape_preservation", unreal.NaniteShapePreservation.PRESERVE_AREA)
    export_params.set_editor_property("collision", False)
    export.set_editor_property("export_settings", export_params)
    return graph, nodes


def leaf_nominal_length_cm(leaf_mesh_path):
    """Longest side of the foliage mesh: what one 'leaf length' of the species is scaled against."""
    bounds = unreal.load_asset(leaf_mesh_path).get_bounds()
    return 2.0 * max(bounds.box_extent.x, bounds.box_extent.y, bounds.box_extent.z)


def triangle_budget(height_m):
    """Most triangles one tree may have, by size class: understorey, canopy, emergent."""
    return 8000 if height_m < 12.0 else (15000 if height_m < 30.0 else 25000)


def configure_foliage(settings, params, leaf_mesh_path, density_scale=1.0):
    """Parametric placement: explicit and repeatable, independent of the growth simulation's hormones.
    density_scale below 1 thins the foliage from the inside of the crown outwards (the triangle budget uses it)."""
    nominal = leaf_nominal_length_cm(leaf_mesh_path)
    settings.set_editor_property("mode", unreal.PVDistributionSettingsMode.PARAMETRIC_SETTINGS)
    settings.set_editor_property("random_seed", int(params["seed"]) + 101)
    if int(params["branch_orders"]) <= 1:
        # Rosette: every frond at the very top of the stem, radiating; the count is the species' frond count.
        scale = params["leaf_length_m"] * 100.0 / nominal
        count = max(4, int(params["crown_leaf_count"]))
        text = ("(SpacingSettings=(BranchDensity=%d,RelativeStart=0.955,RelativeEnd=1.0,LimitStartGeneration=False,LimitEndGeneration=False,SpacingBasis=Plant),"
                "PhyllotaxySettings=(ResetPhyllotaxy=False,PhyllotaxyType=Spiral,PhyllotaxyFormation=Octastichous,MinimumNodeBuds=1,MaximumNodeBuds=1,bSingleBudTip=False),"
                "AngleSettings=(Rotation=0.0,AxilAngle=%.1f,RandomizeAxilAngleMinimum=-8.0,RandomizeAxilAngleMaximum=8.0,AxilAngleRampBasis=Plant,AxilAngleRamp=%s),"
                "ScaleSettings=(ScaleRampBasis=Plant,BaseScale=%.4f,BranchScaleImpact=0.0,RandomizeScaleMinimum=0.85,RandomizeScaleMaximum=1.1,ScaleRamp=%s))") % (
            int(round(count * 1.6)), 35.0 + 30.0 * params["branch_droop"], ramp([(0.0, 1.0), (1.0, 1.0)]), scale, ramp([(0.0, 1.0), (1.0, 1.0)]))
    else:
        # Branching trees: leaf sprays on the outer orders only, never on the trunk; bigger sprays on young, small trees.
        # A spray (twig with leaves) grows with the tree so the crown fills at any size, within limits that keep
        # it reading as foliage at eye height; big-leaved species get slightly bigger sprays.
        spray_m = min(3.0, max(0.7, 0.07 * params["height_m"])) * (0.8 + min(0.5, params["leaf_length_m"]))
        scale = spray_m * 100.0 / nominal
        # Sprays per node: big crowns need more to fill, small trees stay cheap.
        buds = 3 if params["height_m"] >= 20.0 else (2 if params["height_m"] >= 10.0 else 1)
        text = ("(SpacingSettings=(BranchDensity=%d,RelativeStart=%.3f,RelativeEnd=1.0,LimitStartGeneration=True,StartGeneration=2,LimitEndGeneration=False,SpacingBasis=Branch),"
                "PhyllotaxySettings=(ResetPhyllotaxy=True,PhyllotaxyType=Spiral,PhyllotaxyFormation=Octastichous,MinimumNodeBuds=%d,MaximumNodeBuds=%d,bSingleBudTip=False),"
                "AngleSettings=(Rotation=0.0,AxilAngle=50.0,RandomizeAxilAngleMinimum=-15.0,RandomizeAxilAngleMaximum=15.0,AxilAngleRampBasis=Branch,AxilAngleRamp=%s),"
                "ScaleSettings=(ScaleRampBasis=Branch,BaseScale=%.4f,BranchScaleImpact=0.0,RandomizeScaleMinimum=0.8,RandomizeScaleMaximum=1.2,ScaleRamp=%s))") % (
            max(2, int(round((18 + 40 * params["leaf_density"]) * density_scale))), 1.0 - 0.75 * density_scale, buds, buds + 1, ramp([(0.0, 1.0), (1.0, 1.0)]), scale, ramp([(0.0, 1.0), (1.0, 0.8)]))
    set_struct(settings, "parametric_settings", text)


# ---------------------------------------------------------------- run, export, measure
async def execute(graph, timeout=180.0):
    result = pve().execute_graph(graph)
    started = time.monotonic()
    while not result.is_complete:
        if time.monotonic() - started > timeout:
            pve().cancel_execution(graph)
            raise RuntimeError("PVE: execution timed out after %.0f s" % timeout)
        await asyncio.sleep(0.05)
    if result.error:
        raise RuntimeError("PVE: execution failed: %s" % result.error)
    messages = ["%s %s: %s" % (m.severity, m.node_name, m.message) for m in result.value.messages]
    if not result.value.succeeded:
        raise RuntimeError("PVE: execution reported errors: %s" % "; ".join(messages))
    return messages


async def export(graph, timeout=600.0):
    result = pve().export(graph, "", True, True)
    started = time.monotonic()
    while not result.is_complete:
        if time.monotonic() - started > timeout:
            pve().cancel_export(graph)
            raise RuntimeError("PVE: export timed out after %.0f s" % timeout)
        await asyncio.sleep(0.1)
    if result.error:
        raise RuntimeError("PVE: export failed: %s" % result.error)
    created, warnings = [], []
    for exported in result.value.nodes:
        if exported.status != "Exported":
            raise RuntimeError("PVE: export node %s: %s %s" % (exported.node_name, exported.status, exported.message))
        created += list(exported.created_assets)
        warnings += list(exported.warnings)
    return created, warnings


def trunk_base_diameter_m(mesh, height_m):
    """Widest horizontal extent of the bark (material slot 0) near the ground: the trunk's base diameter, flare included."""
    description = mesh.get_static_mesh_description(0)
    limit = max(60.0, min(150.0, height_m * 100.0 * 0.06))
    xs, ys = [], []
    groups = description.get_polygon_group_count()
    if groups < 1:
        return 0.0
    for polygon in description.get_polygon_group_polygons(unreal.PolygonGroupID(0)):
        for vertex in description.get_polygon_vertices(polygon):
            position = description.get_vertex_position(vertex)
            if position.z <= limit:
                xs.append(position.x)
                ys.append(position.y)
    if not xs:
        return 0.0
    return round(max(max(xs) - min(xs), max(ys) - min(ys)) / 100.0, 3)


def measure(mesh):
    bounds = mesh.get_bounds()
    nanite = mesh.get_editor_property("nanite_settings")
    return {
        "height_m": round((bounds.origin.z + bounds.box_extent.z) / 100.0, 2),
        "below_ground_m": round(max(0.0, -(bounds.origin.z - bounds.box_extent.z)) / 100.0, 2),
        "crown_width_m": round(2.0 * max(bounds.box_extent.x, bounds.box_extent.y) / 100.0, 2),
        "triangles": mesh.get_num_triangles(0),
        "vertices": mesh.get_num_vertices(0),
        "lods": mesh.get_num_lods(),
        "nanite": bool(nanite.get_editor_property("enabled")),
        "material_slots": [str(slot.material_slot_name) for slot in mesh.static_materials],
    }


async def generate_tree(folder, species_id, params, leaf_mesh_path, materials, variant_tag):
    """Builds the graph and exports the mesh, then corrects it against the botanical targets: the Scale node
    for the height, the side-branch length for the crown width. Up to six passes; every pass is reported."""
    graph_name = "PVE_%s_%s" % (species_id, variant_tag)
    mesh_name = "SM_TDVeg_%s_%s" % (species_id, variant_tag)
    graph, nodes = build_graph(folder, graph_name, params, leaf_mesh_path, folder, mesh_name, materials["bark"].get_path_name())
    report = {"graph": "%s/%s" % (folder, graph_name), "mesh": "%s/%s" % (folder, mesh_name), "target_height_m": round(params["height_m"], 2),
              "target_crown_width_m": round(params["crown_width_m"], 2), "passes": []}
    scale, crown_scale = params["mature_height_m"] / CANONICAL_HEIGHT_M, 1.0
    mesh = None
    for attempt in range(6):
        apply_grower(nodes, params, crown_scale)
        nodes["scale"].get_settings().set_editor_property("scale", scale)
        messages = await execute(graph)
        created, warnings = await export(graph)
        mesh = next((asset for asset in map(unreal.load_asset, created) if isinstance(asset, unreal.StaticMesh)), None)
        if mesh is None:
            raise RuntimeError("PVE: no static mesh among %s" % created)
        measured = measure(mesh)
        report["passes"].append({"scale": round(scale, 4), "crown_scale": round(crown_scale, 4), "height_m": measured["height_m"],
                                 "crown_width_m": measured["crown_width_m"], "messages": messages, "warnings": warnings})
        if measured["height_m"] <= 0.01:
            break
        height_error = abs(measured["height_m"] - params["height_m"]) / params["height_m"]
        crown_error = abs(measured["crown_width_m"] - params["crown_width_m"]) / params["crown_width_m"]
        rosette = int(params["branch_orders"]) <= 1   # a rosette's width is its frond length: nothing to correct
        if height_error <= 0.03 and (rosette or crown_error <= 0.12):
            break
        scale *= params["height_m"] / measured["height_m"]
        if not rosette:
            # Scaling the whole tree also scales the crown, so correct the branch length for what is left.
            expected_crown = measured["crown_width_m"] * params["height_m"] / measured["height_m"]
            # Damped: height and crown corrections feed each other and overshoot when applied in full.
            crown_scale = min(4.0, max(0.25, crown_scale * (params["crown_width_m"] / max(0.1, expected_crown)) ** 0.8))
    async def rerun(note):
        messages = await execute(graph)
        created, warnings = await export(graph)
        result = next(asset for asset in map(unreal.load_asset, created) if isinstance(asset, unreal.StaticMesh))
        now = measure(result)
        report["passes"].append({"scale": round(scale, 4), "crown_scale": round(crown_scale, 4), "height_m": now["height_m"],
                                 "crown_width_m": now["crown_width_m"], "triangles": now["triangles"], "note": note, "messages": messages, "warnings": warnings})
        return result, now

    measured = measure(mesh)
    # The triangle budget: thin the foliage from the inside of the crown until the tree fits its size class.
    budget = triangle_budget(params["height_m"])
    density = 1.0
    while measured["triangles"] > budget and density > 0.25 and int(params["branch_orders"]) > 1:
        density *= 0.6
        configure_foliage(nodes["foliage"].get_settings(), params, leaf_mesh_path, density)
        mesh, measured = await rerun("foliage density x%.2f for the %d triangle budget" % (density, budget))
    # The height has the last word: pure rescales, the grower is not touched again.
    for _ in range(2):
        if measured["height_m"] <= 0.01 or abs(measured["height_m"] - params["height_m"]) / params["height_m"] <= 0.03:
            break
        scale *= params["height_m"] / measured["height_m"]
        nodes["scale"].get_settings().set_editor_property("scale", scale)
        mesh, measured = await rerun("height rescale")
    report["triangle_budget"] = budget
    report["foliage_density_scale"] = round(density, 3)
    save(mesh)
    save(graph.get_outer())
    report.update(measure(mesh))
    report["target_trunk_diameter_m"] = round(params["trunk_diameter_m"], 3)
    try:
        report["trunk_base_diameter_m"] = trunk_base_diameter_m(mesh, report["height_m"])
    except Exception as error:   # a measurement must never lose the generated tree
        report["trunk_base_diameter_m"] = None
        report["trunk_measure_error"] = str(error)[:200]
    report["scale"] = round(scale, 4)
    report["crown_scale"] = round(crown_scale, 4)
    return report
