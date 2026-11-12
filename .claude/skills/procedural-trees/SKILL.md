---
name: procedural-trees
description: Makes believable background and mid-ground trees for Triad Drift from one prompt, using Unreal Engine 5.9's Procedural Vegetation Editor (PVE) driven through Unreal's built-in MCP server. From a setting ("alien jungle for a crash site", "trees for an 1870s Kentucky town", "the 10 most common trees in a Baltic forest") it picks species, looks up sourced botanical data, maps each species to one of seven crown archetypes with a tested C++ function, writes a parametric PVE grower preset and graph per species with growth-stage and seed variation, exports Nanite static meshes, and verifies scale against the mannequin, triangle counts and captures. Use whenever someone asks for trees, a forest, jungle, woodland, palms, tree ferns, vegetation or foliage for a level, wants more variations or a different growth stage of an existing species, wants a new biome's flora, or asks why a generated tree is the wrong size or shape. Not for hero trees, grass, flowers, mushrooms, rocks or anything made in Blender.
compatibility: Unreal Engine 5.9.0 (MegaJam Preview, CL 60351404) with the ProceduralVegetationEditor plugin enabled for Editor targets; Windows; Python 3 for the shell-side drivers. Everything editor-side runs through the MCP tool execute_python.
---

# Procedural trees (PVE through Unreal MCP)

**Route in use: the real one.** PVE exists in this engine build, is enabled in
`unrealGameJam.uproject` (Editor targets only) and is fully drivable from Python and MCP. No Blender
fallback is used or needed. The facts behind that sentence, and nine measured behaviours the scripts
depend on, are in [references/pve-facts.md](references/pve-facts.md). Read it before changing
`scripts/pve_lib.py`.

## When to use it, and when not

Use it for trees and tree-like plants that fill a level: canopy, mid-ground, understorey, anything
instanced by the hundred. It is built to be fast and repeatable, not to make one perfect tree.

Do not use it for: a hero tree the camera rests on (hand-tune the exported PVE graph, or model it);
grass, flowers, vines, mushrooms, rocks (see `blender-game-asset`); wind animation (the meshes are
static); anything in Nick's folders. Output goes only to `Content/TriadDrift/Gameplay/Vegetation/`.

## Inputs and outputs

| | |
|---|---|
| **Input** | One prompt, abstract or explicit, plus the biome's look (colours, glow) |
| **You write** | `biomes/<name>.json`: the inquiry, species, sourced data, look, variants ([references/botanical-data.md](references/botanical-data.md)) |
| **C++ gives** | `FTDTreeParams` per species, stage and seed: archetype, normalised parameters, the fields that were defaulted ([references/archetypes.md](references/archetypes.md)) |
| **Editor writes** | Under `<content_root>/<SpeciesId>/`: `PVP_<Id>` (PVE grower preset), `PVE_<Id>_S<stage>_<seed>` (PVE graph, one per variant), `SM_TDVeg_<Id>_S<stage>_<seed>` (static mesh), `MI_TDVeg_<Id>_Foliage` / `_Bark`, `SM_TDVeg_<Id>_Leaf` |
| **Shell writes** | `biomes/<name>.report.json` (targets, measurements, every calibration pass), `<name>.layout.json` (where each tree stands on the demo map) |

## The pipeline

### 1. Inquiry
Work out the setting and choose about six species (one or two per forest layer), common and Latin
names. For a fictional or alien setting choose real species whose forms fit, as **structural
analogues**, and say so in the file. Rules: [references/botanical-data.md](references/botanical-data.md).

### 2. Data
Look up height, crown shape and width, trunk diameter, branching habit, leaf type and size,
evergreen or deciduous. Record the sentence and its reference for every value. Leave out what you
cannot find: the matcher fills it and says so. Never invent a value.

### 3. Archetype matching (C++)
`UTDVegetationLibrary::BuildTreeParams(Species, GrowthStage, Seed)` in
`Source/unrealGameJam/TriadDrift/Vegetation/`. Deterministic, Blueprint-pure, no editor dependency.

| Archetype | Chosen when | Behaviour |
|---|---|---|
| ConicalConifer | conical crown, or needle / scale leaves | one leader, whorled near-horizontal limbs |
| Columnar | columnar crown, or crown under 0.25 of the height | one leader, steep ascending limbs |
| BroadSpreading | spreading / umbrella / vase crown, or crown at least 0.8 of the height | leader forks into heavy limbs above a clear trunk, flat top |
| RoundOval | round or oval crown; also the fallback | rounded crown on a trunk |
| Weeping | weeping crown or pendulous branches | limbs sag under heavy gravity |
| Palm | fronds, a rosette, or an unbranched stem | bare stem, fronds in one terminal crown |
| MultiStemShrub | multi-stem habit, or 4 m or less with no habit given | low forking, dense, no clear trunk |

