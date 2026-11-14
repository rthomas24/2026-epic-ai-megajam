#!/usr/bin/env python3
"""Rebuild SM_CinderAnchorTower / Emitter FBX from the editable .blend sources.

Blender 4.5.3 LTS (or 5.2). Run from this folder:

  blender --background --factory-startup --python rebuild.py

Exports FBX with modifiers applied (WeightedNormal stays live in the .blend),
UnitScaleFactor 100, +Y up / -Z forward (Unreal: +Z up, +X forward), centimetres.
"""
import bpy
from pathlib import Path

HERE = Path(__file__).resolve().parent
JOBS = [
    ("SM_CinderAnchorTower.blend", "SM_CinderAnchorTower.fbx", "SM_CinderAnchorTower"),
    ("SM_CinderAnchorEmitter.blend", "SM_CinderAnchorEmitter.fbx", "SM_CinderAnchorEmitter"),
]


def export_fbx(objects, path):
    bpy.ops.object.select_all(action="DESELECT")
    for o in objects:
        o.hide_set(False)
        o.hide_viewport = False
        o.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    bpy.ops.export_scene.fbx(
        filepath=str(path),
        use_selection=True,
        object_types={"MESH", "EMPTY"},
        use_mesh_modifiers=True,
        mesh_smooth_type="FACE",
        use_tspace=True,
        use_triangles=True,
        axis_forward="-Z",
        axis_up="Y",
        apply_unit_scale=True,
        apply_scale_options="FBX_SCALE_UNITS",
        use_space_transform=True,
        bake_space_transform=False,
        global_scale=1.0,
        path_mode="STRIP",
        embed_textures=False,
        bake_anim=False,
        add_leaf_bones=False,
        use_custom_props=False,
    )


def main():
    for blend, fbx, root in JOBS:
        bpy.ops.wm.open_mainfile(filepath=str(HERE / blend))
        scene = bpy.context.scene
        scene.unit_settings.system = "METRIC"
        scene.unit_settings.scale_length = 1.0
        root_obj = bpy.data.objects[root]
        extras = [o for o in bpy.data.objects
                  if o.name.startswith("UCX_") or o.name.startswith("SOCKET_")]
        # do not export SRC_
        export_fbx([root_obj] + extras, HERE / fbx)
        print("exported", fbx, "objects", [root] + [o.name for o in extras])


if __name__ == "__main__":
    main()
