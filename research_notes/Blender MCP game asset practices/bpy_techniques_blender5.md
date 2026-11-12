# Blender 5.x bpy techniques for stylized sci-fi game assets, and API breaks from 4.0 to 5.2

**How this was verified:** every identifier below was checked one of three ways:
- **"verified headless on 5.2.2"** means a run of `blender.exe --background --factory-startup --python <script>` on the installed **Blender 5.2.2 LTS** (hash d13f752e3b9c, built 2026-09-15; Windows 11, RTX 5080, GPU backend reported as `OPENGL` after an EEVEE render).
- The bundled official **5.1 API RST docs** and **manual RST**, cited with their docs.blender.org URLs.
- The official release notes. developer.blender.org returned HTTP 403 to automated fetches, so the identical source markdown was read from `https://projects.blender.org/blender/blender-developer-docs/raw/branch/main/docs/release_notes/<ver>/python_api.md`. The citations below use the canonical developer.blender.org URLs.

**Throwaway test scripts and outputs** (scratchpad, may be deleted later):
- Folder: `C:\Users\ryant\AppData\Local\Temp\claude\bb78036804ebbea9\0915cc49-a623-444a-8b28-4f546c72c7cd\scratchpad\bpy_research\`
- Scripts: `api_probe.py`, `api_probe2.py`, `pipeline_test.py`, `probe3.py`
- Results: `*.json`, plus the `renders\` folder (contact sheet, baked maps, FBX/GLB).

The full end-to-end pipeline took **7.7 s wall time including Blender startup**. It built a hard-surface ship, a rock with crystals scattered by GN, UVs, three bakes, 11 preview renders and a contact sheet, then exported FBX and GLB.

---

## 1. Which modifier stacks and parameters give clean, game-ready hard-surface results, and how should they be applied before export?

### Takeaway
Since 4.1 there is no auto smooth; on 5.2 the stack below was verified to give custom normals (`normals_domain == 'CORNER'`) that survive `object.convert`:
1. Call `mesh.shade_smooth()`.
2. Add any Mirror / Boolean(EXACT) / Array / Solidify you need.
3. Add **Bevel**: `limit_method='ANGLE'`, `use_clamp_overlap=True`, `harden_normals=True`, `miter_outer='MITER_ARC'`, 2–3 segments.
4. Add **Weighted Normal**: `mode='FACE_AREA'`, `weight=50`, `keep_sharp=True`.
5. For parts without bevels, use the **"Smooth by Angle"** GN modifier (`bpy.ops.object.shade_auto_smooth`) or write `sharp_edge` directly.

**Before export:**
- Apply the stack with `bpy.ops.object.convert(target='MESH')`.
- **Triangulate n-gons** (Triangulate modifier, `min_vertices=5`, `keep_custom_normals=True`), or the FBX exporter silently drops tangents for that mesh.
- Remove the empty material slots that Boolean `material_mode='INDEX'` creates.

### Cited Findings

**Bevel modifier on 5.2.2** (property: default, then options) — verified headless on 5.2.2 ([API: BevelModifier](https://docs.blender.org/api/5.1/bpy.types.BevelModifier.html)):
- `width`: 0.1
- `segments`: 1
- `affect`: `'EDGES'` (`'VERTICES'`)
- `limit_method`: `'ANGLE'` (`'NONE','ANGLE','WEIGHT','VGROUP'`)
- `angle_limit`: 0.5236 rad (30°)
- `use_clamp_overlap`: True
- `offset_type`: `'OFFSET'` (`'OFFSET','WIDTH','DEPTH','PERCENT','ABSOLUTE'`)
- `profile_type`: `'SUPERELLIPSE'`
- `profile`: 0.5
- `harden_normals`: False
- `mark_sharp` / `mark_seam`: False
- `miter_outer`: `'MITER_SHARP'` (`'MITER_SHARP','MITER_PATCH','MITER_ARC'`)
- `miter_inner`: `'MITER_SHARP'` (`'MITER_SHARP','MITER_ARC'`)
- `face_strength_mode`: `'FSTR_NONE'` (`'FSTR_NONE','FSTR_NEW','FSTR_AFFECTED','FSTR_ALL'`)
- `vmesh_method`: `'ADJ'` (`'ADJ','CUTOFF'`)
- `edge_weight`: `'bevel_weight_edge'`; `vertex_weight`: `'bevel_weight_vert'` (attribute names)

**What Harden Normals does:** "the per-vertex face normals of the bevel faces are adjusted to match the surrounding faces, and the normals of the surrounding faces are not affected" — [Manual: Bevel Modifier](https://docs.blender.org/manual/en/latest/modeling/modifiers/generate/bevel.html).

**Weighted Normal on 5.2.2** — verified headless on 5.2.2 ([API: WeightedNormalModifier](https://docs.blender.org/api/5.1/bpy.types.WeightedNormalModifier.html)):
- `mode`: `'FACE_AREA'` (`'FACE_AREA','CORNER_ANGLE','FACE_AREA_WITH_ANGLE'`)
- `weight`: 50
- `thresh`: 0.01
- `keep_sharp`: False
- `use_face_influence`: False
- `vertex_group`: `""`

**What Keep Sharp and Face Influence do** ([Manual: Weighted Normal](https://docs.blender.org/manual/en/latest/modeling/modifiers/normals/weighted_normal.html)):
- *Keep Sharp* = "Preserve Sharp edges".
- *Face Influence* uses the face strengths set by the *Set Strength* tool or by the Bevel modifier's *Set Strength* mode (`face_strength_mode`).

**Auto smooth removal (4.1):**
- `Mesh.use_auto_smooth` and `Mesh.auto_smooth_angle` were removed. Angle smoothing is now "a modifier node group asset or an operator that controls the `sharp_edge` attribute". `create_normals_split` / `calc_normals_split` / `free_normals_split` were removed in favour of the auto-updated `Mesh.corner_normals`. `MeshLoop.normal` is read-only, so use `normals_split_custom_set(...)` — [4.1 Python API notes](https://developer.blender.org/docs/release_notes/4.1/python_api/).
- The base state equals old "Auto Smooth at 180°". The operators are "Set Sharpness by Angle" (edit mode) and "Shade Smooth by Angle" (other modes) — [4.1 Modeling notes](https://developer.blender.org/docs/release_notes/4.1/modeling/).
- On 5.2.2, `hasattr(mesh, 'use_auto_smooth')`, `'auto_smooth_angle'`, `'calc_normals'` and `'calc_normals_split'` are all False — verified headless on 5.2.2.

**`bpy.ops.object.shade_auto_smooth(use_auto_smooth=True, angle=0.523599)`** — verified headless on 5.2.2; [Manual: Smooth by Angle](https://docs.blender.org/manual/en/latest/modeling/modifiers/normals/smooth_by_angle.html) calls it "a geometry nodes asset" from the Essentials library:
- It adds a modifier named **"Smooth by Angle"** (`type == 'NODES'`), placed **last** in the stack.
- Its node group, also called **"Smooth by Angle"**, is **linked** from `<install>\5.2\datafiles\assets\nodes\geometry_nodes_essentials.blend` (`ng.library` is not None, `ng.asset_data` is set).
- Group interface: `Input_0` "Mesh", `Input_1` "Angle" (float), `Socket_1` "Ignore Sharpness" (bool), `Output_2` "Mesh".
- A second call on another object reuses the same group.
- It works in `--background`.

**Smooth-by-angle variants that do not add a modifier** — verified headless on 5.2.2 ([API: bpy.ops.object](https://docs.blender.org/api/5.1/bpy.ops.object.html), [API: Mesh](https://docs.blender.org/api/5.1/bpy.types.Mesh.html)):
- `bpy.ops.object.shade_smooth_by_angle(angle=0.523599, keep_sharp_edges=True)` writes the `sharp_edge` EDGE attribute and adds no modifier.
- Edit mode: `bpy.ops.mesh.set_sharpness_by_angle(angle=0.523599, extend=False)`.
- Data API: `Mesh.set_sharp_from_angle(angle=3.14159)` ("Reset and fill the "sharp_edge" attribute based on the angle of faces neighboring manifold edges").
- `Mesh.shade_smooth()` removes `sharp_face`; `Mesh.shade_flat()` sets `sharp_face` True on every face.
- `bpy.ops.object.shade_smooth(keep_sharp_edges=True)` and `shade_flat(keep_sharp_edges=True)` also remove the Smooth by Angle modifier ([manual tip](https://docs.blender.org/manual/en/latest/modeling/modifiers/normals/smooth_by_angle.html)).

**Boolean modifier on 5.2.2** — verified headless on 5.2.2. The `FAST` → `FLOAT` rename is in the [5.0 Python API notes](https://developer.blender.org/docs/release_notes/5.0/python_api/). The manual defines Float as fast but without support for overlapping geometry, Exact as best results but "much slower", and Manifold as "usually fastest but only works on Manifold meshes" ([Manual: Boolean](https://docs.blender.org/manual/en/latest/modeling/modifiers/generate/booleans.html)).
- `solver` items are `'FLOAT','EXACT','MANIFOLD'`, default `'EXACT'`. The old `'FAST'` was renamed `'FLOAT'` in 5.0.
- `operation`: `'INTERSECT','UNION','DIFFERENCE'` (default DIFFERENCE).
- `operand_type`: `'OBJECT','COLLECTION'`.
- `material_mode`: `'INDEX','TRANSFER'`.
- `use_self`, `use_hole_tolerant`, `double_threshold` (1e-6).

**Empty material slot from Boolean** — verified headless on 5.2.2:
- With `material_mode='INDEX'` and a cutter that has no material, the result gains an **empty (None) material slot**. It survives `object.convert` and the join (the test ship exported with a `None` slot).
- `material_mode='TRANSFER'` avoided it.
- `bpy.ops.object.material_slot_remove_unused()` removes it after applying.

**Mirror / Array / Solidify / Subsurf defaults on 5.2.2** — verified headless on 5.2.2 ([API: MirrorModifier](https://docs.blender.org/api/5.1/bpy.types.MirrorModifier.html), [ArrayModifier](https://docs.blender.org/api/5.1/bpy.types.ArrayModifier.html), [SolidifyModifier](https://docs.blender.org/api/5.1/bpy.types.SolidifyModifier.html), [SubsurfModifier](https://docs.blender.org/api/5.1/bpy.types.SubsurfModifier.html)):
- **Mirror:** `use_axis=(True,False,False)`, `use_bisect_axis`, `use_bisect_flip_axis`, `use_clip=False`, `use_mirror_merge=True`, `merge_threshold=0.001`, `mirror_object`.
- **Array:** `fit_type` (`'FIXED_COUNT','FIT_LENGTH','FIT_CURVE'`), `count=2`, `use_relative_offset=True`, `relative_offset_displace=(1,0,0)`, `use_constant_offset`, `constant_offset_displace`, `use_merge_vertices=False`, `merge_threshold=0.01`, `use_object_offset`, `offset_object`, `start_cap`, `end_cap`.
- **Solidify:** `solidify_mode` (`'EXTRUDE','NON_MANIFOLD'`), `thickness=0.01`, `offset=-1.0`, `use_even_offset=False`, `use_rim=True`, `use_quality_normals`, `edge_crease_inner/outer/rim`, `bevel_convex`, `nonmanifold_thickness_mode`.
- **Subsurf:** `levels=1`, `render_levels=2`, `quality=3`, `use_creases=True`, `use_custom_normals=False`, `boundary_smooth='ALL'`, `uv_smooth='PRESERVE_BOUNDARIES'`, `use_limit_surface=True`. 5.2 also has `use_adaptive_subdivision`, `adaptive_space`, `adaptive_pixel_size` and `adaptive_object_edge_length` on the modifier.

**Creases for subdivision are an attribute:** write `crease_edge` (EDGE, FLOAT, 1.0 = fully sharp) via `bm.edges.layers.float.new('crease_edge')` or `mesh.edge_creases_ensure()`. A canopy with creased base edges and Subsurf level 2 evaluated correctly (192 tris, `normals_domain == 'POINT'`) — verified headless on 5.2.2; [4.0 Python API notes](https://developer.blender.org/docs/release_notes/4.0/python_api/) (`MeshEdge.crease` → `crease_vert`/`crease_edge` attributes).

**Panel lines, insets and greebles with bmesh** — verified headless on 5.2.2 (signatures from [API: bmesh.ops](https://docs.blender.org/api/5.1/bmesh.ops.html)):
- `bmesh.ops.inset_individual(bm, faces=top, thickness=0.04, depth=-0.02, use_even_offset=True)` recesses each face. The **original face objects become the inner panels**, so setting `f.material_index` on them after the call colours the panels (visible in the test render).
- `bmesh.ops.extrude_discrete_faces(bm, faces=...)` followed by `bmesh.ops.translate(bm, vec=f.normal*0.06, verts=f.verts)` makes raised greeble blocks.
- An Array (count 5, relative offset (0,1.6,0)) plus Mirror on a small box makes greeble strips.

**Test-asset stats on 5.2.2** (evaluated mesh) — verified headless on 5.2.2:

| Object | Stack | Result |
|---|---|---|
| Hull | inset/extrude + Bevel(0.015, 3 seg, ANGLE 35°, harden) + WeightedNormal(keep_sharp) | 788 tris, 0 non-manifold edges, `has_custom_normals=True`, `normals_domain='CORNER'` |
| Engine pod | Boolean EXACT + Mirror + Bevel + WN | 1,528 tris, custom normals |
| Wing | Mirror(bisect, clip) + Bevel + Smooth by Angle | 216 tris, custom normals |
| Whole ship, joined | — | 2,996 tris |

**Applying modifiers:**
- `bpy.ops.object.convert(target='MESH')` applied every modifier, including the linked "Smooth by Angle" group, and **kept custom normals** on hull, wing, pod and fin — verified headless on 5.2.2.
- The data-API alternative is `bpy.data.meshes.new_from_object(ob.evaluated_get(dg), preserve_all_data_layers=True, depsgraph=dg)` (signature verified; used in the bake test).
- Operators work in background through `bpy.context.temp_override(...)`: `object.modifier_apply` with `temp_override(object=a, active_object=a, selected_objects=[a], selected_editable_objects=[a])` and `object.join` with `temp_override(active_object=a, selected_editable_objects=[a, b], selected_objects=[a, b])` both returned FINISHED — verified headless on 5.2.2.

**Tangents and n-gons:**
- Exporting FBX with `use_tspace=True` printed `Warning: Mesh 'SM_Hull' has polygons with more than 4 vertices, cannot compute/export tangent space for it`.
- Adding `TRIANGULATE` with `min_vertices=5` (n-gons only) and `keep_custom_normals=True` after Weighted Normal gave max polygon size 4, kept `has_custom_normals=True`, and exported with no warning.
- Verified headless on 5.2.2 ([API: TriangulateModifier](https://docs.blender.org/api/5.1/bpy.types.TriangulateModifier.html): `quad_method` default `'SHORTEST_DIAGONAL'`, `ngon_method` `'BEAUTY'`, `min_vertices=4`, `keep_custom_normals=False`).

**Practitioner opinion on face-weighted normals:**
- They make bevels read well with less reliance on normal maps.
- A face-weighted bevel should have **no UV split**, because a split doubles vertices.
- Test in the target engine, because importing with "unlocked" normals resets custom normals.

These are forum opinions, not official docs — [Polycount discussion](https://polycount.com/discussion/comment/2577165), [Polycount discussion](https://polycount.com/discussion/comment/2337231).

#### Copy-ready: hard-surface part (verified headless on 5.2.2)
```python
import bpy, bmesh, math
from mathutils import Matrix

