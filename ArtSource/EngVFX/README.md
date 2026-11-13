# EngVFX: engineering-built Niagara effects (features.md ENG-37, board message R-026)

Everything here is original and generated from text: one procedural noise texture (numpy), nine unlit materials whose
shapes are pure shader maths (no sprite sheets, so nothing can look low-resolution), and Niagara systems assembled from
engine template emitters by a spec script through Unreal 5.9's Niagara toolset.

| Path | What |
|---|---|
| `scripts/gen_textures.py` | writes `textures/T_TD_VFX_Noise.png` (run with Blender's bundled Python: needs numpy only) |
| `scripts/build_materials.py` | texture import + `M_TD_VFX_*` materials and instances (run in the editor, not during PIE) |
| `scripts/tdvfx.py` | the builder library (emitter spec -> Niagara toolset calls) |
| `scripts/build_tier1.py` | the tier-1 specs: tracers, muzzle flashes, engines, impacts, explosions. **Source of truth for the systems** |
| `scripts/run_editor_py.py`, `g.sh`, `cap.py`, `game_spawn.py`, `sheet_tier1.sh` | run a script in the editor session; PIE preview spawning and captures |
| `review/tier1_rN_*.png` | review sheets per round (kept) |

Rebuild: start the editor session (`Tools/td-play-mcp.ps1 -Editor -Port 8017`), then
`python ArtSource/EngVFX/scripts/run_editor_py.py 8017 ArtSource/EngVFX/scripts/build_materials.py` and
`... build_tier1.py`. Systems are rebuilt in place; commit between rounds to keep the previous round.

## Conventions
- Unreal assets: `/Game/TriadDrift/Gameplay/VFX/{Textures,Materials,Systems}`. Systems `NS_TD_<Thing>[_<Variant>]`.
- +X is forward. Tracers attach to the projectile root; muzzle flashes and impacts are spawned at a point with +X along
  the shot direction / the surface normal; engines attach at the nozzle and blow along -X.
- User parameters on every system: `Color` (linear colour) and `Scale` (float, multiplies every size and speed).
  Engines also have `Throttle` and `Boost` (0..1). Gameplay code sets no Niagara parameters today, so each colour is a
  separate system with the colour as the default; the parameters are there for code to drive later.
- Materials: `Intensity` scalar on each (global brightness trim for the game's fixed exposure).

## Materials
| Material | Blend | Shape |
|---|---|---|
| `M_TD_VFX_Glow` | additive | soft halo with a hot core, zero at the quad edge |
| `M_TD_VFX_Fire` | additive | noise-eroded flame puff: distorted panning noise, heat ramp, erodes with particle age |
| `M_TD_VFX_Smoke` | translucent | noise-eroded haze puff |
| `M_TD_VFX_Spark` | additive | thin streak for velocity-aligned sprites, white-hot centre line |
| `M_TD_VFX_Ribbon` (+ `_Clean`) | additive | trail profile with panning plasma noise (`Wisp` 0..1) |
| `M_TD_VFX_Ring` | additive | shockwave ring broken up by noise |
| `M_TD_VFX_Flare` | additive | star flash with random spikes |
| `M_TD_VFX_Shield` | additive | expanding energy ripple with cell noise |
| `M_TD_VFX_Jet` (+ `_Rev`) | additive | teardrop flame with noise flowing along it; wide end at the leading end of the velocity (`_Rev`: trailing end) |

## Round 3 (Nov 13)
What changed and why, for whoever continues:
- **The player sees his own shots and his own engines end-on** (chase camera). Round 2 was reviewed only from the side;
  in play its tracers (velocity-aligned sprites and a ribbon) collapsed to a hairline. Every effect that matters from
  behind now has a **mesh with volume** or a camera-facing sprite. `rig3.py` shoots each sheet from the side *and* from
  behind; `seq_wired.sh` + `play.py` capture the real game.
- **Meshes** (`scripts/gen_meshes.py` -> `meshes/*.fbx` -> `scripts/import_meshes.py`, Nanite off):
  `SM_TD_VFX_Bolt` (spindle), `SM_TD_VFX_Flame` (flame body, nozzle at the origin, tip at +X), `SM_TD_VFX_Disc`
  (quad with normal +X), `SM_TD_VFX_Shard_A/B/C` (debris).
- **New materials**: `M_TD_VFX_Bolt`, `M_TD_VFX_Flame` (+ `_Boost` with shock diamonds), `M_TD_VFX_Fireball` (black-body
  ramp over life), `M_TD_VFX_SmokeHot`, `M_TD_VFX_Shock`, `M_TD_VFX_Debris` (opaque, cooling edges), `M_TD_VFX_Hex`.
  Mesh materials get `Fres` (0 facing the camera, 1 at the silhouette) from the absolute facing, so mirrored meshes work.
- **Grok Bot's textures** (AI-generated, accepted by the Lead; sources, prompts and licence in git under
  `ArtSource/EngAI/VFX/<Kind>/r1/`): imported as `T_TD_VFX_Grok_Explosion_8x8`, `_Smoke_8x8`, `_Fire_8x8`,
  `_ShockwaveRing`. Used by `M_TD_VFX_FlipExplosion` (8x8 one-shot, frame-blended), `M_TD_VFX_FlipSmoke` (8x8 loop,
  frame-blended) and `M_TD_VFX_FlipRing`. The Fire flipbook is imported but not used yet.
- **Rebuild**: `bash scripts/rebuild_all.sh [mats] A B C`. It restarts the editor between batches: the Niagara toolset
  grows the editor by about 2 GB per rebuilt system (27 GB after seven systems), so never build everything in one editor.
- **Engines** read `User.Throttle` (0..1, default 0.6) and `User.Boost` (0/1) every frame.
- **Known trap**: a system holding the mesh bolt, the ribbon and a `single` emitter with an added `ScaleSpriteSize`
  module compiled clean but never activated (`is_active()` false right after spawn). Removing that emitter fixed it; the
  cause was not found. After building, always spawn each system once and check it is active (`probe` in the notes below).
- Calling the editor's Python from Git Bash: never put Python triple quotes in a heredoc, write the file instead.
- **Second pass (Grok r2 textures)**: `T_TD_VFX_Grok_EngineExhaust_8x8` wrapped round the flame mesh (`M_TD_VFX_Flame`),
  `_PlasmaBolt` as the head-to-tail profile of the bolt mesh (`M_TD_VFX_Bolt`), `_ShieldHexRipple_8x8`
  (`M_TD_VFX_FlipShield`, shield impact), `_DebrisChunks_4x4` (`M_TD_VFX_FlipDebris`, explosion scrap).
  `_ElectricArc_8x8` and `_Fire_8x8` are imported and unused (EMP, tier 2).
- **Tracers scale with camera distance** (`Engine.Owner.LODDistance`, 0.5x near to 9x far) so a shot stays readable far
  away and does not fill the screen when it passes the camera.
- **Wiring** (`scripts/wire_tier1.py`): on by default for the weapon FX assets in `Gameplay/Characters/FX` (tracer,
  impacts, detonation; `WIRE_MUZZLE`, `WIRE_SHIELD` add the muzzle and the shield impact). The ship visuals
  (`Characters/Squad`, `Characters/NullFleet`: ship-death Explosion, HullHit, ShieldHit, EngineTrail, BoostLoop) are behind
  `WIRE_SHIPS` / `WIRE_ENGINES` and were NOT written by the ENG-37 builder (outside its paths). `SAVE = False` wires in
  memory only, for a Play-In-Editor preview. Stop any game using the assets first, or the saves fail.
- Sheets: `review/tier1_r3_*.png` (side and behind views), `review/wired_r2_before_after.png`,
  `review/wired_r3_before_after.png` (real game captures), `review/wire_sound_check.txt`.

## Round 4 (Nov 13)
- **Shield impact is small** (`NS_TD_Impact_Shield`): ring 0.4 m to 2.5 m radius in 0.3 s at half brightness, a tinted
  procedural hexagon ripple (`M_TD_VFX_Hex`; Grok's white hex flipbook is no longer used here), a small glow, 7 sparks.
  `NEAR` (`Engine.Owner.LODDistance / 1700`, 0.28..1) shrinks a hit that lands close to the camera (a wingman beside the
  chase camera, which sits 17 m behind the ship, FOV 99).
- **Ship-level hit cues are their own light systems**: `NS_TD_Ship_HullHit` (sparks, flecks) and `NS_TD_Ship_ShieldHit`
  (glow, motes). An enemy ship plays its own `HullHit` / `ShieldHit` cue in addition to the weapon's impact cue at the
  same point and with no rotation, so wiring the full impact there doubled it.
- **Tracers**: white-hot core (bolt driven over 1, white core sprite), saturated sheath, longer trail; the enemy bolt is a
  short round ball (230 x 125) where the squad's are long needles. **Trap found in play:** a pooled projectile only
  `Deactivate()`s its tracer on release, and the persistent (`single`) particles then hung in space where the shot ended
  and filled the screen when the ship flew through them. Every `single` emitter now has Life Cycle Mode `Self` and
  Inactive Response `Kill` (the input is hidden until the life cycle mode is `Self`).
- **Wiring** (`wire_tier1.py` with `WIRE_SHIPS`, `WIRE_SHIELD`, `WIRE_ENGINES`, `WIRE_MUZZLE`): squad and Null fleet ship
  visuals (Explosion, HullHit, ShieldHit; EngineTrail for the squad only), carrier `BayDestroyed` / `TurretDestroyed`,
  `DA_DominionTurret_Visuals`, `DA_Dominion_Visuals` (GeneratorExplosion, ReactorPhaseChange, ReactorDestroyed).
  `BoostLoop` is left empty: the engine system holds the boost flame and reads `User.Throttle` / `User.Boost`.
- **Explosions**: 26:1 spark streaks, warm grey smoke, Large secondaries over 1.5 s. `NS_TD_Explosion_HullSection`
  (50 m radius) and `NS_TD_Explosion_FinalBlast` (500 m radius) are six-emitter sprite-only variants for the Dominion
  break-up (not wired: their fields arrive with a later code commit).
- **Game captures**: `Saved/EngVFX/burst.sh`-style loops (slow motion 0.05, capture, resume) through `play.py`.
  `TD.World.KillNearest` kills the NEAREST enemy, which is rarely the one just spawned: check the log line.
