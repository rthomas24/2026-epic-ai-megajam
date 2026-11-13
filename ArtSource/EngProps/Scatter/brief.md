# Scatter kit: brief

**Purpose and camera.** Meshes the planet scatter (`ATDTerrainScatter`, T4; `docs/eng/WORLD-DENSITY-PLAN.md` 3.1 and 4.2)
instances by the thousand. Ground cover is seen on foot from 1 to 120 m; boulders and charred trees on foot and from
a ship at 65 m/s; canopy clumps only from 800 m to 4 km, from the air.

**Budget.** About 2.5 MB of Unreal assets for the whole kit, no new 2K texture: one shared 512 px atlas (colour ramps
plus a charred-bark pattern) and the terrain's existing rock textures.

| Group | Meshes | Size | Triangles | Collision | Nanite |
|---|---|---|---|---|---|
| Verdance ground cover | 2 grass, 1 fern, 2 glow flora | 0.4 to 1 m tall, 1 to 2 m patch | 50 to 200 | none | no (two-sided blades) |
| Verdance canopy clumps | 3 | 35 to 85 m across, 30 to 55 m tall | 300 to 800 | none | yes, like the trees |
| Boulders, both planets | 4 | 4 to 12 m | 300 to 900 | one convex hull | yes |
| Cinder charred trees | 2 | 4.5 and 7.5 m | 200 to 500 | one hull on the trunk | no |

**Shape language.** Verdance: rounded, flowing, glowing tips (teal and violet, cyan and magenta glow). Cinder: black,
broken, ember-lit. Boulders: slab-cut facets, not potatoes.

**Not made.** Basalt column clusters: `SM_Cind_BasaltColumn_01/02` already are clusters (2.5 m and 5 m).

**Pivot.** Base centre on the ground, +Z up; every piece is sunk 3 to 60 cm so it never floats on a slope.

**Owner.** Ryan's side (asset builder for the Lead). Sources here, Unreal assets in
`Content/TriadDrift/Gameplay/Props/Scatter/`.
