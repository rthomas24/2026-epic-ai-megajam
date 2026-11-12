# Brief: Cinder ground prop kit (engineering-made head start)

**Purpose and camera.** Dressing for the ground the player walks on in first person (eye height about
154 cm) around the Cinder landing pad. Seen from 1 to 15 m, scattered as instances by gameplay code.
Small pieces only: no hero pieces (outpost, jammer tower, terrain, magma rivers).

**World.** Cinder is a volcanic planet: black basalt, magma glow, cyan and amber crystal spires
(features.md ART-06; palette in the blender-game-asset skill's `art-direction.md`: basalt `#1A1718`,
ash grey `#4A4446`, cyan crystal `#2EE6FF`, amber crystal `#FFB347`, magma `#FF5A1F`).

**Shape language.** Vertical, crystalline, sharp (the opposite of Verdance's rounded, flowing
growth): faceted prisms, fractured blocks, hexagonal columns. Everything is flat shaded on purpose.

**Pieces, size, budget.**

| Asset | Size | Triangles | Signature feature | Collision |
|---|---|---|---|---|
| SM_Cind_Crystal_01 | ~80 cm tall, cyan | 600-2,500 | urchin: a low fan of seven splayed shards | none |
| SM_Cind_Crystal_02 | ~180 cm tall, amber | 600-2,500 | windswept: five flat blades leaning one way | hull |
| SM_Cind_Crystal_03 | ~300 cm tall, cyan | 600-2,500 | twin stepped spires in a narrow V | hull |
| SM_Cind_Crystal_04 | ~500 cm tall, amber | 600-2,500 | monolith: one massive stepped spire with fused buttress crystals | hull |
| SM_Cind_Basalt_01 | ~80 cm across | 500-2,000 | a block split in two | none |
| SM_Cind_Basalt_02 | ~200 cm across | 500-2,000 | a wedge slab tilted up on a footing | hull |
| SM_Cind_Basalt_03 | ~350 cm across | 500-2,000 | a ridge of three leaning fins, faint ember cracks (the only one) | hull |
| SM_Cind_BasaltColumn_01 | ~250 cm tall | up to 2,500 | nine fused hexagonal columns stepping down from a peak | hull |
| SM_Cind_BasaltColumn_02 | ~500 cm tall | up to 2,500 | an organ of fifteen columns with two summits | hull |

**Materials.** One material slot per mesh. Three shared 1024 x 1024 texture sets for the whole kit
(crystals, basalt rocks, columns): base colour, normal (DirectX), packed ORM, emissive as its own
map. 60 / 30 / 10: basalt black dominant, ash grey secondary, crystal and magma glow as the accent.
Crystals: emission strength 1.5 with every facet at its own brightness, hot at the root and dim at
the point, so they glow in a dark scene and the facets still read.

**Technical.** Real scale in cm, +Z up, pivot at the base centre on the ground plane, base sunk
8-30 cm below it. Triangulated FBX export, no LODs, one convex `UCX_` hull where listed.

**Route.** Code-built in Blender 5.2 headless (the human lead asked for these as engineering head-start
props; the skill's concept and blockout approval gates are replaced by this brief and the art
director reviews afterwards). Original designs only; no references to existing games or films.

**Owner and folder.** Engineering (Ryan's side) under the `ArtSource/EngProps/Cinder/**` claim
(`MESSAGEBOARD.md` R-015). The art director (Nick) may restyle or replace any piece.
