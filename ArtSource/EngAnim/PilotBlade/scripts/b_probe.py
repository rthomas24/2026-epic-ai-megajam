# Probe an FBX: armatures, bones (name, parent, head), meshes, actions, unit scale. Read-only.
import bpy, sys
p = sys.argv[sys.argv.index("--") + 1]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=p)
for o in bpy.data.objects:
    print("OBJ", o.type, o.name, "scale", tuple(round(s, 4) for s in o.scale), "rot", tuple(round(r, 3) for r in o.rotation_euler), "loc", tuple(round(r, 3) for r in o.location), "parent", o.parent.name if o.parent else None)
    if o.type == "MESH":
        print("  verts", len(o.data.vertices), "tris", sum(len(f.vertices) - 2 for f in o.data.polygons), "mats", [m.name for m in o.data.materials], "dims", tuple(round(d, 3) for d in o.dimensions), "vgroups", len(o.vertex_groups))
    if o.type == "ARMATURE":
        for b in o.data.bones:
            h = o.matrix_world @ b.head_local; t = o.matrix_world @ b.tail_local
            print("  BONE %-22s parent %-18s head (%.3f %.3f %.3f) tail (%.3f %.3f %.3f)" % (b.name, b.parent.name if b.parent else "-", h.x, h.y, h.z, t.x, t.y, t.z))
for a in bpy.data.actions:
    print("ACTION", a.name, tuple(a.frame_range))
print("fps", bpy.context.scene.render.fps)
