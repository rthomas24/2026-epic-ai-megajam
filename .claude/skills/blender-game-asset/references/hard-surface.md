# Hard-surface recipes and the hardsurface.py API

Load it in each call:
`hs = runpy.run_path(r"<repo>\.claude\skills\blender-game-asset\scripts\hardsurface.py")`.

**Conventions:** units are metres; the asset's front faces **-Y** (Blender's Front view) and up is
+Z; object scale stays 1, with all sizing done in mesh data. The exporter turns -Y into Unreal's +X.
Modifier order: Mirror → Boolean(s) → Bevel → Weighted Normal (each `add_*` appends in call order).

## API (all tested on Blender 5.2.2)

| Function | Makes / does |
|---|---|
| `loft(name, sections, ring_verts=16, collection, location, cap=True)` | Hull or fuselage from superellipse cross-sections along +Y. A section is `{"y", "w", "h", "n" (2 = ellipse … 6 = boxy), "x", "z", "groove", "groove_width"}`; `w` or `h` = 0 makes a point. `groove` cuts a circumferential panel line at that station (e.g. 0.008–0.015 m deep, 0.02–0.03 m wide) |
| `wing(name, root_chord, tip_chord, span, sweep, thickness, tip_thickness, dihedral_deg, root_x, root_y, root_z, ...)` | Tapered, swept airfoil slab toward +X. Mirror it with `add_mirror(..., mirror_object=hull)`. Negative `sweep` = forward-swept (Lancer) |
| `lathe(name, profile[(pos, radius)...], segments, axis="Y", location)` | Round parts: nacelles, nozzles, barrels, turret domes. Radius 0 closes an end |
| `box(name, size, location)`, `cylinder(name, radius, depth, segments, axis, location, radius_top)` | Sized primitives (exact dimensions; cylinder axis set in mesh data) |
| `add_bevel(obj, width, segments=3, angle_deg=30, harden=True)` | Light-catching chamfers; turns on smooth shading + sharp edges for harden normals |
| `add_weighted_normal(obj)` | Flat-looking big faces with soft bevels (always after the bevel) |
| `smooth_by_angle(obj, deg)` | The 4.1+ replacement for auto smooth |
| `add_mirror(obj, axis="X", bisect, clip, mirror_object)` | Symmetry; use `mirror_object=hull` for parts offset from centre |
| `add_solidify`, `add_array` | Shells; repeated slats or vents |
| `boolean(obj, cutter, operation="DIFFERENCE", solver="EXACT")` | Renames the cutter `CUT_*`, hides it from render, and gives it the target's material (assign the target's material **first**) |
| `inset_panels(obj, normal, min_dot, fraction, thickness, depth, material, seed, min_area)` | Recessed panels, vents or glow strips on faces facing `normal`; skips tiny faces. Run on the base mesh **before** bevel |
| `plates(target, normal, min_dot, region, thickness, gap, lift, bevel, material)` | Layered armour plates with seam gaps, as a separate object parented to the target. Stack 2–3 passes (top, sides) |
| `greebles(target, count, seed, size, height, normal, min_dot, region, material)` | Small mechanical boxes. **Always pass `region`** to cluster them near functional areas |
| `apply_modifiers(obj)`, `apply_transform(obj, location, rotation, scale)`, `set_origin(obj, "BOTTOM_CENTER" / "CENTER" / "MIN_CORNER" / (x, y, z))`, `join(objects, name)` | Context-free utilities. `join` applies modifiers and drops unused material slots |
| `paint(name, color, metallic=0.35, roughness=0.38, coat)`, `glow(name, color, strength=4)`, `glass(name, color)`, `hex_color("#RRGGBB")`, `assign_material(obj, mat)` | Materials (Principled values Unreal can rebuild as material instances) |
| `smart_uv(obj)` | UV unwrap that works live and headless |

## Recipe: code-built ship (route C enemies and proxies; hero ships go route A)
1. **Brief and concept** (gate). Note the silhouette idea, signature feature and sizes from
   `art-direction.md`.
