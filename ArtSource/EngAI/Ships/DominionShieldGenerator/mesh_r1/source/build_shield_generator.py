#!/usr/bin/env python3
"""SM_DominionShieldGenerator + SM_DominionShieldGeneratorCore - Route C hard-surface, Blender 4.5.3.

Issue #41. Authored in metres, Unreal axes (+X forward, +Y starboard, +Z up, origin = actor /
SOCKET_Generator). The repo FBX convention (axis_forward='-Z', axis_up='Y', apply_unit_scale +
FBX_SCALE_UNITS) maps Blender (x, y, z) m -> Unreal (100x, -100y, 100z) cm with UnitScaleFactor=100
and uniform import scale 1.0. The generator is radially symmetric, so the Y sign flip is a no-op
for placement.

C++ contract (TDShieldGenerator.cpp):
  Mesh (GeneratorMesh) is the root, scale 1 when art is assigned.
  Core (GeneratorCoreMesh) stays attached at RelativeLocation (0, 0, 60) cm; Init() does not
  reset that offset, only SetRelativeScale3D(OneVector). Therefore the core mesh is authored
  with its visual centre at local Z = +10.40 m so that, once placed at +0.60 m, it sits in the
  pylon cradle at Z = +11.00 m.
  ShowDestroyed() hides Core and scales Mesh by (1, 1, 0.4) about the Mesh origin (Z = 0).
  The lower 40% of the pylon (base ring + armored collar, Z = -0.50 .. +7.30 m) is a solid
  stump that still reads as wreckage after that squash. Base seats on the outrigger plinth
  (top Z = -0.50 m). After 0.4x Z the base lifts 30 cm to Z = -0.20 m (documented, ~2% of
  the 14.8 m footprint — visually still planted).

usage: blender -b --factory-startup --python build_shield_generator.py -- TEX2K_DIR OUT_DIR
"""
import bpy, bmesh, math, random, sys, os, json, shutil
from mathutils import Vector, Matrix

argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
TEX = argv[0] if len(argv) > 0 else "/tmp/dsg_r1/tex2k"
OUT = argv[1] if len(argv) > 1 else "/tmp/dsg_r1/export"
os.makedirs(OUT, exist_ok=True)

PYLON = "SM_DominionShieldGenerator"
CORE  = "SM_DominionShieldGeneratorCore"
RNG = random.Random(41041)

# --- gameplay dimensions (metres) ---
BASE_Z = -0.50
TOP_Z = 19.00
CORE_Z = 11.00          # visual cradle centre (pylon / world)
CORE_COMP_Z = 0.60      # C++ Core->SetRelativeLocation Z, metres
CORE_MESH_Z = CORE_Z - CORE_COMP_Z  # 10.40 — authored offset in the Core FBX
CORE_R = 2.55
STUMP_TOP = 7.30        # lower 40% of 19.5 m visual height
MAX_R = 7.40            # foot lip; keep-clear cylinder is r = 18 m

# ---------------------------------------------------------------- atlases (must match make_textures.py)
PYLON_ATLAS = {  # name: (u0, v0, u1, v1, world_w_m, world_h_m, mode)
    "ARMOR":      (0.0,   0.5,    0.5,    1.0,    8, 8, "uniform"),
    "ARMOR_DARK": (0.5,   0.5,    1.0,    1.0,    8, 8, "uniform"),
    "TRIM_RED":   (0.0,   0.25,   0.25,   0.5,    4, 4, "uniform"),
    "BANNER":     (0.25,  0.0,    0.5,    0.5,    0, 0, "fit"),
    "METAL":      (0.0,   0.0,    0.25,   0.25,   4, 4, "uniform"),
    "RECESS":     (0.5,   0.25,   0.75,   0.5,    4, 4, "uniform"),
    "AMBER":      (0.5,   0.0,    0.625,  0.125,  0, 0, "fit"),
    "WHITE":      (0.625, 0.0,    0.75,   0.125,  0, 0, "fit"),
    "CYAN":       (0.5,   0.125,  0.75,   0.25,   0, 0, "fit"),
    "PLATE":      (0.75,  0.0,    1.0,    0.5,    6, 6, "uniform"),
}
CORE_ATLAS = {
    "ENERGY":     (0.0,   0.5,    0.5,    1.0,    0, 0, "fit"),
    "ENERGY_HOT": (0.5,   0.5,    1.0,    1.0,    0, 0, "fit"),
    "RIB":        (0.0,   0.0,    0.5,    0.5,    4, 4, "uniform"),
    "RIB_DARK":   (0.5,   0.0,    0.75,   0.5,    4, 4, "uniform"),
    "CYAN":       (0.75,  0.0,    1.0,    0.25,   0, 0, "fit"),
    "AMBER":      (0.75,  0.25,   1.0,    0.5,    0, 0, "fit"),
}

bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.unit_settings.system = "METRIC"
scene.unit_settings.scale_length = 1.0

def V(*a):
    return Vector(a)

def ensure_col(name):
    c = bpy.data.collections.get(name) or bpy.data.collections.new(name)
    if c.name not in scene.collection.children:
        scene.collection.children.link(c)
    return c

def move(o, col):
    for c in list(o.users_collection):
        c.objects.unlink(o)
    col.objects.link(o)

