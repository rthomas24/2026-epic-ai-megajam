# Stage 10 (step C): build the Null Trooper form on the Manny body. Re-runnable.
# blender --background --factory-startup NullTrooper_v00_manny.blend --python s10_form.py -- --out NullTrooper_v01.blend
import bpy, bmesh, os, sys, math, json, importlib
from mathutils import Vector, Matrix
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import nt_lib as L
importlib.reload(L)
from nt_lib import V, pw, lerp

argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
OUT = argv[argv.index("--out") + 1] if "--out" in argv else "NullTrooper_v01.blend"
ONLY = argv[argv.index("--only") + 1].split(",") if "--only" in argv else None

arm = bpy.data.objects["root"]
manny = bpy.data.objects["Manny_Body"]
body = L.Body(manny, arm)
B = body.bone
FRONT, BACK, UP, LEFT = V(0, -1, 0), V(0, 1, 0), V(0, 0, 1), V(1, 0, 0)


def want(tag):
    return ONLY is None or tag in ONLY


def fin(ob, mirrored=False):
    return L.finish(ob, mirrored)


# ------------------------------------------------------------------ undersuit (the Manny body itself)
def build_body():
    global body
    me = manny.data.copy(); me.name = "NT_Body"
    ob = bpy.data.objects.new("NT_Body", me)
    L.collection().objects.link(ob)
    for g in manny.vertex_groups:
        ob.vertex_groups.new(name=g.name)
    bm = bmesh.new(); bm.from_mesh(me)
    kill = ("head", "foot_", "ball_")
    dom = body.dom
    dead = [f for f in bm.faces if all(dom[v.index].startswith(kill) for v in f.verts)]
    bmesh.ops.delete(bm, geom=dead, context='FACES')
    loose = [v for v in bm.verts if not v.link_faces]
    bmesh.ops.delete(bm, geom=loose, context='VERTS')
    # slightly fuller limbs: the concept is stockier than the mannequin (pushed along normals, hands excluded)
    bm.normal_update()
    dl = bm.verts.layers.deform.verify()
    names = {g.index: g.name for g in ob.vertex_groups}
    for v in bm.verts:
        w = 0.0
        for gi, wt in v[dl].items():
            n = names[gi]
            if n.startswith("thigh"): w += wt * 0.014
            elif n.startswith("calf"): w += wt * 0.010
            elif n.startswith("upperarm"): w += wt * 0.010
            elif n.startswith("lowerarm"): w += wt * 0.008
            elif n.startswith(("hand", "index", "middle", "ring", "pinky", "thumb")): w += wt * 0.004
            elif n.startswith(("spine_04", "spine_05", "clavicle")): w += wt * 0.018
            elif n.startswith(("spine_01", "spine_02", "spine_03")): w += wt * 0.012
            elif n.startswith("pelvis"): w += wt * 0.009
            elif n.startswith("neck"): w += wt * 0.004
        v.co += v.normal * w
    bmesh.ops.holes_fill(bm, edges=[e for e in bm.edges if e.is_boundary], sides=0)
    bm.to_mesh(me); bm.free()
    hi = bpy.data.objects.new("FIT_BodyHi", me.copy())
    bpy.context.scene.collection.objects.link(hi)
    for g in manny.vertex_groups: hi.vertex_groups.new(name=g.name)
    hi.hide_render = True
    body = L.Body(hi, arm)
    me.materials.clear(); me.materials.append(L.material("suit"))
    for p in me.polygons: p.material_index = 0; p.use_smooth = True
    d = ob.modifiers.new("Dec", 'DECIMATE'); d.ratio = 0.13; d.use_collapse_triangulate = True
    dg = bpy.context.evaluated_depsgraph_get()
    new = bpy.data.meshes.new_from_object(ob.evaluated_get(dg), preserve_all_data_layers=True, depsgraph=dg)
    ob.modifiers.clear(); ob.data = new; new.name = "NT_Body"; bpy.data.meshes.remove(me)
    ob["nt_bone"] = "keep"
    ob["nt_bevel"] = json.dumps([0, 'ANGLE', 30, 2])
    L.PARTS.append(ob)
    return ob


# ------------------------------------------------------------------ torso
TA, TB = V(0, -0.02, 0), V(0, -0.02, 1)  # torso axis: u == height in metres


def T(th0, th1, z0, z1):
    return L.sweep(TA, TB, FRONT, LEFT, th0, th1, z0, z1)


def torso_point(th, z, bvh=None, lift=0.0):
    d = FRONT * math.cos(math.radians(th)) + LEFT * math.sin(math.radians(th))
    o = V(0, -0.02, z)
    r = L.raycast_r(bvh or body.bvh(L.R_TORSO), o, d)
    return o + d * ((r or 0.1) + lift), d


