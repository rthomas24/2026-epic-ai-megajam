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
VARIANT = argv[argv.index("--variant") + 1] if "--variant" in argv else "trooper"
# what each enemy type carries (same body, same skeleton)
F = dict(pack=True, pack_scale=1.0, lames=("A", "B", "C"), pauldron_scale=1.0, thigh_outer=True, thigh_back=True, shin_low=True, calf=True,
         shin_off=0.0, thigh_off=0.0, fins=0.15, crest=False, ovals="both", wrist_band=False, scapula=True, flank=True, upperarm=True, hip=True,
         hoses=True, pouches=(42, 72, 112, 142), gorget=False, chest2=False, shield=False, shin_stripes=True, crest_blade=False, ear_r=0.06,
         bracer=True, elbow=True)
if VARIANT == "grunt":      # melee, fast, fragile: less armour, more undersuit, short fins, glowing right wrist band
    F.update(pack=False, lames=("A",), pauldron_scale=0.6, crest_blade=True, ear_r=0.04, bracer=False,
             thigh_outer=False, thigh_back=False, shin_low=False, calf=False, shin_off=-0.008, fins=0.0, ovals="none",
             wrist_band=True, scapula=False, flank=False, upperarm=False, hip=False, hoses=False, pouches=(42, 142), shin_stripes=False)
elif VARIANT == "heavy":    # shielded, slow, tough: bigger pauldrons, gorget, second chest plate, left forearm shield, crest instead of fins
    F.update(pack_scale=1.25, lames=("A", "B", "C", "D"), pauldron_scale=1.3, shin_off=0.008, thigh_off=0.008, fins=0.0, crest=True, ovals="right",
             gorget=True, chest2=True, shield=True)

arm = bpy.data.objects["root"]
manny = bpy.data.objects["Manny_Body"]
body = L.Body(manny, arm)
B = body.bone
FRONT, BACK, UP, LEFT = V(0, -1, 0), V(0, 1, 0), V(0, 0, 1), V(1, 0, 0)


def want(tag):
    return ONLY is None or tag in ONLY


def fin(ob, mirrored=False):
    return L.finish(ob, mirrored)


