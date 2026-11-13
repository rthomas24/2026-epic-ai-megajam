# Brief: the Dominion (Null Armada mothership), exterior kit

Engineering-made (route C: code-built from this written design, Blender 5.2 headless), like the Null Fleet.
Nicholas keeps art direction: a `DA_Dominion_Visuals` in `/Game/TriadDrift/Presentation/` always wins over the
engineering one in `/Game/TriadDrift/Gameplay/Characters/Dominion/`.

## Purpose and camera
The finale's enemy capital ship (ENG-18). Seen from 2 km on approach (fills ~15% of a 1080p frame), circled at
200-800 m while the player kills the four shield generators, and flown at 30-80 m inside the dorsal reactor trench.
It must read as a menacing black capital ship from 2 km and as a detailed surface at 50 m.

## Size, pivot, budget (code contract: `ATDDominion`, `HullExtent` = 30000 x 8000 x 5000 cm half-size)
All Unreal cm, actor-relative, +X forward (bow), +Y right, +Z up. Every mesh at scale 1, pivot at the point the
code places it.

| Mesh | Placed at | Envelope (cm) | Tris | Collision |
|---|---|---|---|---|
| `SM_Dominion_Hull` | actor origin | ~60000 x 16000 x 10000 (+ bow jaws, ventral blade and engine bells beyond) | <= 300k, Nanite | the mesh's own UCX hulls (the hull component is BlockAllDynamic; its collision now comes from this mesh). Trench floor stays at Z = 5000 |
| `SM_Dominion_Bridge` | (-19500, 0, 8000) | 7500 x 4000 x 6000 | 10-60k | UCX hulls inside the old stand-in box |
| `SM_Dominion_TrenchWall` | (3000, -2500, 6500), mirrored for +Y | 42000 x 400 x 3000 (inner face = +Y) | 10-60k | UCX hulls = the old wall box (trench cover) |
| `SM_Dominion_ShieldEmitter` | (16500 / -10500, +/-6400, 7000) | 1500 x 1500 x 4000 (the code's cylinder 15 x 15 x 40) | 5-30k | none (component is NoCollision) |

Keep clear: trench floor |Y| < 2300 at Z 5000 from X -18000 to 24000 (reactor sphere 25 m at (16500, 0, 6500),
trench turrets at |Y| 1700, Z 7700); hull turrets on the deck edge at (+/-18000, +/-8000, 5500); the hangar launch
point (33000, 0, -2000) must stay in open space under the bow; generators float at radius 388 m (not on the hull).

## Shape language
"Blade, crescent, fortress". Null Armada family (see `ArtSource/EngShips/NullFleet/review/*_r4/r5_review.png`): matte
black / graphite layered armour, forward-raked plates, red-orange recessed seams, glowing white eye slits, orange-red
engine rings, heavier at the front.

## Design
- **Primary (3):** a long keeled hull (octagonal section, flat dorsal deck, V keel), a heavy armoured bow head that
  ends in a forward-raked chevron prow over the hangar mouth, a wide engine block at the stern.
- **Signature / focal point:** the bow "face" a third of the way in from the front: a heavy brow over a row of huge
  white eye slits, mandible jaws sweeping forward either side of the glowing hangar mouth, fighters launch between them.
- **Secondary:** layered flank armour shingles raked forward with red-orange glowing gaps, a row of white eye lights
  along each upper flank, weapon sponsons, a ventral keel blade, dorsal decks stepping up to the trench walls,
  five engine bells with hot cores, the command tower (bridge) leaning forward at the trench's aft end.
- **Tertiary:** world-aligned panel material (tile 32 m + 256 m macro, procedural texture), trench floor conduits
  and ribs and a glowing centre channel, vents, hatches, antenna spikes.
- **Palette 60/30/10:** graphite panel armour / near-black trim and recesses / red-orange seams + engines and white
  eyes (emissive <= 5% of surface).

## Materials (no UV reliance)
- `M_TD_DominionPanel` (master) + `MI_TD_DominionPanel` (hull armour) + `MI_TD_DominionTrim` (darker metal):
  triplanar in the object's local space, scale-aware (works on the placeholder's scaled cube and on the art meshes),
  two tile scales, world-space normal from a tiling panel normal map, sparse red-orange light strips.
- `M_TD_DominionGlow` + `MI_TD_DominionGlowRed / GlowWhite / GlowEngine`: emissive colour x strength (+ pulse).
- Textures: `T_Dominion_PanelMask` (R tone, G cavity/seam, B height, A light strips) and `T_Dominion_PanelN`,
  2048 tileable.

## Owner and folders
Ryan's side (engineering). Sources `ArtSource/EngShips/Dominion/**`; Unreal `Content/TriadDrift/Gameplay/Props/Dominion/**`
(materials, textures, meshes) and `Content/TriadDrift/Gameplay/Characters/Dominion/DA_Dominion_Visuals`.

## References
Original design. Inspiration only: generic "capital ship" vocabulary (keeled hull, bow hangar, stern engine banks,
dorsal trench); no shapes copied from any film or game.
