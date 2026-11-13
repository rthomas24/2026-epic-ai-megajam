# measure.py: silhouette numbers for the review (run on a form .blend)
import bpy
from mathutils import Vector
def ext(pre, axis=0, zr=None):
    lo, hi = 1e9, -1e9
    for o in bpy.data.objects:
        if o.type == 'MESH' and o.name.startswith(pre) and not o.name.startswith("FIT_"):
            for v in o.data.vertices:
                if zr and not (zr[0] <= v.co.z <= zr[1]): continue
                lo = min(lo, v.co[axis]); hi = max(hi, v.co[axis])
    return lo, hi
def w(pre, axis=0, zr=None):
    a, b = ext(pre, axis, zr); return b - a
def radial(pre, a, b):
    ax = (b - a).normalized(); m = 0
    for o in bpy.data.objects:
        if o.type == 'MESH' and o.name.startswith(pre):
            for v in o.data.vertices:
                p = v.co - a; m = max(m, (p - ax * p.dot(ax)).length)
    return m
arm = bpy.data.objects["root"]; B = lambda n: arm.data.bones[n].head_local.copy()
helm = w(("NT_HelmDome", "NT_HelmFace"))
sh = w("NT_Pauldron")
chest = w(("NT_Chest", "NT_Pec", "NT_Flank0"))
waist = w(("NT_Ab3", "NT_Band_T0", "NT_Lumbar2"))
ua = 2 * radial("NT_UpperArm_L", B("upperarm_l"), B("lowerarm_l"))
br = 2 * radial(("NT_Bracer_L", "NT_BracerPanel_L"), B("lowerarm_l"), B("hand_l"))
boot = w("NT_Boot", 1)
print("MEASURE helmet_w %.3f shoulders %.3f (%.2f head-widths) chest %.3f waist %.3f bracer %.3f upperarm %.3f (ratio %.2f) boot_len %.3f sole_top %.3f all_x %.3f height %.3f" % (
    helm, sh, sh / helm, chest, waist, br, ua, br / ua, boot, ext("NT_BootSole")[1] if False else max(v.co.z for v in bpy.data.objects["NT_BootSole_L"].data.vertices), w("NT_"), ext("NT_", 2)[1]))
