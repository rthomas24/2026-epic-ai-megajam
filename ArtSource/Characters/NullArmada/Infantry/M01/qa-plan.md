# M01 geometry, scale and review verification

Status: planned checks; **none executed on a character model yet**.

## Resource and input gates

- Obtain a fresh explicit Lead grant for each heavy window. Hero's slot remains
  exclusive until released. Check process inventory and memory immediately before
  launch; available RAM alone is not clearance. Record actual PID, times and exit.
- First Blender operation reports actual version, scene, units, engines and device
  using the core skill environment report. Never edit another live user's scene.
- Read-only export only the agreed Manny foundation from actual package bytes.
  Record package/file hashes, source commit, export options and native exit result.
  Verify nonzero FBX, import scale/axes, base height and compatible bone names.
- Preserve original base and editable versioned source. No fresh anatomy, new rig,
  skin weights, animation or private gameplay writes in M01.

## Per-stage technical checks

Use repo `asset_report.py` / `assembly_check.py` and host `blender-asset-qa` on all
asset components together, with a documented component list. Inspect base and
evaluated geometry where modifiers change counts. Fix every error; explicitly
explain accepted warnings rather than disabling checks to obtain a green report.

| Area | Evidence required |
|---|---|
| Inventory | Objects, mesh datablocks, evaluated tris per object/whole asset, material roles/slots, dimensions, units, source names and applied transforms |
| Topology | No degenerate faces, zero-length edges, unintended duplicate vertices, wire/nonmanifold geometry, inverted islands or inconsistent winding; justify intentionally open hidden body regions |
| Surface attachment | No z-fighting or coincident shells; no unmounted parts; inspect intersections/enclosed/hidden surfaces visually in addition to numerical tolerance checks |
| Shading | Clean bevel highlights, no broad-plane gradients, curved faceting or pinching; sensible normals at plate seams |
| UV/material | Correct assignments and color spaces; no missing external image dependencies; UVs suitable for planned later texturing, no claim of final baked textures in M01 |
| Scale | Feet near ground/origin, about180cm standing and compatible Manny joint centers; record actual bounds and coordinate conversion rather than inferring from filenames |
| Articulation | Neck, shoulder, elbow, wrist/thumb, hip/groin, knee/popliteal and ankle/toe gaps visible; no rigid plate bridging a flex joint |
| Budget | Target35–55k tris, ceiling60k,4 shared roles; count evaluated geometry and report estimated future batching separately from current source object count |

Rigging is not authorized. Therefore rest-pose geometric clearance can be verified,
but running/crouching/aiming/melee deformation is **not** established. Later extreme
pose/skin-weight tests require the approved rigging stage. Preserve useful original
base bone metadata as reference without claiming a rigged final armor model.

## Required visual review

Four orthographic images: front, rear, left and right. One three-quarter beauty.
Use consistent subject scale, ground line, neutral studio lights, exposure and
uncropped full-body framing. Orthographic views share resolution and ortho scale;
no per-view geometry edits. Start with lightweight renders (e.g.1024px height),
increase only if Lead clears resources and readability warrants it. No animation.

Also inspect a clay silhouette sheet and wireframe/component-color sheet. Open
every delivered image. Compare helmet identity, crown/brow, chest/back plates,
limb proportions, belt and foot construction against all four reference views.
Document reference relaxed-arm versus Manny A-pose differences. Use independent
review through Lead and the repository AAA rubric; a fresh reviewer must see the
brief, source reference, views and measured results, not only the build history.

## Delivery audit

- Reopen the saved .blend in the next cleared session; verify exact saved scene,
  materials, local dependencies, geometry counts and render reproducibility.
- Verify paths, file types/nonzero sizes, reference equality and image integrity.
  Produce SHA256 manifest and report which checks were actually executed.
- Source, final four orthographic views, beauty, measured QA reports and provenance
  stay under this claimed M01 root. No unrelated/native/template/shared files.
- Send exact changed paths/hashes to Lead for serialized remote/claim preflight,
  independent review, owned-only commit and push. No main merge by this worker.
- Nick visual approval is required before rigging/native integration or further
  characters. Until then report **technically verified source model**, not
  integrated, gameplay validated or final AAA approved.