Missing or inconsistent metrics fall back to the archetype's defaults and are listed in
`DefaultedFields` / `Notes`. Tests: `Tools\td-test.ps1 -Filter TriadDrift.Vegetation` (7 tests).

### 4. Output (PVE)
One command per biome; it runs steps 3 and 4 for every species and variant:

```powershell
powershell -File Tools\td-build.ps1                                        # the editor needs this worktree's module
powershell -File Tools\td-play-mcp.ps1 -Editor -Port 8017 -Map /Engine/Maps/Entry   # your own port (AGENTS.md section 11)
cd .claude\skills\procedural-trees
python scripts\ue_exec.py --port 8017 scripts\make_materials.py            # once: M_TDVeg_Foliage, M_TDVeg_Bark
python scripts\ue_exec.py --port 8017 scripts\make_leaf_meshes.py          # once: the four shared leaf meshes
python scripts\run_biome.py --port 8017 biomes\verdance.json               # all species, all variants
python scripts\run_biome.py --port 8017 biomes\verdance.json --only Kapok --variants 0   # one tree again
```

If a call answers "the Unreal Editor is running a slow task", the editor is still loading: wait and
retry. What happens inside, per variant (`scripts/pve_lib.py`):

1. The species entry becomes `unreal.TDTreeSpeciesData`; `unreal.TDVegetationLibrary.build_tree_params`
   returns the normalised parameters.
2. `grower_text()` turns them into `FPVGrowerParams` text. A new PVE graph is built with
   `ProceduralVegetationEditorSubsystem.create_graph` / `add_node` and the PCG graph's `add_edge`:
   `[Bole Grower ->] Grower -> Scale -> Mesh Builder -> Foliage Distributor (<- Foliage Palette) -> Export`.
3. `execute_graph`, then `export` (static mesh, Nanite, foliage merged, no collision).
4. The mesh is measured. Height is corrected with the Scale node and crown width with the
   side-branch length, up to six passes. Then the foliage is thinned until the tree fits its
   triangle budget, and the height is rescaled once more. Every pass is in the report.
   (Verdance, 18 trees: about six minutes.)
5. The mature variant's parameters are saved as the species' grower preset `PVP_<Id>`.

To get **more variations** of a species, add `{"stage": s, "seed": n}` entries to its `variants` and
run again with `--only <Id>`. Stage changes size, proportions, branch orders and growth cycles; the
seed changes the branching itself.

### 5. Verification
```powershell
python scripts\run_demo_map.py --port 8017 biomes\verdance.report.json --forest Kapok=6 BrazilNut=12 TrumpetTree=60 StiltPalm=80 TreeFern=200 Cacao=160
python scripts\capture.py --port 8017 --out Saved\TDShots\veg\lineup.png --location -11000 6500 1200 --look-at 0 6500 2200
python scripts\capture.py --port 8017 --out Saved\TDShots\veg\eye.png --location -2500 9000 154 --look-at 0 9500 500      # eye height 154 cm
python scripts\capture.py --port 8017 --out Saved\TDShots\veg\nanite.png --mode VisualizeNanite --sub-mode Triangles --location ... --look-at ...
python ..\..\..\Tools\td_mcp.py --port 8017 call PerfToolset.StatsToolset GetFrameTimings
powershell -File ..\..\..\Tools\td-play-mcp.ps1 -Stop -Port 8017
```

The demo map `L_VegetationDemo` (not World Partition: one `.umap`, nothing in `__ExternalActors__`)
holds a line-up of every variant, the template mannequin (180.5 cm) at the origin, and a forest patch
of hierarchical instanced static meshes. Then check, and write down:

- **Scale:** each tree's measured height against its target in the report (within 3%) and against
  the mannequin in the capture. Open the PNG and look.
- **Structure:** does each species read as its archetype: clear trunk where the data says so, crown
  where it should start, fronds at the top of a palm?
- **Performance:** triangles per mesh from the report, Nanite on, one bark slot and one foliage slot.
  Budgets used for Verdance: understorey under 8,000 triangles, canopy under 15,000, emergent under
  25,000. Frame timings from a hidden editor are indicative only; say so.
