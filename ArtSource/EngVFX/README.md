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

## Round 5 (Nov 13)
- **A cue's `VFXScale` did nothing to sprites or world-space particles** (only local-space mesh particles follow the
  component's scale), so the carrier's 2.5x explosion and every scaled Dominion break-up burst came out at 1x. Every size,
  radius and speed of a sprite / world-space emitter now multiplies by `SCL = User.Scale * Engine.Owner.Scale.x`
  (`build_tier1.py`). Local-space mesh particles keep `User.Scale` (the component already scales them).
- **Wiring** (`scripts/wire_r5.py`; Sound fields read before and after, plus `scripts/sound_check.py <git rev>` which
  compares the audio asset paths stored in every data asset against a git revision): enemy `EngineTrail` =
  `NS_TD_Engine_Enemy` on all five Null fleet ships (VFXScale 1 / 1 / 2 gunship / 1.5 bomber / 5 carrier),
  `HullSectionExplosion`, `BreakupFinalBlast`, carrier `Explosion` VFXScale 2.5, Dominion `ShieldIdle` / `ShieldCollapse`.
- **Explosions** are now procedural in the main: three layers of noise-eroded black-body fire sprites (`Core`, `Fireball`,
  `Licks`), Grok's flipbook kept as one dim layer (0.42), real spark trails, three kinds of tumbling mesh debris,
  rim-lit smoke (`M_TD_VFX_SmokeRim`), no painted scrap sprites.
- **Real spark trails**: the toolset's `AddEmitter` accepts an emitter that lives INSIDE a template system, by sub-object
  path (`...Templates/Systems/AttributeReaderTrails.AttributeReaderTrails:Leaders_0` and `:Followers_0`). `Followers`
  spawns particles along every particle of the emitter named `Leaders` (keep that name). `spark_trails()` in
  `build_tier1.py`. Trap: leaders spawn at the exact centre, so velocity mode `From Point` gives them no direction; use
  `In Cone` with a 360 degree angle.
- **Tracers**: a dark translucent body under the bolt (`M_TD_VFX_BoltShadow`, `SortOrderHint` -10) and a white needle
  core, so a magenta shot reads against the magenta nebula.
- **Ship `HullHit` / `ShieldHit`** are now the ram / beam / splash / melee look (T4, fe7ca8f: a projectile hit plays only
  the weapon's impact cue): a directional spray along +X.
- **Memory**: one 18-emitter explosion rebuild takes the editor from 4 GB to 25 GB (4.5 minutes). One explosion per editor
  session, always; nothing else in that session.
- **Tier 2 and story effects**: `scripts/build_tier2.py` (run through `rebuild_all.sh T2:<prefixes>`; it reuses the tier 1
  helpers). Each system's intended slot is in the comment above it. New materials: `M_TD_VFX_Field` (distortion shell on
  the engine sphere), `M_TD_VFX_FlipArc` (Grok's electric arc), `M_TD_VFX_Glitch`, `M_TD_VFX_Swirl`.
- **Game captures**: `TD.Dev.KillCentral <Id>` kills what is in front of the camera (set `TD.Dev.CaptureOnDeath 0` to keep
  the player's camera); frame rate from the log's frame counter between two `TD.World.DominionStatus` lines.
- **Later in round 5**: a system whose emitters reference sub-objects of the engine's template system builds but does
  NOT save (and `save_loaded_asset` failed silently; `tdvfx` now logs `SAVE FAILED`). `scripts/make_trail_emitters.py`
  copies the two emitters into `/Game/TriadDrift/Gameplay/VFX/Emitters/NE_TD_Leaders` / `NE_TD_Followers` with
  `AssetTools.duplicate_asset`; the builder's `leaders` / `followers` bases use those.
- A system made only of persistent (`single`) emitters compiles and never activates: it needs one spawning emitter.
- A big system can report "not active" in `check_active.py` right after the editor starts (still compiling): run it twice.
- The distortion field is wired (`ShieldIdle` / `ShieldCollapse`, VFXScale 0.46 = 460 m radius). From inside it overlays
  the sky with a faint violet marbling; lower the `Shell` / `Inner` brightness in `build_tier2.py` if that is too much.
- Sheets: `review/tier1_r5_*` (real `-game` captures), `review/tier2_r5_*` (Play-In-Editor rows, except `_field_wired_game`).