# ================================================================= primitives on a bmesh
class Builder:
    def __init__(self, atlas):
        self.atlas = atlas
        self.REG = list(atlas.keys())
        self.RI = {n: i for i, n in enumerate(self.REG)}
        self.bm = bmesh.new()
        self.coll = []  # (points, label)

    def hexa(self, pts, reg, faces_reg=None):
        vs = [self.bm.verts.new(p) for p in pts]
        idx = {
            "bottom": (0, 3, 2, 1), "top": (4, 5, 6, 7),
            "-v": (0, 1, 5, 4), "+u": (1, 2, 6, 5),
            "+v": (2, 3, 7, 6), "-u": (3, 0, 4, 7),
        }
        for k, ii in idx.items():
            f = self.bm.faces.new([vs[i] for i in ii])
            f.material_index = self.RI[(faces_reg or {}).get(k, reg)]
        return pts

    def box(self, lo, hi, reg, faces_reg=None, coll=None):
        (x0, y0, z0), (x1, y1, z1) = lo, hi
        pts = [V(x0, y0, z0), V(x1, y0, z0), V(x1, y1, z0), V(x0, y1, z0),
               V(x0, y0, z1), V(x1, y0, z1), V(x1, y1, z1), V(x0, y1, z1)]
        self.hexa(pts, reg, faces_reg)
        if coll:
            self.coll.append((pts, coll))
        return pts

    def obox(self, c, u, hu, hv, hw, reg, faces_reg=None, w=V(0, 0, 1), taper=1.0, coll=None):
        u = Vector(u).normalized()
        w = Vector(w)
        v = w.cross(u)
        if v.length < 1e-6:
            v = V(0, 1, 0).cross(u)
        v = v.normalized()
        w = u.cross(v).normalized()
        c = Vector(c)
        # bottom (tapered) then top
        pts = []
        for s, t in ((-1, 1.0), (1, taper)):
            cc = c + u * (s * hu)
            for sv, sw in ((-1, -1), (1, -1), (1, 1), (-1, 1)):
                pts.append(cc + v * (sv * hv * t) + w * (sw * hw * t))
        # reorder to hexa convention: bottom -v-w, +v-w, +v+w, -v+w ... wait
        # my loop: s=-1: (-v-w),(+v-w),(+v+w),(-v+w) then s=+1 same. That's already hexa order
        # if u is the "height" axis. hexa expects bottom then top, with -u-v, +u-v, +u+v, -u+v
        # Here u is longitudinal, v and w are cross. Good enough if faces aren't inverted.
        self.hexa(pts, reg, faces_reg)
        if coll:
            self.coll.append((pts, coll))
        return pts

    def tapered_beam(self, p0, p1, w0, w1, h0, h1, reg, faces_reg=None, coll=None):
        p0, p1 = Vector(p0), Vector(p1)
        u = (p1 - p0)
        if u.length < 1e-8:
            return []
        u = u.normalized()
        w = V(0, 0, 1)
        if abs(u.dot(w)) > 0.92:
            w = V(1, 0, 0)
        v = w.cross(u).normalized()
        w = u.cross(v).normalized()
        pts = [
            p0 - v * (w0 / 2) - w * (h0 / 2),
            p0 + v * (w0 / 2) - w * (h0 / 2),
            p0 + v * (w0 / 2) + w * (h0 / 2),
            p0 - v * (w0 / 2) + w * (h0 / 2),
            p1 - v * (w1 / 2) - w * (h1 / 2),
            p1 + v * (w1 / 2) - w * (h1 / 2),
            p1 + v * (w1 / 2) + w * (h1 / 2),
            p1 - v * (w1 / 2) + w * (h1 / 2),
        ]
        self.hexa(pts, reg, faces_reg)
        if coll:
            self.coll.append((pts, coll))
        return pts

    def lathe(self, profile, segs, cap_reg, side_fn):
        """profile: list of (z, r) along the outer silhouette, starting and ending at r=0 for a solid."""
        rings = []
        for z, r in profile:
            ring = []
            for i in range(segs):
                a = 2 * math.pi * i / segs
                ring.append(self.bm.verts.new(V(math.cos(a) * r, math.sin(a) * r, z)))
            rings.append((z, r, ring))
        # side quads
        for k in range(len(rings) - 1):
            z0, r0, a = rings[k]
            z1, r1, b = rings[k + 1]
            if r0 < 1e-6 and r1 < 1e-6:
                continue
            reg = side_fn(0.5 * (z0 + z1), 0.5 * (r0 + r1))
            for i in range(segs):
                j = (i + 1) % segs
                if r0 < 1e-6:
                    f = self.bm.faces.new([a[0], b[j], b[i]])
                elif r1 < 1e-6:
                    f = self.bm.faces.new([a[i], a[j], b[0]])
                else:
                    f = self.bm.faces.new([a[i], a[j], b[j], b[i]])
                f.material_index = self.RI[reg]
        return rings

    def ring_tube(self, z, radius, tw, th, segs, reg, coll=None):
        """Rectangular-section torus in the XY plane, centred on Z."""
        prof = [
            (z - th / 2, radius - tw / 2),
            (z - th / 2, radius + tw / 2),
            (z + th / 2, radius + tw / 2),
            (z + th / 2, radius - tw / 2),
            (z - th / 2, radius - tw / 2),
        ]
        # don't use lathe (open). Build as a closed annular prism.
        rings = []
        for zz, rr in prof[:-1]:
            ring = [self.bm.verts.new(V(math.cos(2 * math.pi * i / segs) * rr,
                                        math.sin(2 * math.pi * i / segs) * rr, zz))
                    for i in range(segs)]
            rings.append(ring)
        pts_all = []
        for k in range(4):
            a = rings[k]; b = rings[(k + 1) % 4]
            for i in range(segs):
                j = (i + 1) % segs
                f = self.bm.faces.new([a[i], a[j], b[j], b[i]])
                f.material_index = self.RI[reg]
            pts_all.extend([v.co.copy() for v in a])
        if coll:
            self.coll.append((pts_all, coll))
        return pts_all

    def tilted_ring(self, center, radius, tw, th, segs, reg, rot, coll=None):
        """Ring in a plane rotated by Euler `rot` (radians), then translated to center."""
        R = Matrix.Rotation(rot[2], 3, "Z") @ Matrix.Rotation(rot[1], 3, "Y") @ Matrix.Rotation(rot[0], 3, "X")
        # build local ring in XY at origin, transform verts afterwards — construct directly
        prof = [(-th / 2, radius - tw / 2), (-th / 2, radius + tw / 2),
                (th / 2, radius + tw / 2), (th / 2, radius - tw / 2)]
        rings = []
        for z, r in prof:
            ring = []
            for i in range(segs):
                a = 2 * math.pi * i / segs
                local = V(math.cos(a) * r, math.sin(a) * r, z)
                ring.append(self.bm.verts.new(R @ local + Vector(center)))
            rings.append(ring)
        pts_all = []
        for k in range(4):
            a = rings[k]; b = rings[(k + 1) % 4]
            for i in range(segs):
                j = (i + 1) % segs
                f = self.bm.faces.new([a[i], a[j], b[j], b[i]])
                f.material_index = self.RI[reg]
            pts_all.extend([v.co.copy() for v in a])
        if coll:
            self.coll.append((pts_all, coll))
        return pts_all

    def icosphere(self, center, radius, subdivisions, reg, scale=(1, 1, 1)):
        tmp = bmesh.new()
        bmesh.ops.create_icosphere(tmp, subdivisions=subdivisions, radius=1.0)
        bmesh.ops.scale(tmp, vec=Vector(scale) * radius, verts=tmp.verts)
        bmesh.ops.translate(tmp, vec=Vector(center), verts=tmp.verts)
        # copy into self.bm
        src_to = {}
        for v in tmp.verts:
            src_to[v] = self.bm.verts.new(v.co)
        self.bm.verts.ensure_lookup_table()
        for f in tmp.faces:
            nf = self.bm.faces.new([src_to[v] for v in f.verts])
            nf.material_index = self.RI[reg]
        tmp.free()

    def cylinder(self, c, r0, r1, z0, z1, segs, side_reg, cap0=None, cap1=None):
        c = Vector(c)
        a = [self.bm.verts.new(V(c.x + math.cos(2 * math.pi * i / segs) * r0,
                                 c.y + math.sin(2 * math.pi * i / segs) * r0, z0)) for i in range(segs)]
        b = [self.bm.verts.new(V(c.x + math.cos(2 * math.pi * i / segs) * r1,
                                 c.y + math.sin(2 * math.pi * i / segs) * r1, z1)) for i in range(segs)]
        for i in range(segs):
            j = (i + 1) % segs
            f = self.bm.faces.new([a[i], a[j], b[j], b[i]])
            f.material_index = self.RI[side_reg]
        if cap0:
            # fan, or n-gon
            f = self.bm.faces.new(list(reversed(a)))  # downward
            f.material_index = self.RI[cap0]
        if cap1:
            f = self.bm.faces.new(b)
            f.material_index = self.RI[cap1]
        pts = [v.co.copy() for v in a + b]
        return pts

    def to_object(self, name):
        me = bpy.data.meshes.new(name)
        self.bm.normal_update()
        self.bm.to_mesh(me)
        self.bm.free()
        self.bm = None
        obj = bpy.data.objects.new(name, me)
        scene.collection.objects.link(obj)
        # placeholder material slots so UV pass can read material_index
        for n in self.REG:
            mat = bpy.data.materials.new("R_" + name + "_" + n)
            me.materials.append(mat)
        return obj