def build_torso():
    reg = L.R_TORSO_ARMS
    # --- breastplate (layer 1)
    chest = L.shell("NT_Chest", T(-66, 66, [(0, 1.30), (0.2, 1.262), (0.5, 1.235), (0.8, 1.262), (1, 1.30)],
                                  [(0, 1.40), (0.16, 1.475), (0.36, 1.492), (0.5, 1.452), (0.64, 1.492), (0.84, 1.475), (1, 1.40)]),
                    26, 12, body, reg, offset=0.016, smooth=5, bone={"spine_05": 1.0}, bevel=0.004,
                    grooves=[('s', 0.5, 0.012, 0.004, (0.0, 0.5)), ('t', 0.34, 0.03, 0.004)], edge_drop=0.006)
    fin(chest)
    cb = L.obj_bvh(chest)
    # --- pectoral plates (layer 2), angular
    pec = L.shell("NT_Pec_L", T([(0, 7), (1, 16)], [(0, 50), (0.6, 60), (1, 44)],
                                [(0, 1.318), (0.45, 1.292), (1, 1.335)], [(0, 1.425), (0.5, 1.452), (1, 1.43)]),
                  10, 8, body, reg, offset=0.034, smooth=5, bone={"spine_05": 1.0}, bevel=0.004,
                  grooves=[('t', 0.42, 0.035, 0.0035), ('s', 0.86, 0.0120, 0.001, None, 'stripe')], edge_drop=0.004)
    fin(pec, True)
    # --- sternum housing + hexagonal core
    p, d = torso_point(0, 1.395, cb)
    hexo = lambda r: [(r * math.cos(math.radians(a)), r * math.sin(math.radians(a))) for a in range(0, 360, 60)]
    fin(L.prism("NT_CoreHousing", [(-0.07, 0.06), (0.07, 0.06), (0.086, 0.0), (0.05, -0.076), (-0.05, -0.076), (-0.086, 0.0)],
                p - d * 0.03, LEFT, UP, d, 0.05, taper=0.84, bone={"spine_05": 1.0}, bevel=0.005))
    fin(L.prism("NT_CoreBezel", hexo(0.056), p + d * 0.014, LEFT, UP, d, 0.014, taper=0.9, bone={"spine_05": 1.0}, bevel=0.003, mat="armor2"))
    fin(L.prism("NT_Core", hexo(0.045), p + d * 0.024, LEFT, UP, d, 0.012, taper=0.92, cap_mat="emis",
                inset=(0.01, 0.005), bone={"spine_05": 1.0}, bevel=0.0025))
    # --- ribbed hoses in U-loops: core housing -> ports on the lower ribs
    for sgn in (1, -1):
        for k in range(2):
            pc, dc = torso_point(sgn * (46 + 10 * k), 1.318 - 0.034 * k, cb)
            start = p + LEFT * sgn * (0.03 + 0.026 * k) + UP * -0.058 + d * 0.006
            sag, ds = torso_point(sgn * (17 + 9 * k), 1.262 - 0.022 * k, cb, lift=0.034)
            path = [start + UP * 0.03, start, start + UP * -0.03 + d * 0.016, sag, pc + dc * 0.04 + UP * -0.022, pc + dc * 0.014, pc - dc * 0.006]
            fin(L.tube("NT_Hose_%s%d" % ("L" if sgn > 0 else "R", k), path, 0.0125, segs=7, ribs=10, rib_depth=0.003, bone={"spine_05": 1.0}))
            fin(L.lathe("NT_HosePort_%s%d" % ("L" if sgn > 0 else "R", k), pc - dc * 0.012, dc,
                        [(0.0, 0.025), (0.02, 0.025), (0.027, 0.019), (0.027, 0.0)], segs=8, bone={"spine_05": 1.0}))
    # --- shoulder straps (over the trapezius), one per side
    sa, sb = V(0.0, 0.01, 1.385), V(1.0, 0.01, 1.385)
    strap = L.shell("NT_Strap_L", L.sweep(sa, sb, FRONT, UP, [(0, -18), (1, -18)], [(0, 196), (1, 196)], 0.055, 0.118),
                    5, 18, body, ("spine", "clavicle"), offset=0.022, smooth=3, bone={"spine_05": 0.7, "clavicle_l": 0.3}, bevel=0.004,
                    grooves=[('t', 0.3, 0.02, 0.004), ('t', 0.7, 0.02, 0.004)])
    fin(strap, True)
    # --- collar ring at the base of the neck
    col = L.shell("NT_Collar", T(0, 360, const_s(1.485), [(0, 1.535), (0.25, 1.54), (0.5, 1.56), (0.75, 1.54), (1, 1.535)]),
                  24, 3, body, L.R_TORSO, offset=0.014, smooth=2, wrap=True, bone={"spine_05": 0.5, "neck_01": 0.5}, bevel=0.003)
    fin(col)
    # --- flank (rib) plates, 2 lames per side
    for k, (z0, z1, off) in enumerate(((1.215, 1.305, 0.013), (1.135, 1.225, 0.011))):
        fl = L.shell("NT_Flank%d_L" % k, T([(0, 34 - 6 * k), (1, 40 - 6 * k)], [(0, 118), (1, 112)], [(0, z0 + 0.02), (1, z0)], [(0, z1 - 0.012), (0.3, z1), (1, z1 + 0.01)]),
                     10, 5, body, L.R_TORSO_ARMS, far=0.24, offset=off + 0.002, smooth=3, bone={"spine_04": 1.0} if k == 0 else {"spine_03": 1.0}, bevel=0.0035,
                     grooves=[('t', 0.5, 0.05, 0.003)])
        fin(fl, True)
    # --- abdominal segments
    for k, (z0, z1, bn, hw, off) in enumerate(((1.212, 1.278, "spine_04", 37, 0.024), (1.162, 1.226, "spine_03", 35, 0.02),
                                               (1.112, 1.176, "spine_03", 33, 0.016), (1.056, 1.126, "spine_02", 31, 0.012))):
        ab = L.shell("NT_Ab%d" % k, T([(0, -hw), (1, -hw - 2)], [(0, hw), (1, hw + 2)], [(0, z0 + 0.014), (0.5, z0), (1, z0 + 0.014)],
                                      [(0, z1 + 0.01), (0.5, z1 - 0.004), (1, z1 + 0.01)]),
                     10, 3, body, L.R_TORSO, offset=off, smooth=3, bone={bn: 1.0}, bevel=0.004,
                     grooves=[('s', 0.5, 0.03, 0.003), ('s', 0.2, 0.03, 0.0025), ('s', 0.8, 0.03, 0.0025)])
        fin(ab)
    # --- back: upper back plate, shoulder-blade plates, lumbar segments
    bk = L.shell("NT_Back", T(180 - 62, 180 + 62, [(0, 1.30), (0.25, 1.255), (0.5, 1.24), (0.75, 1.255), (1, 1.30)],
                               [(0, 1.41), (0.2, 1.475), (0.5, 1.50), (0.8, 1.475), (1, 1.41)]),
                 22, 10, body, reg, offset=0.014, smooth=5, bone={"spine_05": 1.0}, bevel=0.004,
                 grooves=[('t', 0.4, 0.03, 0.004)], edge_drop=0.005)
    fin(bk)
    sb_ = L.shell("NT_Scapula_L", T([(0, 180 - 16), (1, 180 - 22)], [(0, 180 - 52), (0.5, 180 - 64), (1, 180 - 50)],
                                    [(0, 1.30), (0.5, 1.285), (1, 1.33)], [(0, 1.45), (0.5, 1.47), (1, 1.44)]),
                  9, 8, body, reg, offset=0.03, smooth=5, bone={"spine_05": 1.0}, bevel=0.004,
                  grooves=[('t', 0.5, 0.035, 0.0035), ('s', 0.85, 0.0120, 0.001, None, 'stripe')], edge_drop=0.004)
    fin(sb_, True)
    for k, (z0, z1, bn) in enumerate(((1.165, 1.235, "spine_03"), (1.095, 1.16, "spine_02"), (1.045, 1.09, "spine_01"))):
        lb = L.shell("NT_Lumbar%d" % k, T(180 - 30 + 2 * k, 180 + 30 - 2 * k, [(0, z0 + 0.01), (0.5, z0), (1, z0 + 0.01)], [(0, z1 + 0.01), (0.5, z1), (1, z1 + 0.01)]),
                     8, 3, body, L.R_TORSO, offset=0.014, smooth=3, bone={bn: 1.0}, bevel=0.0035, grooves=[('s', 0.5, 0.04, 0.003)])
        fin(lb)
    # --- backpack
    yb = 0.13
    pk = {"spine_05": 0.6, "spine_04": 0.4}
    pack = L.hull("NT_Pack", [(0.088, yb, 1.50), (0.094, yb, 1.37), (0.06, yb, 1.185), (0.0, yb, 1.165),
                              (0.06, 0.245, 1.487), (0.068, 0.262, 1.385), (0.044, 0.236, 1.212), (0.0, 0.228, 1.19),
                              (0.0, 0.243, 1.512), (0.036, 0.272, 1.41), (0.03, 0.26, 1.29)],
                  mirror_x=True, bone=pk, bevel=0.006, angle=14)
    fin(pack)
    pb = L.obj_bvh(pack)
    ps, pn = L.surf(pb, V(0, 0.1, 1.36), BACK)
    slot_o = [(-0.02, -0.09), (0.02, -0.09), (0.03, -0.076), (0.03, 0.076), (0.02, 0.09), (-0.02, 0.09), (-0.03, 0.076), (-0.03, -0.076)]
    fin(L.prism("NT_PackSlot", slot_o, V(0, ps.y - 0.02, 1.352), V(-1, 0, 0), UP, BACK, 0.028, taper=0.94, cap_mat="emis",
                inset=(0.016, 0.011), bone=pk, bevel=0.003))
    for sgn in (1, -1):
        fin(L.hull("NT_PackRail_%s" % ("L" if sgn > 0 else "R"), [(sgn * x_, y_, z_) for x_, y_, z_ in
                   ((0.07, 0.14, 1.47), (0.098, 0.14, 1.47), (0.07, 0.215, 1.46), (0.098, 0.2, 1.455),
                    (0.062, 0.14, 1.25), (0.092, 0.14, 1.25), (0.058, 0.205, 1.262), (0.086, 0.19, 1.262))],
                   bone=pk, bevel=0.004, angle=14, mat="armor2"))
    # --- belt, buckle, pouches
    belt = L.shell("NT_Belt", T(0, 360, const_s(0.992), const_s(1.046)), 32, 2, body, L.R_TORSO, offset=0.015, smooth=3, wrap=True,
                   bone={"pelvis": 1.0}, bevel=0.003, mat="armor2")
    fin(belt)
    bb = L.obj_bvh(belt)

    def belt_box(name, th, w, h, dep, zc=1.019, sink=0.008, **kw):
        d = FRONT * math.cos(math.radians(th)) + LEFT * math.sin(math.radians(th))
        o = V(0, -0.02, zc)
        r = L.raycast_r(bb, o, d)
        ex = UP.cross(d)
        return L.box(name, o + d * (r - sink), ex, UP, d, w, h, dep + sink, bone={"pelvis": 1.0}, **kw)
    fin(belt_box("NT_Buckle", 0, 0.085, 0.062, 0.016, bevel=0.004))
    fin(belt_box("NT_BuckleCore", 0, 0.036, 0.03, 0.022, bevel=0.003, mat="armor2"))
    fin(belt_box("NT_BeltBackPlate", 180, 0.075, 0.06, 0.016, bevel=0.004))
    fin(belt_box("NT_BeltBackCore", 180, 0.03, 0.03, 0.021, bevel=0.003, mat="armor2"))
    for th in (40, 68, 114, 142):
        for sgn in (1, -1):
            nm = "NT_Pouch%d_%s" % (th, "L" if sgn > 0 else "R")
            fin(belt_box(nm, sgn * th, 0.056, 0.074, 0.034, zc=1.012, bevel=0.005, mat="armor2"))
            fin(belt_box(nm + "_Flap", sgn * th, 0.06, 0.034, 0.039, zc=1.036, bevel=0.004, mat="armor2"))
    # --- pelvis: codpiece, hip plates, rear plate
    cod = L.shell("NT_Cod", T([(0, -26), (1, -5)], [(0, 26), (1, 5)], const_s(0.992), [(0, 0.90), (0.5, 0.842), (1, 0.90)]),
                  8, 6, body, L.R_TORSO + ("thigh",), offset=0.014, smooth=3, bone={"pelvis": 1.0}, bevel=0.004,
                  grooves=[('s', 0.5, 0.04, 0.003)])
    fin(cod)
    hip = L.shell("NT_Hip_L", T([(0, 46), (1, 62)], [(0, 128), (1, 112)], const_s(0.992), [(0, 0.93), (0.5, 0.885), (1, 0.93)]),
                  8, 4, body, L.R_TORSO + ("thigh",), offset=0.02, smooth=3, bone={"pelvis": 1.0}, bevel=0.004,
                  grooves=[('t', 0.5, 0.06, 0.003)])
    fin(hip, True)
    rear = L.shell("NT_Rear", T([(0, 180 - 34), (1, 180 - 14)], [(0, 180 + 34), (1, 180 + 14)], const_s(0.992), [(0, 0.93), (0.5, 0.895), (1, 0.93)]),
                   8, 4, body, L.R_TORSO + ("thigh",), offset=0.013, smooth=3, bone={"pelvis": 1.0}, bevel=0.004,
                   grooves=[('s', 0.5, 0.05, 0.003)])
    fin(rear)


