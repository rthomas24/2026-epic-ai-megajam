# Crown archetypes

The matcher is C++: `UTDVegetationLibrary` in `Source/unrealGameJam/TriadDrift/Vegetation/`
(`MatchTreeArchetype`, `GetTreeArchetypeDefaults`, `BuildTreeParams`; Blueprint-pure, runtime-safe,
about a microsecond a call). The table below is the hard-coded behaviour in
`TDVegetationLibrary.cpp` (`GArchetypeTable`); change the two together.

## Why these seven

Field guides and forestry manuals describe crowns with a short, shared vocabulary: pyramidal /
conical, columnar / fastigiate, spreading / vase / umbrella, round, oval, weeping; palms and shrubs
are treated as habits of their own. Each archetype is one of those words, and each needs grower
behaviour the others cannot fake: a leader or none, where the crown starts, ascending or hanging
limbs, one stem or several, a rosette instead of branches. Vase, umbrella, pagoda and flat-topped
fold into BroadSpreading; pyramidal into ConicalConifer; round and oval share RoundOval; anything
irregular or unknown falls back to RoundOval, the commonest broadleaf form. The "about 80% of
species" coverage is a judgement from that vocabulary, not a measured figure.

Not covered: lianas and climbers, bamboo, cacti and other succulents, mangrove prop-root forms,
strangler figs, cushion and prostrate shrubs, and any tree whose identity is in its trunk (baobab,
banyan) rather than its crown.

## The table

| Archetype | Typical species | Height m | Crown / height | Trunk / height | Crown base | Apical dominance | Branch angle | Droop | Branch density | Orders | Flatness | Stems | Leaf m | Fronds |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| ConicalConifer | spruce, fir | 25 | 0.30 | 0.025 | 0.15 | 0.95 | 80 | 0.25 | 0.90 | 3 | 0.0 | 1 | 0.20 | - |
| Columnar | Lombardy poplar, Italian cypress | 18 | 0.18 | 0.025 | 0.10 | 0.85 | 20 | 0.00 | 0.80 | 3 | 0.0 | 1 | 0.10 | - |
| BroadSpreading | oak, kapok, acacia | 22 | 1.00 | 0.045 | 0.45 | 0.25 | 65 | 0.20 | 0.60 | 4 | 0.7 | 1 | 0.15 | - |
| RoundOval | maple, beech, most broadleaf trees | 18 | 0.65 | 0.035 | 0.35 | 0.45 | 50 | 0.15 | 0.75 | 4 | 0.2 | 1 | 0.12 | - |
| Weeping | weeping willow, weeping birch | 12 | 0.80 | 0.040 | 0.30 | 0.35 | 60 | 0.90 | 0.80 | 4 | 0.3 | 1 | 0.10 | - |
| Palm | palms, tree ferns, cycads | 12 | 0.35 | 0.020 | 0.90 | 1.00 | 60 | 0.50 | 0.00 | 1 | 0.0 | 1 | 2.50 | 12 |
| MultiStemShrub | hazel, elder | 3 | 1.00 | 0.020 | 0.05 | 0.10 | 40 | 0.10 | 0.90 | 3 | 0.3 | 5 | 0.08 | - |

Height is used only when the species has none. Crown / height and trunk / height give the crown
width and trunk diameter when the species has none. Branch angle is from the trunk axis (0 = up,
90 = horizontal). Orders: 1 = stem only.

## Matching rules (first that fires wins; the result names the rule)

1. Frond leaves, a rosette crown, or an unbranched stem -> **Palm**
2. Multi-stem habit -> **MultiStemShrub**
3. Weeping crown or pendulous branches -> **Weeping**
4. Stated crown shape: columnar -> **Columnar**; conical -> **ConicalConifer**; spreading ->
   **BroadSpreading**; round or oval -> **RoundOval**
5. No shape stated (unknown or irregular):
   needle or scale leaves and not decurrent -> **ConicalConifer**;
   crown width under 0.25 of the height -> **Columnar**;
   height 4 m or less and no habit given -> **MultiStemShrub**;
   crown width at least 0.8 of the height -> **BroadSpreading**;
   any other known proportion -> **RoundOval**