def apply_atlas_uvs(obj, atlas, seed=77):
    REG = list(atlas.keys())
    bm = bmesh.new(); bm.from_mesh(obj.data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=0.0004)
    bmesh.ops.dissolve_degenerate(bm, edges=bm.edges, dist=0.0004)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    # flip any inward faces of isolated solids — recalc should be enough
    uvl = bm.loops.layers.uv.new("UVMap") if not bm.loops.layers.uv else bm.loops.layers.uv[0]
    urng = random.Random(seed)
    for f in bm.faces:
        name = REG[min(f.material_index, len(REG) - 1)]
        u0, v0, u1, v1, ww, wh, mode = atlas[name]
        n = f.normal
        ax = max(range(3), key=lambda i: abs(n[i]))
        sgn = 1 if n[ax] >= 0 else -1
        if ax == 0:
            U, Vv = V(0, sgn, 0), V(0, 0, 1)
        elif ax == 1:
            U, Vv = V(-sgn, 0, 0), V(0, 0, 1)
        else:
            U, Vv = V(1, 0, 0), V(0, sgn, 0)
        co = [(l.vert.co.dot(U), l.vert.co.dot(Vv)) for l in f.loops]
        us = [c[0] for c in co]; vs = [c[1] for c in co]
        umin, vmin = min(us), min(vs)
        eu, ev = max(max(us) - umin, 1e-6), max(max(vs) - vmin, 1e-6)
        m = 0.02
        if mode == "fit":
            for l, (cu, cv) in zip(f.loops, co):
                a = m + (cu - umin) / eu * (1 - 2 * m)
                b = m + (cv - vmin) / ev * (1 - 2 * m)
                l[uvl].uv = (u0 + a * (u1 - u0), v0 + b * (v1 - v0))
        else:
            fu, fv = eu / ww, ev / wh
            k = min(1.0, (1 - 2 * m) / max(fu, fv, 1e-9))
            ou = m + urng.random() * max(0.0, (1 - 2 * m) - fu * k)
            ov = m + urng.random() * max(0.0, (1 - 2 * m) - fv * k)
            for l, (cu, cv) in zip(f.loops, co):
                l[uvl].uv = (u0 + (ou + (cu - umin) / ww * k) * (u1 - u0),
                             v0 + (ov + (cv - vmin) / wh * k) * (v1 - v0))
    # triangulate n-gons only (keep quads for bevel)
    bmesh.ops.triangulate(bm, faces=[f for f in bm.faces if len(f.verts) > 4], ngon_method="BEAUTY")
    bm.to_mesh(obj.data); bm.free()


