# Kit: the planet-outpost modular kit, and the walkable sites built from it

Ryan, Nov 13 evening, after watching the walk test on Grok Bot's Rodin relay: the AI meshes are blobby and the
pilot sinks into them; "we need to use something else to polish these up and make them sharp and walkable on and
in." **Rule since then: nothing the player walks on, into or through is an AI mesh.** The Rodin relay and wreck stay
as dressing (sunk under the kit floors, their own collision only on parts out of reach); the Rodin outpost cabin is
no longer used (history: `docs/eng/ART-HISTORY.md`).

Route C (code-built), original work, engineering-made (Environment moved to Ryan's side on Nov 13). Brief:
[`brief.md`](brief.md). Built on the DominionInterior helper library (forked, not shared: `scripts/kit_lib.py`).

## Pieces (`/Game/TriadDrift/Gameplay/Props/Kit/`)

Unreal cm, +X forward, pivot on the walkable plane (Z = 0) at the centre of the footprint, every size on the 50 cm
grid. Bounds are exact (bevels that poke out at T-junctions are clamped back onto the authored box, so pieces meet
with no gap or overlap). Slots: `Trim` (MI_Kit_Trim), `Glow` (MI_Kit_Glow_Verdance teal by default, MI_Kit_Glow_Cinder
ember), `Screen` (MI_Kit_Screen). Collision is authored: walkable tops equal the visual tops exactly.

| Mesh | Bounds min .. max (cm) | Tris | Hulls | Notes |
|---|---|---|---|---|
| `SM_Kit_Floor_400` | (-200,-200,-20) .. (200,200,0) | 108 | 1 | top at 0; four 200 cm plates, walk-lane lines |
| `SM_Kit_Platform_400` | (-200,-200,0) .. (200,200,100) | 236 | 1 | riser block, top 100; hazard nosing flush with the top |
| `SM_Kit_Platform_400x200` | (-200,-200,0) .. (200,200,200) | 236 | 1 | the same, top 200 |
| `SM_Kit_Wall_400` | (-15,-200,0) .. (15,200,300) | 800 | 1 | wall along Y, 20 thick, kick plates, rib, light strip at 236..244, cap |
| `SM_Kit_Wall_400_Window` | (-16,-200,0) .. (16,200,300) | 1384 | 1 | window Y -120..120, Z 110..230, dark glass; the glass blocks the pawn |
| `SM_Kit_DoorFrame_400` | (-19.5,-200,0) .. (19.5,200,300) | 2222 | 5 | **clear opening 160 x 260** (Y -80..80, Z 0..260), collision too; hazard jambs, light bar |
| `SM_Kit_Ramp_400` | (-200,-100,0) .. (200,100,210) | 1604 | 3 | rises 100 toward +X (14.04 deg), 200 wide, rails 110 both sides (188 clear) |
| `SM_Kit_Stairs_400` | (-200,-108,0) .. (200,108,220) | 2720 | 3 | 8 level treads of 50 cm, 12.5 cm risers, rise 100 toward +X; ramp collision through the middle of each tread (feet within 6.25 cm), a 25 cm lip before the first riser; rails end 40 cm short of the top |
| `SM_Kit_Catwalk_400` | (-200,-100,-45) .. (200,100,110) | 2412 | 3 | deck top 0, grating, beams under, rails 110 both sides, toe boards |
| `SM_Kit_Railing_400` | (-200,0,0) .. (200,6,110) | 864 | 1 | edge railing on +Y of its line (put it on the inside of an edge) |
| `SM_Kit_Cover_200` | (-100,-21,0) .. (100,21,110) | 248 | 1 | waist-high cover: 110 high, 40 deep, chamfered top |
| `SM_Kit_Platform_400x50` | (-200,-200,0) .. (200,200,50) | 142 | 1 | low deck tile, top 50, hazard nosing |
| `SM_Kit_RampLow_400` | (-100,-200,0) .. (100,200,50) | 80 | 1 | approach onto a 50 deck: 400 wide, 200 run, 14 deg, no rails |
| `SM_Kit_BeaconMast` | (-60,-60,0) .. (60,60,1660) | 1036 | 2 | 16 m light mast: glow strips (Glow) and a lamp head (`Beacon` slot: MI_Kit_Beacon_Verdance / _Cinder, strength 60, slow pulse) |
| `SM_Kit_Pillar` | (-25,-25,0) .. (25,25,300) | 348 | 1 | |
| `SM_Kit_Corner` | (-20,-20,0) .. (20,20,300) | 438 | 1 | octagonal post for an outside corner; its light strip faces +X +Y |
| `SM_Kit_Roof_400` | (-200,-200,296) .. (200,200,320) | 316 | 1 | place at the SAME transform as the floor below; two light strips under it |
| `SM_Kit_Console` | (-30,-83,0) .. (44,83,106.6) | 916 | 1 | screen slopes toward +X; the operator stands at +X (keep 150 clear). The glass has 0..1 UVs and the `Screen` slot uses `MI_Kit_Screen` of **`Materials/M_Kit_Screen`**: `ScreenTexture` (default `T_Kit_ScreenDefault`), `ScreenPower` (3), `ScreenTint` (white) for T2's scan images (#62) |
| `SM_Kit_LightStrip_200` | (-100,-5,0) .. (100,5,6) | 174 | 0 | surface light, no collision |

Glow strength (r4): the strips are at 4 (8 flooded the outpost room), the Light instance at 10.

Edge finish: every edge sharper than 30 degrees bevelled 1.5 cm in 2 segments, hardened normals, weighted normals.
Trim sheet: `textures/T_Kit_Trim_{BC,N,ORM}.png` (2K, procedural, `scripts/kit_atlas.py`; layout `scripts/kit_regions.py`).

## Sites built from the kit (`sites/*.manifest.json`, actor classes rebuilt by `scripts/ue_make_sites.py`)

The class names T5 places are kept (each class is emptied of its mesh components and refilled, never deleted).
Every manifest has the pieces with site-relative transforms, the interact point, the walk path and the placement
rules; it is the placement contract for T5.

| Site | Class | What | Interact point (site cm) |
|---|---|---|---|
| Verdance relay defence (Ryan, Nov 13 evening: a 60-90 s data-transfer defence, three waves) | `Props/Relay/BP_Relay`, `BP_Relay_Awake` | 1600 x 1600 deck, top Z 100, three ramps (+X, +Y, -Y), railings on the other edges, six low covers, the device console in front of the ring; the Rodin relay sunk 50 cm as dressing (ring, cross and pylons still block) | device: stand (190, 0, 100) facing yaw 180; console at (80, 0, 100) |
| Verdance wreck walk-through | `Props/Wreck/BP_Wreck` | a raised walkway (top Z 200, 400 wide) straight through the ruptured midsection along Y, kit walls inside the hull, a console on the forward wall, ramps down both sides to Y +-1600; the Rodin hull sunk 60 cm | core / log: stand (0, 0, 200) facing yaw 0; console at (220, 0, 200) |
| Survey outpost (Verdance teal / Cinder ember) | `Props/Outpost/BP_Outpost_Site_Verdance`, `_Cinder` | one 800 x 400 room, floor flush with the ground, doorway 160 x 260 on +X, windows both sides, roof at 300, console on the back wall | console: stand (-250, 0, 0) facing yaw 180 |
| Cinder dig-site cabin | `Props/Kit/Sites/BP_DigSite_Cabin` (new) | raised deck 1200 x 1200 (top Z 200), cabin 400 x 800 at the back, wrap-around walkway with railings, one 800 cm stair flight on +X | console: stand (-430, 0, 200) facing yaw 180 |

**Moved interact points (tell T5):** relay wake / device point was (150, -2, 135), now (190, 0, 100); wreck core
point was (75, 0, 252), now (0, 0, 200); outpost console point was (76, 81.5, 153) yaw 180, now (-250, 0, 0) yaw 180.

### Relay defence arena (r4, to T5's needs of Nov 13 night)

`BP_Relay` / `BP_Relay_Awake`: a flat 2800 x 2800 deck (`SM_Kit_Platform_400x50`, top 50), low ramps on the west (two),
north and south; the device console 4 m in from the west edge (console (-1000, 0, 50), the pilot stands at
(-890, 0, 50) facing yaw 180, 10.9 m of open floor east of him); five 110 cm covers, the nearest 8.8 m from the stand
point; the Rodin relay on a solid plinth at the east side (X 200..1400, Y -800..800, top 300 = 250 above the deck, out
of the 180 cm mantle) with the beacon mast at its south-east corner; a suggested dormant-drone spot (700, 1100, 50).
Walked (r4b, `-game`, Verdance): up the west, north and south ramps onto the deck (feet 52 = deck 50 + 2), walking into
the plinth stops at its face (no mantle at 250), every ramp lane clear (two covers that sat on the north / south lanes
were moved off them).
Manifest: `sites/Relay_Site.manifest.json` (`device_interact`, `arena_cm`).

## Dominion set (`/Game/TriadDrift/Gameplay/Props/Kit/Dominion/`, `scripts/kit_pieces_d.py`, `--set dominion`)

The walkable shell of the Dominion interior rebuilt with the kit's construction, in the Dominion palette
(`T_KitD_Trim_*`: gloss black armour, dark gunmetal, red / orange marks; `MI_KitD_Trim`, `MI_KitD_Glow` red,
`MI_KitD_Light` white). **Each piece keeps the name, size, pivot and opening of the DominionInterior piece it replaces**
(`../DominionInterior/README.md`), so T5's course places them unchanged once its folder string points at
`/Game/TriadDrift/Gameplay/Props/Kit/Dominion/` (`KitMesh` and the door's `DoorMesh` in `TDDominionInteriorCourse.inl`).

| Mesh | Bounds (cm) | Notes |
|---|---|---|
| `SM_DI_Room_Floor` | (-200,-200,-20) .. (200,200,0) | |
| `SM_DI_Room_Ceiling` | (-200,-200,596) .. (200,200,620) | at the floor's transform; white work light at 596..600 |
| `SM_DI_Room_Wall` | (-30,-200,0) .. (30,200,600) | same both faces; red strips at 236 and 536 |
| `SM_DI_Corridor_Straight` | (-200,-200,-20) .. (200,200,620) | inner faces at +-170, portal rib at X 0, 4 hulls |
| `SM_DI_Doorway` | (-32,-200,0) .. (32,200,600) | clear opening exactly 300 x 400, 3 hulls |
| `SM_DI_Door` | (-16,-150,0) .. (16,150,400) | slides +Z by 400 |
| `SM_DI_Cover_Low` | (-30.8,-100,0) .. (30.8,100,110) | |
| `SM_DI_Hangar_Pad` | (-600,-600,-40) .. (600,600,0.3) | collision top exactly 0 |

The reactor pieces (core, rings, shield, conduit) are not walked on and stay the DominionInterior ones.
`BP_KitD_InteriorPreview` (walk test only) lays the set out with T5's own constants (hangar, deflect corridor, arena,
links, Warden room, core chamber).

### Docking bay interior (#54)

`BP_DockingBay_Interior`: the pocket behind the bow bay mouth, 6000 deep x 5000 x 3500, the Dominion set at scale 2.5
(flown into, not walked), a 24 m landing pad near the back. Pivot = the centre of the bay mouth; attach it to the hull
at hull-local (hull max X, 0, -2000) (`sites/DockingBay_Interior.manifest.json`).

## Walk tests (r4, `-game`, real pawn, transient spawns; captures `review/game/<Site>/walk_sheet.jpg`)

- **Cinder** (`TD.Stage CinderOnFoot`): `BP_Outpost_Site_Cinder` door -> room; `BP_DigSite_Cabin` stairs (feet 40 -> 61
  -> 129 -> 199 -> 202), door, console, side walkway to the back rail (held at -560). Placement note: the spots used
  had course props (a beacon ring, a pillar) and crystals inside the footprints; T5 places on a cleared spot.
- **Dominion preview** (`BP_KitD_InteriorPreview`, floated over Cinder): walked from the hangar (X -1100) through all
  six doorways, the 40 m deflect corridor, the arena, both links and the Warden room into the core chamber to its far
  wall (X 14736 = 14800 - 30 - 34), feet at 0 the whole way.
- **Docking bay** (`BP_DockingBay_Interior`): stood in it for scale (floor = ground); captures from outside the mouth
  and from the back. Seen on Cinder's terrain only, so terrain crystals poke into the captures; the hull is not in them.