def new_obj(name, bm, coll=None):
    me = bpy.data.meshes.new(name); bm.to_mesh(me); bm.free()
    ob = bpy.data.objects.new(name, me)
    (coll or bpy.context.scene.collection).objects.link(ob)
    return ob

def activate(ob, *others):                       # selection + active, needed by most ops
    for o in bpy.context.view_layer.objects: o.select_set(False)
    for o in others: o.select_set(True)
    ob.select_set(True); bpy.context.view_layer.objects.active = ob

def hard_surface_stack(ob, bevel_width=0.015, segments=3, angle_deg=35, triangulate=True):
    ob.data.shade_smooth()                       # 4.1+: no use_auto_smooth; custom normals drive shading
    bev = ob.modifiers.new('Bevel', 'BEVEL')
    bev.width = bevel_width                      # scale-dependent; ~0.5-1% of part size worked here
    bev.segments = segments
    bev.limit_method = 'ANGLE'; bev.angle_limit = math.radians(angle_deg)
    bev.use_clamp_overlap = True
    bev.harden_normals = True
    bev.miter_outer = 'MITER_ARC'
    wn = ob.modifiers.new('WeightedNormal', 'WEIGHTED_NORMAL')
    wn.mode = 'FACE_AREA'; wn.weight = 50; wn.keep_sharp = True
    if triangulate:                              # n-gons -> tris so FBX can export tangents
        tri = ob.modifiers.new('TriNgons', 'TRIANGULATE')
        tri.min_vertices = 5; tri.keep_custom_normals = True
    return ob

# hull with recessed panel lines (inner faces get material 1) and raised greebles (material 2)
bm = bmesh.new()
bmesh.ops.create_cube(bm, size=1.0)              # NB: bmesh create_* ops default size/radius = 0
bmesh.ops.scale(bm, vec=(1.0, 4.0, 0.6), verts=bm.verts)
long_edges = [e for e in bm.edges if abs((e.verts[0].co - e.verts[1].co).y) > 1.0]
bmesh.ops.subdivide_edges(bm, edges=long_edges, cuts=3, use_grid_fill=True)
for v in bm.verts:                               # taper nose (+Y), dorsal hump
    t = (v.co.y + 2.0) / 4.0
    v.co.x *= (1.0 - 0.7 * t ** 1.5); v.co.z *= (1.0 - 0.5 * t ** 2)
    if v.co.z > 0: v.co.z += 0.12 * math.sin(t * math.pi)
bm.normal_update()
top = [f for f in bm.faces if f.normal.z > 0.7]
bmesh.ops.inset_individual(bm, faces=top, thickness=0.04, depth=-0.02, use_even_offset=True)
for f in top: f.material_index = 1               # originals are now the inner panels
bm.normal_update()
side = [f for f in bm.faces if abs(f.normal.x) > 0.8 and f.calc_center_median().y < -0.5]
ext = bmesh.ops.extrude_discrete_faces(bm, faces=side[:4])
for f in ext['faces']:
    bmesh.ops.translate(bm, vec=f.normal * 0.06, verts=f.verts); f.material_index = 2
bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
hull = new_obj('SM_Hull', bm)
hard_surface_stack(hull)

# engine pod: boolean nozzle (EXACT) -> mirror -> bevel/WN
bm = bmesh.new()
rot = Matrix.Translation((0.9, -2.0, -0.1)) @ Matrix.Rotation(math.radians(90), 4, 'X')
bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=24, radius1=0.35, radius2=0.3, depth=1.4, matrix=rot)
pod = new_obj('SM_EnginePod', bm)
bm = bmesh.new()
rot2 = Matrix.Translation((0.9, -2.75, -0.1)) @ Matrix.Rotation(math.radians(90), 4, 'X')
bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=24, radius1=0.26, radius2=0.26, depth=0.6, matrix=rot2)
cutter = new_obj('CUT_Nozzle', bm); cutter.display_type = 'WIRE'; cutter.hide_render = True
boo = pod.modifiers.new('Nozzle', 'BOOLEAN')
boo.operation = 'DIFFERENCE'; boo.solver = 'EXACT'; boo.object = cutter
boo.material_mode = 'TRANSFER'                   # 'INDEX' left an empty material slot in testing
mir = pod.modifiers.new('Mirror', 'MIRROR'); mir.use_axis = (True, False, False)
hard_surface_stack(pod, bevel_width=0.012, angle_deg=40)

# wing: mirror with bisect/clip + bevel + Smooth by Angle GN modifier (appended last)
bm = bmesh.new()
bmesh.ops.create_cube(bm, size=1.0, matrix=Matrix.Translation((1.2, -0.6, 0)) @ Matrix.Diagonal((1.6, 1.2, 0.08, 1)))
wing = new_obj('SM_Wing', bm)
m = wing.modifiers.new('Mirror', 'MIRROR')
m.use_axis = (True, False, False); m.use_bisect_axis = (True, False, False); m.use_clip = True
b = wing.modifiers.new('Bevel', 'BEVEL'); b.width = 0.01; b.segments = 2; b.harden_normals = True
activate(wing)
bpy.ops.object.shade_auto_smooth(angle=math.radians(30))

# fin: single quad + solidify; canopy: subsurf with creased base edges
bm = bmesh.new()
vs = [bm.verts.new(c) for c in ((0, -2.0, 0.35), (0, -1.0, 0.35), (0, -1.6, 1.1), (0, -2.1, 1.1))]
bm.faces.new(vs)
fin = new_obj('SM_Fin', bm)
so = fin.modifiers.new('Solidify', 'SOLIDIFY'); so.thickness = 0.06; so.offset = 0.0; so.use_even_offset = True
hard_surface_stack(fin, bevel_width=0.01, segments=2)
bm = bmesh.new()
bmesh.ops.create_cube(bm, size=1.0, matrix=Matrix.Translation((0, 0.6, 0.42)) @ Matrix.Diagonal((0.35, 0.7, 0.18, 1)))
cr = bm.edges.layers.float.new('crease_edge')    # 4.0+: bm.edges.layers.crease no longer exists
for e in bm.edges:
    if all(v.co.z < 0.42 for v in e.verts): e[cr] = 1.0
can = new_obj('SM_Canopy', bm); can.data.shade_smooth()
ss = can.modifiers.new('Subsurf', 'SUBSURF'); ss.levels = 2; ss.render_levels = 2; ss.use_creases = True
result = {'objects': [o.name for o in (hull, pod, wing, fin, can)]}   # MCP: return JSON-serialisable dict
```

#### Copy-ready: apply and export for UE (verified headless on 5.2.2; UE-side behaviour untested)
```python
parts = [o for o in bpy.context.scene.objects if o.type == 'MESH' and o.name.startswith('SM_')]
root = bpy.data.objects['SM_Hull']
activate(root, *parts)
bpy.ops.object.convert(target='MESH')            # applies all modifiers, keeps custom normals
with bpy.context.temp_override(active_object=root, selected_editable_objects=parts, selected_objects=parts):
    bpy.ops.object.join()
activate(root)
bpy.ops.object.material_slot_remove_unused()     # drops empty slots left by booleans
bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
bpy.ops.export_scene.fbx(filepath=r'C:\path\SM_Ship.fbx', use_selection=True, object_types={'MESH'},
    apply_unit_scale=True, apply_scale_options='FBX_SCALE_UNITS', axis_forward='-Z', axis_up='Y',
    use_mesh_modifiers=True, mesh_smooth_type='FACE', use_tspace=True, use_triangles=False,
    add_leaf_bones=False, bake_anim=False, path_mode='AUTO')
