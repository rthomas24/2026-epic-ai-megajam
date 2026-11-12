# procedural-trees: facts, what was generated, results

How to use the skill is in [SKILL.md](SKILL.md). This file is the record: what was established before
anything was written, what was generated and with which tool, and what was measured.

## 1. Facts established first (2026-11-12)

| Question | Answer |
|---|---|
| Engine | Unreal Engine **5.9.0**, MegaJam Preview, changelist 60351404 (`C:\Program Files\Epic Games\UE_5.9`, `Engine/Build/Build.version`) |
| Is there a Procedural Vegetation Editor plugin? | **Yes**: `Engine/Plugins/Experimental/ProceduralVegetationEditor`, plugin name **`ProceduralVegetationEditor`**, experimental, modules `ProceduralVegetation` (Runtime) and `ProceduralVegetationEditor` (Editor) |
| Enabled by default? | **No.** Added to `unrealGameJam.uproject` for Editor targets only (the one change to that file) |
| Exposed to Python? | **Yes.** `unreal.ProceduralVegetationEditorSubsystem`: create graph, add node, execute, export, list presets. Node settings and grower presets are editor properties. The engine ships its own Python tests for it (`Content/Python/pve_tests`) |
| Exposed to MCP? | **Yes.** The same 18 functions are the toolset `ProceduralVegetationEditor.ProceduralVegetationEditorSubsystem` |
| Exposed to Blueprint? | The subsystem's functions are not `BlueprintCallable`. Our own matcher is (`UTDVegetationLibrary`) |
| Are the MCP plugin and toolsets enabled here? | **Yes**, 21 toolset plugins for Editor targets. Python runs through `PythonScriptingTools.execute_python`; actors and levels through `EditorToolset.LevelToolset` / `ActorToolset`; captures through `SceneCaptureToolset.Capture`; performance through `PerfToolset.StatsToolset`; mesh facts through `EditorToolset.StaticMeshToolset` |
| What existed for vegetation? | Engine: PVE, the Foliage system, PCG, Nanite (Nanite Foliage off in this project). This codebase: nothing |
| The reference article | x.com/UnrealEngine/status/2119840871168528687 could **not** be fetched (HTTP 402) and no copy was found. The two Epic documentation pages were read. The pipeline follows the brief's description of the workflow |

Detail, the full tool lists and nine measured behaviours: [references/pve-facts.md](references/pve-facts.md).

**Route taken: PVE, for real.** No fallback. Every tree mesh here was grown by PVE's Grower, meshed
by its Mesh Builder, given foliage by its Foliage Distributor and written by its Export node, all
called from Python through the MCP server of a hidden editor on port 8017.

### Choices made in one pass, then questioned

| Choice | Second look |
|---|---|
| Build each graph from scratch instead of copying an Epic sample | Kept. Copies point their export and foliage into engine-plugin content, which a packaged game cannot use while the plugin is editor-only |
| Our own leaf meshes and materials (Geometry Script, no textures) | Kept: original content, opaque triangles are cheap. Cost: leaves are stylised |
| Grow at the botanical size | **Reversed.** The grower is not scale-free (a 60 m tree lost its branches). Trees grow at a canonical 12 m and the Scale node sizes them |
| Grow the species' real trunk girth | **Reversed.** Thin trunks collapsed. Girth is canonical while growing and corrected in the Mesh Builder |
| Clear trunk from the grower's axillary ramp | **Reversed.** Measured not to work. Two Growers in series do it |
| Static meshes, Nanite on, foliage merged | Kept. Nanite Foliage is off in the project; skeletal export would need the DynamicWind plugin at runtime |
| Plugin enabled for Editor targets only | Kept: the game needs only the exported meshes and materials. PVE graphs and presets are editor data |

## 2. What was generated, and with which tool

Everything is original and was made on 2026-11-12, inside the jam. Nothing was downloaded; no Epic
sample asset is referenced by any file here.

