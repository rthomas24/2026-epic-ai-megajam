# Bastion textures_r2 - source record

Close-range polish of `Bastion/mesh_r1/T_Bastion_{BC,N,ORM,E}.png`, for R-030 #1 / `docs/eng/SHIP-QUALITY.md`.
**Same UVs as `mesh_r1`** (texture-only drop; use with `mesh_r1/SM_Bastion.fbx`). 2048, Grok Bot, 2026-11-13,
Python + OpenCV (`tex_squad.py`), no generator.

| File | Change |
|---|---|
| `T_Bastion_BC.png` (sRGB) | bilateral edge-preserving clean, panel-line high-pass re-injected, paint chroma calmed; only island pixels + 4 px gutter change |
| `T_Bastion_N.png` (DirectX Y-) | low band (sigma 3 px) x0.5 to soften AI lumps, panel lines kept |
| `T_Bastion_ORM.png` (linear) | R = 255, roughness median + bilateral, metallic median + smoothstep |
| `T_Bastion_E.png` (sRGB) | only bright saturated amber (hue 28 +-40, low-saturation warm nozzle interiors included and saturated) kept; grey/dim muddy background zeroed; saturation floor 0.7, x1.35, unsharp |

Metrics (`squad_metrics.json`, before -> after): see file. Preview: `SquadDetail_r1/contact_sheet.png` (bottom right).