```
FBX exporter defaults on 5.2.2 — verified headless on 5.2.2 ([API: export_scene.fbx](https://docs.blender.org/api/5.1/bpy.ops.export_scene.html#bpy.ops.export_scene.fbx)):
- `apply_scale_options='FBX_SCALE_NONE'` (options `'FBX_SCALE_NONE','FBX_SCALE_UNITS','FBX_SCALE_CUSTOM','FBX_SCALE_ALL'`)
- `mesh_smooth_type='OFF'` (`'OFF','FACE','EDGE','SMOOTH_GROUP'`)
- `use_tspace=False`, `add_leaf_bones=True`, `bake_anim=True`
- `axis_forward='-Z'`, `axis_up='Y'`
- `colors_type='SRGB'`, `use_triangles=False`
- `batch_mode` (`'OFF','SCENE','COLLECTION',...`)

`export_scene.gltf(filepath=..., export_format='GLB', use_selection=True)` also exported (152 KB) — verified headless on 5.2.2.

### Inferences
- Bevel widths and angles above (0.01–0.015 m on a 4 m ship, 30–40°) gave clean edges at 512 px. Bevel width is in scene units, so scale it to part size or use `offset_type='PERCENT'`.
- Keep cutters as hidden `display_type='WIRE'` objects in their own collection until export.
- Prefer `material_mode='TRANSFER'` on booleans.
- Put Triangulate (n-gons only) after Weighted Normal.
- Use `mesh_smooth_type='FACE'` and `use_tspace=True` for UE. The defaults `'OFF'` / `False` are rarely what a game exporter wants.
- `object.convert` is the most robust one-call "apply everything" in headless/MCP code. `modifier_apply` per modifier needs a correct context override.
- Inset/boolean geometry produces n-gons (9 on the test hull). Either triangulate or follow up with `bmesh.ops.triangulate` on faces with `len(f.verts) > 4`.

### Gaps
- UE5's import side was not tested: normal import method, scale (`FBX_SCALE_UNITS` vs scene unit scale) and axis conventions. Whether UE keeps Blender custom normals depends on UE import settings — see the Polycount caution.
- Bevel `face_strength_mode` + WN `use_face_influence` was not compared visually against plain Face Area weighting.
- Boolean `'MANIFOLD'` solver performance was not measured. Its introduction version (4.5?) was not confirmed from the fetched notes.

---

## 2. What are the exact current names/identifiers in Blender 5.1/5.2 (engines, Principled sockets, bake, UV ops, GN, Smooth by Angle, attributes)?

### Takeaway
On 5.2.2 the render engine enum is exactly `('BLENDER_EEVEE','BLENDER_WORKBENCH','CYCLES')`; `'BLENDER_EEVEE_NEXT'` raises. Principled BSDF uses the 4.0 names (`'Emission Color'`, `'Emission Strength'`, `'Coat Weight'`, `'Specular IOR Level'`, `'Transmission Weight'`…). GN group sockets are made with `ng.interface.new_socket(...)`. **New in 5.2:** GN modifier inputs are set via `getattr(mod.properties.inputs, identifier).value`; `mod[identifier]` now raises TypeError. Crease, sharp and bevel data live in the attributes `crease_edge`, `crease_vert`, `sharp_edge`, `sharp_face`, `bevel_weight_edge`, `bevel_weight_vert` and `uv_seam`.

### Cited Findings

#### Render engines and per-engine settings
- `scene.render.engine` valid values on 5.2.2: `('BLENDER_EEVEE', 'BLENDER_WORKBENCH', 'CYCLES')`. Setting `'BLENDER_EEVEE_NEXT'` raises `TypeError: bpy_struct: item.attr = val: enum "BLENDER_EEVEE_NEXT" not found in ('BLENDER_EEVEE', 'BLENDER_WORKBENCH', 'CYCLES')` — verified headless on 5.2.2. History:
  - 4.2 changed the id to `BLENDER_EEVEE_NEXT` — [4.2 Python API notes](https://developer.blender.org/docs/release_notes/4.2/python_api/).
  - 5.0 renamed it back to `BLENDER_EEVEE` — [5.0 Python API notes](https://developer.blender.org/docs/release_notes/5.0/python_api/).
- Version-robust helper (logic from the notes above): `eng = 'BLENDER_EEVEE_NEXT' if (4,2,0) <= bpy.app.version < (5,0,0) else 'BLENDER_EEVEE'`.
- **`scene.eevee` on 5.2.2** — verified headless on 5.2.2 ([API: SceneEEVEE](https://docs.blender.org/api/5.1/bpy.types.SceneEEVEE.html)):
  - `taa_render_samples` (default 64), `taa_samples` (viewport, 16)
  - `use_raytracing` (False), `ray_tracing_method` (`'PROBE','SCREEN'`)
  - `use_shadows`, `shadow_ray_count`, `shadow_step_count`, `shadow_resolution_scale`
  - `use_fast_gi`, `fast_gi_method` (`'AMBIENT_OCCLUSION_ONLY','GLOBAL_ILLUMINATION'`)
  - `gi_diffuse_bounces`, `volumetric_tile_size`, `use_volumetric_shadows`
  - **No** `use_bloom`, `use_gtao` or `use_ssr`. Bloom etc. were deprecated in 4.2 and removed in 4.3 ([4.3 Python API notes](https://developer.blender.org/docs/release_notes/4.3/python_api/)); `use_gtao` was removed in 5.0.
- **`scene.cycles` on 5.2.2** — verified headless on 5.2.2:
  - `device` (`'CPU','GPU'`), `samples` (default 4096), `preview_samples` (1024)
  - `use_denoising` (True), `denoiser` (`'OPENIMAGEDENOISE'`)
  - `use_adaptive_sampling`, `adaptive_threshold` (0.01), `max_bounces` (12), `time_limit`
  - `bake_type` (see Bake below)
- **Workbench, `scene.display.shading`** — verified headless on 5.2.2 ([API: View3DShading](https://docs.blender.org/api/5.1/bpy.types.View3DShading.html)):
  - `light`: `'STUDIO','MATCAP','FLAT'`
  - `color_type`: `'MATERIAL','OBJECT','RANDOM','VERTEX','TEXTURE','SINGLE'`. `'MATERIAL'` uses `Material.diffuse_color`, not the node colour.
  - `studio_light` is dynamic and depends on `light`:
    - STUDIO: `'Default','basic.sl','outdoor.sl','paint.sl','rim.sl','studio.sl'`
    - MATCAP: e.g. `'basic_bright.exr','basic_dark.exr','basic_grey.exr','basic_side.exr','ceramic_dark.exr','ceramic_lightbulb.exr','check_normal+y.exr','check_rim_dark.exr','check_rim_light.exr','clay_brown.exr','clay_studio.exr','hard_surface_grey.exr','hard_surface_red.exr','metal_carpaint.exr','toon_dark.exr','toon_light.exr'`
  - Cavity: `show_cavity`, `cavity_type` (`'WORLD','SCREEN','BOTH'`), `cavity_ridge_factor`, `cavity_valley_factor`, `curvature_ridge_factor`, `curvature_valley_factor`
  - Outline: `show_object_outline`, `object_outline_color`
  - `show_shadows`, `shadow_intensity`, `show_specular_highlight`, `show_xray`, `xray_alpha`, `single_color`
  - `scene.display.render_aa`: `'OFF','FXAA','5','8','11','16','32'` (default `'8'`)
- **Bundled world HDRIs:** `bpy.utils.system_resource('DATAFILES', path='studiolights/world')` contains `city.exr, courtyard.exr, forest.exr, interior.exr, night.exr, studio.exr, sunrise.exr, sunset.exr` — verified headless on 5.2.2.
- **Colour management** — verified headless on 5.2.2:
  - View transforms: `'Standard','ACES 1.3','ACES 2.0','Khronos PBR Neutral','AgX','Filmic','Filmic Log','False Color','Raw'` (default `'AgX'`).
  - Looks: `'None','AgX - Punchy','AgX - High Contrast'…`
  - Image colourspaces include `'sRGB','Non-Color','Linear Rec.709','ACEScg'`.

#### Principled BSDF and shader nodes (5.2.2)
- **Principled BSDF inputs, in order** (name == identifier for all) — verified headless on 5.2.2 ([API: ShaderNodeBsdfPrincipled](https://docs.blender.org/api/5.1/bpy.types.ShaderNodeBsdfPrincipled.html)):
  - `Base Color`, `Metallic`, `Roughness`, `IOR`, `Alpha`, `Thin Wall` (bool), `Normal`, `Weight`, `Diffuse Roughness`
  - `Subsurface Weight`, `Subsurface Radius`, `Subsurface Scale`, `Subsurface IOR`, `Subsurface Anisotropy`
  - `Specular IOR Level`, `Specular Tint` (colour), `Anisotropic`, `Anisotropic Rotation`, `Tangent`
  - `Transmission Weight`
  - `Coat Weight`, `Coat Roughness`, `Coat IOR`, `Coat Tint`, `Coat Normal`
  - `Sheen Weight`, `Sheen Roughness`, `Sheen Tint`
  - `Emission Color` (default white), `Emission Strength` (default 0.0)
  - `Thin Film Thickness`, `Thin Film IOR`
  - Output: `BSDF`. `distribution`: `'GGX','MULTI_GGX'` (default MULTI_GGX). `subsurface_method`: `'BURLEY','RANDOM_WALK','RANDOM_WALK_SKIN','RANDOM_WALK_LEGACY'`.
- **The 4.0 renames that still trip old code** — [4.0 Python API notes](https://developer.blender.org/docs/release_notes/4.0/python_api/):
  - `Subsurface`→`Subsurface Weight`
  - `Specular`→`Specular IOR Level`
  - `Transmission`→`Transmission Weight`
  - `Coat`→`Coat Weight`
  - `Sheen`→`Sheen Weight`
  - `Emission`→`Emission Color`
  - `Subsurface Color` removed
  - `Specular Tint` is now a colour
- **Other shader nodes on 5.2.2** — verified headless on 5.2.2:
  - Emission: inputs `Color`, `Strength`, `Weight` → output `Emission`.
  - Noise Texture: inputs `Vector, W, Scale, Detail, Roughness, Lacunarity, Offset, Gain, Distortion` → `Factor` (identifier `Fac`), `Color`; `noise_type` `'MULTIFRACTAL','RIDGED_MULTIFRACTAL','HYBRID_MULTIFRACTAL','FBM','HETERO_TERRAIN'`; `normalize`.
  - Mix (`ShaderNodeMix`): input identifiers `Factor_Float, Factor_Vector, A_Float, B_Float, A_Vector, B_Vector, A_Color, B_Color, A_Rotation, B_Rotation`; `data_type` `'FLOAT','VECTOR','RGBA','ROTATION'`.
  - Also available: `ShaderNodeLayerWeight` (Fresnel/Facing), `ShaderNodeBevel`, `ShaderNodeAmbientOcclusion`, `ShaderNodeWireframe`, `ShaderNodeLightPath` (`Is Camera Ray`…), `ShaderNodeBsdfToon`, `ShaderNodeShaderToRGB`.
  - `ShaderNodeTexMusgrave` is **undefined**, so use Noise with `noise_type`.
  - `ShaderNodeBsdfGlossy` creates a `ShaderNodeBsdfAnisotropic` node (merged Glossy/Anisotropic).
- **New materials and worlds come with nodes (5.x)** — verified headless on 5.2.2; [5.0 Python API notes](https://developer.blender.org/docs/release_notes/5.0/python_api/):
  - `bpy.data.materials.new()` already has `node_tree` with `Principled BSDF` + `Material Output`.
  - `bpy.data.worlds.new()` has `Background` + `World Output`.
  - Reading `Material.use_nodes` emits `'Material.use_nodes' is expected to be removed in Blender 6.0`. `AreaLight.use_nodes` gives the same warning.
- **Material properties on 5.2.2** — verified headless on 5.2.2:
  - `surface_render_method` (`'DITHERED','BLENDED'`)
  - `blend_method` still exists (deprecated, `'OPAQUE','CLIP','HASHED','BLEND'`)
  - `use_transparency_overlap`, `use_raytrace_refraction`, `use_backface_culling`, `use_backface_culling_shadow`
  - `displacement_method` (`'BUMP','DISPLACEMENT','BOTH'`), `thickness_mode`
  - `shadow_method` is gone (removed 4.3 — [4.3 notes](https://developer.blender.org/docs/release_notes/4.3/python_api/))

#### Bake identifiers (5.2.2)
- **`bpy.ops.object.bake` signature** — verified headless on 5.2.2; [API: bpy.ops.object.bake](https://docs.blender.org/api/5.1/bpy.ops.object.html#bpy.ops.object.bake):
  ```
  bake(type='COMBINED', pass_filter=set(), filepath="", width=512, height=512, margin=16,
       margin_type='EXTEND', use_selected_to_active=False, max_ray_distance=0.0,
       cage_extrusion=0.0, cage_object="", normal_space='TANGENT',
       normal_r='POS_X', normal_g='POS_Y', normal_b='POS_Z', target='IMAGE_TEXTURES',
       save_mode='INTERNAL', use_clear=False, use_cage=False, use_split_materials=False,
       use_automatic_name=False, uv_layer="")
  ```
- **Enum values for the operator:**
  - `type`: `'COMBINED','AO','SHADOW','POSITION','NORMAL','UV','ROUGHNESS','EMIT','ENVIRONMENT','DIFFUSE','GLOSSY','TRANSMISSION'`
  - `pass_filter`: `'NONE','EMIT','DIRECT','INDIRECT','COLOR','DIFFUSE','GLOSSY','TRANSMISSION'`
  - `margin_type`: `'ADJACENT_FACES','EXTEND'`
  - `normal_space`: `'OBJECT','TANGENT'`
  - `target`: `'IMAGE_TEXTURES','VERTEX_COLORS'`
- **`scene.render.bake` (BakeSettings) fields** ([API: BakeSettings](https://docs.blender.org/api/5.1/bpy.types.BakeSettings.html)):
  - `margin` (16), `margin_type` (scene default **`'ADJACENT_FACES'`**, while the operator default is `'EXTEND'`)
  - `cage_extrusion`, `max_ray_distance`, `use_cage`, `cage_object` (pointer)
  - `use_selected_to_active`, `use_clear` (scene default True)
  - `normal_space`, `target`, `save_mode`, `view_from` (`'ABOVE_SURFACE','ACTIVE_CAMERA'`)
  - `use_pass_emit/direct/indirect/color/diffuse/glossy/transmission`
  - **Trap:** `scene.render.bake.type` is the *multires* baker enum (`'NORMALS','DISPLACEMENT','VECTOR_DISPLACEMENT'`), not the Cycles bake type. Cycles uses `scene.cycles.bake_type`, which has the same values as the operator `type`.
- **What Cage, Cage Extrusion, Max Ray Distance and margin mean** ([Manual: Baking](https://docs.blender.org/manual/en/latest/render/cycles/baking.html)):
  - Cage Extrusion is "Distance to use for the inward ray cast when using *Selected to Active* and *Cage*".
  - Max Ray Distance is only available when not using a cage.
  - A cage object must have the same topology as the low poly.
  - Margin type is *Extend* (extend border pixels) or *Adjacent Faces* (fill from faces across seams).

#### UV operators (5.2.2; edit mode, faces selected) — verified headless on 5.2.2 ([API: bpy.ops.uv](https://docs.blender.org/api/5.1/bpy.ops.uv.html))
- **`uv.smart_project`:**
  - `angle_limit` is **radians**: default 1.1519 (66°), hard range [0, π/2]
  - `margin_method` (`'SCALED','ADD','FRACTION'`), `rotate_method` (`'AXIS_ALIGNED','AXIS_ALIGNED_X','AXIS_ALIGNED_Y'`)
  - `island_margin` (0–1), `area_weight`, `correct_aspect`, `scale_to_bounds`
- **`uv.unwrap`:** `method` (`'ANGLE_BASED','CONFORMAL','MINIMUM_STRETCH'`, default `'CONFORMAL'`), `fill_holes` (False), `correct_aspect`, `use_subsurf_data`, `margin_method`, `margin` (0.001), `no_flip`, `iterations` (10), `use_weights`, `weight_group`, `weight_factor`.
- **`uv.pack_islands`:**
  - `udim_source`, `rotate` (True), `rotate_method` (`'ANY','CARDINAL','AXIS_ALIGNED','AXIS_ALIGNED_X','AXIS_ALIGNED_Y'`), `scale` (True)
  - `merge_overlap`, `margin_method`, `margin` (0.001), `pin`, `pin_method`
  - `shape_method` (`'CONCAVE','CONVEX','AABB'`)
- **`uv.lightmap_pack`:** `PREF_CONTEXT` (`'SEL_FACES','ALL_FACES'`), `PREF_PACK_IN_ONE`, `PREF_NEW_UVLAYER`, `PREF_BOX_DIV` (12), `PREF_MARGIN_DIV` (0.1).
- **Others:** `uv.cube_project(cube_size, correct_aspect, clip_to_bounds, scale_to_bounds)`, `uv.average_islands_scale(scale_uv, shear)`, `uv.seams_from_islands(mark_seams, mark_sharp)`, `uv.follow_active_quads(mode)`.
- **Island Margin semantics:** with *Fraction*, margin is a fraction of UV space, so 1/1024 = 1 px on a 1024 map — [Manual: UV](https://docs.blender.org/manual/en/latest/modeling/meshes/editing/uv.html).

#### Geometry Nodes from Python (5.2.2)
- **Creating groups and sockets** — verified headless on 5.2.2 ([API: NodeTreeInterface](https://docs.blender.org/api/5.1/bpy.types.NodeTreeInterface.html); [4.0 Python API notes](https://developer.blender.org/docs/release_notes/4.0/python_api/); `parent` was removed from `new_panel` in 4.2 — [4.2 notes](https://developer.blender.org/docs/release_notes/4.2/python_api/)):
  - Group: `bpy.data.node_groups.new(name, 'GeometryNodeTree')`; Python-made groups have `is_modifier == False`.
  - Socket: `ng.interface.new_socket(name, description="", in_out='INPUT', socket_type='DEFAULT', parent=None)`.
  - Panel: `ng.interface.new_panel(name, description="", default_closed=False)`.
  - Iterate `ng.interface.items_tree`; each item has `item_type` `'SOCKET'`/`'PANEL'`, `identifier` (`'Socket_0'`, `'Socket_1'`…), `in_out` and `socket_type`.
  - `ng.inputs` / `ng.outputs` and the `NodeSocketInterface*` types no longer exist.
- **Valid `socket_type` values (5.2.2):** `'NodeSocketFloat','NodeSocketInt','NodeSocketBool','NodeSocketVector','NodeSocketColor','NodeSocketRotation','NodeSocketMatrix','NodeSocketString','NodeSocketMenu','NodeSocketGeometry','NodeSocketObject','NodeSocketCollection','NodeSocketImage','NodeSocketMaterial','NodeSocketFont','NodeSocketSound','NodeSocketBundle','NodeSocketClosure'` — verified headless on 5.2.2 (enum error message).
- **Modifier inputs in 5.2** — [5.2 Python API notes](https://developer.blender.org/docs/release_notes/5.2/python_api/); verified headless on 5.2.2:
  - **New API:**
    - Value: `getattr(mod.properties.inputs, 'Socket_2').value = 3.0`.
    - Attribute toggle: `.type` (`'VALUE','ATTRIBUTE'`).
    - Attribute name: `.attribute_name`.
    - Outputs: `mod.properties.outputs.<id>.attribute_name`.
  - **Verified behaviour:**
    - Object inputs accept `.value = obj`; vector inputs accept tuples.
    - Evaluation reflected the new value (cube Z offset by 3 gave z ∈ [2, 4]).
    - `mod['Socket_2'] = x` raises `TypeError: bpy_struct[key] = val: id properties not supported for this type`.
    - `mod.keys()` raises `TypeError: bpy_struct.keys(): this type doesn't support IDProperties`.
    - `mod.properties.inputs['Socket_2']` returns an `IDPropertyGroup` **without** `.value`, so always use `getattr`.
