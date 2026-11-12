# Hero M02 — isolated visual benchmark

This additive candidate refines Hero only. Baseline source/native files, Combat,
Verdance, shared maps and gameplay settings are protected. Exact ownership is the
two HeroM02 roots in CLAIMS.md, published by Lead at 85119ed. See brief.md for the
technique, reference provenance and manual approval gate.

Review entry point after native captures: `review/compare.html`. Full-resolution
matched images are `review/Before_Hero.png` and `review/After_Hero.png`, plus
Detail, Zenith and Wrap pairs. Build and capture create actual reports; their
absence means those stages have not passed. No authoring study substitutes for a
native screenshot.

Native asset root:
`/Game/TriadDrift/Environment/SpaceFoundation/HeroM02/`.
Main review instance is `MI_HeroM02_Benchmark`; isolated review scene is
`Review/L_HeroM02_Benchmark`. The separate actor is `BP_GalaxySkybox_HeroM02`.
Do not place it on a shared map without Lead/map-owner coordination. Engineering
is reviewing the existing gameplay fallback's recognition of sky-rig light and
post-process components (N-004); no gameplay fix is made in this lane.

## Reproduction

1. Obtain Lead's serialized preflight and exclusive heavy-process window first.
2. Source bake: run `generate_hero.py --width 8192 --stars-width 8192` in the
   NumPy-capable bundled Python documented in the script. No Blender app/bpy runs.
   Existing outputs are refused; preserve previous evidence before any new run.
3. Run `launch_hero.ps1 -Stage Build`, then `-Stage Capture` sequentially.
   The disposable host and all diagnostics stay in ignored `Saved/`. Private MCP
   port is 8024. The launcher requires at least 4 GiB available RAM and no competing
   heavy application. Neither baseline maps nor the game project are saved.
4. Run `finalize_diagnostics.py` with UE's bundled Python. It requires successful
   actual process exits, engine result files and the bundled engine log digest.
5. Inspect every matched pair and repeat-capture stability. Record limits and
   measured timings in `review/report.md`, then run `finalize_delivery.py`.
6. Freeze and ask Lead for branch-only publication. Stop for Nicholas's visual
   approval before Combat/Verdance propagation or shared integration.

The performance report uses synchronized scene-capture wall time including GPU
readback, rather than gameplay FPS or GPU timestamp measurements. Compressed-mip
texture budgets are configured estimates; they are not measured GPU residency.
Real-map motion/readability, packaged inclusion and integration require the map
owner's later gameplay validation.
