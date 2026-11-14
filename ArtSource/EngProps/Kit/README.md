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
| `SM_Kit_Cover_200` | (-100,-21,0) .. (100,21,100) | 248 | 1 | low cover: 100 high, 40 deep, chamfered top |
| `SM_Kit_Pillar` | (-25,-25,0) .. (25,25,300) | 348 | 1 | |
| `SM_Kit_Corner` | (-20,-20,0) .. (20,20,300) | 438 | 1 | octagonal post for an outside corner; its light strip faces +X +Y |
| `SM_Kit_Roof_400` | (-200,-200,296) .. (200,200,320) | 316 | 1 | place at the SAME transform as the floor below; two light strips under it |
| `SM_Kit_Console` | (-30,-83,0) .. (44,83,106.6) | 916 | 1 | screen slopes toward +X; the operator stands at +X (keep 150 clear) |
| `SM_Kit_LightStrip_200` | (-100,-5,0) .. (100,5,6) | 174 | 0 | surface light, no collision |

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