def assign_pbr(obj, mat_name, tex_dir, prefix):
    me = obj.data
    me.materials.clear()
    mat = bpy.data.materials.new(mat_name)
    mat.use_nodes = True
    me.materials.append(mat)
    nt = mat.node_tree; nt.nodes.clear()
    outn = nt.nodes.new("ShaderNodeOutputMaterial")
    bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled")
    nt.links.new(bsdf.outputs[0], outn.inputs[0])

    def img(name, cs):
        n = nt.nodes.new("ShaderNodeTexImage")
        path = os.path.join(tex_dir, name)
        n.image = bpy.data.images.load(path)
        n.image.colorspace_settings.name = cs
        return n

    bc = img(f"{prefix}_BC.png", "sRGB")
    nt.links.new(bc.outputs[0], bsdf.inputs["Base Color"])
    orm = img(f"{prefix}_ORM.png", "Non-Color")
    sep = nt.nodes.new("ShaderNodeSeparateColor")
    nt.links.new(orm.outputs[0], sep.inputs[0])
    nt.links.new(sep.outputs[1], bsdf.inputs["Roughness"])
    nt.links.new(sep.outputs[2], bsdf.inputs["Metallic"])
    ngl = img(f"{prefix}_N_OpenGL.png", "Non-Color")
    nm = nt.nodes.new("ShaderNodeNormalMap")
    nt.links.new(ngl.outputs[0], nm.inputs["Color"])
    nt.links.new(nm.outputs[0], bsdf.inputs["Normal"])
    em = img(f"{prefix}_E.png", "sRGB")
    nt.links.new(em.outputs[0], bsdf.inputs["Emission Color"])
    bsdf.inputs["Emission Strength"].default_value = 6.0 if "Core" in prefix else 4.0
    return mat


def add_live_polish(obj, width, min_len, min_area, min_ang, budget=45000, tag=""):
    me = obj.data
    bm = bmesh.new(); bm.from_mesh(me)
    bm.edges.ensure_lookup_table(); bm.faces.ensure_lookup_table()
    bw = bm.edges.layers.float.get("bevel_weight_edge") or bm.edges.layers.float.new("bevel_weight_edge")
    n = 0
    for e in bm.edges:
        w = 0.0
        if len(e.link_faces) == 2 and e.calc_length() >= min_len:
            if min(f.calc_area() for f in e.link_faces) >= min_area and math.degrees(e.calc_face_angle(0)) >= min_ang:
                w = 1.0; n += 1
        e[bw] = w
    bm.to_mesh(me); bm.free()
    print(f"{tag} bevel-weighted edges", n, flush=True)
    me.shade_smooth()
    me.set_sharp_from_angle(angle=math.radians(30))
    bev = obj.modifiers.new("Bevel", "BEVEL")
    bev.width = width; bev.segments = 2; bev.limit_method = "WEIGHT"
    bev.harden_normals = True; bev.profile = 0.5
    bev.use_clamp_overlap = True; bev.miter_outer = "MITER_ARC"
    wn = obj.modifiers.new("WeightedNormal", "WEIGHTED_NORMAL")
    wn.keep_sharp = True; wn.weight = 50; wn.mode = "FACE_AREA"; wn.use_face_influence = True
    dg = bpy.context.evaluated_depsgraph_get()
    ev = obj.evaluated_get(dg); me_ev = ev.to_mesh()
    tris = sum(len(p.vertices) - 2 for p in me_ev.polygons)
    raw_f, raw_v, ev_v = len(me.polygons), len(me.vertices), len(me_ev.vertices)
    ev.to_mesh_clear()
    print(f"{tag} EVAL tris {tris} raw_faces {raw_f} raw_verts {raw_v} eval_verts {ev_v}", flush=True)
    if tris > budget:
        raise RuntimeError(f"{tag} evaluated triangle budget exceeded: {tris} > {budget}")
    return {"eval_tris": tris, "raw_faces": raw_f, "raw_verts": raw_v, "eval_verts": ev_v, "bevel_edges": n}


