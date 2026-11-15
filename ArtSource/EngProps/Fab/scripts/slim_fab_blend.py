# SPDX-License-Identifier: MIT
# Keep the Fab working .blend files small: drop every image no material on a mesh still uses (the glTF importer packs the
# seller's 2K maps, which the prepared PNGs in export/ replace), purge orphans, save compressed without a .blend1 backup.
#   blender --background --factory-startup <file.blend> --python slim_fab_blend.py
import os

import bpy

used = set()
for o in bpy.data.objects:
    if o.type != "MESH":
        continue
    for m in o.data.materials:
        if m and m.node_tree:
            for n in m.node_tree.nodes:
                if n.type == "TEX_IMAGE" and n.image:
                    used.add(n.image.name)
before = os.path.getsize(bpy.data.filepath)
dropped = [im.name for im in bpy.data.images if im.name not in used and im.name != "Render Result"]
for name in dropped:
    bpy.data.images.remove(bpy.data.images[name])
bpy.data.orphans_purge(do_local_ids=True, do_linked_ids=True, do_recursive=True)
bpy.context.preferences.filepaths.save_version = 0
bpy.ops.wm.save_mainfile(compress=True)
print("__RESULT__ %s %d -> %d bytes, dropped %d images" % (os.path.basename(bpy.data.filepath), before,
                                                          os.path.getsize(bpy.data.filepath), len(dropped)))
