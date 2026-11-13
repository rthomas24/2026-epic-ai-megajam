# Stage 00: import the Manny reference FBX, convert to metres (object scales 1), save the base .blend,
# export a round-trip test FBX (unchanged Manny) and report bone transforms.
# blender --background --factory-startup --python s00_base.py
import bpy, os, sys, json
from mathutils import Matrix, Vector
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import nt_io

for o in list(bpy.data.objects): bpy.data.objects.remove(o)
arm, mesh = nt_io.import_ue_fbx(os.path.join(ROOT, "source", "SKM_Manny_Simple.fbx"))
mesh.name = "Manny_Body"; mesh.data.name = "Manny_Body"
ws = [mesh.matrix_world @ v.co for v in mesh.data.vertices]
print("height", max(v.z for v in ws), "min z", min(v.z for v in ws), "arm scale", tuple(arm.scale), "bones", len(arm.data.bones))
bones = {b.name: [list(map(float, r)) for r in (arm.matrix_world @ b.matrix_local)] for b in arm.data.bones}
os.makedirs(os.path.join(ROOT, "work"), exist_ok=True)
json.dump(bones, open(os.path.join(ROOT, "work", "manny_bones_ref.json"), "w"))
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(ROOT, "NullTrooper_v00_manny.blend"))
out = os.path.join(ROOT, "work", "RT_Manny.fbx")
nt_io.export_skeletal_fbx(arm, [mesh], out)
print("exported", out, os.path.getsize(out))
