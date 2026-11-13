**Agent:** Ships & Weapons Agent
**Role:** Ship and modular weapon visuals
**Task:** Material correction proposal only — not executed

Nick: **Hold for material correction.** Preserve Material04 v02 and all earlier models. No export, native or Main promotion.

Evidence: paint is already nonmetallic (Graphite Metallic0), correctly linearized dark albedo, but broad planes still reflect pale violet/silver light. Material04 reduced Graphite roughness .50→.38 and retained .08 clearcoat. The inherited comparison studio has a violet rim (RGB .8,.72,1) and cool fill. Those are plausible contributors; their individual influence has not yet been isolated. More dark assignment alone did not close the gap.

Proposed next bounded test, only after a fresh Lead execution grant:

1. Start from SHA-pinned v02, preserve all20mesh geometry/UV/normals/smoothing/transforms,36frames and per-face roles. Keep colors, microfinish, metal roles, glass and cyan fixed. Change only Graphite nominal roughness to .56 and clearcoat to0 as an explicit painted-armor test. Keep physically standard IOR/specular. Record linked roughness-driver values and all material node inputs before/after; do not mistake the linked BSDF default socket for evaluated roughness.
2. Render the unchanged bright-studio hero/top/rear views beside v02 first. Preserve the violet rig for honest comparisons. Add clearly labeled neutral-white-light diagnostic views with identical cameras, exposure, energy and size to isolate light tint; never replace or relabel them as the original studio. No extra emissive, camera angle or exposure changes to hide surfaces.
3. Judge whether broad armor reads as painted charcoal while barrel/nozzle trim remains distinct. Rerun material/geometry/frame/clearance gates and get a fresh visual review. If the gap remains, stop and ask Nick to choose the next material/modeling route; no repeated parameter grind.

Panel-value/roughness variation and small purposeful service markings are a subsequent art decision, not part of this isolated response test. This proposal does not claim that roughness alone can close the reference's mechanical-detail gap. No code or heavy process for this correction has been run.