def fin_side(ob, keep):
    """Build a left-side part, keep only the 'L' or the mirrored 'R' copy."""
    obs = L.finish(ob, True)
    drop = obs[0] if keep == "R" else obs[1]
    L.PARTS.remove(drop); bpy.data.objects.remove(drop)
    return obs[1] if keep == "R" else obs[0]


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
    while new.uv_layers: new.uv_layers.remove(new.uv_layers[0])
    new.uv_layers.new(name="UVMap")
    L.smart_unwrap(ob)
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
    chest = L.shell("NT_Chest", cross=True, sample=T(-66, 66, [(0, 1.30), (0.2, 1.262), (0.5, 1.235), (0.8, 1.262), (1, 1.30)],
                                  [(0, 1.40), (0.16, 1.475), (0.36, 1.492), (0.5, 1.452), (0.64, 1.492), (0.84, 1.475), (1, 1.40)]),
                    ns=26, nt=12, body=body, region=reg, offset=0.016, smooth=5, bone={"spine_05": 1.0}, bevel=0.004,
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
    p, d = torso_point(0, 1.385, cb)
    hexo = lambda r: [(r * math.cos(math.radians(a)), r * math.sin(math.radians(a))) for a in range(0, 360, 60)]
    fin(L.prism("NT_CoreHousing", [(-0.082, 0.074), (0.082, 0.074), (0.1, 0.0), (0.06, -0.088), (-0.06, -0.088), (-0.1, 0.0)],
                p - d * 0.03, LEFT, UP, d, 0.052, taper=0.86, bone={"spine_05": 1.0}, bevel=0.006, mat="armor2"))
    fin(L.prism("NT_CoreBezel", hexo(0.074), p + d * 0.016, LEFT, UP, d, 0.014, taper=0.92, bone={"spine_05": 1.0}, bevel=0.003))
    fin(L.prism("NT_Core", hexo(0.064), p + d * 0.03, LEFT, UP, d, 0.012, taper=0.94, cap_mat="emiscore",
                inset=(0.01, 0.006), bone={"spine_05": 1.0}, bevel=0.0025, mat="armor2"))
    # --- hoses: one dark ribbed U-loop per side, housing lower corner -> port on the side of the ribcage, on the surface
    hr = 0.013
    tb = body.bvh(L.R_TORSO_ARMS)

    def on_torso(th, z, lift):
        dd = FRONT * math.cos(math.radians(th)) + LEFT * math.sin(math.radians(th))
        o = V(0, -0.02, z)
        r = L.raycast_r(tb, o, dd, 0.24)
        rc = L.raycast_r(cb, o, dd)
        r = max((r or 0.0) + 0.02, rc or 0.0)
        if r < 0.06: r = 0.15
        return o + dd * (r + lift), dd
    if F["hoses"]:
        for sgn in (1, -1):
            th0, th1, z0, z1 = 17, 60, 1.288, 1.282
            path = [p + LEFT * sgn * 0.05 + UP * -0.07 - d * 0.012]
            for q in range(7):
                f_ = q / 6.0
                pt, dd = on_torso(sgn * (th0 + (th1 - th0) * f_), z0 + (z1 - z0) * f_ - 0.06 * math.sin(math.pi * f_) ** 0.8, hr + 0.002)
                path.append(pt)
            pe, de = on_torso(sgn * th1, z1, 0.0)
            path.append(pe - de * 0.01)
            sfx = "L" if sgn > 0 else "R"
            fin(L.tube("NT_Hose_%s0" % sfx, path, hr, segs=7, ribs=14, rib_depth=0.003, bone={"spine_05": 0.5, "spine_04": 0.5},
                       mat="rubber", groove_mat="emisline", groove_every=3))
            fin(L.lathe("NT_HosePort_%s0" % sfx, pe - de * 0.01, de, [(0.0, 0.024), (0.026, 0.024), (0.034, 0.018), (0.034, 0.0)], segs=10,
                        bone={"spine_04": 1.0}, mat="armor2"))
    # --- shoulder straps (over the trapezius), one per side
    sa, sb = V(0.0, 0.01, 1.385), V(1.0, 0.01, 1.385)
    strap = L.shell("NT_Strap_L", L.sweep(sa, sb, FRONT, UP, [(0, -18), (1, -18)], [(0, 196), (1, 196)], 0.055, 0.118),
                    5, 18, body, ("spine", "clavicle"), offset=0.022, smooth=3, bone={"spine_05": 0.7, "clavicle_l": 0.3}, bevel=0.004,
                    grooves=[('t', 0.3, 0.02, 0.004), ('t', 0.7, 0.02, 0.004)])
    fin(strap, True)
    # --- collar ring at the base of the neck
    col = L.shell("NT_Collar", T(0, 360, const_s(1.485), [(0, 1.515), (0.25, 1.535), (0.5, 1.56), (0.75, 1.535), (1, 1.515)]),
                  24, 3, body, L.R_TORSO, offset=0.014, smooth=2, wrap=True, bone={"spine_05": 0.5, "neck_01": 0.5}, bevel=0.003)
    fin(col)
    if F["flank"]:
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
    bk = L.shell("NT_Back", cross=True, sample=T(180 - 62, 180 + 62, [(0, 1.30), (0.25, 1.255), (0.5, 1.24), (0.75, 1.255), (1, 1.30)],
                               [(0, 1.41), (0.2, 1.475), (0.5, 1.50), (0.8, 1.475), (1, 1.41)]),
                 ns=22, nt=10, body=body, region=reg, offset=0.014, smooth=5, bone={"spine_05": 1.0}, bevel=0.004,
                 grooves=[('t', 0.4, 0.03, 0.004)], edge_drop=0.005)
    fin(bk)
    if F["scapula"]:
        sb_ = L.shell("NT_Scapula_L", T([(0, 180 - 12), (1, 180 - 15)], [(0, 180 - 52), (0.5, 180 - 64), (1, 180 - 50)],
                                        [(0, 1.30), (0.5, 1.285), (1, 1.33)], [(0, 1.45), (0.5, 1.47), (1, 1.44)]),
                      9, 8, body, reg, offset=0.03, smooth=5, bone={"spine_05": 1.0}, bevel=0.004,
                      grooves=[('t', 0.5, 0.035, 0.0035), ('s', 0.85, 0.0120, 0.001, None, 'stripe')], edge_drop=0.004)
        fin(sb_, True)
    for k, (z0, z1, bn) in enumerate(((1.165, 1.235, "spine_03"), (1.095, 1.16, "spine_02"), (1.045, 1.09, "spine_01"))):
        lb = L.shell("NT_Lumbar%d" % k, T(180 - 30 + 2 * k, 180 + 30 - 2 * k, [(0, z0 + 0.01), (0.5, z0), (1, z0 + 0.01)], [(0, z1 + 0.01), (0.5, z1), (1, z1 + 0.01)]),
                     8, 3, body, L.R_TORSO, offset=0.014, smooth=3, bone={bn: 1.0}, bevel=0.0035, grooves=[('s', 0.5, 0.04, 0.003)])
        fin(lb)
    # --- backpack
    if F["pack"]:
        yb = 0.14
        pk = {"spine_05": 0.6, "spine_04": 0.4}
        bs, bd_ = torso_point(180, 1.36, body.bvh(L.R_TORSO))
        y1 = bs.y + 0.014 + 0.08 * F["pack_scale"] ** 2          # 8 cm proud of the back plate (trooper)
        pack = L.hull("NT_Pack", [(x_ * F["pack_scale"], y_, 1.345 + (z_ - 1.345) * min(1.08, F["pack_scale"])) for x_, y_, z_ in [(0.082, yb, 1.488), (0.09, yb, 1.37), (0.062, yb, 1.20), (0.0, yb, 1.185),
                                  (0.05, y1 - 0.03, 1.478), (0.064, y1, 1.42), (0.064, y1, 1.30), (0.04, y1 - 0.03, 1.215), (0.0, y1 - 0.034, 1.2),
                                  (0.0, y1 - 0.03, 1.494), (0.0, y1 + 0.004, 1.36)]],
                      mirror_x=True, bone=pk, bevel=0.01, angle=14)
        fin(pack)
        pb = L.obj_bvh(pack)
        ps, pn = L.surf(pb, V(0, 0.1, 1.36), BACK)
        slot_o = [(-0.022, -0.105), (0.022, -0.105), (0.031, -0.092), (0.031, 0.092), (0.022, 0.105), (-0.022, 0.105), (-0.031, 0.092), (-0.031, -0.092)]
        fin(L.prism("NT_PackSlot", slot_o, V(0, ps.y - 0.02, 1.352), V(-1, 0, 0), UP, BACK, 0.026, taper=0.95, cap_mat="emis",
                    inset=(0.016, 0.01), bone=pk, bevel=0.003))
        for sx_, z_ in ((0.05, 1.445), (-0.05, 1.445), (0.048, 1.262), (-0.048, 1.262)):
            hp_, hn_ = L.surf(pb, V(sx_, 0.1, z_), BACK)
            if hp_:
                fin(L.lathe("NT_PackLight%d" % int((sx_ > 0) * 2 + (z_ > 1.3)), hp_ - BACK * 0.004, BACK,
                            [(0, 0.011), (0.007, 0.011), (0.008, 0.0075), (0.0065, 0.0068), (0.0065, 0)], segs=10, ring_mats={3: "emis"}, bone=pk))
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
    fin(belt_box("NT_Buckle", 0, 0.118, 0.072, 0.022, bevel=0.005))
    fin(belt_box("NT_BuckleCore", 0, 0.05, 0.036, 0.03, bevel=0.004, mat="armor2"))
    fin(belt_box("NT_BeltBackPlate", 180, 0.075, 0.06, 0.016, bevel=0.004))
    fin(belt_box("NT_BeltBackCore", 180, 0.03, 0.03, 0.021, bevel=0.003, mat="armor2"))
    for th in F["pouches"]:
        for sgn in (1, -1):
            nm = "NT_Pouch%d_%s" % (th, "L" if sgn > 0 else "R")
            fin(belt_box(nm, sgn * th, 0.07, 0.078, 0.048, zc=1.01, bevel=0.006, mat="armor2"))
            fin(belt_box(nm + "_Flap", sgn * th, 0.074, 0.034, 0.053, zc=1.036, bevel=0.004, mat="armor2"))
    # --- pelvis: codpiece, hip plates, rear plate
    cod = L.shell("NT_Cod", T([(0, -26), (1, -5)], [(0, 26), (1, 5)], const_s(0.992), [(0, 0.90), (0.5, 0.842), (1, 0.90)]),
                  8, 6, body, L.R_TORSO + ("thigh",), offset=0.014, smooth=3, bone={"pelvis": 1.0}, bevel=0.004,
                  grooves=[('s', 0.5, 0.04, 0.003)])
    fin(cod)
    if F["hip"]:
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
    th = L.shell("NT_ThighFront_L", cross=True, sample=S([(0, -34), (1, -38)], [(0, 120), (1, 100)],
                                      [(0, 0.34), (0.3, 0.17), (0.62, 0.10), (1, 0.06)], [(0, 0.74), (0.34, 0.86), (0.7, 0.82), (1, 0.72)]),
                 ns=16, nt=12, body=body, region=reg, offset=0.016 + F["thigh_off"], smooth=5, bone={"thigh_l": 1.0}, bevel=0.004, edge_drop=0.005,
                 bulge=lambda s, t: 0.009 * max(0.0, 1 - abs(s - 0.8) / 0.09),
                 grooves=[('s', 0.36, 0.02, 0.0035), ('t', 0.62, 0.025, 0.0035, (0.36, 0.93)),
                          ('s', 0.07, 0.013, 0.0008, (0.07, 0.93), 'stripe'), ('s', 0.93, 0.013, 0.0008, (0.07, 0.93), 'stripe'),
                          ('t', 0.07, 0.012, 0.0008, (0.07, 0.93), 'stripe'), ('t', 0.93, 0.012, 0.0008, (0.07, 0.93), 'stripe')])
    fin(th, True)
    if F["thigh_outer"]:
        tho = L.shell("NT_ThighOuter_L", S([(0, 48), (1, 60)], [(0, 132), (1, 118)], [(0, 0.16), (0.5, 0.10), (1, 0.14)],
                                           [(0, 0.5), (0.5, 0.64), (1, 0.52)]),
                      8, 8, body, reg, offset=0.024 + F["thigh_off"], smooth=5, bone={"thigh_l": 1.0}, bevel=0.004, edge_drop=0.004,
                      grooves=[('t', 0.3, 0.03, 0.0035), ('s', 0.5, 0.0120, 0.0008, (0.36, 0.9), 'stripe')])
        fin(tho, True)
    if F["thigh_back"]:
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
    flare = lambda s, t: 0.02 * max(0.0, 1 - t / 0.1) + 0.008 * max(0.0, 1 - abs(s - 0.5) / 0.28)
    sh = L.shell("NT_Shin_L", cross=True, sample=C(-100, 100, [(0, 0.2), (0.25, 0.15), (0.5, 0.20), (0.75, 0.15), (1, 0.2)],
                                [(0, 0.80), (0.5, 0.93), (1, 0.80)]),
                 ns=16, nt=12, body=body, region=L.R_LEG_L, offset=lambda s, t: 0.024 + F["shin_off"] - 0.013 * t, smooth=5, bulge=flare, bone={"calf_l": 1.0}, bevel=0.004, edge_drop=0.004,
                 grooves=[('s', 0.26, 0.018, 0.0035, (0.06, 0.94)), ('s', 0.74, 0.018, 0.0035, (0.06, 0.94)), ('t', 0.34, 0.025, 0.0035, (0.26, 0.74)),
                          ('s', 0.06, 0.014, 0.0008, (0.06, 0.94), 'stripe'), ('s', 0.94, 0.014, 0.0008, (0.06, 0.94), 'stripe'),
                          ('t', 0.06, 0.012, 0.0008, (0.06, 0.94), 'stripe'), ('t', 0.94, 0.012, 0.0008, (0.06, 0.94), 'stripe')])
    fin(sh, True)
    if F["shin_low"]:
        sh2 = L.shell("NT_ShinLow_L", C([(0, -50), (1, -42)], [(0, 50), (1, 42)], [(0, 0.56), (0.5, 0.46), (1, 0.56)], [(0, 0.86), (0.5, 0.97), (1, 0.86)]),
                      8, 7, body, L.R_LEG_L, offset=0.024, smooth=5, bone={"calf_l": 1.0}, bevel=0.004, edge_drop=0.004,
                      bulge=lambda s, t: 0.008 * max(0.0, 1 - abs(s - 0.5) / 0.4),
                      grooves=[('t', 0.5, 0.04, 0.0035)])
        fin(sh2, True)
    if F["calf"]:
        cf = L.shell("NT_Calf_L", C(96, 264, [(0, 0.14), (0.5, 0.10), (1, 0.14)], [(0, 0.80), (0.5, 0.72), (1, 0.80)]),
                     12, 10, body, L.R_LEG_L, offset=lambda s, t: 0.016 - 0.006 * t, smooth=4, bone={"calf_l": 1.0}, bevel=0.0035, edge_drop=0.004,
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
    vamp = [dict(y=Lb - 0.004, w=0.07, h=0.024, z=zb + 0.012, n=2.6), dict(y=Lb - 0.02, w=0.11, h=0.045, z=zb + 0.018, n=2.8),
            dict(y=Lb - 0.06, w=0.127, h=0.06, z=zb + 0.024, n=3.0), dict(y=Lb - 0.11, w=0.13, h=0.072, z=zb + 0.03, n=3.0),
            dict(y=0.16, w=0.124, h=0.11, z=zb + 0.048, n=2.8), dict(y=0.115, w=0.108, h=0.14, z=zb + 0.065, n=2.6),
            dict(y=0.06, w=0.104, h=0.15, z=zb + 0.07, n=2.6), dict(y=0.012, w=0.098, h=0.13, z=zb + 0.06, n=2.6),
            dict(y=-0.004, w=0.07, h=0.09, z=zb + 0.045, n=2.4)]
    fin(L.loft("NT_BootVamp_L", vamp[::-1], o, side, f, UP, ring=16, bone=bone, mat="armor2"), True)
    shaft = [dict(y=0.075, w=0.104, h=0.132, z=0.058), dict(y=0.13, w=0.106, h=0.13, z=0.056), dict(y=0.19, w=0.112, h=0.128, z=0.052),
             dict(y=0.215, w=0.12, h=0.136, z=0.05), dict(y=0.225, w=0.112, h=0.128, z=0.05)]
    fin(L.loft("NT_BootShaft_L", shaft, o, side, UP, f, ring=16, bone=bone, mat="armor2"), True)
    # armoured toe cap, heel counter, straps, buckle, heel light
    toe = [dict(y=Lb + 0.001, w=0.076, h=0.027, z=zb + 0.011, n=2.6), dict(y=Lb - 0.02, w=0.118, h=0.05, z=zb + 0.02, n=2.8),
           dict(y=Lb - 0.06, w=0.135, h=0.066, z=zb + 0.026, n=3.0), dict(y=Lb - 0.098, w=0.138, h=0.078, z=zb + 0.031, n=3.0)]
    fin(L.loft("NT_BootToe_L", toe[::-1], o, side, f, UP, ring=16, bone=bone, bevel=0.003, angle=40), True)
    fin(L.hull("NT_BootHeel_L", [o + side * sx * 0.058 + f * yy + UP * zz for sx in (-1, 1) for yy, zz in
                                 ((-0.016, 0.04), (0.06, 0.04), (-0.018, 0.105), (0.05, 0.125), (-0.008, 0.14))] +
               [o + side * sx * 0.04 + f * -0.024 + UP * zz for sx in (-1, 1) for zz in (0.045, 0.1)],
               bone=bone, bevel=0.004, angle=18), True)
    fin(L.box("NT_BootHeelLight_L", o - f * 0.02 + UP * 0.072, side, UP, -f, 0.05, 0.014, 0.008, bone=bone, bevel=0.002, mat="emis"), True)
    strap1 = [dict(y=0.142, w=0.146, h=0.122, z=zb + 0.05, n=2.8), dict(y=0.186, w=0.15, h=0.104, z=zb + 0.043, n=2.8)]
    fin(L.loft("NT_BootStrapA_L", strap1, o, side, f, UP, ring=16, bone=bone, bevel=0.003, angle=40), True)
    strap2 = [dict(y=0.112, w=0.128, h=0.156, z=0.056), dict(y=0.154, w=0.13, h=0.156, z=0.054)]
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
    pc = sh + V(0.022, 0.004, -0.034)
    PH = lambda ph0, ph1, la0, la1: L.sphere(pc, FRONT, UP, ph0, ph1, la0, la1)
    # FRONT x UP = (-1,0,0): positive phi turns toward the neck; negative phi goes outward and down the arm.
    lam = [("A", -72, 48, 0.142, 0.132, 0.126, {"clavicle_l": 0.4, "upperarm_l": 0.6}, 64),
           ("B", -108, -52, 0.128, 0.12, 0.118, {"clavicle_l": 0.2, "upperarm_l": 0.8}, 58),
           ("C", -140, -92, 0.114, 0.108, 0.11, {"upperarm_l": 1.0}, 52),
           ("D", -162, -122, 0.102, 0.098, 0.102, {"upperarm_l": 1.0}, 48)]
    PSC = F["pauldron_scale"]
    lam = [(n_, a_, b_, rx_ * PSC, ry_ * PSC, rz_ * (1 + (PSC - 1) * 0.6), bn_, lw_) for n_, a_, b_, rx_, ry_, rz_, bn_, lw_ in lam if n_ in F["lames"]]
    for nm, p0, p1, rx, ry, rz, bn, lw in lam:
        shp = L.ellipsoid(pc, rx, ry, rz)
        pd = L.shell("NT_Pauldron%s_L" % nm, segs=2, sample=PH([(0, p0 + 10), (0.5, p0), (1, p0 + 10)], [(0, p1 - 6), (0.5, p1), (1, p1 - 6)], -lw, lw),
                     ns=10, nt=7, body=body, region=L.R_SHOULDER_L, shape=shp, smooth=2, clear=0.01, bone=bn, bevel=0.005, inner=True, thick=0.016, maxskirt=0.018,
                     grooves=[('t', 0.5, 0.035, 0.003)] if nm == "A" else [('s', 0.5, 0.03, 0.003)], far=0.5, cross=(nm == "A"))
        fin(pd, True)
        if nm in ("A", "B"):
            pbv = L.obj_bvh(pd)
            studs = ((-46, -28), (-46, 28)) if nm == "A" else ((-84, 0),)
            for q, (ph, la) in enumerate(studs):
                smp = L.sphere(pc, FRONT, UP, ph, ph, la, la)
                _, dd = smp(0, 0)
                hp, hn = L.surf(pbv, pc, dd)
                if hp:
                    fin(L.lathe("NT_Stud%s%d_L" % (nm, q), hp - dd * 0.004, dd, [(0, 0.019), (0.008, 0.019), (0.01, 0.0135), (0.0085, 0.0125), (0.0085, 0)],
                                segs=12, ring_mats={3: "emis"}, bone=bn, bevel=0.0), True)
    # upper-arm plate
    U = lambda th0, th1, u0, u1: L.sweep(sh, el, FRONT, out_u, th0, th1, u0, u1)
    if F["upperarm"]:
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
    F_ = lambda th0, th1, u0, u1: L.sweep(el, wr, top_f, out_f, th0, th1, u0, u1)
    flare = lambda s, t: 0.006 * max(0.0, 1 - t / 0.2) ** 2
    if F["bracer"]:
        br = L.shell("NT_Bracer_L", F_(0, 360, [(0, 0.16), (0.25, 0.10), (0.5, 0.16), (0.75, 0.22), (1, 0.16)], const_s(0.95)),
                     16, 5, body, reg, offset=lambda s, t: 0.019 - 0.009 * t, smooth=4, wrap=True, bulge=flare, bone={"lowerarm_l": 1.0}, bevel=0.004,
                     grooves=[('t', 0.3, 0.02, 0.003), ('t', 0.85, 0.02, 0.003)])
        fin(br, True)
    # long oval emissive panel (about 22 x 6 cm) on the outer face, hex grid only here
    oval = []
    for q in range(14):
        tc = (q + 0.5) / 14.0
        hw = 0.5 * 0.4 * math.sqrt(max(0.0, 1 - ((tc - 0.5) / 0.47) ** 2)) if abs(tc - 0.5) < 0.47 else 0.0
        if hw > 0.05: oval.append((0.5 - hw, 0.5 + hw, q / 14.0, (q + 1) / 14.0))
    if F["ovals"] != "none":
        hs = L.shell("NT_BracerPanel_L", F_([(0, 36), (0.15, 28), (0.85, 30), (1, 40)], [(0, 144), (0.15, 152), (0.85, 150), (1, 140)],
                                            [(0, 0.1), (0.5, 0.05), (1, 0.1)], [(0, 0.93), (0.5, 0.985), (1, 0.93)]),
                     12, 14, body, reg, offset=lambda s, t: 0.031 - 0.012 * t, smooth=5, bone={"lowerarm_l": 1.0}, bevel=0.004,
                     emis=oval, emis_key="emishex")
        if F["ovals"] == "both": fin(hs, True)
        else: fin_side(hs, "R")
    if F["wrist_band"]:       # the Grunt's baton-arm cue: a bright band round the RIGHT wrist
        wb = L.shell("NT_WristBand_L", F_(0, 360, const_s(0.74), const_s(0.93)), 14, 3, body, reg, offset=lambda s, t: 0.024 - 0.006 * t, smooth=3,
                     wrap=True, bone={"lowerarm_l": 1.0}, bevel=0.003, emis=[(0.0, 1.0, 0.34, 0.67)])
        fin_side(wb, "R")
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
    # ONE smooth helmet form: cranium ellipsoid above, longer below, cut by cheek / jaw planes that converge on the chin
    def base(o, d):
        rz = RZ if d.z >= 0 else 0.215
        return 1.0 / math.sqrt((d.x / RX) ** 2 + (d.y / RY) ** 2 + (d.z / rz) ** 2)
    H = L.planes([((0.76, -0.64, 0.1), 0.108), ((-0.76, -0.64, 0.1), 0.108),
                  ((0.56, -0.7, -0.44), 0.118), ((-0.56, -0.7, -0.44), 0.118),
                  ((0, -0.3, -0.95), 0.176)], p=9, base=base)
    # small dark liner deep inside (only closes the neck opening)
    lin = L.lathe("NT_HelmLiner", hc + V(0, 0.004, -0.03), UP, [(-0.1 * math.cos(math.radians(a)), 0.082 * math.sin(math.radians(a))) for a in range(0, 181, 20)],
                  segs=10, bone=hb, mat="suit")
    fin(lin)
    # sphere(): pole = X, zero = front, positive phi turns DOWN (up is negative phi)
    Sd = lambda ph0, ph1, la0, la1: L.sphere(hc, LEFT, FRONT, ph0, ph1, la0, la1)
    brow = [(0, -4), (0.14, -20), (0.3, -17), (0.5, 1), (0.7, -17), (0.86, -20), (1, -4)]   # V-shaped brow line
    crown = L.shell("NT_HelmDome", Sd([(0, -238), (0.5, -246), (1, -238)], brow, -84, 84),
                    22, 22, shape=lambda o, d: H(o, d) + 0.007, thick=0.012, bone=hb, bevel=0.004,
                    grooves=[('t', 0.34, 0.01, 0.0025)])
    fin(crown)
    ridge = L.shell("NT_HelmRidge", Sd(-236, -1, [(0, -3.5), (0.5, -4.5), (1, -5)], [(0, 3.5), (0.5, 4.5), (1, 5)]), 2, 20,
                    shape=lambda o, d: H(o, d) + 0.013, thick=0.012, bone=hb, bevel=0.003)
    fin(ridge)
    # face: continues the same form under the crown (no gap), down to the pointed chin
    face = L.shell("NT_HelmFace", Sd([(0, -26), (0.5, -30), (1, -26)], [(0, 40), (0.25, 60), (0.5, 84), (0.75, 60), (1, 40)],
                                     [(0, -88), (0.45, -88), (0.75, -72), (1, -30)], [(0, 88), (0.45, 88), (0.75, 72), (1, 30)]),
                   22, 16, shape=H, thick=0.02, bone=hb, bevel=0.004, mat="armor2")
    fin(face)
    # nose guard down the centre
    blade = L.shell("NT_HelmBlade", Sd(-2, 58, [(0, -5), (0.3, -7.5), (1, -4)], [(0, 5), (0.3, 7.5), (1, 4)]), 2, 8,
                    shape=lambda o, d: H(o, d) + 0.008, thick=0.012, bone=hb, bevel=0.003)
    fin(blade)
    # eye panels: angular trapezoids (about 6 x 3.5 cm) glowing at mid strength, three brighter lenses inside
    eye = L.shell("NT_HelmEye_L", Sd([(0, 3), (1, -13)], [(0, 17), (1, 3)], [(0, 9), (1, 10.5)], [(0, 38), (1, 35)]), 4, 3,
                  shape=lambda o, d: H(o, d) + 0.0025, thick=0.008, bone=hb, bevel=0.002, mat="emisdim")
    fin(eye, True)
    for q, (ph, la, r) in enumerate(((8, 15.5, 0.0062), (0, 24, 0.0082), (1, 33, 0.006))):
        _, dd = Sd(ph, ph, la, la)(0, 0)
        rr = H(hc, dd) + 0.0025
        fin(L.lathe("NT_EyeLight%d_L" % q, hc + dd * (rr - 0.002), dd, [(0, r + 0.0028), (0.0055, r + 0.0028), (0.0055, r), (0.0035, r - 0.0008), (0.0035, 0)],
                    segs=16, ring_mats={3: "emis"}, bone=hb, mat="armor"), True)
    # cheek plates converging on the narrow jaw
    jaw = L.shell("NT_HelmJaw_L", Sd([(0, 24), (1, 20)], [(0, 60), (1, 80)], [(0, 34), (0.5, 22), (1, 7)], [(0, 88), (0.5, 70), (1, 26)]), 8, 7,
                  shape=lambda o, d: H(o, d) + 0.009, thick=0.014, bone=hb, bevel=0.004,
                  grooves=[('t', 0.5, 0.05, 0.003), ('s', 0.12, 0.022, 0.0008, None, 'stripe')])
    fin(jaw, True)
    # pointed chin, 3 to 4 cm below the jaw line
    fin(L.hull("NT_HelmChin", [(0.026, -0.098, 1.602), (0.02, -0.066, 1.596), (0.0, -0.114, 1.604), (0.0, -0.094, 1.54), (0.0, -0.072, 1.55),
                               (0.012, -0.1, 1.565)], mirror_x=True, bone=hb, bevel=0.003, angle=16))
    # rear light slit in a frame
    fin(L.shell("NT_HelmRearFrame", Sd(-184, -164, -36, 36), 8, 2, shape=lambda o, d: H(o, d) + 0.011, thick=0.01,
                bone=hb, bevel=0.002, mat="armor2"))
    fin(L.shell("NT_HelmRearLight", Sd(-177, -171, -30, 30), 8, 1, shape=lambda o, d: H(o, d) + 0.013, thick=0.01,
                bone=hb, bevel=0.0015, mat="emis"))
    # ear discs with a raised glowing ring
    ec = V(RX - 0.016, -0.014, 1.688)
    er = F["ear_r"]; ek = er / 0.06
    eh = 0.022 if er >= 0.06 else 0.008
    fin(L.lathe("NT_Ear_L", ec, LEFT, [(0, er), (eh, er), (eh + 0.007, er * 0.9), (eh + 0.007, er * 0.78), (eh + 0.013 * ek, er * 0.75), (eh + 0.013 * ek, er * 0.57),
                                        (eh + 0.007, er * 0.53), (eh + 0.007, er * 0.33), (eh + 0.012 * ek, er * 0.3), (eh + 0.012 * ek, 0)],
                segs=20, ring_mats={4: "emis"}, bone=hb, bevel=0.0), True)
    # antenna fins: blades rooted at the top of the ear disc, 15 cm long, 4.5 cm wide at the root, 8 mm thick, swept back 38 degrees
    if F["fins"] > 0:
        sw = math.radians(38)
        ax = V(0, math.sin(sw), math.cos(sw)); pr = V(0, math.cos(sw), -math.sin(sw))
        root = V(RX + 0.012, -0.004, 1.735)
        kf = F["fins"] / 0.15
        prof = ((-0.012, 0.0225, 0.007), (0.02 * kf, 0.022, 0.007), (0.085 * kf, 0.014, 0.007), (0.14 * kf, 0.006, 0.005), (0.152 * kf, 0.002, 0.003))
        pts = []
        for (along, half, th) in prof:
            for sg in (-1, 1):
                c = root + ax * along + pr * (half * sg) + V(along * 0.08, 0, 0)
                pts += [(c.x - th, c.y, c.z), (c.x + th, c.y, c.z)]
        fin(L.hull("NT_Fin_L", pts, bone=hb, bevel=0.0015, angle=20), True)
        pts = []                                   # 3 mm emissive strip along the trailing edge
        for (along, half, th) in prof[1:]:
            c = root + ax * along + pr * half + V(along * 0.08, 0, 0)
            for off in (-0.0015, 0.0035):
                q = c + pr * off
                pts += [(q.x - th - 0.0012, q.y, q.z), (q.x + th + 0.0012, q.y, q.z)]
        fin(L.hull("NT_FinEdge_L", pts, bone=hb, bevel=0.0, mat="emis"), True)
    if F["crest_blade"]:    # the Grunt: one short swept-back centre blade, 6 cm tall, 14 mm thick, emissive trailing edge
        prof = ((-0.035, 1.845, 0.007), (0.06, 1.815, 0.007), (0.075, 1.88, 0.007), (0.118, 1.928, 0.004), (0.128, 1.918, 0.004))
        fin(L.hull("NT_CrestBlade", [(sx * th, y_, z_) for (y_, z_, th) in prof for sx in (-1, 1)], bone=hb, bevel=0.0015, angle=20))
        edge = ((0.06, 1.815), (0.128, 1.918))
        pts = []
        for (y_, z_) in ((0.062, 1.83), (0.128, 1.918)):
            for dy, dz in ((0.0, 0.0), (0.005, -0.004)):
                pts += [(-0.0082, y_ + dy, z_ + dz), (0.0082, y_ + dy, z_ + dz)]
        fin(L.hull("NT_CrestBladeEdge", pts, bone=hb, bevel=0.0, mat="emis"))
    if F["crest"]:      # the Heavy: no fins, a 3 cm central crest
        fin(L.shell("NT_HelmCrest", Sd(-150, -12, [(0, -2.2), (0.5, -3.2), (1, -3.0)], [(0, 2.2), (0.5, 3.2), (1, 3.0)]), 2, 18,
                    shape=lambda o, d: H(o, d) + 0.034, bulge=lambda s, t: -0.03 * max(0.0, abs(t - 0.5) * 2 - 0.55) / 0.45,
                    thick=0.034, bone=hb, bevel=0.004,
                    grooves=[('s', 0.5, 0.2, 0.0008, (0.08, 0.92), 'stripe')]))
    # neck seal: four stacked rings up into the helmet
    for k, (z0, z1, rr, bn) in enumerate(((1.522, 1.55, 0.078, {"neck_01": 1.0}), (1.552, 1.578, 0.073, {"neck_01": 0.6, "neck_02": 0.4}),
                                          (1.58, 1.606, 0.069, {"neck_02": 1.0}), (1.608, 1.64, 0.066, {"neck_02": 0.4, "head": 0.6}))):
        fin(L.shell("NT_Neck%d" % k, L.sweep(V(0, 0.004, 0), V(0, 0.004, 1), FRONT, LEFT, 0, 360, const_s(z0), const_s(z1)), 14, 1,
                    shape=(lambda r_: (lambda o, d: r_ * (1.0 + 0.12 * abs(d.y))))(rr), thick=0.02, wrap=True, bone=bn, bevel=0.003, mat="armor2"))


# ------------------------------------------------------------------ Heavy only: gorget, second chest plate, left forearm shield
def build_heavy():
    reg = L.R_TORSO_ARMS
    if F["gorget"]:     # raised neck guard to ear height: two stepped lames, chamfered top edges, open at the front
        g0 = L.shell("NT_GorgetLow", T(54, 306, const_s(1.462), [(0, 1.55), (0.2, 1.60), (0.5, 1.615), (0.8, 1.60), (1, 1.55)]), 20, 3,
                     shape=lambda o, d: 0.212 + 0.012 * abs(d.x), bulge=lambda s, t: 0.008 * t, thick=0.016, inner=True, bone={"spine_05": 1.0}, bevel=0.006,
                     grooves=[('s', 0.5, 0.01, 0.003)])
        fin(g0)
        g1 = L.shell("NT_GorgetHigh", T(64, 296, const_s(1.585), [(0, 1.635), (0.2, 1.685), (0.5, 1.70), (0.8, 1.685), (1, 1.635)]), 20, 3,
                     shape=lambda o, d: 0.2 + 0.008 * abs(d.x), bulge=lambda s, t: 0.006 * t, thick=0.014, inner=True, bone={"spine_05": 1.0}, bevel=0.006,
                     grooves=[('t', 0.82, 0.05, 0.0008, None, 'stripe')])
        fin(g1)
    if F["chest2"]:     # second layered chest plate over the pectorals
        c2 = L.shell("NT_ChestOver_L", T([(0, 27), (1, 22)], [(0, 60), (0.5, 68), (1, 52)], [(0, 1.31), (0.5, 1.30), (1, 1.35)], [(0, 1.44), (0.5, 1.47), (1, 1.45)]),
                     10, 7, body, reg, offset=0.054, smooth=5, bone={"spine_05": 1.0}, bevel=0.005, edge_drop=0.005, cross=True,
                     grooves=[('s', 0.9, 0.03, 0.0008, None, 'stripe')])
        fin(c2, True)
    if F["shield"]:     # LEFT forearm shield: tall hexagonal plate about 45 x 30 cm, emissive rim
        el, wr = B("lowerarm_l"), B("hand_l")
        ax = (wr - el).normalized()
        nrm = (V(1, 0, 0.3) * 0.72 + FRONT * 0.66)
        nrm = (nrm - ax * nrm.dot(ax)).normalized()
        wide = ax.cross(nrm).normalized()
        rr = L.raycast_r(body.bvh(L.R_ARM_L), el + (wr - el) * 0.5, nrm, 0.3) or 0.05
        c = el + (wr - el) * 0.52 + nrm * (rr + 0.034)
        hexa = lambda a_, b_, m_: [(-a_ + m_, 0.0), (-a_ * 0.62, b_ - m_), (a_ * 0.62, b_ - m_), (a_ - m_, 0.0), (a_ * 0.62, -b_ + m_), (-a_ * 0.62, -b_ + m_)]
        bn = {"lowerarm_l": 1.0}
        fin(L.prism("NT_ShieldRim", hexa(0.225, 0.15, 0.0), c, ax, wide, nrm, 0.016, bone=bn, bevel=0.003, mat="emis"))
        fin(L.prism("NT_ShieldFace", hexa(0.225, 0.15, 0.014), c - nrm * 0.004, ax, wide, nrm, 0.026, taper=0.97, bone=bn, bevel=0.004, mat="armorflat"))
        fin(L.prism("NT_ShieldBoss", hexa(0.105, 0.072, 0.0), c + nrm * 0.018, ax, wide, nrm, 0.02, taper=0.8, bone=bn, bevel=0.005, mat="armorflat"))
        fin(L.prism("NT_ShieldBossCap", hexa(0.05, 0.034, 0.0), c + nrm * 0.036, ax, wide, nrm, 0.006, taper=0.9, bone=bn, bevel=0.002, mat="armor2"))
        for q, off in enumerate((-0.15, 0.15)):      # two scribed lines across the face
            fin(L.box("NT_ShieldLine%d" % q, c + ax * off + nrm * 0.0205, wide, ax, nrm, 0.2 if abs(off) > 0.1 else 0.26, 0.004, 0.0022, bone=bn, bevel=0.0, mat="armor2"))
        fin(L.prism("NT_ShieldMount", [(-0.07, -0.03), (0.07, -0.03), (0.07, 0.03), (-0.07, 0.03)], c - nrm * 0.05, ax, wide, nrm, 0.05, bone=bn, bevel=0.004, mat="armor2"))


# ------------------------------------------------------------------ undersuit segment bands (raised ribs at the joints only)
def build_bands():
    n = 0
    for z0 in (1.062, 1.102, 1.142, 1.182, 1.222):
        fin(L.shell("NT_Band_T%d" % n, T(0, 360, const_s(z0), const_s(z0 + 0.03)), 20, 1, body, L.R_TORSO_ARMS, far=0.24, offset=0.003, smooth=1,
                    wrap=True, bone="transfer", bevel=0.0, mat="suit", sink=0.003)); n += 1
    sh, el, wr = B("upperarm_l"), B("lowerarm_l"), B("hand_l")
    for k, u0 in enumerate((0.74, 0.86)):
        fin(L.shell("NT_Band_A%d_L" % k, L.sweep(sh, el, FRONT, V(0.82, 0, 0.57), 0, 360, const_s(u0), const_s(u0 + 0.09)), 12, 1, body, L.R_ARM_L,
                    offset=0.003, smooth=1, wrap=True, bone="transfer", bevel=0.0, mat="suit", sink=0.003), True)
    hipj, knee, ank = B("thigh_l"), B("calf_l"), B("foot_l")
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
if VARIANT == "heavy": build_heavy()
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
