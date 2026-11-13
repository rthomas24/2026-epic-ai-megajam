# Lancer textures_r2 - source record

Close-range polish of `Lancer/mesh_r1/T_Lancer_{BC,N,ORM,E}.png`, for R-030 #1 / `docs/eng/SHIP-QUALITY.md`.
**Same UVs as `mesh_r1`** (texture-only drop; use with `mesh_r1/SM_Lancer.fbx`). 2048, Grok Bot, 2026-11-13,
Python + OpenCV (`tex_squad.py`), no generator.

| File | Change |
|---|---|
| `T_Lancer_BC.png` (sRGB) | bilateral edge-preserving clean, panel-line high-pass re-injected, paint chroma calmed; only island pixels + 4 px gutter change |
| `T_Lancer_N.png` (DirectX Y-) | low band (sigma 3 px) x0.5 to soften AI lumps, panel lines kept |
| `T_Lancer_ORM.png` (linear) | R = 255, roughness median + bilateral, metallic median + smoothstep |
| `T_Lancer_E.png` (sRGB) | only bright saturated cyan (hue 192 +-35) kept; grey/dim muddy background zeroed; saturation floor 0.7, x1.35, unsharp |

Metrics (`squad_metrics.json`, before -> after): see file. Preview: `SquadDetail_r1/contact_sheet.png` (bottom right).
