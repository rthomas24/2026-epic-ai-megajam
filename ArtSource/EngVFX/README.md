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