- **Node bl_idnames that exist on 5.2.2**, with their input names — verified headless on 5.2.2:
  - `GeometryNodeDistributePointsOnFaces`: `Mesh, Selection, Distance Min, Density Max, Density, Density Factor, Seed` → `Points, Normal, Rotation`; `distribute_method` `'RANDOM','POISSON'`
  - `GeometryNodeInstanceOnPoints`: `Points, Selection, Instance, Pick Instance, Instance Index, Rotation, Scale` → `Instances`
  - `GeometryNodeRealizeInstances`: `Geometry, Selection, Realize All, Depth`
  - `GeometryNodeObjectInfo`: `Object, As Instance` → `Transform, Location, Rotation, Scale, Geometry`; `transform_space` `'ORIGINAL','RELATIVE'`
  - `GeometryNodeCollectionInfo`: `Collection, Separate Children, Reset Children` → `Instances`
  - `FunctionNodeRandomValue`: `Min, Max, ID, Seed` (`Probability, ID, Seed` for BOOLEAN)
  - `GeometryNodeJoinGeometry`
  - `GeometryNodeTransform`: `Geometry, Mode`(menu), `Translation, Rotation`(rotation socket), `Scale, Transform`(matrix)
  - `GeometryNodeSetPosition`: `Geometry, Selection, Position, Offset`
  - `GeometryNodeSetMaterial`, `GeometryNodeSetShadeSmooth` (`domain` `'EDGE','FACE'`), `GeometryNodeInputMeshEdgeAngle`
  - `GeometryNodeMeshBoolean`: `Mesh 1, Mesh 2, Self Intersection, Hole Tolerant`; solver `'EXACT','FLOAT','MANIFOLD'` (node default `'FLOAT'`)
  - `GeometryNodeSubdivisionSurface`: `Mesh, Level, Edge Crease, Vertex Crease, Limit Surface, Quality, UV Smooth, Boundary Smooth`
  - `GeometryNodeExtrudeMesh`, `GeometryNodeScaleElements`, `GeometryNodeMergeByDistance`, `GeometryNodeDualMesh`, `GeometryNodeConvexHull`
  - `GeometryNodeMeshToVolume`, `GeometryNodeVolumeToMesh`
  - `GeometryNodeTriangulate`: `Quad Method`/`N-gon Method` are menu sockets
  - `GeometryNodeStoreNamedAttribute`, `GeometryNodeInputNamedAttribute`, `GeometryNodeSwitch`, `GeometryNodeIndexSwitch`, `GeometryNodeMenuSwitch`
  - `GeometryNodeRepeatInput/Output`, `GeometryNodeForeachGeometryElementInput/Output`
  - `FunctionNodeAlignRotationToVector` (`axis`, `pivot_axis`); legacy `FunctionNodeAlignEulerToVector` still exists
  - `FunctionNodeRotateRotation`, `FunctionNodeEulerToRotation`
  - `GeometryNodeUVUnwrap` (`Method` menu), **`GeometryNodeUVPackIslands`**, **`GeometryNodeCurvePrimitiveQuadrilateral`**, `GeometryNodeSetMeshNormal`, `GeometryNodeRaycast`, `GeometryNodeProximity`, `GeometryNodeSampleNearestSurface`
  - Shared math nodes: `ShaderNodeMath` (identifiers `Value, Value_001, Value_002`), `ShaderNodeVectorMath`, `ShaderNodeMapRange`, `ShaderNodeTexNoise`, `ShaderNodeValToRGB`