def const_s(v):
    return lambda s: v


# ------------------------------------------------------------------ legs (left, mirrored)
def build_leg():
    hipj, knee, ank = B("thigh_l"), B("calf_l"), B("foot_l")
    reg = L.R_HIP_L
    S = lambda th0, th1, u0, u1: L.sweep(hipj, knee, FRONT, LEFT, th0, th1, u0, u1)
    th = L.shell("NT_ThighFront_L", S([(0, -62), (1, -40)], [(0, 120), (1, 100)],
                                      [(0, 0.30), (0.3, 0.17), (0.62, 0.10), (1, 0.06)], [(0, 0.74), (0.34, 0.86), (0.7, 0.82), (1, 0.72)]),
                 16, 12, body, reg, offset=0.016, smooth=5, bone={"thigh_l": 1.0}, bevel=0.004, edge_drop=0.005,
                 bulge=lambda s, t: 0.009 * max(0.0, 1 - abs(s - 0.8) / 0.09),
                 grooves=[('s', 0.36, 0.02, 0.0035), ('t', 0.62, 0.025, 0.0035, (0.36, 1.0)),
                          ('s', 0.08, 0.0072, 0.0008, (0.1, 0.9), 'stripe'), ('t', 0.9, 0.0072, 0.0008, (0.1, 0.6), 'stripe')])
    fin(th, True)
    tho = L.shell("NT_ThighOuter_L", S([(0, 48), (1, 60)], [(0, 132), (1, 118)], [(0, 0.16), (0.5, 0.10), (1, 0.14)],
                                       [(0, 0.5), (0.5, 0.64), (1, 0.52)]),
                  8, 8, body, reg, offset=0.034, smooth=5, bone={"thigh_l": 1.0}, bevel=0.004, edge_drop=0.004,
                  grooves=[('t', 0.3, 0.03, 0.0035), ('s', 0.5, 0.0120, 0.0008, (0.36, 0.9), 'stripe')])
    fin(tho, True)
    thb = L.shell("NT_ThighBack_L", S([(0, 128), (1, 124)], [(0, 236), (1, 240)], [(0, 0.26), (0.5, 0.33), (1, 0.4)],
                                      [(0, 0.78), (0.5, 0.86), (1, 0.78)]),
                  10, 8, body, L.R_HIP_L, offset=0.013, smooth=4, bone={"thigh_l": 1.0}, bevel=0.0035, edge_drop=0.004,
                  grooves=[('t', 0.5, 0.03, 0.003), ('s', 0.12, 0.0080, 0.0008, (0.1, 0.9), 'stripe')])
    fin(thb, True)
    # knee guard: angular, on the shin
    kc = knee + V(0, 0.012, -0.012)
    kshape = L.planes([((0, -1, 0.08), 0.13), ((0, -0.8, 0.6), 0.122), ((0, -0.8, -0.6), 0.118),
                       ((0.8, -0.6, 0), 0.112), ((-0.8, -0.6, 0), 0.112)], p=9, base=L.ellipsoid(kc, 0.13, 0.15, 0.17))
    kn = L.shell("NT_Knee_L", L.sphere(kc, LEFT, FRONT, [(0, -30), (0.3, -46), (0.5, -80), (0.7, -46), (1, -30)], [(0, 46), (0.5, 68), (1, 46)],
                                       [(0, -16), (0.36, -60), (0.72, -60), (1, -26)], [(0, 16), (0.36, 60), (0.72, 60), (1, 26)]),
                 10, 10, body, L.R_LEG_L, shape=kshape, smooth=2, clear=0.008, bone={"calf_l": 1.0}, bevel=0.004, maxskirt=0.02,
                 grooves=[('s', 0.5, 0.03, 0.003, (0.0, 0.3)), ('t', 0.3, 0.03, 0.003)])
    fin(kn, True)
    C = lambda th0, th1, u0, u1: L.sweep(knee, ank, FRONT, LEFT, th0, th1, u0, u1)
    flare = lambda s, t: 0.02 * max(0.0, 1 - t / 0.3) ** 2 + 0.008 * max(0.0, (t - 0.8) / 0.2) + 0.012 * max(0.0, 1 - abs(s - 0.5) / 0.28)
    sh = L.shell("NT_Shin_L", C(-100, 100, [(0, 0.2), (0.25, 0.15), (0.5, 0.20), (0.75, 0.15), (1, 0.2)],
                                [(0, 0.80), (0.5, 0.93), (1, 0.80)]),
                 16, 12, body, L.R_LEG_L, offset=0.024, smooth=5, bulge=flare, bone={"calf_l": 1.0}, bevel=0.004, edge_drop=0.004,
                 grooves=[('s', 0.26, 0.018, 0.0035), ('s', 0.74, 0.018, 0.0035), ('t', 0.34, 0.025, 0.0035, (0.26, 0.74)),
                          ('s', 0.5, 0.0056, 0.0008, (0.4, 0.92), 'stripe')])
    fin(sh, True)
    sh2 = L.shell("NT_ShinLow_L", C([(0, -50), (1, -42)], [(0, 50), (1, 42)], [(0, 0.56), (0.5, 0.46), (1, 0.56)], [(0, 0.86), (0.5, 0.97), (1, 0.86)]),
                  8, 7, body, L.R_LEG_L, offset=0.044, smooth=5, bone={"calf_l": 1.0}, bevel=0.004, edge_drop=0.004,
                  bulge=lambda s, t: 0.012 * max(0.0, 1 - abs(s - 0.5) / 0.4),
                  grooves=[('t', 0.5, 0.04, 0.0035)])
    fin(sh2, True)
    cf = L.shell("NT_Calf_L", C(96, 264, [(0, 0.14), (0.5, 0.10), (1, 0.14)], [(0, 0.80), (0.5, 0.72), (1, 0.80)]),
                 12, 10, body, L.R_LEG_L, offset=0.018, smooth=4, bone={"calf_l": 1.0}, bevel=0.0035, edge_drop=0.004,
                 grooves=[('t', 0.45, 0.025, 0.003), ('s', 0.5, 0.0080, 0.0008, (0.5, 0.95), 'stripe')])
    fin(cf, True)
    # round joint bolts (knee, ankle), outer and inner
    for nm, c, r, bn in (("Knee", knee + V(0, 0.012, -0.004), 0.027, "calf_l"), ("Ankle", ank + V(0, 0.012, 0.012), 0.022, "foot_l")):
        for side, sg in (("Out", 1),):
            rr = L.raycast_r(body.bvh(L.R_LEG_L), c, LEFT * sg)
            fin(L.lathe("NT_%sBolt%s_L" % (nm, side), c + LEFT * sg * ((rr or 0.05) - 0.006), LEFT * sg,
                        [(0, r), (0.022, r), (0.027, r * 0.8), (0.027, r * 0.5), (0.022, r * 0.42), (0.022, 0)], segs=10,
                        bone={bn: 1.0}, mat="armor2"), True)