6. Nothing usable -> **RoundOval**, reason "fallback"

## Missing and inconsistent data

A metric that is zero, negative, NaN or infinite, or an enum that is `Unknown`, is missing: the
archetype value is used and the field's name is added to `DefaultedFields`. A value that is present
but contradicts the rest is rejected the same way, with a sentence in `Notes`:

| Check | Limit |
|---|---|
| Height | above 130 m (taller than any tree on record: a unit error) |
| Crown width | more than 3 x the height |
| Trunk diameter | more than half the height |
| Clear-bole fraction | above 1 (and never more than 0.95 is used) |
| Leaf length | longer than the tree is tall |

## Growth stage and seed

`BuildTreeParams(Species, GrowthStage, Seed)`. Stage 0 is a sapling, 1 is mature.

| Quantity | At stage s |
|---|---|
| Height | mature x (0.12 + 0.88 x s^0.75) |
| Crown width | mature x height fraction x (0.7 + 0.3 s): young crowns are narrower |
| Trunk diameter | mature x height fraction^1.5: young trunks are more slender |
| Crown base | x (0.35 + 0.65 s): young trees keep their low branches |
| Apical dominance | from at least 0.7 as a sapling to the archetype value |
| Branch orders | one fewer below 0.6, two fewer below 0.25, never under 2 for a branching form |
| Droop, flatness, leaf length, frond count | reduced when young |

The seed (an `FRandomStream`) moves height by up to 7%, crown width 10%, trunk diameter 8%, branch
angle 5 degrees, branch density 0.08, droop 0.05 and crown base 0.04. Same species, stage and seed
always give the same numbers; the seed is also the PVE grower's random seed, so it changes the
branching itself.

## How the normalised numbers reach PVE (`scripts/pve_lib.py`)

| Normalised parameter | PVE setting |
|---|---|
| height | everything is grown at a canonical 12 m (mature) and the Scale node brings it to size; corrected by measuring |
| crown width | Grower `TrunkGrowth.BranchTargetLength` (crown radius) and `BranchScale`; corrected by measuring |
| trunk diameter | Mesh Builder `branch_radius.GenerationScales` (growth uses a fixed `IncrementalRadius`) |
| crown base | a first "Bole" Grower: `crown_base x cycles` cycles of bare trunk; the main Grower resumes from it |
| apical dominance | `ApicalPriorityGradient`, `Auxin.ApicalDominance`, trunk bifurcation on below 0.6, `TrunkReinforcement` |
| branch angle | `Phyllotaxy.AxilAngle`, branch `AxilAngle` x 0.75, bifurcation `SplitAngle` |
| droop | `GravityParams.GravitationalForce`, `CellWeight` |
| branch density | `AxillaryParentGrowth`, `AxillaryChildGrowth` |
| branch orders | `TrunkGrowth.MaxGeneration`; 1 = bare stem, fronds placed at the tip by the Foliage Distributor |
| crown flatness | `TrunkGrowth.Corymb` |
| growth stage | `GrowthCycles` (10 for a sapling to 40 when mature) and the sizes above |
| seed | `RandomSeed`, and the Foliage Distributor's seed |
| leaf length, leaf density, frond count | Foliage Distributor parametric settings: spray scale, `BranchDensity` |
| archetype | `PVE_ARCHETYPE` in `pve_lib.py`: trunk phyllotaxy type (whorled for conifers), which axes may fork, light pruning |

`stem_count` is produced by the matcher but **not yet used** by the PVE writer: a MultiStemShrub is
generated as one low-forking stem. See "How to add a new archetype" in SKILL.md for where it goes.

Only BroadSpreading, RoundOval and Palm have been generated and looked at (the Verdance biome). The
other four have matcher rules, defaults and tests, and PVE settings that are a first estimate: run
`scripts/run_experiment.py` and look at a capture before trusting them.
