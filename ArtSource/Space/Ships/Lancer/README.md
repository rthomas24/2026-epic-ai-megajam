# Lancer M01 — source prototype

Open `Lancer_preview.html` locally for orbit/zoom, module inspection, optional
weapons, attachment markers, exploded assembly and an illustrative emissive slider.
`Lancer_review.svg` has six assembly views plus weapon-module views. These previews
use the actual JSON source vertices, not an artist concept pasted over the model.

**Review status:** the code-built hull is a silhouette proxy. This package is not
a final game-ready hero ship and has not been imported into Unreal. Source-only
authorization remains active; the Lead owns claim publication and integration.

## Files and regeneration

- `generate_lancer.mjs`: dependency-free deterministic Node source generator.
- Six `SM_*.mesh.json` files: closed triangle mesh sources, parts and material IDs.
- `attachments.json`: module instances, socket transforms and adjustable material values.
- `verification.json`: source topology, counts, dimensions and muzzle clearances.
- `preview_source_validation.json`: nine rendering/control states exercised in a
  Node VM with a stub DOM/Canvas; finite-coordinate/runtime checks, not browser QA.
- `source_checksums.json`: SHA-256 fingerprints of the generated mesh/interface
  and preview outputs; deterministic regeneration matched every fingerprint.
- `Lancer_preview.html` / `Lancer_review.svg`: generated text previews.
- `preview_template.html`: editable preview shell (data is embedded during generation).
- `build_in_blender.py`: future Blender source adapter; intentionally not run.
- `brief.md`, `handoff.md`, `provenance.json`: design, integration and credits.

Run `node generate_lancer.mjs` from this directory. It replaces only its named
generated source outputs. No packages, web server, external fonts or downloads
are required. The template is not itself a preview; open `Lancer_preview.html`.

The mesh format is centimetres, +X forward, +Y right, +Z up. Positions in
`attachments.json` are relative to the named parent instance. Instances themselves
are hull-relative; every transform has unit scale. The minigun rotor is a separate
mesh and its source pivot is its spin axis. The loaded pod noses are static preview
geometry; the separate spike rocket is a reusable projectile visual.

## Verification and limits

Verified with Node: each source solid has closed two-face edge adjacency,
consistent winding, positive signed volume, no duplicate positions within a
part and no degenerate triangles. Attachment parents/bases and positive muzzle
clearances are checked. This does not prove absence of intersections, floating
parts or z-fighting between solids. Geometry deliberately overlaps at mechanical
joints. Those checks belong to Blender production QA.

The source assembly is 10,528 triangles, 11 mesh instances, six materials,
798 × 680 × 213 cm and 33 attachment transforms. Texture payload is zero;
draw calls, GPU memory and frame time are not measured. Final material slot
reduction and instancing need Unreal validation.

Browser visual verification could not complete: Windows computer-use stopped
because it could not determine the current browser URL with enough confidence.
The HTML and embedded script were checked with Node, including nine preview
rendering states and Python adapter syntax; no browser screenshot,
Blender render or visual sign-off is claimed.

No .blend, FBX, OBJ, images, uassets, C++, maps or shared configuration were
created or changed. Rift and Bastion production follows Lancer silhouette review.
See the concrete handoff for the remaining art and engineering work.
