# Space set pieces for T6's regions (structures builder, Nov 14 night)

Route C, code-built in the kit's hard-surface style (`scripts/kit_pieces_space.py`, `build_kit.py --set space`): 15 cm
bevels, the kit trim sheets, big shapes for 50 m to 2 km, three LODs (100 / 45 / 18 %), one or two convex hulls each,
static meshes the space field can break. Unreal: `/Game/TriadDrift/Gameplay/Props/Space/SetPieces/` (`ue_import_space.py`),
capture classes `BP_SP_*` (`sites/Space_*.manifest.json`). No names, numbers or dates on any hull.

**Lost patrol (Watchline halo), `SM_SP_Patrol_Bow / _Mid / _Aft / _Wing` (19 / 18 / 16 / 13 m; together about 55 m).**
A Triad patrol corvette torn into four: the wedge bow with its dark canopy band and a ragged back of staggered plates
and frames; the mid hull cut open down one side, its frames standing out at broken heights over an exposed keel deck;
the aft section with its two engine pods and dead nozzles; a sheared-off wing plate. The squad's light hull and teal
trim (`MI_SP_PatrolHull`: the kit trim with heavy scorch grime), so it reads as ours, burnt. `BP_SP_PatrolWreck` shows the
pieces drifting apart.

**Research expedition (Quiet Graveyard), `SM_SP_Research_A` (158 m) and `SM_SP_Research_B` (108 m).**
Older and civilian beside the patrol's facets. A: a lattice spine with four round modules, a ring frame, a blunt command
block with a few lit windows and three engine bells. B: a long box tender with stacked equipment blocks, a lit bridge row,
a broken dish on a mast and two engines. `MI_SP_ResearchHull` (weathered trim), `MI_SP_Window` (warm lit windows).

**Null survey rig (Survey Scar), `SM_SP_NullRig` (37 x 37 x 56 m with the drill string).** A black octagonal drill
head on four clamp legs bitten into a rock, a red glow seam, a ring of white "eye" lights round the head and the drill
string bored into the rock: the enemy's colour code. Pivot = the rock surface. `BP_SP_NullRig` clamps it to
`SM_Asteroid_03` at scale 80.

**Mined-out rock (Survey Scar), `SM_SP_HollowRock` (70 m).** A lumpy asteroid with one face cut flat by the rigs and a
bore driven 26 m into it: the yield taken, the pattern visible. Slots Rock / Cut / Bore (`MI_Asteroid_03`,
`MI_SpaceDebris_03`, `MI_SpaceDebris_01`).

Captures (in game, L_Galaxy, 100 m and 1 km, the 1 km frame cropped 2.7x round the reticle):
`review/game/Space/space_set_pieces_sheet.jpg`.
