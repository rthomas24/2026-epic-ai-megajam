---
name: blender-unreal-export
description: Validates and exports finished Blender 5.2 assets to Unreal Engine 5 (5.7+ / 5.9 preview) as one game-ready FBX per asset with correct scale (cm), forward axis (+X), pivot, SM_/T_/M_ naming, UCX_ convex collision, SOCKET_ sockets, triangulation, smoothing and a manifest. Includes a calibration asset and an Unreal Python import script (Interchange) that sets texture compression and Nanite and builds material instances. Use whenever an asset is finished in Blender or someone says export, send or get it into Unreal or UE, and whenever an imported asset has wrong scale, rotation, pivot, shading, collision, sockets or materials in Unreal.
compatibility: Blender 5.2.x (scripts/export_for_unreal.py, tested); Unreal Engine 5.7+ Python (scripts/ue_import_assets.py, untested until the calibration import passes). Read blender-mcp first.
---

# Blender → Unreal Engine 5

**One asset = one FBX.** An FBX carrying sockets may hold only one render mesh, and with several
meshes only the first mesh's custom collision is imported. The exporter joins an asset's parts into
one `SM_<Asset>` and works on **temporary copies**, so the user's scene is never modified (verified).

## Before the first real export: calibrate once per project
```powershell
& $blender --background --factory-startup --python "<repo>\.claude\skills\blender-unreal-export\scripts\export_for_unreal.py" -- --calibration --out-dir "<repo>\ArtSource\Shared\Calibration\export"
```
Import `SM_TD_CalibrationArrow.fbx` into Unreal and place it at the origin. It must be **100 units
long, point +X, with its fin up (+Z)**. In Blender re-import it already does (verified). If Unreal
disagrees, fix the import settings or `front_axis` *now*, not after 30 assets.

## Export

```python
import runpy
ex = runpy.run_path(r"<repo>\.claude\skills\blender-unreal-export\scripts\export_for_unreal.py")
result = ex["main"](root="SM_Ship_Rift", out_dir=r"<repo>\ArtSource\Space\Ships\Rift\export")
```
Headless: `... --python export_for_unreal.py -- --root SM_Ship_Rift SM_Ship_Lancer --out-dir <dir>`.

What it does, all on temporary copies:
1. **Collects** the root plus its children, then evaluates (applies) all modifiers and joins the
   render parts into one mesh named exactly `SM_<Asset>`.
   - Skips `CUT_*` and `SRC_*` helpers.
   - Authored `UCX_` / `UBX_` / `USP_` / `UCP_` objects become `<PREFIX>_SM_<Asset>_##`.
   - With none authored, `collision="auto"` builds convex hulls (64 support points or fewer) of the
     six largest parts.
   - `SOCKET_*` empties export as sockets.
2. **Re-bases** on the root's origin (the pivot) and rotates the authoring front (-Y) to Unreal +X.
   Pass `front_axis="+X"` if you modelled nose-on-+X.
3. **Cleans up** the mesh: welds doubles closer than 0.01 mm, triangulates (beauty), checks for UVs
   and empty material slots (errors block the export), size sanity and material count.
4. **Writes the FBX:** Apply Unit + **FBX Units Scale**, axis -Z forward / Y up, Face smoothing,
   tangent space, triangulated, no leaf bones or animation.
5. **Writes `SM_<Asset>.manifest.json`:** tris, size in cm, collision, sockets, the
   `material_params` Unreal can rebuild as material instances, the `T_<Asset>_*` textures found next
   to it, a Nanite recommendation, and the import settings to use.

Verified on 5.2.2 by re-import: exact names, object scale 1, dimensions preserved, nose on +X,
sockets rotated with the asset, materials preserved.

## Before you export: checklist
- `asset_report.py` shows no errors; tris are within budget (blender-game-asset `art-direction.md`).
  `assembly_check.py` is OK.
- **Pivot:** the root's origin is where Unreal will rotate and place it.
  - Ships: the centre of mass, or the bounding-box centre.
  - Props and kit pieces: bottom-centre or the grid corner. Use `hardsurface.set_origin`.
- **Names:**
  - The root is `SM_<Asset>`, with no spaces and no `.001`.
  - Materials are named `M_<Asset>_<Role>`; they become Unreal slot names and MI names.
  - Textures are `T_<Asset>_BC/_N/_ORM/_E`.
- Procedural or node-driven materials are **baked** (`blender-texturing`). Flat Principled values
  can stay as they are.
- Glass, translucent and energy-blade parts go in a **separate asset**. Nanite supports only
  opaque and masked materials.
- Sockets are named `SOCKET_<Purpose>` and parented under the root.

## Import into Unreal
- **Scripted:** `scripts/ue_import_assets.py` (untested; run it on the calibration arrow first).
  Run it from the Unreal Output Log
  (`py "<repo>/.claude/skills/blender-unreal-export/scripts/ue_import_assets.py" "<export dir>" "/Game/TriadDrift/<Lane>/<Category>"`)
  or through the Unreal MCP's Python tool. It handles:
  - texture settings (BC/E sRGB; N as Normalmap with **no** green flip; ORM as Masks with sRGB off)
  - FBX via Interchange with a saved pipeline (UCX collision on, lightmap UVs off), with a legacy
    importer fallback
  - Nanite per manifest
  - material instances from `M_TD_Flat` / `M_TD_Textured`, if a developer has created those masters
    (parameter names in `references/unreal-import.md`)
- **Manual:** the Content Browser import dialog with the settings in
  `references/unreal-import.md`.
- **UE 5.9 MegaJam Preview** has an experimental built-in MCP with an `ImportFile` tool. Enable All
  Toolsets, pass `bReplaceExisting` / `bSave` = true, then run the post-fix part of the script.
- Destination: `Content/TriadDrift/<Lane>/...`, matching the ownership in features.md §2. Import
  only into your own lane's folders.

## When something is wrong in Unreal

| Symptom | Cause / fix |
|---|---|
| 100× too big or small | Scene unit scale isn't 1.0, or "FBX Units Scale" was overridden. Use the exporter, which sets it |
| Faces the wrong way | Authoring front wasn't -Y: pass `front_axis`. Recheck with the calibration arrow |
| Odd pivot | The root origin is wrong. Use `set_origin` on the root, then re-export |
| Collision missing | UCX names must be `UCX_<exact mesh name>_##` (the exporter guarantees this). Turn on "collision according to mesh name" in the import pipeline |
| Faceted or wrong shading | Normals must be *imported*, not recomputed: Normal Import Method = Import Normals |
| Inverted-looking bumps | Normal map flipped twice. Our maps are already DirectX; leave Flip Green off |
| Materials show as plain grey | Expected for flat MIs until the `M_TD_*` masters exist. Then run the script again |
| Sockets missing | Check the manifest `sockets` list. Interchange support for FBX sockets is unverified, so add them in the Static Mesh editor if needed |
