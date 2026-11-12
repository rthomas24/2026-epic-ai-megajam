# Inquiry and botanical data: how to fill a biome file

A biome file (`biomes/<name>.json`) is the record of steps 1 and 2. `biomes/verdance.json` is the
worked example. Nothing in it may be made up: a reader must be able to trace every number to a
sentence in a source.

## Step 1: inquiry

Write down, in the file's `inquiry` block:

- **setting**: what the prompt means as a real-world vegetation type (climate, forest structure,
  layers). For an explicit prompt ("the 10 most common trees in a Baltic forest") this is the place
  and its forest type.
- **method**: `"documented"` when the species are the ones recorded for that place and time, or
  `"structural analogue"` when the setting is fictional or alien. For an analogue, say so in plain
  words: real species were chosen because their FORMS fit; the look is changed with materials only.
- **not_covered**: what the prompt asks for that a tree generator cannot make.

Choose species to cover the forest's layers (emergent, canopy, sub-canopy, understorey) and as many
different archetypes as the setting honestly has, not the ten most famous trees. For "most common"
prompts use an inventory or flora for the region (national forest inventories, a regional flora),
and record it as the source of the species list.

## Step 2: data

For each species record `common_name`, `latin_name`, `layer`, then under `data` any of:

| Key | Meaning | Matcher field |
|---|---|---|
| `height_m` | typical mature height | HeightM |
| `crown_width_m` | mature crown spread | CrownWidthM |
| `trunk_diameter_m` | diameter at breast height | TrunkDiameterM |
| `clear_bole_fraction` | 0..1, branch-free share of the height | ClearBoleFraction |
| `leaf_length_m` | one leaf, or one frond | LeafLengthM |
| `crown_leaf_count` | rosette forms: fronds in the crown | CrownLeafCount |
| `crown_shape` | Conical, Columnar, Spreading, Round, Oval, Weeping, Rosette, Irregular | CrownShape |
| `branching` | Excurrent, Decurrent, Unbranched, MultiStem, Pendulous | Branching |
| `leaf_type` | Broadleaf, CompoundBroadleaf, Needle, Scale, Frond | LeafType |
| `persistence` | Evergreen, Deciduous | Persistence |

Each entry is `{"value": ..., "source": "<where>: '<the sentence>' (<the reference that source cites>)"}`.

### Sources, in order of preference

1. Floras and forestry references: Kew Plants of the World Online, USDA Forest Service *Silvics of
   North America* and species leaflets, FAO and World Agroforestry (ICRAF) species profiles, PROTA,
   regional floras, monographs.
2. Curated databases that cite such references for each statement: Useful Tropical Plants
   (tropical.theferns.info), Plants For A Future, the Gymnosperm Database, Palmpedia.
3. Wikipedia, only for a sentence that carries a citation; name that citation. An uncited sentence
   may be used for a non-critical field if it is marked "low confidence"; never for height or habit.

Never use a number from memory, from a game or film wiki, or from a nursery catalogue.

### Rules that keep the data honest

- **A range** ("25 - 56 metres"): use the midpoint, keep the range in `"range"`.
- **A deliberate extreme**: when the brief needs the largest documented size (emergent giants),
  use it and mark `"basis": "upper"` with `"why"`.
- **Two sources disagree**: keep the one with the better citation and say what the other said.
- **Qualitative statements** never become invented numbers, with one exception that is a fixed
  table, not a judgement. Mark such a value `"derived"`:

  | The source says | `clear_bole_fraction` |
  |---|---|
  | "unbranched until near the top", "branches arising high on the trunk", "long clear bole" | 0.6 |
  | "short bole", "low-branching", "branches near the ground" | 0.15 |

- **Not found**: leave the key out and list it in `not_found`. The matcher fills it from the
  archetype and reports it. Do not put a guess in.
- **Known traits the generator cannot build** (buttresses, stilt roots, spines, flowers): list them
  in `not_modelled` so nobody assumes they are there.

### The look

`look` belongs to the biome, not to botany: `leaf_mesh` (`Broad`, `Palmate`, `Frond`, `FernFrond`),
`foliage`, `foliage_tip`, `glow`, `bark` (linear RGB), `glow_strength`, `bark_glow`. It becomes two
material instances per species. Changing it never changes the tree's structure.

### Variants

`variants` is a list of `{"stage": 0..1, "seed": int}`. One mesh is exported per entry. Three per
species (mature, half-grown, young) with different seeds is enough for a forest that does not look
stamped; add seeds, not species, when it does.