| Assets | Tool |
|---|---|
| Tree meshes `SM_TDVeg_<Species>_S<stage>_<seed>`, PVE graphs `PVE_*`, grower presets `PVP_*` (`Content/TriadDrift/Gameplay/Vegetation/Verdance/`) | Unreal Engine 5.9 **Procedural Vegetation Editor**, driven by `scripts/pve_lib.py` through Unreal MCP |
| Leaf meshes `SM_TDVeg_Leaf_{Broad,Palmate,Frond,FernFrond}` (`.../Vegetation/Foliage/`) and the per-species copies | Unreal **Geometry Script** from Python (`scripts/make_leaf_meshes.py`): geometry computed in the script |
| Materials `M_TDVeg_Foliage`, `M_TDVeg_Bark` and the instances | Unreal `MaterialEditingLibrary` from Python (`scripts/make_materials.py`) |
| Demo map `L_VegetationDemo` | Unreal editor scripting through MCP (`scripts/build_demo_map.py`) |
| Scripts, C++ matcher, tests, documents | Claude Code |
| Botanical data | Read from the sources named in `biomes/verdance.json`; no value was generated |

For `CREDITS.md` (the Lead adds the rows): tools Claude Code, Unreal Engine 5.9 Procedural
Vegetation Editor, Geometry Script, Unreal MCP. Data sources to acknowledge: Useful Tropical Plants
Database (tropical.theferns.info) and Wikipedia, with the references they cite.

## 3. The first biome: Verdance

Prompt (abstract): "a bioluminescent alien jungle, giant mushrooms and ferns, mist" (features.md
ART-06). Read as a dense lowland tropical rainforest. Six real Neotropical species were chosen as
**structural analogues**, by layer; their structure is botanical, their colours are the planet's
(deep teal and violet foliage, cyan / magenta / lime glow on the leaf tips), set only through
material instances. Mushrooms are not trees and are not covered.

### Species and archetypes