- **Look:** glow is an accent on leaf tips, not the whole leaf; colours match the biome brief.

Captures are offscreen renders with no temporal history. They prove scale, silhouette and materials.
They do not prove the trees look good in the game: that needs the real map, the real lighting and a
person. An agent's work here reaches "technically verified" at most (`AAA.md`).

## Tuning without guessing

```powershell
python scripts\run_experiment.py --port 8017 biomes\verdance.json BrazilNut --seed 21 --grower "base=()" "short=(TrunkGrowth=(BranchTargetLength=1.0))" --keep short
python scripts\ue_exec.py --port 8017 scripts\place_probe.py --args "{\"mesh\": \"/Game/TriadDrift/Gameplay/Vegetation/Verdance/_Experiment/SM_Experiment\", \"x\": -6000}"
python scripts\capture.py --port 8017 --out probe.png --location -11000 0 800 --look-at -6000 0 1900
```

One run prints height, crown width and triangles for each override. This is how every measured
behaviour in `pve-facts.md` was found. Delete `_Experiment` when done (`--keep` leaves it).

## How to add a new archetype

1. **C++** (`TDVegetationTypes.h`, `TDVegetationLibrary.cpp`): add the enum value before `Count`,
   one row in `GArchetypeTable`, and the rule in `MatchTreeArchetype` at the right place in the
   order. Add any new input the rule needs to `FTDTreeSpeciesData` (and to `_ENUMS` / `_NUMBERS` in
   `pve_lib.py`).
2. **Tests** (`Vegetation/Tests/TDVegetationTests.cpp`): a species that must match it, one that must
   not, and the table sanity loop picks the row up by itself. Build and run the tests.
3. **PVE** (`pve_lib.py`): one entry in `PVE_ARCHETYPE` (trunk phyllotaxy, which axes may fork, light
   pruning). If the form needs something the normalised parameters do not carry, add the field to
   `FTDTreeParams` rather than special-casing the archetype name in Python.
4. **Look at it:** `run_experiment.py` on a species of that archetype, `place_probe.py`, a capture.
   Do not ship an archetype nobody has looked at.
5. Update the tables in `references/archetypes.md` and above.

## Known limits

- Background and mid-ground vegetation. Leaves are flat four-triangle blades on shared sprays; at
  arm's length they read as stylised, not photographic.
- Only BroadSpreading, RoundOval and Palm have been generated and looked at. ConicalConifer,
  Columnar, Weeping and MultiStemShrub are matched and tested in C++ but their PVE settings are a
  first estimate, and `stem_count` is not used yet (a shrub is one low-forking stem).
- Not modelled: buttresses, stilt roots, spines, flowers, fruit, bark texture, wind.
- Static meshes with foliage merged in. Nanite Foliage and Nanite Assemblies are off in this
  project; turning them on is a project-settings decision for the Lead.
- The crown-base rule uses two Growers; a few stray twigs can remain on the clear trunk (their
  foliage is masked off).
- PVE is experimental: Epic says assets from one engine version may not open in the next. The biome
  file and the scripts are the source of truth; the `.uasset` files can always be regenerated.
- The plugin is enabled for Editor targets only. Exported meshes and materials do not need it at
  runtime; PVE graphs and presets are editor data and must not be referenced by game assets.

## Files

| Path | What |
|---|---|
| `README.md` | Facts established, what was generated and with which tool, results |
| `biomes/verdance.json`, `.report.json`, `.layout.json` | The first biome: data, measurements, map layout |
| `references/pve-facts.md` | Engine, plugin, API, MCP tools, measured behaviours |
| `references/archetypes.md` | The seven archetypes, rules, defaults, growth model, PVE mapping |
| `references/botanical-data.md` | How to do inquiry and data honestly |
| `scripts/ue_exec.py` | Run a Python file in the editor over MCP |
| `scripts/pve_lib.py` | Editor-side library: matcher call, grower text, graph, export, calibration |
| `scripts/run_biome.py` + `generate_biome.py` | Generate a biome |
| `scripts/run_demo_map.py` + `build_demo_map.py` | Build the verification map |
| `scripts/capture.py` | Render a PNG from a camera |
| `scripts/run_experiment.py` + `experiment.py`, `place_probe.py` | Measure what a setting does |
| `scripts/make_leaf_meshes.py`, `make_materials.py` | Shared leaf meshes and master materials |