# ------------------------------------------------------------------ boots (left, mirrored)
def build_boot():
    ank, ball = B("foot_l"), B("ball_l")
    f = V(ball.x - ank.x, ball.y - ank.y, 0).normalized()
    side = V(-f.y, f.x, 0) * -1.0  # points to the character's left for the left foot (outer side)
    if side.x < 0: side = -side
    heel = V(ank.x, ank.y, 0) - f * 0.085
    Lb, Wb = 0.312, 0.138
    bone = "fn:boot_l"
    o = heel
    # midsole + tread
    def outline(w, l, toe=0.045, n=6):
        pts = [(-w * 0.42, 0.0), (w * 0.42, 0.0), (w * 0.5, 0.03), (w * 0.5, l - toe - 0.03)]
        for k in range(n + 1):
            a = math.pi * k / n
            pts.append((w * 0.5 * math.cos(a) * (1.0 if k in (0, n) else 0.96), l - toe + toe * math.sin(a)))
        pts += [(-w * 0.5, l - toe - 0.03), (-w * 0.5, 0.03)]
        return pts
    fin(L.prism("NT_BootSole_L", outline(Wb + 0.012, Lb + 0.008), o + UP * 0.015 - f * 0.004, side, f, UP, 0.03, taper=0.97,
                bone=bone, bevel=0.004, mat="armor2"), True)
    fin(L.prism("NT_BootHeelTread_L", [(-0.058, 0.0), (0.058, 0.0), (0.062, 0.085), (-0.062, 0.085)], o - f * 0.004, side, f, UP, 0.018,
                taper=1.04, bone=bone, bevel=0.003, mat="armor2"), True)
    for k in range(4):
        y0 = 0.112 + k * 0.047
        w = (Wb + 0.012) * (1.0 if k < 3 else 0.82)
        fin(L.prism("NT_BootLug%d_L" % k, [(-w / 2, y0), (w / 2, y0), (w / 2, y0 + 0.036), (-w / 2, y0 + 0.036)], o, side, f, UP, 0.018,
                    taper=1.03, bone=bone, bevel=0.003, mat="armor2"), True)
    # upper: vamp (toe -> ankle) and shaft
    zb = 0.043
    vamp = [dict(y=Lb - 0.004, w=0.07, h=0.03, z=zb + 0.012, n=2.6), dict(y=Lb - 0.02, w=0.11, h=0.06, z=zb + 0.024, n=2.8),
            dict(y=Lb - 0.06, w=0.127, h=0.08, z=zb + 0.032, n=3.0), dict(y=Lb - 0.11, w=0.13, h=0.09, z=zb + 0.038, n=3.0),
            dict(y=0.16, w=0.124, h=0.11, z=zb + 0.048, n=2.8), dict(y=0.115, w=0.108, h=0.14, z=zb + 0.065, n=2.6),
            dict(y=0.06, w=0.104, h=0.15, z=zb + 0.07, n=2.6), dict(y=0.012, w=0.098, h=0.13, z=zb + 0.06, n=2.6),
            dict(y=-0.004, w=0.07, h=0.09, z=zb + 0.045, n=2.4)]
    fin(L.loft("NT_BootVamp_L", vamp[::-1], o, side, f, UP, ring=16, bone=bone, mat="armor2"), True)
    shaft = [dict(y=0.075, w=0.104, h=0.132, z=0.058), dict(y=0.13, w=0.106, h=0.13, z=0.056), dict(y=0.19, w=0.112, h=0.128, z=0.052),
             dict(y=0.215, w=0.12, h=0.136, z=0.05), dict(y=0.225, w=0.112, h=0.128, z=0.05)]
    fin(L.loft("NT_BootShaft_L", shaft, o, side, UP, f, ring=16, bone=bone, mat="armor2"), True)
    # armoured toe cap, heel counter, straps, buckle, heel light
    toe = [dict(y=Lb + 0.001, w=0.076, h=0.034, z=zb + 0.013, n=2.6), dict(y=Lb - 0.02, w=0.118, h=0.066, z=zb + 0.026, n=2.8),
           dict(y=Lb - 0.06, w=0.135, h=0.087, z=zb + 0.034, n=3.0), dict(y=Lb - 0.098, w=0.138, h=0.096, z=zb + 0.038, n=3.0)]
    fin(L.loft("NT_BootToe_L", toe[::-1], o, side, f, UP, ring=16, bone=bone, bevel=0.003, angle=40), True)
    fin(L.hull("NT_BootHeel_L", [o + side * sx * 0.058 + f * yy + UP * zz for sx in (-1, 1) for yy, zz in
                                 ((-0.016, 0.04), (0.06, 0.04), (-0.018, 0.105), (0.05, 0.125), (-0.008, 0.14))] +
               [o + side * sx * 0.04 + f * -0.024 + UP * zz for sx in (-1, 1) for zz in (0.045, 0.1)],
               bone=bone, bevel=0.004, angle=18), True)
    fin(L.box("NT_BootHeelLight_L", o - f * 0.02 + UP * 0.072, side, UP, -f, 0.05, 0.014, 0.008, bone=bone, bevel=0.002, mat="emis"), True)
    strap1 = [dict(y=0.148, w=0.136, h=0.118, z=zb + 0.05, n=2.8), dict(y=0.186, w=0.14, h=0.106, z=zb + 0.045, n=2.8)]
    fin(L.loft("NT_BootStrapA_L", strap1, o, side, f, UP, ring=16, bone=bone, bevel=0.003, angle=40), True)
    strap2 = [dict(y=0.118, w=0.118, h=0.146, z=0.056), dict(y=0.152, w=0.12, h=0.146, z=0.054)]
    fin(L.loft("NT_BootStrapB_L", strap2, o, side, UP, f, ring=16, bone=bone, bevel=0.003, angle=40), True)
    fin(L.box("NT_BootBuckleA_L", o + side * 0.06 + f * 0.167 + UP * 0.09, f, UP, side, 0.034, 0.034, 0.012, bone=bone, bevel=0.003), True)
    fin(L.box("NT_BootBuckleB_L", o + side * 0.056 + f * 0.062 + UP * 0.135, f, UP, side, 0.036, 0.03, 0.012, bone=bone, bevel=0.003), True)
    cuff = [dict(y=0.2, w=0.124, h=0.142, z=0.05), dict(y=0.232, w=0.128, h=0.146, z=0.05)]
    fin(L.loft("NT_BootCuff_L", cuff, o, side, UP, f, ring=16, bone=bone, bevel=0.003, angle=40), True)