def make_ucx(coll, name, collection):
    ucx_meta = []
    for idx, (pts, label) in enumerate(coll):
        cb = bmesh.new()
        for p in pts:
            cb.verts.new(Vector(p))
        if len(cb.verts) < 4:
            cb.free(); continue
        res = bmesh.ops.convex_hull(cb, input=list(cb.verts), use_existing_faces=False)
        for g in res.get("geom_interior", []) + res.get("geom_unused", []):
            if isinstance(g, bmesh.types.BMVert) and g.is_valid:
                cb.verts.remove(g)
        bmesh.ops.remove_doubles(cb, verts=cb.verts, dist=0.001)
        bmesh.ops.recalc_face_normals(cb, faces=cb.faces)
        bmesh.ops.triangulate(cb, faces=cb.faces)
        nm = f"UCX_{name}_{idx:02d}"
        cm = bpy.data.meshes.new(nm); cb.to_mesh(cm); cb.free()
        co = bpy.data.objects.new(nm, cm); scene.collection.objects.link(co)
        co.display_type = "WIRE"; co.hide_render = True
        move(co, collection)
        ucx_meta.append({"name": nm, "part": label, "verts": len(cm.vertices), "tris": len(cm.polygons)})
    return ucx_meta


def export_fbx_glb(obj, name, out_dir, extra_select_prefixes):
    """Evaluate modifiers, triangulate a temp copy, export FBX + GLB, then restore live object."""
    export_obj = obj.copy(); export_obj.data = obj.data.copy()
    scene.collection.objects.link(export_obj)
    bpy.ops.object.select_all(action="DESELECT")
    export_obj.select_set(True); bpy.context.view_layer.objects.active = export_obj
    for m in list(export_obj.modifiers):
        bpy.ops.object.modifier_apply(modifier=m.name)
    bm_e = bmesh.new(); bm_e.from_mesh(export_obj.data)
    bmesh.ops.triangulate(bm_e, faces=bm_e.faces, quad_method="BEAUTY", ngon_method="BEAUTY")
    bm_e.to_mesh(export_obj.data); bm_e.free()
    if export_obj.data.uv_layers:
        export_obj.data.calc_tangents()
    live_name = obj.name
    obj.name = name + "_SRC_LIVE"; obj.hide_render = True; obj.hide_set(True)
    export_obj.name = name
    socks = [o for o in scene.objects if o.name.startswith("SOCKET_") and o.parent == obj]
    for e in socks:
        mw = e.matrix_world.copy()
        e.parent = export_obj
        e.matrix_parent_inverse = Matrix.Identity(4)
        e.matrix_world = mw

    bpy.ops.object.select_all(action="DESELECT")
    export_obj.select_set(True)
    for o in scene.objects:
        if any(o.name.startswith(p) for p in extra_select_prefixes):
            o.select_set(True)
    bpy.context.view_layer.objects.active = export_obj
    fbx = os.path.join(out_dir, name + ".fbx")
    bpy.ops.export_scene.fbx(
        filepath=fbx, use_selection=True, object_types={"MESH", "EMPTY"},
        apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS", global_scale=1.0,
        axis_forward="-Z", axis_up="Y",
        mesh_smooth_type="OFF", use_tspace=True, use_custom_props=False,
        add_leaf_bones=False, bake_anim=False, path_mode="AUTO",
        embed_textures=False, batch_mode="OFF",
    )
    glb = os.path.join(out_dir, name + ".glb")
    bpy.ops.export_scene.gltf(
        filepath=glb, export_format="GLB", use_selection=True,
        export_apply=True, export_yup=True, export_tangents=True,
        export_image_format="AUTO", export_extras=False,
    )
    # restore
    export_obj.name = name + "_EXPORT_TMP"
    obj.name = live_name; obj.hide_render = False; obj.hide_set(False)
    for e in socks:
        mw = e.matrix_world.copy()
        e.parent = obj
        e.matrix_parent_inverse = Matrix.Identity(4)
        e.matrix_world = mw
    bpy.data.objects.remove(export_obj, do_unlink=True)
    print("exported", fbx, glb, flush=True)
    return fbx, glb


