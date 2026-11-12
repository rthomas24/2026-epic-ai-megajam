# Environment assets: asteroids, rocks, crystals, flora

Load it with `rocks = runpy.run_path(r"<repo>\.claude\skills\blender-game-asset\scripts\procedural_rocks.py")`.
Everything here uses the data API (bmesh + mathutils.noise), so it behaves the same live and
headless.

## API (tested on Blender 5.2.2)

| Function | Notes |
|---|---|
| `make_asteroid(name, radius, seed, subdivisions=5, elongation, roughness, ridge, craters, crater_size, crater_depth, target_tris=None, collection, location, material, sharp_angle_deg=None)` | Lumpy and cratered, origin at the centre. Density comes from `subdivisions`: 4 = 1,280 tris, **5 = 5,120** (default), 6 = 20,480, 7 = 81,920 (Nanite hero rock). Prefer `subdivisions` over `target_tris`: collapse-decimating noise leaves shard-like triangles |
| `make_crystal_cluster(name, seed, count, height, radius, spread, max_tilt_deg, sides=6, tip_ratio, collection, location, material, glow, base_color, glow_strength=0.6)` | Faceted hexagonal crystals with pointed tips, origin at the base (place it on the ground); about 200 tris. Keep `glow_strength` at 0.3–1.5: strong emission flattens the facets into a white silhouette |
| `make_rock_material(name, dark, light, accent, noise_scale=5, bump_strength=0.6, roughness=0.82)` | Two-tone procedural rock driven by bounding-box coordinates (same look at any size), with a fine-noise bump so the **baked** normal map carries detail. Procedural → bake before Unreal (`blender-texturing`) |
| `decimate_to(obj, tris)`, `apply_modifiers(obj)`, `smart_uv(obj)`, `box_uv(obj)`, `ensure_collection(name)` | Helpers |

## Recipes
- **Asteroid field kit (combat zone):** make 5–8 variants with different seeds, elongation 0.2–0.5
  and radii 3–40 m.
  - **Big ones:** `subdivisions=7` and Nanite.
  - **Mid ones:** `subdivisions=5–6`, baked BC/N/ORM at 1K–2K.
  - **Filler pebbles:** `subdivisions=4`.

  Use one shared rock material per family (palette from `art-direction.md`), and accent colours
  (rusty, icy) via `make_rock_material(dark=..., light=..., accent=...)`. Unreal's foliage or
  instancing tools handle placement; Blender only makes the variants.
- **Cinder crystals:** 4–6 clusters (seeds × count 5–12 × height 1–4 m) in cyan and amber
  variants. Flat shading is intended. Their material can stay a flat material instance in Unreal
  (base colour + emissive); no bake is needed.
- **Crystals growing out of rocks:** build the asteroid, then place crystal clusters on surface
  points (`ray_cast` along the face normal, aligning +Z to the normal), embedding the base 5–10 cm
  so nothing floats. Join them, or keep them as separate meshes for different materials.
- **Rocks for planet surfaces:** `make_asteroid` with `elongation=0.5`, `craters=0`, `ridge=0.6`,
  then flatten the bottom (scale Z 0.6, `set_origin` "BOTTOM_CENTER") so they sit on terrain.
  Embed 10–30 cm below the ground in Unreal.

## Geometry Nodes scatter (when you need it in Blender)
Create node groups in Python with `bpy.data.node_groups.new(name, "GeometryNodeTree")`, sockets
with `tree.interface.new_socket(...)`, Poisson scatter via `GeometryNodeDistributePointsOnFaces`
(`distribute_method="POISSON"`) → `GeometryNodeInstanceOnPoints` with rotation aligned to normal →
`GeometryNodeRealizeInstances` before export.

**5.2 trap:** set modifier inputs with
`getattr(mod.properties.inputs, "Socket_2").value = v`. `mod["Socket_2"] = v` raises
`TypeError`. Random Value sockets are `Min`, `Max`, `ID`, `Seed`. Apply or realize before export:
Unreal doesn't run Blender's GN.

## Flora (Verdance): route A or B, not hand-coded
Code-modelled plants look like toys. Use AI image-to-3D (Rodin Quad, PBR) or licensed
Fab / Poly Haven plants, clean them up (`blender-ai-mesh-cleanup`), then restyle the materials to the
Verdance palette (teal / violet, bio-glow emissive on the tips). Alpha-masked cards are fine for
leaves (Nanite supports masked); translucent materials are not Nanite-compatible.
