# Editor script (run through ue_exec.py with --lib pve_lib.py): generates the trees of one biome file.
#
#   python ue_exec.py --port 8017 --lib pve_lib.py generate_biome.py --args @args.json --timeout 1800
#
# ARGS: {"biome": <contents of biomes/<name>.json>,            (pass the file with tools/run_biome.py)
#        "only": ["Kapok"],            optional: species ids to (re)generate
#        "variants": [0],              optional: indices into each species' "variants" list
#        "foliage_folder": "/Game/TriadDrift/Gameplay/Vegetation/Foliage",
#        "material_folder": "/Game/TriadDrift/Gameplay/Vegetation/Materials"}
#
# Per species it writes, under <content_root>/<SpeciesId>/:
#   PVP_<Id>                 grower preset (ProceduralVegetationGrowerPreset): the mature, seed-of-variant-0 parameters
#   MI_TDVeg_<Id>_Foliage / _Bark   the biome look
#   SM_TDVeg_<Id>_Leaf       the species' copy of a shared leaf mesh, with the species' foliage material
#   PVE_<Id>_<tag>           one PVE graph per variant (open it in the editor to keep tuning by hand)
#   SM_TDVeg_<Id>_<tag>      the exported static mesh (Nanite when the project supports it)
# <tag> is S<stage percent>_<seed>, e.g. S100_11. Returns a report with the normalised parameters,
# what was defaulted, and what was measured.
BIOME = ARGS["biome"]
ONLY = set(ARGS.get("only") or [])
VARIANTS = ARGS.get("variants")
FOLIAGE_FOLDER = ARGS.get("foliage_folder", "/Game/TriadDrift/Gameplay/Vegetation/Foliage")
MATERIAL_FOLDER = ARGS.get("material_folder", "/Game/TriadDrift/Gameplay/Vegetation/Materials")
ROOT = BIOME["content_root"]

report = {"biome": BIOME["biome"], "species": []}
for species in BIOME["species"]:
    species_id = species["id"]
    if ONLY and species_id not in ONLY:
        continue
    folder = "%s/%s" % (ROOT, species_id)
    ensure_folder(folder)
    look = species["look"]
    source_leaf = "%s/SM_TDVeg_Leaf_%s" % (FOLIAGE_FOLDER, look["leaf_mesh"])
    materials = {
        "foliage": material_instance(folder, "MI_TDVeg_%s_Foliage" % species_id, MATERIAL_FOLDER + "/M_TDVeg_Foliage",
                                     {"FoliageColor": look["foliage"], "TipColor": look["foliage_tip"], "GlowColor": look["glow"]},
                                     {"GlowStrength": look.get("glow_strength", 1.0)}),
        "bark": material_instance(folder, "MI_TDVeg_%s_Bark" % species_id, MATERIAL_FOLDER + "/M_TDVeg_Bark",
                                  {"BarkColor": look["bark"], "GlowColor": look["glow"]}, {"RimGlow": look.get("bark_glow", 0.05)}),
    }
    # The species' own copy of the shared leaf mesh, carrying the species' foliage material: PVE keeps the
    # foliage mesh's material on the exported tree, so this is how the look reaches the foliage slot.
    leaf_mesh = "%s/SM_TDVeg_%s_Leaf" % (folder, species_id)
    delete_asset(leaf_mesh)
    leaf_asset = unreal.EditorAssetLibrary.duplicate_asset(source_leaf, leaf_mesh)
    leaf_asset.set_material(0, materials["foliage"])
    save(leaf_asset)
    entry = {"id": species_id, "common_name": species["common_name"], "latin_name": species["latin_name"], "layer": species.get("layer", ""), "variants": []}
    for index, variant in enumerate(species["variants"]):
        if VARIANTS is not None and index not in VARIANTS:
            continue
        params = tree_params(species, variant["stage"], variant["seed"])
        if index == 0:
            write_preset(folder, "PVP_%s" % species_id, grower_text(params))
            entry["archetype"] = params["archetype"]
            entry["archetype_reason"] = params["archetype_reason"]
            entry["defaulted"] = params["defaulted"]
            entry["notes"] = params["notes"]
            entry["preset"] = "%s/PVP_%s" % (folder, species_id)
        tag = "S%03d_%d" % (int(round(variant["stage"] * 100)), variant["seed"])
        result = await generate_tree(folder, species_id, params, leaf_mesh, materials, tag)
        result["stage"] = variant["stage"]
        result["seed"] = variant["seed"]
        result["params"] = {key: (round(value, 3) if isinstance(value, float) else value) for key, value in params.items() if key not in ("defaulted", "notes", "archetype_reason", "common_name", "latin_name")}
        entry["variants"].append(result)
        await asyncio.sleep(0)
    report["species"].append(entry)
report