- **Undefined names** that LLMs guess: `GeometryNodeCurveQuadrilateral`, `GeometryNodePackUVIslands`, `GeometryNodeLegacyDistributePointsOnFaces`, `GeometryNodeAttributeTransfer`, `GeometryNodeSmoothByAngle` — verified headless on 5.2.2.
- **Dynamic sockets in 5.2.2:** Random Value has the **same identifiers `Min`/`Max`** whose socket type follows `data_type` (FLOAT → VALUE, FLOAT_VECTOR → VECTOR, INT → INT); BOOLEAN shows `Probability`. Compare has only `A`/`B`, and their type follows `data_type` — verified headless on 5.2.2. The 5.2 notes say the identifiers "changed on the Compare and Random Value nodes" ([5.2 notes](https://developer.blender.org/docs/release_notes/5.2/python_api/)).
- **Address sockets by identifier, not index:** since 4.1, index-based socket access on dynamic nodes is unreliable — [4.1 Python API notes](https://developer.blender.org/docs/release_notes/4.1/python_api/).

#### Smooth by Angle, normals and mesh attributes (5.2.2)
- **Smooth by Angle names:** the operator is `bpy.ops.object.shade_auto_smooth`; the modifier and the node group are both named "Smooth by Angle"; the angle socket is `Input_1`; `shade_smooth_by_angle` and `set_sharpness_by_angle` are the no-modifier alternatives. The `bpy.ops.object.modifier_add_node_group(asset_library_type, asset_library_identifier, relative_asset_identifier, session_uid, use_selected_objects)` operator also exists, with `asset_library_type` `'ALL','LOCAL','ESSENTIALS','ONLINE_ESSENTIALS','CUSTOM'` — verified headless on 5.2.2 (details in §1).
- **Attributes written by bmesh on 5.2.2** — verified headless on 5.2.2:
  - `crease_edge` (EDGE FLOAT) and `bevel_weight_edge` (EDGE FLOAT), from `bm.edges.layers.float.new(name)`
  - `sharp_edge` (EDGE BOOLEAN), from `BMEdge.smooth = False`
  - **`uv_seam`** (EDGE BOOLEAN), from `BMEdge.seam = True`
  - `sharp_face` (FACE BOOLEAN), from flat faces / `shade_flat`
  - The vertex counterparts `crease_vert` and `bevel_weight_vert` are named in the [4.0 notes](https://developer.blender.org/docs/release_notes/4.0/python_api/). `bevel_weight_vert` is also the Bevel modifier's `vertex_weight` default on 5.2.2.
  - Internal attributes start with `.` (`.edge_verts`, `.corner_vert`, `.corner_edge`, `.select_vert`, `.uv_select_vert`…).
- **Removed vs kept on 5.2.2** — verified headless on 5.2.2:
  - Removed: `MeshEdge.crease`, `MeshEdge.bevel_weight`, `MeshVertex.bevel_weight`, `bm.edges.layers.crease` and `bm.edges.layers.bevel_weight`.
  - Kept: `MeshEdge.use_edge_sharp` and `MeshEdge.use_seam`.
  - `Mesh.edge_creases` returns a `FloatAttribute` named `crease_edge`; `edge_creases_ensure()` creates it, and `.data.foreach_set('value', [...])` works.
- **Normals API:** `Mesh.corner_normals`, `vertex_normals`, `polygon_normals`, `normals_domain` (`'POINT'`, `'FACE'` or `'CORNER'` were observed), `has_custom_normals`, `normals_split_custom_set(...)` and `normals_split_custom_set_from_vertices(...)` — verified headless on 5.2.2.

#### Exporters / importers present with `--factory-startup` on 5.2.2 — verified headless on 5.2.2
- **Present:** `export_scene.fbx`, `export_scene.gltf`, `wm.obj_export`, `wm.obj_import`, `wm.stl_export`, `wm.stl_import`, `wm.ply_export`, `wm.usd_export`, `wm.alembic_export`, `import_scene.fbx`, `import_scene.gltf`, `image.import_as_mesh_planes`.
- **Absent:** `export_scene.obj`, `import_scene.obj`, `export_mesh.stl`, `import_mesh.stl`, `export_mesh.ply`, `import_image.to_plane`.
- **Enabled add-ons:** `bl_pkg, cycles, io_anim_bvh, io_curve_svg, io_mesh_uv_layout, io_scene_fbx, io_scene_gltf2, pose_library`.
- **Bundled but disabled:** `node_wrangler, rigify, hydra_storm, ui_translate, viewport_vr_preview`.
- **Not available:** Bool Tool (`object.booltool_auto_difference`), LoopTools (`mesh.looptools_relax`) and Extra Objects (`mesh.primitive_round_cube_add`).
- The OBJ/PLY operator renames are in the [4.0 notes](https://developer.blender.org/docs/release_notes/4.0/python_api/).
- The 4.2 notes describe the switch from `bl_info` to `blender_manifest.toml` extensions ([4.2 notes](https://developer.blender.org/docs/release_notes/4.2/python_api/)). The per-add-on list of what left the bundle was not retrievable, so the absent/present lists above come from the 5.2.2 check.

### Inferences
- Agents should never hard-code `BLENDER_EEVEE_NEXT` on 5.x. Use `'BLENDER_EEVEE'`, or the version helper if the skill must also run on 4.2–4.5.
- Always look up GN sockets by `item.name` → `item.identifier` from `ng.interface.items_tree`, then use the 5.2 `properties.inputs` API. The identifiers (`Socket_N`) depend on creation order, including output sockets.
- When an enum is dynamic (render engine, studio_light, colourspace), the fastest way to learn the valid values at runtime is to assign an invalid string and read the TypeError text, which lists every valid item (technique used throughout this verification).

### Gaps
- The 5.1 docs were used for URLs; the 5.2 online API pages were not fetched. All names above were checked on 5.2.2 itself.
- Cycles GPU device setup (`preferences.addons['cycles'].preferences.compute_device_type`, `get_devices()`) was not tested. Bakes and renders here used CPU.
- The exact previous (≤5.1) identifiers of Random Value/Compare sockets were not checked on an older build. From memory they were suffixed `_001`, `_002` per type — unverified.

---

## 3. How do you script procedural asteroids, rocks and crystals, and Geometry Nodes scattering, in 5.2?

### Takeaway
Rock recipe verified on 5.2.2: ico sphere (subdivisions 5) → two Displace modifiers driven by legacy `CLOUDS` textures (one broad, one with `noise_basis='VORONOI_F1'` for facets) → `REMESH` voxel → `DECIMATE` collapse → `shade_auto_smooth`. The result is a watertight ~2k-tri asteroid. Crystals are a 6-sided `bmesh.ops.create_cone` poked at the top. The GN scatter verified on 5.2.2 is Distribute Points on Faces (Poisson) → Instance on Points (`Object Info` as instance, `Rotation` from the distribute node, Random Value scale) → Realize → Join. Its inputs must be set via the 5.2 `mod.properties.inputs` API.

### Cited Findings
- **Legacy texture types for Displace** on 5.2.2: `'NONE','BLEND','CLOUDS','DISTORTED_NOISE','IMAGE','MAGIC','MARBLE','MUSGRAVE','NOISE','STUCCI','VORONOI','WOOD'`. The `MUSGRAVE` texture data-block still exists even though the *shader node* was removed.
  - CLOUDS fields: `noise_scale` (0.25), `noise_depth` (2), `noise_type` (`'SOFT_NOISE','HARD_NOISE'`), `noise_basis` (`'BLENDER_ORIGINAL','ORIGINAL_PERLIN','IMPROVED_PERLIN','VORONOI_F1'…'VORONOI_CRACKLE','CELL_NOISE'`), `intensity`, `contrast`, `nabla`.
  - Verified headless on 5.2.2.
- **Displace, Remesh and Decimate on 5.2.2** — verified headless on 5.2.2 ([API: DisplaceModifier](https://docs.blender.org/api/5.1/bpy.types.DisplaceModifier.html), [RemeshModifier](https://docs.blender.org/api/5.1/bpy.types.RemeshModifier.html), [DecimateModifier](https://docs.blender.org/api/5.1/bpy.types.DecimateModifier.html)):
  - **Displace:** `strength` (1.0), `mid_level` (0.5), `direction` (`'X','Y','Z','NORMAL','CUSTOM_NORMAL','RGB_TO_XYZ'`), `space` (`'LOCAL','GLOBAL'`), `texture_coords` (`'LOCAL','GLOBAL','OBJECT','UV'`), `texture_coords_object`, `texture`.
  - **Remesh:** `mode` (`'BLOCKS','SMOOTH','SHARP','VOXEL'`, default VOXEL), `voxel_size` (0.1), `adaptivity` (0), `use_remove_disconnected` (True), `use_smooth_shade` (False).
  - **Decimate:** `decimate_type` (`'COLLAPSE','UNSUBDIV','DISSOLVE'`), `ratio`, `iterations`, `angle_limit`, `use_collapse_triangulate`, `use_symmetry`, `symmetry_axis`, `delimit`, `face_count` (read-only).
- **Verified rock result:** Displace 0.6 + 0.15, voxel 0.05, decimate 0.12 gave 980 verts / 1,956 tris, **0 non-manifold edges**, ~2.1–2.2 m across. With crystals scattered (Poisson density max 4, distance min 0.25, realized) it reached 2,902 tris with materials `M_Crystal` and `M_Rock`. Both rendered correctly in EEVEE (crystals glowing) — verified headless on 5.2.2.
- **Voxel remesh alternatives:** `bpy.ops.object.voxel_remesh` and `bpy.ops.object.quadriflow_remesh` exist on 5.2.2 — verified headless on 5.2.2.
- **Crystal:** `bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=6, radius1=0.12, radius2=0.09, depth=0.8, matrix=...)` then `bmesh.ops.poke(bm, faces=[top], offset=0.25)` gives a 13-vert hex crystal with a pointed tip — verified headless on 5.2.2 (signatures from [API: bmesh.ops](https://docs.blender.org/api/5.1/bmesh.ops.html)).
- **bmesh pitfall:** indexing `bm.edges[i]` after creation ops raises `IndexError: BMElemSeq[index]: outdated internal index table, run ensure_lookup_table() first` — verified headless on 5.2.2. Call `bm.verts/edges/faces.ensure_lookup_table()` before indexing.

#### Copy-ready: asteroid / rock (verified headless on 5.2.2)
```python
import bpy, math
def make_asteroid(name='SM_Asteroid', loc=(0, 0, 0), radius=1.0, big=0.6, facets=0.15,
                  voxel=0.05, ratio=0.12, seed_offset=(0, 0, 0)):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=5, radius=radius, location=loc)
    rock = bpy.context.active_object; rock.name = name
    t_big = bpy.data.textures.new(name + '_Big', 'CLOUDS'); t_big.noise_scale = 0.9; t_big.noise_depth = 2
    t_fac = bpy.data.textures.new(name + '_Facets', 'CLOUDS')
    t_fac.noise_basis = 'VORONOI_F1'; t_fac.noise_scale = 0.35     # chunky facets; try 'VORONOI_CRACKLE'
    d1 = rock.modifiers.new('DisplaceBig', 'DISPLACE'); d1.texture = t_big; d1.strength = big
    d2 = rock.modifiers.new('DisplaceFacets', 'DISPLACE'); d2.texture = t_fac; d2.strength = facets
    rm = rock.modifiers.new('Remesh', 'REMESH'); rm.mode = 'VOXEL'; rm.voxel_size = voxel
    dec = rock.modifiers.new('Decimate', 'DECIMATE'); dec.decimate_type = 'COLLAPSE'; dec.ratio = ratio
    bpy.ops.object.shade_auto_smooth(angle=math.radians(40))      # active object = rock
    return rock
```

#### Copy-ready: GN crystal scatter (verified headless on 5.2.2)
```python
import bpy
def build_scatter_group(name='GN_Scatter'):
    ng = bpy.data.node_groups.new(name, 'GeometryNodeTree'); ng.is_modifier = True
    itf = ng.interface
    itf.new_socket(name='Geometry', in_out='INPUT', socket_type='NodeSocketGeometry')
    itf.new_socket(name='Geometry', in_out='OUTPUT', socket_type='NodeSocketGeometry')
    s = itf.new_socket(name='Density', in_out='INPUT', socket_type='NodeSocketFloat'); s.default_value = 6.0; s.min_value = 0.0
    itf.new_socket(name='Instance', in_out='INPUT', socket_type='NodeSocketObject')
    itf.new_socket(name='Seed', in_out='INPUT', socket_type='NodeSocketInt')
    s = itf.new_socket(name='Scale Min', in_out='INPUT', socket_type='NodeSocketFloat'); s.default_value = 0.4
    s = itf.new_socket(name='Scale Max', in_out='INPUT', socket_type='NodeSocketFloat'); s.default_value = 1.2
    N, L = ng.nodes, ng.links
    gi, go = N.new('NodeGroupInput'), N.new('NodeGroupOutput')
    dist = N.new('GeometryNodeDistributePointsOnFaces'); dist.distribute_method = 'POISSON'
    dist.inputs['Distance Min'].default_value = 0.25
    oi = N.new('GeometryNodeObjectInfo'); oi.inputs['As Instance'].default_value = True
    rv = N.new('FunctionNodeRandomValue'); rv.data_type = 'FLOAT'
    iop = N.new('GeometryNodeInstanceOnPoints')
    real = N.new('GeometryNodeRealizeInstances')
    join = N.new('GeometryNodeJoinGeometry')
    L.new(gi.outputs['Geometry'], dist.inputs['Mesh'])
    L.new(gi.outputs['Density'], dist.inputs['Density Max'])     # POISSON uses 'Density Max'; RANDOM uses 'Density'
    L.new(gi.outputs['Seed'], dist.inputs['Seed'])
    L.new(gi.outputs['Instance'], oi.inputs['Object'])
    L.new(gi.outputs['Scale Min'], rv.inputs['Min']); L.new(gi.outputs['Scale Max'], rv.inputs['Max'])
    L.new(gi.outputs['Seed'], rv.inputs['Seed'])
    L.new(dist.outputs['Points'], iop.inputs['Points'])
    L.new(dist.outputs['Rotation'], iop.inputs['Rotation'])      # aligned to surface normal
    L.new(oi.outputs['Geometry'], iop.inputs['Instance'])
    L.new(rv.outputs['Value'], iop.inputs['Scale'])
    L.new(iop.outputs['Instances'], real.inputs['Geometry'])
    L.new(gi.outputs['Geometry'], join.inputs['Geometry'])
    L.new(real.outputs['Geometry'], join.inputs['Geometry'])
    L.new(join.outputs['Geometry'], go.inputs['Geometry'])
    return ng

def set_gn_input(mod, name, value):
    ident = next(it.identifier for it in mod.node_group.interface.items_tree
                 if it.item_type == 'SOCKET' and it.in_out == 'INPUT' and it.name == name)
    if bpy.app.version >= (5, 2, 0):
        getattr(mod.properties.inputs, ident).value = value      # 5.2+ (verified)
    else:
        mod[ident] = value                                       # <=5.1 IDProperty API (per 5.2 notes; not re-tested)

rock = bpy.data.objects['SM_Asteroid']; crystal = bpy.data.objects['SM_Crystal']
mod = rock.modifiers.new('CrystalScatter', 'NODES'); mod.node_group = build_scatter_group()
set_gn_input(mod, 'Instance', crystal); set_gn_input(mod, 'Density', 4.0); set_gn_input(mod, 'Seed', 7)
```

### Inferences
- **Per-asteroid variety:**
  - Change `noise_basis`/`noise_scale`, or switch Displace to `texture_coords='OBJECT'` with a randomly placed empty in `texture_coords_object` (the property exists; varying it was not tested).
  - Also randomize non-uniform object scale before applying.
- **Voxel size:** about radius/20 kept facets crisp at 2 m. Smaller voxels raise the pre-decimate cost, so tune `ratio` to hit a tri budget (the test hit ~2k tris).
- **Alien flora:** the stem nodes `GeometryNodeCurvePrimitiveLine` (`Start, End, Direction, Length`), `GeometryNodeResampleCurve` (`Curve, Selection, Mode, Count, Length`), `GeometryNodeSetCurveRadius`, `GeometryNodeCurvePrimitiveCircle` (`Resolution, Point 1-3, Radius`) and `GeometryNodeCurveToMesh` (`Curve, Profile Curve, Scale, Fill Caps`) exist with those sockets on 5.2.2. A stem graph built from them was **not tested**.
- **Glowing tips and crystals:** use the emissive material recipe in §4.

### Gaps
- GN-based rock displacement (`ShaderNodeTexNoise` → Set Position along Normal) was not run end-to-end. Only the node and socket names were verified.
- Performance of `REMESH` with much smaller voxels, and for the giant mothership scale, was not measured.

---

## 4. What material, UV and baking recipes work on 5.2 (emission glow, saturated stylized looks, unwrap/pack, Cycles bake)?

### Takeaway
On 5.x a new material already has a Principled BSDF, so set `inputs['Base Color' | 'Metallic' | 'Roughness' | 'Coat Weight' | 'Emission Color' | 'Emission Strength']`. Also set `Material.diffuse_color` so Workbench previews and viewports show the same colour. Don't touch `use_nodes`.

**UVs:** `uv.smart_project` (angle in radians) + `uv.pack_islands` both run headless in edit mode.

**Baking (Cycles, CPU OK):** needs UVs on the active object and an image node set as `node_tree.nodes.active`. Selected-to-active normal bakes with `cage_extrusion`/`max_ray_distance`, plus AO, DIFFUSE(COLOR) and EMIT bakes, each finished in about 0.1 s at 256² on 5.2.2. Hide the high-poly (`hide_render=True`) for self-bakes, or it occludes them.

### Cited Findings
- **Verified material helper** — verified headless on 5.2.2; [5.0 Python API notes](https://developer.blender.org/docs/release_notes/5.0/python_api/) (`material.use_nodes` deprecated, always True):
  - It sets `Base Color` (RGBA tuple), `Metallic`, `Roughness`, `Coat Weight`, `Emission Color` and `Emission Strength`.
  - It falls back to `use_nodes = True` only when `node_tree is None` (pre-5.0).
  - The rendered result was saturated magenta/yellow hulls and cyan engine glow (strength 25) with the Standard view transform.
- **Glow/bloom in 5.x is a compositor job** — verified headless on 5.2.2 (render shows bloom); [5.0 Python API notes](https://developer.blender.org/docs/release_notes/5.0/python_api/):
  - Create a `CompositorNodeTree` group, add an output socket via `interface.new_socket(... 'OUTPUT', 'NodeSocketColor')`, wire `CompositorNodeRLayers` → `CompositorNodeGlare` → `NodeGroupOutput`, and assign it to `scene.compositing_node_group`.
  - All Glare settings are input sockets (`Image, Type, Quality, Threshold`(id `Highlights Threshold`)`, Smoothness, Clamp, Maximum, Strength, Saturation, Tint, Size, Streaks, …`). `Type` is a MENU socket (default `'Streaks'`); set `inputs['Type'].default_value = 'Bloom'`.
  - The old `glare_type`, `quality`, `threshold`, `size` and `mix` properties are gone.
  - `scene.node_tree` and `CompositorNodeComposite` do not exist on 5.2.2.
- **UV unwrap and lightmap pack run headless.** `smart_project(angle_limit=radians(66), island_margin=0.02)` + `pack_islands(rotate=True, margin=0.02, shape_method='CONCAVE')` on the hull and `lightmap_pack(PREF_CONTEXT='ALL_FACES', PREF_PACK_IN_ONE=True, PREF_BOX_DIV=12, PREF_MARGIN_DIV=0.2)` into a second UV layer both returned FINISHED in background — verified headless on 5.2.2.
- **Bakes on a 292-face low poly from a 1,871-face high poly** (256², Cycles CPU, 16 samples) — verified headless on 5.2.2:

  | Bake | Time |
  |---|---|
  | `type='NORMAL'`, `use_selected_to_active=True`, `cage_extrusion=0.05`, `max_ray_distance=0.2`, `normal_space='TANGENT'`, `margin=8`, `margin_type='EXTEND'` | 0.07 s |
  | `'AO'` | 0.12 s |
  | `'DIFFUSE'` with `pass_filter={'COLOR'}` (albedo) | 0.10 s |
  | `'EMIT'` | FINISHED |

  Images were saved with `img.filepath_raw=...; img.file_format='PNG'; img.save()`.
- **Hide the high-poly for self-bakes:** for AO/DIFFUSE/EMIT bakes that are not selected-to-active, any other renderable object still occludes. A low-poly plane under a cube averaged AO 0.576, vs 1.0 with the cube's `hide_render = True`. Set `high.hide_render = True` for those bakes; `object.bake` respects it — verified headless on 5.2.2.
- **Image creation:** `bpy.data.images.new(name, width, height, alpha=False, float_buffer=False, stereo3d=False, is_data=False, tiled=False)`. Use `is_data=True` plus `colorspace_settings.name='Non-Color'` for normal/AO/roughness data — verified headless on 5.2.2.
- **Cage/extrusion semantics, and the matching-topology requirement for cage objects:** [Manual: Baking](https://docs.blender.org/manual/en/latest/render/cycles/baking.html).

#### Copy-ready: materials (verified headless on 5.2.2)
```python
def make_mat(name, base, metallic=0.0, rough=0.4, emission=None, strength=0.0, coat=0.0):
    m = bpy.data.materials.new(name)
    if m.node_tree is None:            # only needed before 5.0
        m.use_nodes = True
    b = m.node_tree.nodes.get('Principled BSDF')
    b.inputs['Base Color'].default_value = (*base, 1.0)
    b.inputs['Metallic'].default_value = metallic
    b.inputs['Roughness'].default_value = rough
    b.inputs['Coat Weight'].default_value = coat
    if emission:
        b.inputs['Emission Color'].default_value = (*emission, 1.0)
        b.inputs['Emission Strength'].default_value = strength
    m.diffuse_color = (*(emission or base), 1.0)   # Workbench 'MATERIAL' colour + viewport
    return m
paint = make_mat('M_Paint_Magenta', (0.95, 0.05, 0.45), metallic=0.2, rough=0.35, coat=0.5)
trim  = make_mat('M_Trim_Yellow', (1.0, 0.75, 0.0), metallic=0.6, rough=0.3)
glow  = make_mat('M_EngineGlow', (0, 0, 0), emission=(0.0, 0.9, 1.0), strength=25.0)
```

#### Copy-ready: bloom compositor for glow previews (verified headless on 5.2.2)
```python
def setup_glare_compositor(scene, strength=0.8):
    ng = bpy.data.node_groups.new('PreviewComp', 'CompositorNodeTree')
    ng.interface.new_socket(name='Image', in_out='OUTPUT', socket_type='NodeSocketColor')
    rl = ng.nodes.new('CompositorNodeRLayers'); gl = ng.nodes.new('CompositorNodeGlare'); go = ng.nodes.new('NodeGroupOutput')
    gl.inputs['Type'].default_value = 'Bloom'          # MENU socket in 5.x
    gl.inputs['Strength'].default_value = strength
    ng.links.new(rl.outputs['Image'], gl.inputs['Image']); ng.links.new(gl.outputs['Image'], go.inputs['Image'])
    scene.compositing_node_group = ng                  # 5.0+: scene.node_tree is gone
    scene.render.use_compositing = True
```

#### Copy-ready: UV + bake (verified headless on 5.2.2)
```python
import bpy, math, os
def uv_unwrap(ob, margin=0.02):
    activate(ob)                                       # helper from §1
    bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=margin)   # RADIANS, max pi/2
    bpy.ops.uv.pack_islands(rotate=True, margin=margin, shape_method='CONCAVE')
    bpy.ops.object.mode_set(mode='OBJECT')

def bake_maps(low, high, out_dir, size=1024, extrusion=0.05, ray=0.2):
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'; scene.cycles.device = 'CPU'; scene.cycles.samples = 16
    mat = low.active_material
    node = mat.node_tree.nodes.new('ShaderNodeTexImage')
    mat.node_tree.nodes.active = node                  # bake writes into the ACTIVE image node
    out = {}
    for kind, kw, is_data in (('NORMAL', dict(use_selected_to_active=True, cage_extrusion=extrusion,
                                               max_ray_distance=ray, normal_space='TANGENT'), True),
                              ('AO', {}, True),
                              ('DIFFUSE', dict(pass_filter={'COLOR'}), False),
                              ('EMIT', {}, False)):
        img = bpy.data.images.new(f'{low.name}_{kind}', size, size, alpha=False, is_data=is_data)
        if is_data: img.colorspace_settings.name = 'Non-Color'
        node.image = img
        s2a = bool(kw.get('use_selected_to_active'))
        high.hide_render = not s2a                     # a visible high-poly OCCLUDES self-bakes (AO 0.58 vs 1.0)
        if s2a: activate(low, high)                    # high selected, low active
        else: activate(low)
        bpy.ops.object.bake(type=kind, margin=8, margin_type='EXTEND', use_clear=True, **kw)
        high.hide_render = False
        img.filepath_raw = os.path.join(out_dir, img.name + '.png'); img.file_format = 'PNG'; img.save()
        out[kind] = img.filepath_raw
    mat.node_tree.nodes.remove(node)                   # don't leave the bake node active in the material
    return out
```
Only part of this has been run as written:
- The test ran the same calls inline: NORMAL with the parameters shown (`max_ray_distance=0.2`), then AO, DIFFUSE(COLOR) and EMIT. Only the loop structure here is new.
- The `hide_render` toggle was added after a separate check on 5.2.2. A low-poly plane's AO bake averaged **0.576 with a visible occluder vs 1.0 with `occluder.hide_render = True`**, so `hide_render` is respected by `object.bake`. The pipeline test had left the high-poly visible during its AO/albedo bakes.

### Inferences
- **Colour management for checks:** for stylized saturated reviews in Blender, render previews with `view_settings.view_transform='Standard'` (used in all verified renders). The 5.2 default is AgX, whose highlight handling makes saturated colours look less punchy. That AgX behaviour was not A/B tested here.
- **UE5 colour:** UE5 applies its own tonemapper, so treat Blender colour checks as relative.
- **Emission across export:** emission *strength* and bloom will not carry into UE5 through FBX in a usable way. Recreate emissive materials in UE using the baked EMIT map or a parameterised emissive colour (not tested).
- **Bake defaults:** pass every bake parameter explicitly. The scene `BakeSettings` defaults differ from the operator defaults (`margin_type`, `use_clear`), and operator arguments not passed may fall back to scene settings (not tested).

### Gaps
- GPU (OptiX/CUDA) baking was not tested.
- Bakes larger than 256² were not timed.
- A bake with an explicit `cage_object` was not tested.
- How `margin` interacts with island spacing at 2K was not checked.
- How the FBX exporter maps Principled emission and the coat inputs into UE materials was not verified.

---

## 5. Which Python API changes in Blender 4.0–5.2 most often break older tutorials or LLM-generated code, and what replaces them?

### Takeaway
The highest-frequency breaks for asset scripts:
- **4.1** removed auto smooth / `calc_normals_split`.
- **4.0** renamed Principled sockets, replaced node-group `inputs`/`outputs` with `interface.new_socket`, moved creases and bevel weights to attributes, removed the context-dict operator override, and renamed the OBJ/PLY operators.
- **4.2–4.5** used the EEVEE id `BLENDER_EEVEE_NEXT` and removed EEVEE bloom / `blend_method` / `shadow_method`.
- **5.0** reverted to `BLENDER_EEVEE`, moved the compositor from `scene.node_tree` to `scene.compositing_node_group` (no Composite node), deprecated `use_nodes`, removed dict access to `bpy.props`, removed `bgl`, renamed Boolean `FAST`→`FLOAT`, removed the legacy `action.fcurves` API, and requires `media_type` before `file_format`.
- **5.2** replaced GN modifier `mod["Socket_N"]` with `mod.properties.inputs.Socket_N.value` and changed the Random Value/Compare socket identifiers.

### Cited Findings

| Ver. | Old (breaks) | Replacement | Source / check |
|---|---|---|---|
| 4.0 | `bpy.ops.x.y(ctx_dict, ...)` | `with bpy.context.temp_override(**ctx): bpy.ops.x.y(...)` | [4.0 notes](https://developer.blender.org/docs/release_notes/4.0/python_api/); on 5.2.2 a dict positional arg raises `ValueError: 1-2 args execution context is supported` (verified headless on 5.2.2) |
| 4.0 | `tree.inputs.new()` / `tree.outputs.new()` / iterate `tree.inputs` | `tree.interface.new_socket(name=..., in_out='INPUT'/'OUTPUT', socket_type=...)`, `tree.interface.remove()`, iterate `tree.interface.items_tree` | [4.0 notes](https://developer.blender.org/docs/release_notes/4.0/python_api/); `hasattr(ng,'inputs')` False on 5.2.2 (verified) |
| 4.0 | `MeshEdge.crease`, `MeshEdge/MeshVertex.bevel_weight`, `bm.edges.layers.crease/bevel_weight` | attributes `crease_edge`/`crease_vert`, `bevel_weight_edge`/`bevel_weight_vert`; `mesh.edge_creases_ensure()`; `bm.edges.layers.float.new('crease_edge')` | [4.0 notes](https://developer.blender.org/docs/release_notes/4.0/python_api/); verified headless on 5.2.2 |
| 4.0 | Face maps | integer attribute (face maps removed) | [4.0 notes](https://developer.blender.org/docs/release_notes/4.0/python_api/); `face_maps` absent on 5.2.2 |
| 4.0 | `Mesh.calc_normals()` | not needed (normals always up to date) | [4.0 notes](https://developer.blender.org/docs/release_notes/4.0/python_api/) |
| 4.0 | Principled `Subsurface`, `Specular`, `Transmission`, `Coat`, `Sheen`, `Emission`, `Subsurface Color` | `Subsurface Weight`, `Specular IOR Level`, `Transmission Weight`, `Coat Weight`, `Sheen Weight`, `Emission Color` (+`Emission Strength`); use `Base Color` | [4.0 notes](https://developer.blender.org/docs/release_notes/4.0/python_api/); verified headless on 5.2.2 |
| 4.0 | `bpy.ops.export_scene.obj` / `import_scene.obj` / `import_mesh.ply` | `bpy.ops.wm.obj_export` / `wm.obj_import` / `wm.ply_import` (+`wm.ply_export`) | [4.0 notes](https://developer.blender.org/docs/release_notes/4.0/python_api/); verified headless on 5.2.2 |
| 4.0 | `node.inputs[i]` assumptions | index/name lookup now respects identifiers and availability; use identifiers | [4.0 notes](https://developer.blender.org/docs/release_notes/4.0/python_api/), [4.1 notes](https://developer.blender.org/docs/release_notes/4.1/python_api/) |
| ≤5.2 | `bpy.ops.export_mesh.stl` / `import_mesh.stl` | `bpy.ops.wm.stl_export` / `wm.stl_import` | absent/present on 5.2.2 (verified); removal version not confirmed from fetched notes |
| 4.1 | `mesh.use_auto_smooth = True`, `mesh.auto_smooth_angle = a` | `bpy.ops.object.shade_auto_smooth(angle=a)` (GN "Smooth by Angle" modifier), `bpy.ops.object.shade_smooth_by_angle(angle=a)`, `mesh.set_sharp_from_angle(angle=a)` | [4.1 notes](https://developer.blender.org/docs/release_notes/4.1/python_api/), [4.1 modeling](https://developer.blender.org/docs/release_notes/4.1/modeling/); verified headless on 5.2.2 |
| 4.1 | `calc_normals_split()`, `create_normals_split()`, `free_normals_split()`, writing `MeshLoop.normal` | read `mesh.corner_normals`; write via `mesh.normals_split_custom_set()` / `_from_vertices()` | [4.1 notes](https://developer.blender.org/docs/release_notes/4.1/python_api/); verified absent on 5.2.2 |
| 4.1 | Light probe `'CUBEMAP'/'PLANAR'/'GRID'`; `mat.cycles.displacement_method` | `'SPHERE'/'PLANE'/'VOLUME'`; `Material.displacement_method` | [4.1 notes](https://developer.blender.org/docs/release_notes/4.1/python_api/) |
| ≤5.2 | `ShaderNodeTexMusgrave` | `ShaderNodeTexNoise` with `noise_type` (`'MULTIFRACTAL','RIDGED_MULTIFRACTAL','HYBRID_MULTIFRACTAL','FBM','HETERO_TERRAIN'`) | undefined on 5.2.2 (verified); removal version (believed 4.1) not confirmed from fetched notes |
| 4.2 | `scene.render.engine = 'BLENDER_EEVEE'` (pre-4.2 meaning EEVEE Legacy) | 4.2–4.5: `'BLENDER_EEVEE_NEXT'` | [4.2 notes](https://developer.blender.org/docs/release_notes/4.2/python_api/) |
| 4.2→4.3 | `scene.eevee.use_bloom` / `bloom_*`, `use_ssr*`, `use_gtao_*`, `Material.shadow_method` | bloom → compositor Glare; SSR/GTAO settings removed (raytracing / fast GI settings instead); shadow_method removed | [4.2 notes](https://developer.blender.org/docs/release_notes/4.2/python_api/), [4.3 notes](https://developer.blender.org/docs/release_notes/4.3/python_api/); absent on 5.2.2 (verified) |
| 4.2 | `Material.blend_method`, `show_transparent_back`, `use_screen_refraction`, `light.cycles.cast_shadow`, `scene.eevee.use_motion_blur` | `surface_render_method` (`'DITHERED','BLENDED'`), `use_transparency_overlap`, `use_raytrace_refraction`, `light.use_shadow`, `scene.render.use_motion_blur` (the notes page misspells it `user_motion_blur`) | [4.2 notes](https://developer.blender.org/docs/release_notes/4.2/python_api/); all new names, including `scene.render.use_motion_blur` / `motion_blur_shutter`, verified present on 5.2.2 |
| 4.2 | Assigning float to an int GN/bpy.props property | types are now fixed; float→int assignment fails | [4.2 notes](https://developer.blender.org/docs/release_notes/4.2/python_api/) |
| 4.2 | `interface.new_panel(..., parent=...)` | `parent` argument removed: `new_panel(name, description="", default_closed=False)` | [4.2 notes](https://developer.blender.org/docs/release_notes/4.2/python_api/); signature verified on 5.2.2 |
| 4.2 | Add-on `bl_info` packaging; scripts calling Bool Tool / LoopTools / Extra Objects operators | `blender_manifest.toml` ([4.2 notes](https://developer.blender.org/docs/release_notes/4.2/python_api/)); those add-ons are **absent** from the 5.2.2 bundle and need the Extensions platform. Images-as-Planes is core: `bpy.ops.image.import_as_mesh_planes` | Presence/absence verified on 5.2.2. The claim that they moved to Extensions *in 4.2* is from memory: the 4.2 add-ons notes page returned 404 |
| 4.3 | `bpy.types.AttributeGroup`; `active_color*` on generic groups | `AttributeGroupMesh` / `...PointCloud` / `...Curves` / `...GreasePencil` | [4.3 notes](https://developer.blender.org/docs/release_notes/4.3/python_api/) |
| 4.4 | `obj.animation_data.action = act` expected to animate | also set `obj.animation_data.action_slot = act.slots[0]`; `action.fcurves` deprecated → channelbags | [4.4 notes](https://developer.blender.org/docs/release_notes/4.4/python_api/) |
| 4.4 | `bpy.types.Sequence*`, `context.sequences` | `bpy.types.Strip*`, `context.strips` | [4.4 notes](https://developer.blender.org/docs/release_notes/4.4/python_api/) |
| 4.4 | Subclass `__init__` without args | `super().__init__(*args, **kwargs)` | [4.4 notes](https://developer.blender.org/docs/release_notes/4.4/python_api/) |
| 4.5 | `gpu.types.GPUShader(vert, frag)` | `gpu.shader.create_from_info()` (constructor removed in 5.0) | [4.5 notes](https://developer.blender.org/docs/release_notes/4.5/python_api/) |
| 5.0 | `'BLENDER_EEVEE_NEXT'` | `'BLENDER_EEVEE'` | [5.0 notes](https://developer.blender.org/docs/release_notes/5.0/python_api/); verified (NEXT raises) |
| 5.0 | `scene.node_tree`, `scene.use_nodes = True`, `CompositorNodeComposite` | `ng = bpy.data.node_groups.new(n,'CompositorNodeTree'); scene.compositing_node_group = ng`, output via `NodeGroupOutput` + interface socket; `use_nodes` deprecated no-op | [5.0 notes](https://developer.blender.org/docs/release_notes/5.0/python_api/); verified headless on 5.2.2 |
| 5.0 | `material.use_nodes = True`, `world.use_nodes = True` (and lights) | not needed: new materials/worlds have node trees; deprecated, removal in 6.0 | [5.0 notes](https://developer.blender.org/docs/release_notes/5.0/python_api/); deprecation warnings verified on 5.2.2 |
| 5.0 | `scene['cycles']`, `ob['my_bpy_prop']` (dict access to `bpy.props`) | use RNA attributes (`scene.cycles.samples`, `ob.my_bpy_prop`); only true custom props via `ob['x']` | [5.0 notes](https://developer.blender.org/docs/release_notes/5.0/python_api/); KeyError verified on 5.2.2 |
| 5.0 | `import bgl`, `Image.bindcode` | `gpu` module, `gpu.texture.from_image(image)` | [5.0 notes](https://developer.blender.org/docs/release_notes/5.0/python_api/); `ModuleNotFoundError` verified on 5.2.2 |
| 5.0 | Boolean `solver='FAST'` | `solver='FLOAT'` (also `'EXACT'`, `'MANIFOLD'`) | [5.0 notes](https://developer.blender.org/docs/release_notes/5.0/python_api/); verified headless on 5.2.2 |
| 5.0 | `action.fcurves`, `action.groups`, `action.id_root`, `fcurves.new(action_group=...)` | `bpy_extras.anim_utils.action_get_channelbag_for_slot(act, slot).fcurves` / `action_ensure_channelbag_for_slot`; `slot.target_id_type`; `group_name=` | [5.0 notes](https://developer.blender.org/docs/release_notes/5.0/python_api/); verified headless on 5.2.2 (`keyframe_insert` still works and creates slot `'OBCube'`) |
| 5.0 | `image_settings.file_format = 'FFMPEG'` / `'OPEN_EXR_MULTILAYER'` directly | set `image_settings.media_type = 'VIDEO'` / `'MULTI_LAYER_IMAGE'` first | [5.0 notes](https://developer.blender.org/docs/release_notes/5.0/python_api/); with `media_type='IMAGE'` both assignments raise (verified headless on 5.2.2) |
| 5.0 | `ShaderNodeSeparateRGB/CombineRGB/SeparateHSV/CombineHSV`, `ShaderNodeTexPointDensity`; compositor `CompositorNodeSepRGBA`, `CompositorNodeMixRGB`, `CompositorNodeGamma` | `ShaderNodeSeparateColor`/`ShaderNodeCombineColor` (mode), `CompositorNodeSeparateColor`, `ShaderNodeMix` / `ShaderNodeGamma` (shader nodes usable in compositor) | [5.0 notes](https://developer.blender.org/docs/release_notes/5.0/python_api/); undefined/ok verified on 5.2.2 |
| 5.0 | `scene.eevee.gtao_distance`, `use_gtao`; render pass names `DiffCol`/`IndexMA`/`Z` | `view_layer.eevee.ambient_occlusion_distance`; "Diffuse Color"/"Material Index"/"Depth" | [5.0 notes](https://developer.blender.org/docs/release_notes/5.0/python_api/) |
| 5.0 | `GreasePencilv3`, `bpy.data.grease_pencils_v3`; old `GreasePencil` (annotations) | `GreasePencil`, `bpy.data.grease_pencils`; annotations → `Annotation*` | [5.0 notes](https://developer.blender.org/docs/release_notes/5.0/python_api/) |
| 5.0 | UV selection per layer (`MeshUVLoopLayer.vertex_selection`, `BMLoopUV.select`) | shared UV selection (new `.uv_select_*` attributes seen on 5.2.2) | [5.0 notes](https://developer.blender.org/docs/release_notes/5.0/python_api/) |
| 5.1 | Python 3.11 assumptions; `sculpt.sample_color`; brush `use_airbrush/use_space/...` | Python **3.13** (`python313.dll` ships with 5.2.2); `paint.sample_color`; `brush.stroke_method` | [5.1 notes](https://developer.blender.org/docs/release_notes/5.1/python_api/) |
| 5.2 | `mod["Socket_2"] = v`, `mod["Socket_2_use_attribute"]`, `mod["Socket_2_attribute_name"]` | `getattr(mod.properties.inputs, "Socket_2").value = v`, `.type = 'ATTRIBUTE'`, `.attribute_name`; outputs `mod.properties.outputs.<id>.attribute_name` | [5.2 notes](https://developer.blender.org/docs/release_notes/5.2/python_api/); verified headless on 5.2.2 (old form raises TypeError) |
| 5.2 | Random Value / Compare socket identifiers | `Min`, `Max`, `ID`, `Seed` (`Probability` for BOOLEAN); Compare `A`, `B` (types follow `data_type`) | [5.2 notes](https://developer.blender.org/docs/release_notes/5.2/python_api/); verified headless on 5.2.2 |
| 5.2 | `PreferencesFilePaths.active_asset_library` as index into `asset_libraries` | indices shifted ("All Libraries" and "Essentials" first) | [5.2 notes](https://developer.blender.org/docs/release_notes/5.2/python_api/) |

**Operators that fail in `--background`** — verified headless on 5.2.2:
- `bpy.ops.view3d.view_all()` → `poll() Expected a view3d region`
- `bpy.ops.view3d.snap_cursor_to_center()` → `poll() failed, context is incorrect`
- `bpy.ops.render.opengl()` → `Cannot use OpenGL render in background mode (no opengl context)`
- `bpy.ops.screen.screenshot()` → poll failed
- `bpy.ops.view3d.camera_to_view_selected()` fails only when there is no scene camera ("No active camera").

**Operators that work in `--background`** — verified headless on 5.2.2:
- `object.mode_set`, `mesh.select_all`, `uv.smart_project`, `uv.pack_islands`, `uv.lightmap_pack`
- `object.bake`, `render.render(write_still=True)`
- `object.convert`, `object.shade_auto_smooth`
- `object.modifier_apply` / `object.join` under `temp_override`
- `export_scene.fbx` / `gltf`
- `view3d.camera_to_view_selected` (with a scene camera and selection)

**Other 5.x traps** — verified headless on 5.2.2:
- `gpu.platform.backend_type_get()` raises "requires the gpu module to be initialized" in background *before* any GPU render. After an EEVEE render it returns `'OPENGL'`.
- `object.modifier_apply` has parameters `modifier, report, merge_customdata, single_user, all_keyframes, use_selected_objects`.

### Inferences
- **Version guard:** put `assert bpy.app.version >= (5, 2, 0), bpy.app.version_string` at the top of every skill script. Most breaking names above fail loudly, but the 5.2 GN-input change and the `use_nodes` deprecation do not fail the same way on 5.1.
- **MCP and the live session:** `execute_blender_code` in the live UI session has real 3D-view areas, so view3d operators may poll successfully there. The same code fails under `execute_blender_code_for_cli` (background). Prefer data-API equivalents (`camera_fit_coords`, `bpy.data.*`, `temp_override(object=...)`) for code that must run in both.
- **Never reset the scene in the live session:** never call `bpy.ops.wm.read_factory_settings(use_empty=True)` in the live MCP session. It was used here only in throwaway headless runs, and in a live session it would wipe the user's unsaved scene.

### Gaps
- The 4.2 add-ons release-note page path (`release_notes/4.2/add_ons.md`) returned 404, so exactly which add-ons moved to extensions is inferred from the 5.2.2 bundle listing.
- The removal versions for the Python STL add-on and the Musgrave node were not confirmed from fetched notes.
- `execute_blender_code` itself (the MCP `result` dict convention, timeouts) was not exercised. All runs used `blender.exe --background` directly.

---

## 6. What preview-render and inspection recipes let an agent reliably judge silhouette, proportion, shading and topology from images?

### Takeaway
Render a **4×3 contact sheet of 512 px tiles** and read it once:
- **Workbench:** studio + cavity + outline in 3/4, front ortho, side ortho and top ortho views; a FLAT black-on-white **silhouette**; a `check_normal+y.exr` **matcap** for shading errors; a **wireframe overlay** for topology.
- **EEVEE:** bundled HDRI + key light + compositor bloom, for glow and colour (3/4 and rear views).
- **Cycles:** an optional 16-spp check.

Pair the images with a numeric mesh report (tris, n-gons, non-manifold edges, custom normals, dimensions). On 5.2.2 / RTX 5080 each tile took 0.04–0.9 s (EEVEE 0.11–0.58 s), the 11-image sheet about 4 s, and the whole build + preview + export run 7.7 s.

### Cited Findings
- **Render timings** at 512×512 (renders after the first are faster because shaders are cached) — verified headless on 5.2.2:

  | Mode | Time |
  |---|---|
  | Workbench studio (first render) | 0.48 s |
  | Workbench studio (later views) | 0.04–0.94 s |
  | Silhouette | 0.07 s |
  | Matcap | 0.10 s |
  | Wireframe overlay | 0.10 s |
  | EEVEE, 16 `taa_render_samples`, HDRI + area light + Glare bloom | 0.58 s first, 0.11 s next |
  | Cycles CPU, 16 samples + OpenImageDenoise | 0.91 s |
- **Quality of each view** — verified by inspecting the contact sheet (headless on 5.2.2):
  - **Silhouette:** Workbench `light='FLAT'`, `color_type='SINGLE'`, `single_color=(0,0,0)` and `scene.world.color=(1,1,1)` (Workbench uses the world viewport colour as background) gave a clean black silhouette on white.
  - **Matcap:** `light='MATCAP'` + `studio_light='check_normal+y.exr'` showed normal directions clearly.
  - **Wireframe:** a duplicate object with a `WIREFRAME` modifier (`thickness=0.006`, `use_replace=True`) plus `color_type='OBJECT'` (`dup.color` black, original light grey) showed the final evaluated topology over the shaded mesh.
- **Matcap ordering:** set `shading.light = 'MATCAP'` **before** `studio_light`. With `light='STUDIO'` the enum only accepts `'Default','basic.sl','outdoor.sl','paint.sl','rim.sl','studio.sl'` — verified headless on 5.2.2; [Manual: Workbench lighting](https://docs.blender.org/manual/en/latest/render/workbench/lighting.html).
- **Camera fitting without view3d** — verified headless on 5.2.2 ([API: Object.camera_fit_coords](https://docs.blender.org/api/5.1/bpy.types.Object.html#bpy.types.Object.camera_fit_coords)):
  - `cam_obj.camera_fit_coords(depsgraph, flat_coords)` returns `(location, ortho_scale)` for the current camera rotation.
  - `bpy.ops.view3d.camera_to_view_selected()` also returned FINISHED headless once `scene.camera` existed and the objects were selected.
- **HDRI lighting without the HDRI backdrop** — verified headless on 5.2.2: a world built as `Mix Shader(Fac = Light Path 'Is Camera Ray', Background(HDRI), Background(flat grey))` lit the model with the bundled `forest.exr` while the camera saw flat grey. The background pixel measured 0.482 sRGB ≈ 0.2 linear. The plain HDRI backdrop (`studio.exr`) was visually busy in the first test.
- **Contact sheet:** `bpy.data.images.load` + `pixels.foreach_get/foreach_set` with numpy assembled 11 renders into one 2048×1536 PNG — verified headless on 5.2.2. Blender pixel rows start at the bottom, so tile row order must be flipped.
- **Mesh inspection report:** `ob.evaluated_get(dg).to_mesh()` → bmesh gives counts of tris, n-gons, non-manifold edges and loose verts, plus `has_custom_normals`, `normals_domain`, dimensions, UV layers and materials. It caught both the open glow discs (48 non-manifold edges) and the hull n-gons — verified headless on 5.2.2.
- **Outline / cavity options:** Workbench render options include Outline (object outline) and cavity / specular highlighting — [Manual: Workbench options](https://docs.blender.org/manual/en/latest/render/workbench/options.html).

#### Copy-ready: preview kit (verified headless on 5.2.2)
```python
import bpy, math, os
from mathutils import Vector
scene = bpy.context.scene

def world_bbox(objs):
    pts = [o.matrix_world @ Vector(c) for o in objs for c in o.bound_box]
    mn = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
    mx = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
    return mn, mx

def frame(objs, direction, ortho=False, margin=1.12, lens=50):
    cam = bpy.data.objects.get('PreviewCam') or bpy.data.objects.new('PreviewCam', bpy.data.cameras.new('PreviewCam'))
    if cam.name not in scene.collection.objects: scene.collection.objects.link(cam)
    scene.camera = cam
    mn, mx = world_bbox(objs); center = (mn + mx) / 2; radius = max((mx - mn).length / 2, 1e-3)
    d = Vector(direction).normalized(); cam.data.lens = lens
    if ortho:
        cam.data.type = 'ORTHO'; cam.data.ortho_scale = 2 * radius * margin; dist = radius * 3
    else:
        cam.data.type = 'PERSP'; dist = radius * margin / math.sin(cam.data.angle / 2)
    cam.location = center + d * dist
    cam.rotation_euler = (center - cam.location).to_track_quat('-Z', 'Y').to_euler()
    cam.data.clip_start = max(dist - radius * 2, 0.01); cam.data.clip_end = dist + radius * 2
    return cam
# tighter perspective fit (verified API): co, _ = cam.camera_fit_coords(bpy.context.evaluated_depsgraph_get(),
#     [c for o in objs for v in o.bound_box for c in (o.matrix_world @ Vector(v))]); cam.location = co

VIEWS = {'three_q': (1, 1.2, 0.7), 'front': (0, 1, 0.15), 'side': (1, 0, 0.1), 'top': (0, 0.001, 1),
         'rear_three_q': (-1, -1.3, 0.5), 'below': (0.4, 0.6, -1)}   # ship nose = +Y in this test

def setup_workbench(mode='studio'):
    scene.render.engine = 'BLENDER_WORKBENCH'; scene.display.render_aa = '8'
    scene.world = scene.world or bpy.data.worlds.new('PreviewWorld')
    sh = scene.display.shading
    if mode == 'studio':
        sh.light = 'STUDIO'; sh.studio_light = 'Default'; sh.color_type = 'MATERIAL'
        sh.show_cavity = True; sh.cavity_type = 'BOTH'
        sh.cavity_ridge_factor = sh.cavity_valley_factor = 1.0
        sh.curvature_ridge_factor = sh.curvature_valley_factor = 1.0
        sh.show_object_outline = True; sh.object_outline_color = (0, 0, 0)
        sh.show_specular_highlight = True; scene.world.color = (0.18, 0.18, 0.2)
    elif mode == 'matcap':
        sh.light = 'MATCAP'; sh.studio_light = 'check_normal+y.exr'   # set light FIRST
        sh.color_type = 'SINGLE'; sh.single_color = (0.8, 0.8, 0.8)
        sh.show_cavity = False; sh.show_object_outline = False; scene.world.color = (0.05, 0.05, 0.05)
    elif mode == 'silhouette':
        sh.light = 'FLAT'; sh.color_type = 'SINGLE'; sh.single_color = (0, 0, 0)
        sh.show_cavity = False; sh.show_object_outline = False; sh.show_specular_highlight = False
        scene.world.color = (1, 1, 1)
    elif mode == 'wire':                                   # pair with wire duplicates (below)
        sh.light = 'STUDIO'; sh.studio_light = 'Default'; sh.color_type = 'OBJECT'
        sh.show_cavity = False; sh.show_object_outline = False; scene.world.color = (0.9, 0.9, 0.9)

def add_wire_duplicates(objs, thickness=0.006):
    dups = []
    for o in objs:
        if o.type != 'MESH': continue
        d = o.copy(); d.name = 'WIRE_' + o.name; scene.collection.objects.link(d)
        w = d.modifiers.new('Wire', 'WIREFRAME'); w.thickness = thickness; w.use_replace = True; w.use_even_offset = True
        d.color = (0, 0, 0, 1); o.color = (0.85, 0.85, 0.85, 1); dups.append(d)
    return dups                                            # delete with bpy.data.objects.remove(d) afterwards

def setup_eevee_preview(hdri='forest.exr', strength=1.0, samples=16, backdrop=(0.2, 0.2, 0.2)):
    scene.render.engine = 'BLENDER_EEVEE'; scene.eevee.taa_render_samples = samples; scene.eevee.use_raytracing = False
    w = scene.world or bpy.data.worlds.new('PreviewWorld'); scene.world = w
    nt = w.node_tree; nt.nodes.clear()                     # 5.x: world.node_tree exists already
    env = nt.nodes.new('ShaderNodeTexEnvironment')
    env.image = bpy.data.images.load(os.path.join(bpy.utils.system_resource('DATAFILES', path='studiolights/world'), hdri), check_existing=True)
    bg_l = nt.nodes.new('ShaderNodeBackground'); bg_l.inputs['Strength'].default_value = strength
    bg_c = nt.nodes.new('ShaderNodeBackground'); bg_c.inputs['Color'].default_value = (*backdrop, 1)
    lp = nt.nodes.new('ShaderNodeLightPath'); mix = nt.nodes.new('ShaderNodeMixShader'); out = nt.nodes.new('ShaderNodeOutputWorld')
    nt.links.new(env.outputs['Color'], bg_l.inputs['Color'])
    nt.links.new(lp.outputs['Is Camera Ray'], mix.inputs['Fac'])
    nt.links.new(bg_l.outputs['Background'], mix.inputs[1]); nt.links.new(bg_c.outputs['Background'], mix.inputs[2])
    nt.links.new(mix.outputs['Shader'], out.inputs['Surface'])
    scene.view_settings.view_transform = 'Standard'; scene.view_settings.look = 'None'

def add_key_light(objs):
    mn, mx = world_bbox(objs); c = (mn + mx) / 2; r = (mx - mn).length / 2
    li = bpy.data.objects.get('KeyLight') or bpy.data.objects.new('KeyLight', bpy.data.lights.new('KeyLight', 'AREA'))
    if li.name not in scene.collection.objects: scene.collection.objects.link(li)
    li.data.size = r; pos = c + Vector((1.5, 1.0, 1.6)).normalized() * r * 3
    li.location = pos; li.rotation_euler = (c - pos).to_track_quat('-Z', 'Y').to_euler()
    li.data.energy = 60.0 * (r * 3) ** 2 / 4.0             # ~1.4 kW for a 4.7 m ship looked right

def render_to(path, res=512):
    scene.render.resolution_x = scene.render.resolution_y = res; scene.render.resolution_percentage = 100
    scene.render.image_settings.media_type = 'IMAGE'; scene.render.image_settings.file_format = 'PNG'
    scene.render.filepath = path; bpy.ops.render.render(write_still=True); return path

def contact_sheet(paths, cols, out_path):
    import numpy as np
    imgs = [bpy.data.images.load(p, check_existing=False) for p in paths]
    w, h = imgs[0].size; rows = math.ceil(len(imgs) / cols)
    sheet = np.ones((rows * h, cols * w, 4), dtype=np.float32)
    for i, im in enumerate(imgs):
        px = np.empty(w * h * 4, dtype=np.float32); im.pixels.foreach_get(px)
        r, c = divmod(i, cols); rr = rows - 1 - r          # pixel rows start at the bottom
        sheet[rr * h:(rr + 1) * h, c * w:(c + 1) * w] = px.reshape(h, w, 4)
    out = bpy.data.images.new('ContactSheet', cols * w, rows * h, alpha=True)
    out.pixels.foreach_set(sheet.ravel()); out.filepath_raw = out_path; out.file_format = 'PNG'; out.save()
    for im in imgs: bpy.data.images.remove(im)
    return out_path

def mesh_report(ob):
    import bmesh
    dg = bpy.context.evaluated_depsgraph_get(); ev = ob.evaluated_get(dg); me = ev.to_mesh()
    bm = bmesh.new(); bm.from_mesh(me)
    rep = {'tris': sum(len(f.verts) - 2 for f in bm.faces), 'ngons': sum(len(f.verts) > 4 for f in bm.faces),
           'non_manifold_edges': sum(not e.is_manifold for e in bm.edges),
           'loose_verts': sum(not v.link_edges for v in bm.verts),
           'custom_normals': me.has_custom_normals, 'normals_domain': me.normals_domain,
           'dims_m': [round(x, 3) for x in ev.dimensions], 'uv_layers': [u.name for u in me.uv_layers],
           'materials': [m.name if m else None for m in me.materials]}
    bm.free(); ev.to_mesh_clear(); return rep
```
The verified run used these functions with only cosmetic differences:
- In that run, `setup_eevee_preview` was split into a plain-HDRI world (pipeline test) and the camera-ray world (probe3).
- The `media_type` line was not in the verified preview loop. It was exercised separately: setting `media_type='IMAGE'` then `file_format='PNG'` worked.

### Inferences
- **A good per-asset loop for agents:**
  1. `mesh_report` (budget and topology checks).
  2. A 4×3 sheet: Workbench studio 3/4, front, side and top; silhouette top, side and front; matcap 3/4; wire 3/4; EEVEE 3/4 and rear with bloom; optionally a scale-reference cube or capsule.
  3. Read the single sheet image, judge, fix, re-render.
  4. 512 px tiles were enough to judge bevel highlights, panel lines and glow. A 2048×1536 sheet is displayed at about 2000 px.
- **Silhouette and proportion:** judge them in the ortho silhouettes; bold silhouettes should read without interior detail. Judge **shading artifacts** in the matcap (bent normals and smoothing seams stand out), **topology** in the wire view, and **colour/glow** in EEVEE with the Standard view transform.
- **Top-view orientation:** the direction-based top view rendered with the ship's nose pointing *down* in the image. For canonical orthographic views use fixed rotations instead: top = rotation (0,0,0) above the object; front (looking +Y) = (90°,0,0) placed at −Y; right side = (90°,0,90°) placed at +X. These follow from the camera looking down its local −Z but were **not rendered** in this session.
- **Machines without a usable GPU:** Workbench and EEVEE in background need a GPU context (this machine reported OpenGL on an RTX 5080). Cycles CPU at 16 spp + denoise (~0.9 s at 512²) is a viable fallback for every view. Not tested on a GPU-less machine.

### Gaps
- Freestyle line rendering (as an alternative to the wire duplicates) was not tested.
- 5.2's new `Window.screenshot` pixel API ([5.2 notes](https://developer.blender.org/docs/release_notes/5.2/python_api/)) was not tested. It needs a window, so it is not usable headless.
- No source was found that quantifies which camera angles best predict in-game readability. The view set above is an inference from standard turnaround practice plus this test.