# ================================================================= PYLON GEOMETRY
def build_pylon():
    B = Builder(PYLON_ATLAS)
    segs = 48

    def side_reg(z, r):
        if z < 0.15:
            return "METAL"
        if 1.95 <= z <= 2.25:
            return "TRIM_RED"
        if 3.55 <= z <= 3.85:
            return "TRIM_RED"
        if 5.50 <= z <= 5.75:
            return "METAL"
        if 7.15 <= z <= 7.80:
            return "ARMOR_DARK"
        if z >= 7.40 and r < 3.0:
            return "RECESS"
        if z < 3.8:
            return "ARMOR"
        if z < 7.4:
            return "ARMOR_DARK"
        return "ARMOR"

    profile = [
        (BASE_Z, 0.00),
        (BASE_Z, 7.15),
        (-0.32, 7.40),   # foot lip — seats on plinth Z = -0.50
        (-0.12, 7.15),
        (0.30, 6.95),
        (0.55, 6.95),
        (0.72, 6.45),
        (1.95, 6.55),
        (2.08, 6.72),    # crimson collar lip
        (2.28, 6.45),
        (3.50, 6.35),
        (3.72, 6.50),
        (3.90, 5.85),
        (5.45, 5.75),
        (5.62, 5.20),
        (7.15, 5.10),
        (7.32, 5.00),    # stump cap (lower 40% ends ~ here)
        (7.55, 4.05),
        (7.72, 3.50),    # lantern rim
        (7.72, 2.20),
        (7.42, 2.20),    # recessed deck
        (7.42, 0.00),
    ]
    B.lathe(profile, segs, "METAL", side_reg)

    # six-fold details
    for i in range(6):
        a = math.radians(30 + 60 * i)  # face centres of a hex
        u = V(math.cos(a), math.sin(a), 0)
        t = V(-math.sin(a), math.cos(a), 0)

        # collar armor plate (stump) — every other face gets a trident banner
        banner = (i % 2 == 0)
        # obox "top" is the +u cap (outward face). Portrait plate so the trident reads upright.
        pc = u * 6.62 + V(0, 0, 2.95)
        B.obox(pc, u, 0.16, 1.20, 1.70,
               "ARMOR_DARK",
               faces_reg={"top": "BANNER" if banner else "PLATE",
                          "bottom": "ARMOR_DARK",
                          "+u": "TRIM_RED", "-u": "ARMOR_DARK"},
               taper=1.0)
        # crimson cap strip above the plate
        B.obox(u * 6.58 + V(0, 0, 4.10), u, 0.14, 1.45, 0.12, "TRIM_RED")
        # metal sill
        B.obox(u * 6.58 + V(0, 0, 1.62), u, 0.14, 1.45, 0.10, "METAL")

        # lower stump buttress foot (planted on first collar, inside stump 40%)
        foot0 = u * 6.55 + V(0, 0, 0.85)
        foot1 = u * 5.85 + V(0, 0, 3.40)
        B.tapered_beam(foot0, foot1, 1.70, 1.45, 1.15, 1.00, "ARMOR",
                       {"top": "ARMOR_DARK"}, coll=f"buttress_foot_{i}")
        # crimson leading edge on the foot
        B.tapered_beam(foot0 + u * 0.12 + V(0, 0, 0.45),
                       foot1 + u * 0.10 + V(0, 0, 0.35),
                       0.18, 0.16, 0.18, 0.16, "TRIM_RED")

        # main pylon / flying buttress — from stump cap up past the cradle
        pA = u * 5.55 + V(0, 0, 5.20)
        pB = u * 4.25 + V(0, 0, 7.55)   # lantern rim attach
        pC = u * 3.70 + V(0, 0, 11.00)  # equator, outside the core
        pD = u * 3.25 + V(0, 0, 14.40)
        pE = u * 2.55 + V(0, 0, 16.80)  # crown spring
        B.tapered_beam(foot1, pA, 1.45, 1.25, 1.00, 0.95, "ARMOR_DARK", {"top": "ARMOR"})
        B.tapered_beam(pA, pB, 1.25, 1.10, 0.95, 0.85, "ARMOR", {"top": "ARMOR_DARK"},
                       coll=f"pylon_{i}_low")
        B.tapered_beam(pB, pC, 1.10, 0.95, 0.85, 0.75, "ARMOR", {"top": "ARMOR_DARK"},
                       coll=f"pylon_{i}_mid")
        B.tapered_beam(pC, pD, 0.95, 0.80, 0.75, 0.65, "ARMOR_DARK", {"top": "ARMOR"},
                       coll=f"pylon_{i}_up")
        B.tapered_beam(pD, pE, 0.80, 0.62, 0.65, 0.50, "ARMOR", {"top": "METAL"})

        # amber indicator strip along the outer spine of each pylon
        B.tapered_beam(foot0 + u * 0.22 + V(0, 0, 0.15),
                       pD + u * 0.16,
                       0.10, 0.08, 0.10, 0.08, "AMBER")

        # emitter prongs cradling the upper core (claw from equator up and inward)
        # path around the core with ~0.7 m clearance
        def prong_pt(phi_deg, rad):
            phi = math.radians(phi_deg)
            # phi=0 at outward equator, +phi upward
            return u * (rad * math.cos(phi)) + V(0, 0, CORE_Z + rad * math.sin(phi))

        q0 = pC + u * 0.15                    # attach at equator pylon
        q1 = prong_pt(25, 3.45)
        q2 = prong_pt(50, 3.35)
        q3 = prong_pt(72, 3.15)
        q4 = V(0, 0, 0) * 0 + u * 1.70 + V(0, 0, 17.15)  # tip leans in
        B.tapered_beam(q0, q1, 0.55, 0.48, 0.42, 0.38, "ARMOR_DARK", {"top": "METAL"},
                       coll=f"prong_{i}_a")
        B.tapered_beam(q1, q2, 0.48, 0.42, 0.38, 0.32, "METAL")
        B.tapered_beam(q2, q3, 0.42, 0.34, 0.32, 0.26, "METAL", coll=f"prong_{i}_b")
        B.tapered_beam(q3, q4, 0.34, 0.22, 0.26, 0.18, "METAL")
        # cyan emitter tip
        B.obox(q4 + u * (-0.05) + V(0, 0, 0.22), V(0, 0, 1), 0.28, 0.14, 0.14, "CYAN",
               w=u, taper=0.55)

        # upward shield-emitter spike from the crown
        tip = u * 2.15 + V(0, 0, TOP_Z)
        B.tapered_beam(pE, tip, 0.50, 0.16, 0.40, 0.12, "METAL", {"top": "CYAN"})
        B.obox(tip, V(0, 0, 1), 0.18, 0.16, 0.16, "CYAN", taper=0.4)

        # greeble blocks on the stump collar
        for (dz, rr, hw, hh) in ((0.40, 6.85, 0.35, 0.22), (4.70, 6.05, 0.28, 0.18),
                                  (6.40, 5.35, 0.25, 0.16)):
            B.obox(u * rr + V(0, 0, dz) + t * 0.55, u, 0.18, hw, hh, "METAL")
            B.obox(u * rr + V(0, 0, dz) - t * 0.55, u, 0.18, hw, hh, "METAL")

        # vent grille boxes (recess) on lower collar flanks
        B.obox(u * 6.70 + V(0, 0, 1.10) + t * 1.15, t, 0.55, 0.08, 0.28, "RECESS")

    # containment rings (open lantern around the core — do not fill the cradle)
    B.ring_tube(8.85, 3.85, 0.42, 0.32, 48, "ARMOR_DARK", coll="ring_lower")
    B.ring_tube(11.00, 4.05, 0.38, 0.28, 48, "METAL", coll="ring_equator")
    B.ring_tube(13.25, 3.70, 0.36, 0.26, 48, "ARMOR_DARK", coll="ring_upper")
    B.ring_tube(16.55, 2.70, 0.34, 0.30, 36, "METAL", coll="ring_crown")
    # crimson / amber trim rings
    B.ring_tube(8.85, 4.12, 0.10, 0.10, 48, "TRIM_RED")
    B.ring_tube(11.00, 4.30, 0.10, 0.08, 48, "AMBER")
    B.ring_tube(13.25, 3.95, 0.10, 0.08, 48, "AMBER")
    B.ring_tube(16.55, 2.95, 0.10, 0.10, 36, "CYAN")

    # radial spokes from lantern rim inward (but not into the core)
    for i in range(6):
        a = math.radians(30 + 60 * i + 30)  # mid-bay, between pylons
        u = V(math.cos(a), math.sin(a), 0)
        B.tapered_beam(u * 3.50 + V(0, 0, 7.72), u * 2.35 + V(0, 0, 7.55),
                       0.28, 0.22, 0.16, 0.14, "METAL")

    # foot bolts / scuppers around the base ring (stump language)
    for k in range(24):
        a = 2 * math.pi * k / 24
        u = V(math.cos(a), math.sin(a), 0)
        B.obox(u * 7.22 + V(0, 0, -0.18), u, 0.22, 0.16, 0.14, "METAL")
        if k % 2 == 0:
            B.obox(u * 7.28 + V(0, 0, 0.05), u, 0.10, 0.08, 0.06, "AMBER")

    # UCX for the solid stump (base + collar) as three stacked convex hulls
    def cyl_pts(z0, z1, r, n=16):
        pts = []
        for z in (z0, z1):
            for i in range(n):
                a = 2 * math.pi * i / n
                pts.append(V(math.cos(a) * r, math.sin(a) * r, z))
        return pts
    B.coll.insert(0, (cyl_pts(BASE_Z, 2.30, 7.45), "base_ring"))
    B.coll.insert(1, (cyl_pts(2.20, 5.70, 6.75), "collar"))
    B.coll.insert(2, (cyl_pts(5.55, 7.80, 5.25), "stump_cap"))

    obj = B.to_object(PYLON)
    apply_atlas_uvs(obj, PYLON_ATLAS, seed=41041)
    assign_pbr(obj, "M_DominionShieldGenerator", TEX, "T_DominionShieldGenerator")
    metrics = add_live_polish(obj, width=0.07, min_len=0.55, min_area=0.06, min_ang=35.0,
                              budget=40000, tag="pylon")
    return obj, B.coll, metrics