2. **Blockout:** a `loft` hull with 5–7 sections, plus `wing` and `lathe` nacelles at true size, no
   detail. Clay-review it, then get the **human silhouette OK**.
   - Give the nose an ogive: add an extra small section before the tip. Sharp cones make the bevel
     collapse.
   - Vary section squareness `n` along the hull: rounder at the nose, boxier at the engines.
3. **Secondary forms:**
   - **Intakes:** `inset_panels` with deeper `depth`, plus a dark trim material.
   - **Nozzles:** `lathe` bell plus an inner `glow` disc set slightly inside the bell.
   - **Canopy:** a loft bubble, `glass`, with frame bars from thin boxes.
   - **Weapons:** `lathe` barrels on `box` hardpoints.
   - **Plates:** dorsal and lateral passes.
   - **Mounting:** every part overlaps its mount by 2–5 cm. Run `assembly_check.py` until it's OK.
4. **Tertiary detail:**
   - `groove` stations on the hull, about every 0.8–1.5 m.
   - Glow strips as `inset_panels` on side faces, with `fraction` ≤ 0.4.
   - Greebles only in two or three regions: around the engines, behind the cockpit, near weapon
     hardpoints.
5. **Shading:** `add_bevel` (width ≈ 0.5–1% of part size, 2 segments) + `add_weighted_normal` on
   every part. The bevel must be narrower than any recess depth, or it collapses into duplicate
   vertices.
6. **Materials:** `paint` (dominant + secondary), `paint` with high metallic for trim, `glow` at
   strength 1.5–3 (higher washes out), `glass`. Stick to 60/30/10.
7. **Sockets:** empties named `SOCKET_<Purpose>` (muzzles, engine FX, cockpit camera) parented to
   the hull, with their -Y/+Z axes as the authoring frame.

## Recipe: props (consoles, crates, doors, pickups, charges)
`box` or `loft` base → `inset_panels` (screens, vents) → `plates` for armour → `boolean` cut-outs
(handles, slots) → bevel + weighted normal → `set_origin(obj, "BOTTOM_CENTER")` so it sits on the
floor at Z = 0 → 1–3 materials. Keep it under about 5k tris.

## Recipe: modular sci-fi corridor kit (Dominion interior)
- **Grid:** 400 cm modules (4 m in Blender). Wall thickness ≥ 10 cm (Lumen needs it). Floor top at
  Z = 0. Pieces snap at exact multiples of 4 m with no gaps or overlaps at the joins.
- **Pivots:** `set_origin(obj, "MIN_CORNER")` for grid-snapped walls and floors, or bottom-centre for
  props. Keep it consistent across the kit.
- **Set:** `SM_Kit_Corridor_Straight_400`, `_Corner_400`, `_T_400`, `_Doorway_400`,
  `_Floor_400`, `_Ceiling_400`, `_Pillar`, `_Door` (separate animated mesh), `_Shaft_400x800`,
  `_Catwalk_400`, `_Railing_400`.
- **Look:** ribbed walls from `add_array` of beveled ribs; red emissive strips from `inset_panels`
  with `glow`; "eye" lenses from `lathe` domes with white `glow`; gloss black and gunmetal per
  `art-direction.md`.
- **Texturing:** share one 2K trim sheet across the kit (UV islands mapped to trim strips), or use
  flat material instances. Don't give each piece its own texture set.
- **Test:** build a 3 × 3 layout from the pieces in a test collection and render it, which shows
  seams, gaps and repetition. Then delete the layout; only the pieces export.

## Recipe: energy blade (first-person weapon)
`lathe` hilt (grip rings via profile steps, metallic trim, `glow` power cell) plus a **separate** thin
`loft` blade with a `glow` material in squad colours. Origin at the grip (hand socket). Blade under
about 300 tris. In Unreal the blade gets an emissive, maybe translucent, material: keep it a
separate non-Nanite mesh.

## Detail density cheat-sheet (per 1 m² of visible surface, chase camera)
- Calm paint plane: 0–1 features. Functional cluster: 6–15 features. Seams every 0.8–1.5 m.
- Smallest detail worth modelling: about 4 cm for hero ships at 6–15 m, about 25 cm for enemies at
  100 m+. Anything smaller belongs in the normal map or texture.