# ------------------------------------------------------------------ arms (left, mirrored)
def build_arm():
    sh, el, wr = B("upperarm_l"), B("lowerarm_l"), B("hand_l")
    ua = (el - sh).normalized(); fa = (wr - el).normalized()
    out_u = V(0.82, 0, 0.57)
    reg = L.R_ARM_L
    # pauldron: three layered lames
    pc = sh + V(0.032, 0.004, 0.0)
    PH = lambda ph0, ph1, la0, la1: L.sphere(pc, FRONT, UP, ph0, ph1, la0, la1)
    # FRONT x UP = (-1,0,0): positive phi turns toward the neck; negative phi goes outward and down the arm.
    lam = [("A", -60, 40, 0.176, 0.160, 0.138, {"clavicle_l": 0.75, "upperarm_l": 0.25}, 62),
           ("B", -94, -50, 0.158, 0.146, 0.13, {"clavicle_l": 0.3, "upperarm_l": 0.7}, 56),
           ("C", -120, -84, 0.136, 0.13, 0.126, {"upperarm_l": 1.0}, 50)]
    for nm, p0, p1, rx, ry, rz, bn, lw in lam:
        shp = L.planes([((0.22, 0, 1), rz * 0.8), ((1, 0, 0.1), rx * 0.82), ((0, -1, 0.12), ry * 0.86), ((0, 1, 0.12), ry * 0.86),
                        ((0.75, 0, 0.75), rx * 0.86)], p=10, base=L.ellipsoid(pc, rx, ry * 1.04, rz))
        pd = L.shell("NT_Pauldron%s_L" % nm, segs=2, sample=PH(p0, p1, -lw, lw),
                     ns=10, nt=7, body=body, region=L.R_SHOULDER_L, shape=shp, smooth=2, clear=0.012, bone=bn, bevel=0.005, inner=True, thick=0.02, maxskirt=0.024,
                     grooves=[('t', 0.5, 0.035, 0.003)] if nm == "A" else [('s', 0.5, 0.03, 0.003)], far=0.5)
        fin(pd, True)
        if nm in ("A", "B"):
            pbv = L.obj_bvh(pd)
            studs = ((-44, -30), (-44, 30), (-8, 38)) if nm == "A" else ((-80, 0),)
            for q, (ph, la) in enumerate(studs):
                smp = L.sphere(pc, FRONT, UP, ph, ph, la, la)
                _, dd = smp(0, 0)
                hp, hn = L.surf(pbv, pc, dd)
                if hp:
                    fin(L.lathe("NT_Stud%s%d_L" % (nm, q), hp - dd * 0.004, dd, [(0, 0.017), (0.008, 0.017), (0.01, 0.012), (0.0085, 0.011), (0.0085, 0)],
                                segs=8, ring_mats={3: "emis"}, bone=bn, bevel=0.0), True)
    # upper-arm plate
    U = lambda th0, th1, u0, u1: L.sweep(sh, el, FRONT, out_u, th0, th1, u0, u1)
    up = L.shell("NT_UpperArm_L", U([(0, 8), (1, 18)], [(0, 172), (1, 160)], [(0, 0.5), (0.5, 0.42), (1, 0.5)], [(0, 0.86), (0.5, 0.95), (1, 0.86)]),
                 12, 8, body, reg, offset=0.015, smooth=4, bone={"upperarm_l": 1.0}, bevel=0.0035, edge_drop=0.004,
                 grooves=[('t', 0.55, 0.03, 0.003), ('s', 0.5, 0.0080, 0.0008, (0.0, 0.5), 'stripe')])
    fin(up, True)
    # elbow guard
    ec = el + V(0, 0.01, 0)
    ed = (BACK * 0.9 + out_u * 0.25 - ua * 0.1 + fa * -0.25).normalized()
    eg = L.shell("NT_Elbow_L", L.sphere(ec, ed.cross(UP).normalized(), ed, [(0, -40), (0.5, -62), (1, -40)], [(0, 40), (0.5, 62), (1, 40)],
                                        [(0, -36), (0.5, -52), (1, -36)], [(0, 36), (0.5, 52), (1, 36)]),
                 8, 8, body, reg, shape=L.planes([(ed, 0.07)], p=6, base=L.ellipsoid(ec, 0.074, 0.074, 0.074)), smooth=2, clear=0.008,
                 bone={"lowerarm_l": 1.0}, bevel=0.004, maxskirt=0.018, grooves=[('t', 0.5, 0.04, 0.003)])
    fin(eg, True)
    # bracer: base ring, outer housing with the long emissive panel, wrist cuff
    out_f = V(1, 0, 0.3)
    top_f = V(0, -0.35, 1.0)
    F = lambda th0, th1, u0, u1: L.sweep(el, wr, top_f, out_f, th0, th1, u0, u1)
    flare = lambda s, t: 0.008 * max(0.0, 1 - t / 0.2) ** 2 + 0.004 * max(0.0, (t - 0.85) / 0.15)
    br = L.shell("NT_Bracer_L", F(0, 360, [(0, 0.16), (0.25, 0.10), (0.5, 0.16), (0.75, 0.22), (1, 0.16)], const_s(0.9)),
                 16, 5, body, reg, offset=0.028, smooth=4, wrap=True, bulge=flare, bone={"lowerarm_l": 1.0}, bevel=0.004,
                 grooves=[('t', 0.3, 0.02, 0.003), ('t', 0.8, 0.02, 0.003)])
    fin(br, True)
    hs = L.shell("NT_BracerPanel_L", F([(0, 32), (0.12, 26), (0.88, 26), (1, 34)], [(0, 148), (0.12, 154), (0.88, 154), (1, 146)],
                                       [(0, 0.16), (0.5, 0.10), (1, 0.16)], [(0, 0.86), (0.5, 0.92), (1, 0.86)]),
                 10, 8, body, reg, offset=0.052, smooth=5, bone={"lowerarm_l": 1.0}, bevel=0.004,
                 emis=[(0.345, 0.655, 0.16, 0.84)],
                 grooves=[('s', 0.33, 0.022, 0.004, (0.13, 0.87)), ('s', 0.67, 0.022, 0.004, (0.13, 0.87)),
                          ('t', 0.14, 0.02, 0.004, (0.33, 0.67)), ('t', 0.86, 0.02, 0.004, (0.33, 0.67))])
    fin(hs, True)
    cu = L.shell("NT_WristCuff_L", F(0, 360, const_s(0.86), const_s(1.0)), 16, 3, body, reg, offset=lambda s, t: 0.036 - 0.012 * t, smooth=3,
                 wrap=True, bone={"lowerarm_l": 1.0}, bevel=0.0035, mat="armor2")
    fin(cu, True)
    # gauntlet: back-of-hand plate, knuckle guard, finger lames
    idx, pnk, mid = B("index_01_l"), B("pinky_01_l"), B("middle_01_l")
    dors = (idx - wr).cross(pnk - wr).normalized()
    if dors.x < 0: dors = -dors
    acr = (idx - pnk).normalized()
    H = L.sweep(wr, mid, dors, acr, [(0, -78), (1, -70)], [(0, 78), (1, 70)], [(0, 0.12), (0.5, 0.04), (1, 0.12)], [(0, 0.9), (0.5, 1.02), (1, 0.92)])
    hp = L.shell("NT_HandPlate_L", H, 6, 4, body, reg, offset=0.013, smooth=3, bone={"hand_l": 1.0}, bevel=0.003,
                 grooves=[('t', 0.5, 0.05, 0.0025)], far=0.2)
    fin(hp, True)
    kn = [B("%s_01_l" % f_) for f_ in ("index", "middle", "ring", "pinky")]
    pts = []
    for q in kn:
        for a_, b_ in ((0.006, -0.016), (0.024, -0.008), (0.024, 0.012), (0.006, 0.02)):
            pts.append(q + dors * a_ + (mid - wr).normalized() * b_)
    pts += [kn[0] + acr * 0.012 + dors * 0.008, kn[3] - acr * 0.01 + dors * 0.006]
    fin(L.hull("NT_Knuckle_L", pts, bone={"hand_l": 1.0}, bevel=0.002, angle=18), True)
    for fng in ("index", "middle", "ring", "pinky", "thumb"):
        j = [B("%s_0%d_l" % (fng, k)) for k in (1, 2, 3)]
        j.append(j[2] + (j[2] - j[1]) * 0.85)
        fd = dors
        if fng == "thumb":
            fd = (j[1] - j[0]).cross(dors).cross(j[1] - j[0]).normalized()
            fd = (dors * 0.4 + (j[0] - mid).normalized() * 0.8).normalized()
        for k in range(3):
            a, b = j[k], j[k + 1]
            side = (b - a).cross(fd).normalized()
            smp = L.sweep(a, b, fd, side, -104, 104, [(0, 0.14), (0.5, 0.06), (1, 0.14)], [(0, 0.82), (0.5, 0.92 if k < 2 else 0.86), (1, 0.82)])
            fp = L.shell("NT_F_%s%d_L" % (fng, k + 1), smp, 4, 1, body, ("%s_" % fng, "hand_l"), offset=0.0075, smooth=1,
                         bone={"%s_0%d_l" % (fng, k + 1): 1.0}, bevel=0.0015, far=0.06, sink=0.002, clear=0.003, segs=1)
            fin(fp, True)