def build_core():
    B = Builder(CORE_ATLAS)
    c = V(0, 0, CORE_MESH_Z)

    # inner energy chamber (hot) and outer faceted shell
    B.icosphere(c, CORE_R * 0.72, 3, "ENERGY_HOT", scale=(1.0, 1.0, 1.05))
    B.icosphere(c, CORE_R, 2, "ENERGY", scale=(1.0, 1.0, 1.08))

    # rib cage: 6 longitudinal meridians + 2 mid belts (already have gyro rings)
    for i in range(6):
        a = math.radians(30 + 60 * i)
        u = V(math.cos(a), math.sin(a), 0)
        # vertical rib running around the sphere in the (u, z) plane — approximate with segments
        rad = CORE_R + 0.18
        pts = []
        for deg in range(-80, 85, 20):
            phi = math.radians(deg)
            pts.append(c + u * (rad * math.cos(phi)) + V(0, 0, rad * math.sin(phi) * 1.08))
        for a0, a1 in zip(pts, pts[1:]):
            B.tapered_beam(a0, a1, 0.22, 0.22, 0.16, 0.16, "RIB")
        # polar caps / clamps
        B.obox(c + V(0, 0, rad * 1.08 + 0.12), V(0, 0, 1), 0.16, 0.55, 0.55, "RIB_DARK", taper=0.7)
        B.obox(c - V(0, 0, rad * 1.08 + 0.12), V(0, 0, 1), 0.16, 0.55, 0.55, "RIB_DARK", taper=0.7)

    # magnetic gyro rings (tilted), seated around the chamber
    B.tilted_ring(c, CORE_R + 0.42, 0.16, 0.14, 36, "RIB", (math.radians(18), math.radians(8), 0))
    B.tilted_ring(c, CORE_R + 0.55, 0.14, 0.12, 36, "RIB_DARK", (math.radians(-28), math.radians(40), math.radians(15)))
    B.tilted_ring(c, CORE_R + 0.70, 0.12, 0.10, 32, "CYAN", (math.radians(8), math.radians(-55), math.radians(70)))

    # equatorial clamp ring
    B.ring_tube(CORE_MESH_Z, CORE_R + 0.32, 0.22, 0.18, 32, "RIB_DARK")
    # small amber status jewels
    for i in range(4):
        a = math.radians(45 + 90 * i)
        u = V(math.cos(a), math.sin(a), 0)
        B.obox(c + u * (CORE_R + 0.38), u, 0.08, 0.08, 0.08, "AMBER")

    obj = B.to_object(CORE)
    apply_atlas_uvs(obj, CORE_ATLAS, seed=41042)
    assign_pbr(obj, "M_DominionShieldGeneratorCore", TEX, "T_DominionShieldGeneratorCore")
    metrics = add_live_polish(obj, width=0.04, min_len=0.25, min_area=0.03, min_ang=32.0,
                              budget=18000, tag="core")
    return obj, metrics


