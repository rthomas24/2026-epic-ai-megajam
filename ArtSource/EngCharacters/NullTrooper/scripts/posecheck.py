# posecheck.py (Heavy form .blend): numeric clearance tests on the separate parts.
#  - helmet vs gorget with the head turned 35 deg left/right and tipped 20 deg up
#  - left forearm shield vs torso / legs with the left arm swung 40 deg forward and 30 deg back
import bpy, json, math
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree
arm = bpy.data.objects["root"]
B = lambda n: arm.data.bones[n].head_local.copy()
parts = [o for o in bpy.data.collections["NullTrooper"].objects if o.type == 'MESH']


def bones_of(o):
    b = o.get("nt_bone", "")
    try: return set(json.loads(b).keys())
    except Exception: return {b}


def geo(objs, M=None, face_ok=None):
    V_, P_ = [], []
    for o in objs:
        base = len(V_)
        V_ += [(M @ v.co) if M else v.co.copy() for v in o.data.vertices]
        for p in o.data.polygons:
            if face_ok is None or face_ok(o, p): P_.append([base + i for i in p.vertices])
    return V_, P_


def rot(pivot, axis, deg):
    return Matrix.Translation(pivot) @ Matrix.Rotation(math.radians(deg), 4, axis) @ Matrix.Translation(-pivot)


helmet = [o for o in parts if bones_of(o) == {"head"}]
gorget = [o for o in parts if o.name.startswith("NT_Gorget")]
gv, gp = geo(gorget); gb = BVHTree.FromPolygons(gv, gp)
hp = B("head")
for yaw, pitch in ((0, 0), (35, 0), (-35, 0), (0, 20), (35, 20), (-35, 20)):
    M = rot(hp, 'Z', yaw) @ rot(hp, 'X', -pitch)
    hv, hpo = geo(helmet, M)
    hb = BVHTree.FromPolygons(hv, hpo)
    ov = hb.overlap(gb)
    dmin = min((gb.find_nearest(v)[3] or 9) for v in hv[::3])
    print("POSECHECK helmet/gorget yaw %4d up %2d: intersecting triangle pairs %d, smallest gap %.1f mm" % (yaw, pitch, len(ov), dmin * 1000))

shield = [o for o in parts if o.name.startswith("NT_Shield")]
LEFT_ARM = ("upperarm_l", "lowerarm_l", "hand_l", "clavicle_l", "index_", "middle_", "ring_", "pinky_", "thumb_", "upperarm_twist", "lowerarm_twist")
body = bpy.data.objects["NT_Body"]
names = {g.index: g.name for g in body.vertex_groups}
dom = []
for v in body.data.vertices:
    g = max(v.groups, key=lambda g: g.weight) if v.groups else None
    dom.append(names[g.group] if g else "")
obst = [o for o in parts if o is not body and o not in shield and not any(b.endswith("_l") and b.startswith(LEFT_ARM) for b in bones_of(o))
        and not (o.name.endswith("_L") and bones_of(o) == {"transfer"} and "Band_A" in o.name)]
ov_, op_ = geo(obst)
bv, bp_ = geo([body], face_ok=lambda o, p: not any(dom[i].endswith("_l") and dom[i].startswith(LEFT_ARM) for i in p.vertices))
off = len(ov_)
ob = BVHTree.FromPolygons(ov_ + bv, op_ + [[off + i for i in p] for p in bp_])
sp = B("upperarm_l")
for name, deg in (("rest", 0), ("forward 40", -40), ("back 30", 30)):
    M = rot(sp, 'X', deg)
    sv, spo = geo(shield, M)
    sb = BVHTree.FromPolygons(sv, spo)
    ov = sb.overlap(ob)
    dmin = min((ob.find_nearest(v)[3] or 9) for v in sv)
    print("POSECHECK shield arm %-10s: intersecting triangle pairs %d, smallest gap to torso/legs %.1f mm" % (name, len(ov), dmin * 1000))