Sources for every value are in `biomes/verdance.json` (Useful Tropical Plants Database and Wikipedia,
each with the reference it cites: Jensen 1999 (FAO), Huxley 1992, Little & Wadsworth 1964 (USDA Forest
Service), Burns & Honkala 1990, Henderson, Galeano & Bernal 1995, Gribel et al. 1999, Carder 2005,
Coley 1986, Berg et al. 2005, PROTA). Five values are marked low confidence there (an uncited or
trade-sourced sentence: leaf type and leaf length of the Brazil nut and the cacao, and the Brazil
nut's branching habit); three clear-bole fractions are "derived" by the fixed table in
`references/botanical-data.md`.

| Species | Latin name | Layer | Archetype | Why | Defaulted from the archetype |
|---|---|---|---|---|---|
| Kapok | *Ceiba pentandra* | emergent | BROAD_SPREADING | spreading crown shape | ClearBoleFraction |
| Brazil nut | *Bertholletia excelsa* | emergent | ROUND_OVAL | round or oval crown shape | CrownWidthM |
| Trumpet tree | *Cecropia peltata* | mid canopy | BROAD_SPREADING | spreading crown shape | CrownWidthM |
| Walking palm (cashapona) | *Socratea exorrhiza* | mid canopy | PALM | frond leaves, a rosette crown or an unbranched stem | CrownWidthM, ClearBoleFraction |
| West Indian tree fern | *Cyathea arborea* | understorey | PALM | frond leaves, a rosette crown or an unbranched stem | CrownWidthM, ClearBoleFraction, CrownLeafCount |
| Cacao | *Theobroma cacao* | understorey | ROUND_OVAL | round or oval crown shape | CrownWidthM |

| Mesh | Stage | Seed | Height m (target) | Mannequins tall | Crown m (target) | Trunk base m (target dbh) | Triangles | Nanite | LODs | Passes |
|---|---|---|---|---|---|---|---|---|---|---|
| `SM_TDVeg_Kapok_S100_11` | 1.00 | 11 | 60.02 (61.80) | 33.3 | 58.57 (58.40) | 1.58 (1.90) | 4649 | yes | 1 | 3 |
| `SM_TDVeg_Kapok_S070_12` | 0.70 | 12 | 48.61 (49.34) | 26.9 | 36.33 (39.60) | 1.30 (1.36) | 2313 | yes | 1 | 5 |
| `SM_TDVeg_Kapok_S030_13` | 0.30 | 13 | 29.82 (29.83) | 16.5 | 23.20 (23.84) | 0.74 (0.65) | 1821 | yes | 1 | 2 |
| `SM_TDVeg_BrazilNut_S100_21` | 1.00 | 21 | 39.12 (38.16) | 21.7 | 25.13 (25.55) | 1.15 (1.45) | 4506 | yes | 1 | 5 |
| `SM_TDVeg_BrazilNut_S070_22` | 0.70 | 22 | 30.44 (30.48) | 16.9 | 16.03 (17.34) | 2.50 (1.04) | 4973 | yes | 1 | 3 |
| `SM_TDVeg_BrazilNut_S030_23` | 0.30 | 23 | 18.44 (18.43) | 10.2 | 9.81 (10.42) | 0.54 (0.49) | 1950 | yes | 1 | 4 |
| `SM_TDVeg_TrumpetTree_S100_31` | 1.00 | 31 | 15.55 (15.27) | 8.6 | 14.32 (14.88) | 0.15 (0.20) | 5574 | yes | 1 | 5 |
| `SM_TDVeg_TrumpetTree_S070_32` | 0.70 | 32 | 12.14 (12.19) | 6.7 | 9.73 (10.10) | 0.10 (0.14) | 2085 | yes | 1 | 8 |
| `SM_TDVeg_TrumpetTree_S030_33` | 0.30 | 33 | 7.42 (7.37) | 4.1 | 5.91 (6.07) | 0.07 (0.07) | 3002 | yes | 1 | 5 |
| `SM_TDVeg_StiltPalm_S100_41` | 1.00 | 41 | 15.71 (15.54) | 8.7 | 3.57 (5.78) | 0.13 (0.15) | 1074 | yes | 1 | 2 |
| `SM_TDVeg_StiltPalm_S070_42` | 0.70 | 42 | 12.56 (12.42) | 7.0 | 3.08 (3.93) | 0.12 (0.11) | 1020 | yes | 1 | 2 |
| `SM_TDVeg_StiltPalm_S030_43` | 0.30 | 43 | 7.65 (7.51) | 4.2 | 2.58 (2.36) | 0.05 (0.05) | 765 | yes | 1 | 2 |
| `SM_TDVeg_TreeFern_S100_51` | 1.00 | 51 | 9.14 (9.05) | 5.1 | 6.26 (3.18) | 0.09 (0.10) | 1833 | yes | 1 | 3 |
| `SM_TDVeg_TreeFern_S070_52` | 0.70 | 52 | 7.37 (7.23) | 4.1 | 5.59 (2.16) | 0.05 (0.07) | 1717 | yes | 1 | 3 |
| `SM_TDVeg_TreeFern_S030_53` | 0.30 | 53 | 4.47 (4.37) | 2.5 | 4.73 (1.30) | 0.02 (0.04) | 1393 | yes | 1 | 3 |
| `SM_TDVeg_Cacao_S100_61` | 1.00 | 61 | 8.59 (8.56) | 4.8 | 6.02 (5.30) | 0.59 (0.26) | 5864 | yes | 1 | 11 |
| `SM_TDVeg_Cacao_S070_62` | 0.70 | 62 | 6.03 (5.94) | 3.3 | 4.01 (3.61) | 1.67 (0.18) | 5655 | yes | 1 | 5 |
| `SM_TDVeg_Cacao_S030_63` | 0.30 | 63 | 3.64 (3.60) | 2.0 | 2.33 (1.77) | 0.53 (0.09) | 5104 | yes | 1 | 8 |

18 meshes, 55298 triangles in all, 3072 on average.
Outside tolerance (height 3%, crown 12%): `SM_TDVeg_StiltPalm_S100_41` (height +1%, crown -38%), `SM_TDVeg_StiltPalm_S070_42` (height +1%, crown -22%), `SM_TDVeg_TreeFern_S100_51` (height +1%, crown +97%), `SM_TDVeg_TreeFern_S070_52` (height +2%, crown +159%), `SM_TDVeg_TreeFern_S030_53` (height +2%, crown +264%), `SM_TDVeg_Cacao_S100_61` (height +0%, crown +14%), `SM_TDVeg_Cacao_S030_63` (height +1%, crown +32%)

Measured on 2026-11-12 from the exported meshes (`biomes/verdance.report.json`). The mannequin is the
template `SKM_Manny_Simple`, 180.5 cm; the player's eye is at 154 cm.

**Reading the table honestly**

- **Height:** all 18 meshes are within 3% of the botanical target.
- **Crown width:** the palm and the tree fern are "outside tolerance" because their targets are the
  archetype's default ratio (no source gives a crown width), while the meshes use the documented
  frond length (1.75 m and 3 m), which is the better data. The young cacao is 32% too wide.
- **Trunk:** the "trunk base" column is the widest bark extent near the ground. It is close to the
  target dbh for clear-trunked trees and **not reliable** for low-branching ones (cacao, the
  half-grown Brazil nut), where low limbs are inside the measured band.
- **Performance:** 765 to 5,864 triangles a tree, 3,072 on average; every mesh is Nanite with one
  bark slot and one foliage slot and no discrete LODs (Nanite replaces them). The forest patch on the
  demo map is 518 hierarchical instances of these 18 meshes. Budgets (8,000 / 15,000 / 25,000 by size
  class) are enforced by the generator. **Frame time was not measured**: `GetFrameTimings` on the
  hidden editor returned an idle editor's numbers (GPU 0.2 ms), which say nothing about the scene.
- **Not modelled:** kapok buttresses and spines, the stilt roots of the palm and the trumpet tree.

### Captures (looked at by the agent that made them)

In `references/shots/` (copies of `Saved/TDShots/vegetation/`): `01_lineup`, `02_eye_understorey`,
`03_eye_emergent`, `04_ship_approach`, `05_forest_floor`, `06_nanite_triangles`.

What they show: the six species read as different forms at the right relative sizes; clear trunks on
the kapok and the Brazil nut with crowns starting where the data says; fronds in a terminal crown on
the palm and the tree fern; teal and violet foliage with cyan, magenta and lime tips. What is weak:
leaves are flat four-triangle blades and read as stylised shards close up; the mature trumpet tree's
crown sags lower than a real *Cecropia*'s; palm and fern fronds glow along most of their length
rather than at the tips; some bare twigs remain on clear trunks; crowns of the giants are thin when
seen from directly below.

### Stage reached (`AAA.md`)

**Technically verified**: generated by the real tool, measured, tested, looked at in offscreen
captures on a demo map. **Not** presentation approved (the art director's call), **not** integrated
(nothing references these assets; `L_Planet_Verdance` is untouched) and **not** gameplay validated
(never seen in the running game, at gameplay speed, under the planet's real lighting).

## 4. Tests and build

`TriadDrift.Vegetation.*`, seven automation tests in
`Source/unrealGameJam/TriadDrift/Vegetation/Tests/TDVegetationTests.cpp`: the archetype table is
sane, the match rules pick the expected archetype (18 cases), the six Verdance species match, missing
data falls back and is reported, inconsistent data is rejected, growth stage and seed vary
deterministically, and 20,000 calls take under a second.

Run on 2026-11-12 in this worktree, after the last asset was generated:

- `Tools	d-build.ps1`: BUILD SUCCEEDED, errors=0 warnings=0.
- `Tools	d-test.ps1 -Filter TriadDrift.Vegetation`: 7 passed, 0 failed.

The whole-project test suite and a packaged build were not run by this task.