# ------------------------------------------------------------------ helmet
def build_helmet():
    hb = {"head": 1.0}
    hc = V(0, -0.02, 1.712)
    RX, RY, RZ = 0.122, 0.146, 0.150
    dome_e = L.ellipsoid(hc, RX, RY, RZ)
    # liner (closes the helmet, hides the hollow neck)
    lin = L.lathe("NT_HelmLiner", hc + V(0, 0, -0.014), UP, [(-0.128 * math.cos(math.radians(a)), 0.1 * math.sin(math.radians(a))) for a in range(0, 181, 15)],
                  segs=14, bone=hb, mat="suit")
    for v in lin.data.vertices: v.co.y = hc.y + 0.004 + (v.co.y - hc.y) * 1.14
    fin(lin)
    # sphere(): pole = X, zero = front, positive phi turns DOWN (up is negative phi)
    Sd = lambda ph0, ph1, la0, la1: L.sphere(hc, LEFT, FRONT, ph0, ph1, la0, la1)
    brow = [(0, -8), (0.2, -17), (0.36, -13), (0.5, -3), (0.64, -13), (0.8, -17), (1, -8)]
    dome = L.shell("NT_HelmDome", Sd([(0, -236), (0.5, -240), (1, -236)], brow, -82, 82),
                   20, 20, shape=dome_e, thick=0.014, bone=hb, bevel=0.004,
                   grooves=[('t', 0.3, 0.012, 0.003)])
    fin(dome)
    # brow ledge: overhangs the visor recess, dips to a point at the centre
    ledge = L.shell("NT_HelmBrow", Sd([(s_, v_ - 15) for s_, v_ in brow], [(s_, v_ + 3) for s_, v_ in brow], -70, 70), 16, 2,
                    shape=L.ellipsoid(hc, RX + 0.013, RY + 0.017, RZ + 0.008), thick=0.02, bone=hb, bevel=0.004)
    fin(ledge)
    # central ridge, front to back
    ridge = L.shell("NT_HelmRidge", Sd(-232, -2, [(0, -4), (0.5, -5), (1, -5.5)], [(0, 4), (0.5, 5), (1, 5.5)]), 2, 20,
                    shape=L.ellipsoid(hc, RX + 0.008, RY + 0.02, RZ + 0.007), thick=0.016, bone=hb, bevel=0.004)
    fin(ridge)
    # faceplate: rounded-polyhedron prow narrowing to a pointed chin, with a recessed visor band
    fc = hc + V(0, 0.0, -0.024)
    prow = L.planes([((0.74, -0.67, 0.12), 0.106), ((-0.74, -0.67, 0.12), 0.106),
                     ((0.5, -0.74, -0.45), 0.121), ((-0.5, -0.74, -0.45), 0.121),
                     ((0, -0.9, 0.44), 0.136), ((0, -0.35, -0.94), 0.16)], p=9,
                    base=L.ellipsoid(fc, 0.126, 0.167, 0.22))
    Sf = lambda ph0, ph1, la0, la1: L.sphere(fc, LEFT, FRONT, ph0, ph1, la0, la1)

    def recess(s, t):
        a = abs(s - 0.5)
        band = max(0.0, 1 - abs(t - 0.2) / 0.13)
        side = min(1.0, max(0.0, (a - 0.035) / 0.03)) * min(1.0, max(0.0, (0.43 - a) / 0.05))
        return 0.0
    face = L.shell("NT_HelmFace", Sf([(0, -10), (0.5, -26), (1, -10)], [(0, 44), (0.3, 58), (0.5, 82), (0.7, 58), (1, 44)],
                                     [(0, -86), (0.4, -84), (0.7, -56), (1, -13)], [(0, 86), (0.4, 84), (0.7, 56), (1, 13)]),
                   22, 16, shape=prow, bulge=recess, thick=0.02, bone=hb, bevel=0.004)
    fin(face)
    # nose guard down the centre, mouth plate under it
    blade = L.shell("NT_HelmBlade", Sf(-26, 40, [(0, -4.5), (0.35, -8), (1, -5)], [(0, 4.5), (0.35, 8), (1, 5)]), 2, 8,
                    shape=lambda o, d: prow(o, d) + 0.009, thick=0.014, bone=hb, bevel=0.003)
    fin(blade)
    mouth = L.shell("NT_HelmMouth", Sf(36, [(0, 60), (0.5, 76), (1, 60)], [(0, -19), (1, -7)], [(0, 19), (1, 7)]), 6, 3,
                    shape=lambda o, d: prow(o, d) + 0.006, thick=0.012, bone=hb, bevel=0.003, mat="armor2",
                    grooves=[('t', 0.4, 0.06, 0.002), ('t', 0.7, 0.06, 0.002)])
    fin(mouth)
    # eye sockets: dark slanted panel in the recess, three round lenses in a tight triangle
    eye = L.shell("NT_HelmEye_L", Sf([(0, -7), (1, -19)], [(0, 13), (1, 5)], [(0, 9), (1, 11)], [(0, 50), (1, 44)]), 5, 3,
                  shape=lambda o, d: prow(o, d) + 0.0035, thick=0.008, bone=hb, bevel=0.002, mat="armor2")
    fin(eye, True)
    for q, (ph, la, r) in enumerate(((-2, 19, 0.0095), (-9, 33, 0.012), (4.5, 31, 0.009))):
        _, dd = Sf(ph, ph, la, la)(0, 0)
        rr = prow(fc, dd) + 0.0035
        fin(L.lathe("NT_EyeLight%d_L" % q, fc + dd * (rr - 0.003), dd, [(0, r + 0.004), (0.0075, r + 0.004), (0.0075, r), (0.0045, r - 0.001), (0.0045, 0)],
                    segs=10, ring_mats={3: "emis"}, bone=hb, mat="armor"), True)
    # cheek plates tapering to the chin
    jaw = L.shell("NT_HelmJaw_L", Sf([(0, 15), (1, 6)], [(0, 54), (1, 72)], [(0, 31), (0.5, 21), (1, 9)], [(0, 86), (0.5, 64), (1, 30)]), 7, 6,
                  shape=lambda o, d: prow(o, d) + 0.01, thick=0.016, bone=hb, bevel=0.004,
                  grooves=[('t', 0.5, 0.05, 0.003), ('s', 0.16, 0.0200, 0.0008, None, 'stripe')])
    fin(jaw, True)
    # nape guard, rear light slit in a frame
    nape = L.shell("NT_HelmNape", Sd(-226, -198, [(0, -66), (1, -54)], [(0, 66), (1, 54)]), 12, 2,
                   shape=L.ellipsoid(hc, RX + 0.008, RY + 0.009, RZ + 0.008), thick=0.016, bone=hb, bevel=0.0035, mat="armor2")
    fin(nape)
    fin(L.shell("NT_HelmRearFrame", Sd(-182, -163, -36, 36), 8, 2, shape=L.ellipsoid(hc, RX + 0.004, RY + 0.005, RZ + 0.004), thick=0.01,
                bone=hb, bevel=0.002, mat="armor2"))
    fin(L.shell("NT_HelmRearLight", Sd(-176, -169, -30, 30), 8, 1, shape=L.ellipsoid(hc, RX + 0.006, RY + 0.007, RZ + 0.006), thick=0.01,
                bone=hb, bevel=0.0015, mat="emis"))
    # ear discs with a raised glowing ring
    ec = V(RX - 0.014, -0.014, 1.688)
    fin(L.lathe("NT_Ear_L", ec, LEFT, [(0, 0.06), (0.02, 0.06), (0.027, 0.054), (0.027, 0.047), (0.033, 0.045), (0.033, 0.035), (0.027, 0.033),
                                        (0.027, 0.02), (0.032, 0.018), (0.032, 0)], segs=18, ring_mats={4: "emis"}, bone=hb, bevel=0.0), True)
    # antenna fins: thin blades from just behind the ear discs, swept back about 30 degrees from vertical
    pts = []
    for (y, z, th) in ((0.016, 1.70, 1.0), (0.05, 1.688, 1.0), (0.078, 1.80, 0.7), (0.099, 1.792, 0.7), (0.154, 1.898, 0.25), (0.161, 1.889, 0.25)):
        cx = RX + 0.014 + (z - 1.7) * 0.05
        pts += [(cx - 0.0055 * th, y, z), (cx + 0.0055 * th, y, z)]
    fin(L.hull("NT_Fin_L", pts, bone=hb, bevel=0.0015, angle=20), True)
    # neck seal: three rings
    for k, (z0, z1, off, bn) in enumerate(((1.532, 1.556, 0.014, {"neck_01": 1.0}), (1.558, 1.582, 0.012, {"neck_01": 0.5, "neck_02": 0.5}),
                                           (1.584, 1.612, 0.010, {"neck_02": 1.0}))):
        fin(L.shell("NT_Neck%d" % k, L.sweep(V(0, 0.004, 0), V(0, 0.004, 1), FRONT, LEFT, 0, 360, const_s(z0), const_s(z1)), 14, 1,
                    shape=(lambda rr: (lambda o, d: rr * (1.0 + 0.12 * abs(d.y))))(0.06 + off), thick=0.02, wrap=True, bone=bn, bevel=0.003, mat="armor2"))


