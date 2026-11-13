# Pilot step 0: import the source GLB (read-only) and print what is in it. No files written.
import bpy, os, sys, json
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE)
REPO = os.path.abspath(os.path.join(ROOT, "..", "..", ".."))
SRC = os.path.join(REPO, "ArtSource", "Characters", "Samples", "SentinelArmor",
                   "Meshy_AI_Sentinel_Armor_Turnaround_20261113_animated_ue5_mannequin_All_animation.glb")
for o in list(bpy.data.objects): bpy.data.objects.remove(o)
bpy.ops.import_scene.gltf(filepath=SRC)
print("FPS", bpy.context.scene.render.fps)
for o in bpy.data.objects:
    print("OBJ", o.name, o.type, "parent", o.parent.name if o.parent else None, "loc", tuple(round(v, 4) for v in o.location),
          "rot", tuple(round(v, 4) for v in o.rotation_euler) if o.rotation_mode != 'QUATERNION' else tuple(round(v, 4) for v in o.rotation_quaternion),
          "scale", tuple(round(v, 4) for v in o.scale), "dims", tuple(round(v, 4) for v in o.dimensions))
arm = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
mesh = next(o for o in bpy.data.objects if o.type == 'MESH')
print("MESH verts", len(mesh.data.vertices), "polys", len(mesh.data.polygons), "mats", [m.name for m in mesh.data.materials],
      "uv", [u.name for u in mesh.data.uv_layers], "vgroups", len(mesh.vertex_groups), "modifiers", [(m.type, m.object.name if getattr(m, 'object', None) else None) for m in mesh.modifiers])
for b in arm.data.bones:
    h = arm.matrix_world @ b.head_local
    print("BONE", b.name, "parent", b.parent.name if b.parent else None, "head", tuple(round(v, 3) for v in h), "deform", b.use_deform)
used = {g.index for v in mesh.data.vertices for g in v.groups if g.weight > 1e-4}
print("WEIGHTED", sorted(mesh.vertex_groups[i].name for i in used))
print("UNWEIGHTED_BONES", sorted(b.name for b in arm.data.bones if b.name not in {mesh.vertex_groups[i].name for i in used}))
for img in bpy.data.images:
    print("IMG", img.name, tuple(img.size), img.colorspace_settings.name, img.file_format, "packed", bool(img.packed_file))
for a in bpy.data.actions:
    print("ACTION", a.name, "range", tuple(a.frame_range), "slots", [s.name_display for s in a.slots] if hasattr(a, "slots") else None)
ad = arm.animation_data
print("ANIMDATA action", ad.action.name if ad and ad.action else None, "nla", [(t.name, [s.action.name for s in t.strips]) for t in ad.nla_tracks] if ad else None)
