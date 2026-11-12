# Brief: Verdance ground prop kit (engineering-made head start)

**Purpose and camera.** Dressing for the ground the player walks on in first person (eye height about
154 cm) around the Verdance landing pad. Seen from 1 to 15 m, scattered as instances by gameplay code.
Small pieces only: no trees (another agent), no hero pieces (wreck, relay spire, terrain).

**World.** Verdance is a bioluminescent alien jungle at dusk-like light: deep teal and violet foliage,
glowing cyan, magenta and lime accents (features.md ART-06; palette in the blender-game-asset skill's
`art-direction.md`: teal `#19C3B1`, violet `#7B3FE4`, deep blue-green `#0F3B3A`, bio-glow `#3CFFE0`).

**Shape language.** Organic, rounded, flowing (the opposite of Cinder's sharp vertical crystals):
curved stems, scalloped caps, spirals, arching fronds, worn boulders.

**Pieces, size, budget.**

| Asset | Size | Triangles | Signature feature | Collision |
|---|---|---|---|---|
| SM_Verd_Mushroom_01 | ~120 cm tall | 600-2,500 | flat dished "shelf" cap, cyan spots and rim, two small companions | hull, whole shape |
| SM_Verd_Mushroom_02 | ~250 cm tall | 600-2,500 | leaning stem, deep bell cap with magenta gills, drooping skirt | hull, stem only |
| SM_Verd_Mushroom_03 | ~450 cm tall | 600-2,500 | three stacked umbrella tiers, cyan gill ridges, hanging lime lures | hull, trunk only |
| SM_Verd_Fern_01 | ~80 cm tall | 300-1,200 | fountain of barbed fronds, glowing tips | none |
| SM_Verd_Fern_02 | ~160 cm tall | 300-1,200 | fiddleheads wound into glowing magenta spirals | none |
| SM_Verd_GlowPlant_01 | 40-70 cm | 300-1,200 | lantern bulbs hanging from arching stalks | none |
| SM_Verd_GlowPlant_02 | 40-70 cm | 300-1,200 | corkscrew stem beaded with lime pods | none |
| SM_Verd_Rock_01 | ~60 cm across | 500-2,000 | worn chiselled boulder + pebbles, moss cap | none |
| SM_Verd_Rock_02 | ~150 cm across | 500-2,000 | stacked flat stones | hull |
| SM_Verd_Rock_03 | ~300 cm across | 500-2,000 | leaning tooth with buttress stones, glowing lichen (the only rock that glows) | hull |

**Materials.** One material slot per mesh. Three shared 1024 x 1024 texture sets for the whole kit
(mushrooms, flora, rocks): base colour, normal (DirectX), packed ORM, emissive as its own map.
60 / 30 / 10: teal dominant, violet secondary, glow accents. Emission strength 2.0 with the
brightness carried by the emissive map, kept below white-out so the form still reads.

**Technical.** Real scale in cm, +Z up, pivot at the base centre on the ground plane, base sunk
5-15 cm below it. Real geometry for leaves (closed blades, no alpha cards, no masked material).
Triangulated FBX export, no LODs, one convex `UCX_` hull where listed.

**Route.** Code-built in Blender 5.2 headless (the human lead asked for these as engineering head-start
props; the skill's concept and blockout approval gates are replaced by this brief and the art
director reviews afterwards). Original designs only; no references to existing games or films.

**Owner and folder.** Engineering (Ryan's side) under the `ArtSource/EngProps/Verdance/**` claim
(`MESSAGEBOARD.md` R-015). The art director (Nick) may restyle or replace any piece.