# ------------------------------------------------------------------ undersuit segment bands (raised ribs where the suit shows)
def build_bands():
    n = 0
    for z0 in (1.062, 1.102, 1.142, 1.182, 1.222):
        fin(L.shell("NT_Band_T%d" % n, T(0, 360, const_s(z0), const_s(z0 + 0.03)), 20, 1, body, L.R_TORSO_ARMS, far=0.24, offset=0.003, smooth=1,
                    wrap=True, bone="transfer", bevel=0.0, mat="suit", sink=0.003)); n += 1
    sh, el = B("upperarm_l"), B("lowerarm_l")
    for k, u0 in enumerate((0.34, 0.5, 0.66, 0.82)):
        fin(L.shell("NT_Band_A%d_L" % k, L.sweep(sh, el, FRONT, V(0.82, 0, 0.57), 0, 360, const_s(u0), const_s(u0 + 0.11)), 12, 1, body, L.R_ARM_L,
                    offset=0.003, smooth=1, wrap=True, bone="transfer", bevel=0.0, mat="suit", sink=0.003), True)
    hipj, knee, ank = B("thigh_l"), B("calf_l"), B("foot_l")
    for k, u0 in enumerate((0.2, 0.33, 0.46, 0.59, 0.72)):
        fin(L.shell("NT_Band_L%d_L" % k, L.sweep(hipj, knee, FRONT, LEFT, -175, -15, const_s(u0), const_s(u0 + 0.095)), 8, 1, body, L.R_LEG_L,
                    offset=0.003, smooth=1, bone="transfer", bevel=0.0, mat="suit", sink=0.003), True)
    fin(L.shell("NT_Band_K0_L", L.sweep(hipj, knee, FRONT, LEFT, 0, 360, const_s(0.87), const_s(0.955)), 12, 1, body, L.R_LEG_L,
                offset=0.003, smooth=1, wrap=True, bone="transfer", bevel=0.0, mat="suit", sink=0.003), True)
    fin(L.shell("NT_Band_K1_L", L.sweep(knee, ank, FRONT, LEFT, 0, 360, const_s(0.02), const_s(0.1)), 12, 1, body, L.R_LEG_L,
                offset=0.003, smooth=1, wrap=True, bone="transfer", bevel=0.0, mat="suit", sink=0.003), True)