# ================================================================= run
col_vis = ensure_col("Visual")
col_col = ensure_col("Collision")
col_sock = ensure_col("Sockets")

pylon, coll, pylon_metrics = build_pylon()
core, core_metrics = build_core()
move(pylon, col_vis)
move(core, col_vis)

ucx_meta = make_ucx(coll, PYLON, col_col)

# SOCKET_Core at the visual cradle centre (pylon local = world)
sock = bpy.data.objects.new("SOCKET_Core", None)
scene.collection.objects.link(sock)
sock.empty_display_type = "ARROWS"
sock.empty_display_size = 1.2
sock.parent = pylon
sock.matrix_parent_inverse = Matrix.Identity(4)
sock.location = (0.0, 0.0, CORE_Z)
sock.rotation_euler = (0, 0, 0)
move(sock, col_sock)

# bounds / clearance checks (authoring space, metres == 0.01 Unreal cm after *100)
def bounds(o):
    dg = bpy.context.evaluated_depsgraph_get()
    ev = o.evaluated_get(dg); me = ev.to_mesh()
    xs = [o.matrix_world @ v.co for v in me.vertices]
    ev.to_mesh_clear()
    mn = Vector((min(v.x for v in xs), min(v.y for v in xs), min(v.z for v in xs)))
    mx = Vector((max(v.x for v in xs), max(v.y for v in xs), max(v.z for v in xs)))
    return mn, mx

p_mn, p_mx = bounds(pylon)
c_mn, c_mx = bounds(core)
p_r = max(abs(p_mn.x), abs(p_mx.x), abs(p_mn.y), abs(p_mx.y))
print("pylon bounds m", tuple(p_mn), tuple(p_mx), "max_r", p_r, flush=True)
print("core  bounds m", tuple(c_mn), tuple(c_mx), flush=True)
if p_r > 9.0:
    raise RuntimeError(f"pylon radius {p_r:.3f} m exceeds 18 m keep-clear (r=9 m)")
if p_mn.z < -0.60 or p_mx.z > 20.5:
    raise RuntimeError(f"pylon Z out of expected seating range: {p_mn.z:.3f} .. {p_mx.z:.3f}")
# core visual should sit around Z=10.40 in its own mesh
c_cx = 0.5 * (c_mn.z + c_mx.z)
print("core visual centre Z", c_cx, "expected", CORE_MESH_Z, flush=True)
if abs(c_cx - CORE_MESH_Z) > 0.35:
    raise RuntimeError(f"core centre Z {c_cx:.3f} != authored {CORE_MESH_Z}")

# pack textures so official blends open elsewhere
for img in bpy.data.images:
    if img.filepath:
        img.pack()

workshop = os.path.join(OUT, "_workshop.blend")
bpy.ops.wm.save_as_mainfile(filepath=workshop)
print("saved workshop", workshop, flush=True)

# export both from the workshop (live modifiers still on the sources)
fbx_p, glb_p = export_fbx_glb(pylon, PYLON, OUT, ["UCX_SM_DominionShieldGenerator_", "SOCKET_Core"])
fbx_c, glb_c = export_fbx_glb(core, CORE, OUT, [])

# isolate official blends
def isolate(dst, keep_pred):
    bpy.ops.wm.open_mainfile(filepath=workshop)
    for o in list(bpy.data.objects):
        if not keep_pred(o.name):
            bpy.data.objects.remove(o, do_unlink=True)
    # drop leftover helper meshes
    for m in list(bpy.data.meshes):
        if m.users == 0:
            bpy.data.meshes.remove(m)
    bpy.ops.wm.save_as_mainfile(filepath=dst)
    print("saved", dst, flush=True)

isolate(os.path.join(OUT, PYLON + ".blend"),
        lambda n: n == PYLON or n.startswith("UCX_SM_DominionShieldGenerator_") or n == "SOCKET_Core")
isolate(os.path.join(OUT, CORE + ".blend"),
        lambda n: n == CORE)

# report
report = {
    "pylon": pylon_metrics,
    "core": core_metrics,
    "pylon_bounds_m": {"min": list(p_mn), "max": list(p_mx), "max_radius_m": p_r},
    "core_bounds_m": {"min": list(c_mn), "max": list(c_mx), "centre_z_m": c_cx},
    "core_contract": {
        "visual_cradle_z_m": CORE_Z,
        "cpp_core_relative_z_m": CORE_COMP_Z,
        "core_mesh_authored_centre_z_m": CORE_MESH_Z,
        "show_destroyed_z_scale": 0.4,
        "stump_z_m": [BASE_Z, STUMP_TOP],
        "base_z_m": BASE_Z,
        "destroyed_base_z_m": BASE_Z * 0.4,
    },
    "ucx": ucx_meta,
    "socket_core_m": [0.0, 0.0, CORE_Z],
}
json.dump(report, open(os.path.join(OUT, "build_report.json"), "w"), indent=2)
print("BUILD OK", json.dumps(report, indent=2)[:2000], flush=True)