# ------------------------------------------------------------------ run
for o in list(bpy.data.objects):
    if o.name.startswith(("NT_", "FIT_")): bpy.data.objects.remove(o)
build_body()
if want("torso"): build_torso()
if want("leg"): build_leg()
if want("boot"): build_boot()
if want("arm"): build_arm()
if want("helmet"): build_helmet()
if want("bands"): build_bands()
manny.hide_render = True; manny.hide_viewport = True
manny.hide_set(True) if manny.name in bpy.context.view_layer.objects else None
tris = 0
for o in L.PARTS:
    o.data.calc_loop_triangles(); tris += len(o.data.loop_triangles)
zs = [(o.matrix_world @ v.co).z for o in L.PARTS for v in o.data.vertices]
print("NT_BUILD parts %d tris %d body_tris %d height %.4f minz %.4f" % (len(L.PARTS), tris, len(bpy.data.objects["NT_Body"].data.loop_triangles), max(zs), min(zs)))
import collections
cat = collections.Counter()
for o in L.PARTS: cat[o.name.split("_")[1][:6]] += len(o.data.loop_triangles)
print("NT_CAT", sorted(cat.items(), key=lambda kv: -kv[1]))
big = sorted(((len(o.data.loop_triangles), o.name) for o in L.PARTS), reverse=True)[:14]
print("NT_BIG", big)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(ROOT, OUT))
