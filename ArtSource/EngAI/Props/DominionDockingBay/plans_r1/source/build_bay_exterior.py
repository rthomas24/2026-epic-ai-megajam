"""Dominion docking bay mouth: the non-walkable exterior pieces that ATDDockingBay's placeholder cubes stand in for.
Blender 4.5.3, procedural hard-surface (bmesh boxes/prisms/lathes + LIVE weight-limited Bevel + WeightedNormal modifiers).
No AI mesh, nothing walkable. Each piece keeps a hidden unmodified copy SRC_<name> and convex UCX_<name>_NN collision in
DominionBay_Exterior.blend; the FBX is exported from a temp copy with the modifiers applied (live object untouched).

  SM_DominionBay_Door_L / _R   300 x 2500 x 3500 cm slabs. Pivot = slab centre in Y and Z, OUTER FACE AT X = -20, so the
                               slab is X -320..-20 and slides fully inside the hull when ATDDockingBay moves it to
                               Y = -/+ DoorHalfWidth * (0.5 + Open01) with the actor at SOCKET_Bay. _L is the -Y (port)
                               door, its seam edge on local +Y; _R is its mirror.
  SM_DominionBay_TractorEmitter  900 x 900 cm, 180 cm deep, mounted on a vertical face: pivot = mount-face centre,
                               local +X = beam direction. Goes on the prow brow above the mouth (hull r1/r2:
                               (31200, 0, 1400) = SOCKET_Bay + (1200, 0, 3400)).
Units: Blender metres = Unreal m (export applies x100). Blender +Y = Unreal -Y (FBX), handled by building in Unreal
coordinates and mirroring at the end (mirror_to_blender), as the hull and kit scripts do.
One material slot per piece, both on the same 2K trim sheet (T_DominionBay_Trim_BC/N/ORM/E, bands in make_trim_textures.py):
M_DominionBay_Trim on the doors (red-orange GlowColor) and M_DominionBay_Emitter on the emitter (cyan GlowColor, as the
reference board's tractor beam). In Unreal: one master material, two instances that differ only in GlowColor.

blender -b --factory-startup --python build_bay_exterior.py -- <outdir> <texdir>
"""
import bpy, bmesh, sys, os, math, json
from mathutils import Vector, Matrix

argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
OUT = os.path.abspath(argv[0] if argv else 'mesh')
TEX = os.path.abspath(argv[1] if len(argv) > 1 else 'textures')
os.makedirs(OUT, exist_ok=True)

bpy.ops.wm.read_factory_settings(use_empty=True)
sc = bpy.context.scene
sc.unit_settings.system = 'METRIC'; sc.unit_settings.scale_length = 1.0

# --- trim bands (UV v ranges), from make_trim_textures.py (px from the top of a 2048 sheet) -----------------------
def vb(p0, p1): return (1 - p1 / 2048.0, 1 - p0 / 2048.0)
BANDS = dict(ARMOR=vb(10, 630), HAZARD=vb(652, 884), GRILLE=vb(912, 1136), GUNMETAL=vb(1158, 1402),
             CRIMSON=vb(1426, 1518), STRIP=vb(1584, 1616), STRIP_HOUSING=vb(1540, 1580), PANEL=vb(1672, 1856))
LIVE_BEVEL = True   # chamfers are a live Bevel modifier (edge weights), not baked into the bmesh
BEVEL_W = {'SM_DominionBay_Door_L': 0.05, 'SM_DominionBay_Door_R': 0.05, 'SM_DominionBay_TractorEmitter': 0.04}
TILE_U = 4.0     # metres per sheet width (2048 px / 400 cm = 5.12 px/cm)

C_SRC = bpy.data.collections.new("SRC (hidden, unmodified)"); sc.collection.children.link(C_SRC)
C_UCX = bpy.data.collections.new("UCX (collision)"); sc.collection.children.link(C_UCX)

def ucx(name, idx, pts_ue_cm):
    """Convex hull of points given in Unreal cm -> UCX_<name>_<idx> (Blender m, Y mirrored)."""
    cb = bmesh.new()
    for (x, y, z) in pts_ue_cm: cb.verts.new((x / 100.0, -y / 100.0, z / 100.0))
    bmesh.ops.convex_hull(cb, input=list(cb.verts))
    loose = [v for v in cb.verts if not v.link_faces]
    if loose: bmesh.ops.delete(cb, geom=loose, context='VERTS')
    bmesh.ops.recalc_face_normals(cb, faces=cb.faces); bmesh.ops.triangulate(cb, faces=cb.faces)
    nm = "UCX_%s_%02d" % (name, idx); me = bpy.data.meshes.new(nm); cb.to_mesh(me); cb.free()
    o = bpy.data.objects.new(nm, me); C_UCX.objects.link(o); o.display_type = 'WIRE'; o.hide_render = True
    return o

def box_pts(lo, hi):
    return [(x, y, z) for x in (lo[0], hi[0]) for y in (lo[1], hi[1]) for z in (lo[2], hi[2])]

def ring_pts(r, x0, x1, n=16):
    return [(x, r * math.cos(2 * math.pi * k / n), r * math.sin(2 * math.pi * k / n)) for x in (x0, x1) for k in range(n)]

def material(name="M_DominionBay_Trim", glow=(1.0, 0.14, 0.02)):
    m = bpy.data.materials.get(name)
    if m: return m
    m = bpy.data.materials.new(name); m.use_nodes = True
    nt = m.node_tree; N = nt.nodes; L = nt.links
    bsdf = N["Principled BSDF"]
    def img(name, cs):
        n = N.new("ShaderNodeTexImage"); p = os.path.join(TEX, name)
        if os.path.exists(p):
            n.image = bpy.data.images.load(p); n.image.colorspace_settings.name = cs
        return n
    bc = img("T_DominionBay_Trim_BC.png", "sRGB"); orm = img("T_DominionBay_Trim_ORM.png", "Non-Color")
    nrm = img("T_DominionBay_Trim_N.png", "Non-Color"); em = img("T_DominionBay_Trim_E.png", "sRGB")
    L.new(bc.outputs[0], bsdf.inputs["Base Color"])
    sep = N.new("ShaderNodeSeparateColor"); L.new(orm.outputs[0], sep.inputs[0])
    L.new(sep.outputs[1], bsdf.inputs["Roughness"]); L.new(sep.outputs[2], bsdf.inputs["Metallic"])
    # the map is DirectX (Unreal); Blender wants OpenGL: invert green for the preview
    s2 = N.new("ShaderNodeSeparateColor"); inv = N.new("ShaderNodeMath"); inv.operation = 'SUBTRACT'; inv.inputs[0].default_value = 1.0
    cmb = N.new("ShaderNodeCombineColor"); L.new(nrm.outputs[0], s2.inputs[0]); L.new(s2.outputs[0], cmb.inputs[0])
    L.new(s2.outputs[1], inv.inputs[1]); L.new(inv.outputs[0], cmb.inputs[1]); L.new(s2.outputs[2], cmb.inputs[2])
    nm = N.new("ShaderNodeNormalMap"); L.new(cmb.outputs[0], nm.inputs["Color"]); L.new(nm.outputs[0], bsdf.inputs["Normal"])
    # emissive: mask x tint (the UE material instance carries GlowColor; preview uses the fleet red-orange)
    tint = N.new("ShaderNodeRGB"); tint.name = "GlowColor"; tint.outputs[0].default_value = (*glow, 1)
    mul = N.new("ShaderNodeMix"); mul.data_type = 'RGBA'; mul.blend_type = 'MULTIPLY'; mul.inputs[0].default_value = 1
    L.new(em.outputs[0], mul.inputs[6]); L.new(tint.outputs[0], mul.inputs[7])
    L.new(mul.outputs[2], bsdf.inputs["Emission Color"]); bsdf.inputs["Emission Strength"].default_value = 8.0
    return m

class Part:
    """Accumulates bmesh geometry in UNREAL cm with a trim band per face."""
    def __init__(self, name):
        self.name = name; self.bm = bmesh.new()
        self.lay = self.bm.faces.layers.int.new("band")
        self.names = list(BANDS)
    def _tag(self, faces, band):
        i = self.names.index(band)
        for f in faces: f[self.lay] = i
    def box(self, lo, hi, band, bevel=0.0, seg=2, side_bands=None):
        """Axis box lo..hi (cm). bevel cm chamfer on every edge. side_bands: {'+x': band, ...} per face orientation."""
        tmp = bmesh.new()
        r = bmesh.ops.create_cube(tmp, size=1.0)
        c = [(a + b) / 2 for a, b in zip(lo, hi)]; s = [b - a for a, b in zip(lo, hi)]
        bmesh.ops.transform(tmp, matrix=Matrix.Translation(c) @ Matrix.Diagonal((*s, 1)), verts=tmp.verts)
        if bevel > 0 and not LIVE_BEVEL:
            bmesh.ops.bevel(tmp, geom=list(tmp.edges), offset=bevel, segments=seg, profile=0.5, affect='EDGES', clamp_overlap=True)
        self._merge(tmp, band, side_bands)
    def prism(self, poly_yz, x0, x1, band, bevel=0.0, seg=2, side_band=None):
        """Extrude a YZ polygon (cm) from x0 to x1 (a front-face shape)."""
        tmp = bmesh.new()
        vs = [tmp.verts.new((x0, y, z)) for (y, z) in poly_yz]
        f = tmp.faces.new(vs)
        r = bmesh.ops.extrude_face_region(tmp, geom=[f])
        nv = [e for e in r['geom'] if isinstance(e, bmesh.types.BMVert)]
        bmesh.ops.translate(tmp, verts=nv, vec=(x1 - x0, 0, 0))
        bmesh.ops.recalc_face_normals(tmp, faces=tmp.faces)
        if bevel > 0 and not LIVE_BEVEL:
            bmesh.ops.bevel(tmp, geom=list(tmp.edges), offset=bevel, segments=seg, profile=0.5, affect='EDGES', clamp_overlap=True)
        self._merge(tmp, band, {'x': band, 'other': side_band or band})
    def cyl(self, r_out, x0, x1, band, n=32, r_in=None, bevel=0.0, seg=2, side_band=None, centre=(0, 0)):
        """A cylinder (or tube when r_in) along X, centred at (y, z) = centre (cm)."""
        tmp = bmesh.new()
        def ring(r, x): return [tmp.verts.new((x, centre[0] + r * math.cos(2 * math.pi * k / n), centre[1] + r * math.sin(2 * math.pi * k / n))) for k in range(n)]
        if r_in is None:
            a, b = ring(r_out, x0), ring(r_out, x1)
            for k in range(n): tmp.faces.new((a[k], a[(k + 1) % n], b[(k + 1) % n], b[k]))
            tmp.faces.new(list(reversed(a))); tmp.faces.new(b)
        else:
            ao, bo, ai, bi = ring(r_out, x0), ring(r_out, x1), ring(r_in, x0), ring(r_in, x1)
            for k in range(n):
                k1 = (k + 1) % n
                tmp.faces.new((ao[k], ao[k1], bo[k1], bo[k])); tmp.faces.new((bi[k], bi[k1], ai[k1], ai[k]))
                tmp.faces.new((bo[k], bo[k1], bi[k1], bi[k])); tmp.faces.new((ai[k], ai[k1], ao[k1], ao[k]))
        bmesh.ops.recalc_face_normals(tmp, faces=tmp.faces)
        if bevel > 0 and not LIVE_BEVEL:
            sharp = [e for e in tmp.edges if e.calc_face_angle(0) > math.radians(50)]
            bmesh.ops.bevel(tmp, geom=sharp, offset=bevel, segments=seg, profile=0.5, affect='EDGES', clamp_overlap=True)
        self._merge(tmp, band, {'x': band, 'other': side_band or band})
    def _merge(self, tmp, band, side_bands):
        me = bpy.data.meshes.new("tmp"); tmp.to_mesh(me); tmp.free()
        before = set(self.bm.faces)
        self.bm.from_mesh(me); bpy.data.meshes.remove(me)
        self.bm.faces.ensure_lookup_table()
        new = [f for f in self.bm.faces if f not in before]
        for f in new:
            b = band
            if side_bands:
                n = f.normal; ax = max(range(3), key=lambda i: abs(n[i])); sgn = '+' if n[ax] > 0 else '-'
                key = sgn + 'xyz'[ax]
                b = side_bands.get(key, side_bands.get('xyz'[ax], side_bands.get('other', band)))
            f[self.lay] = self.names.index(b)
    def build(self):
        """UVs: planar on the dominant axis. Per face, V runs across the face's SHORT side and fits it to its band; U
        runs along the long side at the same texel scale, world-anchored, so trims tile without stretch."""
        bm = self.bm
        bm.normal_update()
        uv = bm.loops.layers.uv.new("UVMap")
        for f in bm.faces:
            n = f.normal; ax = max(range(3), key=lambda i: abs(n[i]))
            a, b = {0: (1, 2), 1: (0, 2), 2: (1, 0)}[ax]
            ca = [l.vert.co[a] for l in f.loops]; cb = [l.vert.co[b] for l in f.loops]
            ea, eb = max(ca) - min(ca), max(cb) - min(cb)
            ua, va, vmin, ev = (a, b, min(cb), eb) if eb <= ea else (b, a, min(ca), ea)
            v0, v1 = BANDS[self.names[f[self.lay]]]
            k = (v1 - v0) / max(ev, 1e-3)
            for l in f.loops:
                co = l.vert.co
                l[uv].uv = (co[ua] * k, v0 + (co[va] - vmin) * k)
        # cm -> m and Unreal -> Blender (mirror Y), then fix winding
        bmesh.ops.scale(bm, vec=(0.01, -0.01, 0.01), verts=bm.verts)
        bmesh.ops.reverse_faces(bm, faces=bm.faces, flip_multires=False)
        me = bpy.data.meshes.new(self.name); bm.to_mesh(me); bm.free()
        me.materials.append(material("M_DominionBay_Emitter", (0.25, 0.85, 1.0)) if "Emitter" in self.name else material())
        for p in me.polygons: p.use_smooth = True
        o = bpy.data.objects.new(self.name, me); sc.collection.objects.link(o)
        # live polish: bevel weight 1 on hard edges (>= 40 deg, >= 3 cm), weight-limited Bevel, then Weighted Normal
        bm2 = bmesh.new(); bm2.from_mesh(me)
        bw = bm2.edges.layers.float.get("bevel_weight_edge") or bm2.edges.layers.float.new("bevel_weight_edge")
        nb = 0
        for e in bm2.edges:
            w = 0.0
            if len(e.link_faces) == 2 and e.calc_length() >= 0.03 and math.degrees(e.calc_face_angle(0)) >= 40:
                w = 1.0; nb += 1
            e[bw] = w
        bm2.to_mesh(me); bm2.free()
        me.set_sharp_from_angle(angle=math.radians(30))
        o["bevel_edges"] = nb
        bev = o.modifiers.new("Bevel", 'BEVEL'); bev.width = BEVEL_W.get(self.name, 0.04); bev.segments = 2
        bev.limit_method = 'WEIGHT'; bev.profile = 0.5; bev.harden_normals = True; bev.use_clamp_overlap = True
        bev.miter_outer = 'MITER_ARC'
        tr = o.modifiers.new("TriangulateNgons", 'TRIANGULATE'); tr.min_vertices = 5
        wn = o.modifiers.new("WeightedNormal", 'WEIGHTED_NORMAL'); wn.keep_sharp = True; wn.weight = 50
        wn.mode = 'FACE_AREA'; wn.use_face_influence = True
        # hidden, unmodified source copy for editing
        src = bpy.data.objects.new("SRC_" + self.name, me.copy()); C_SRC.objects.link(src)
        src.hide_render = True; src.hide_viewport = True; src.hide_set(True) if bpy.context.view_layer.objects.get(src.name) else None
        return o

# ------------------------------------------------------------------ door (L = port, seam on local +Y) ------------
def door(name, mirror):
    p = Part(name)
    W2, H2 = 1250.0, 1750.0
    S = -1.0 if mirror else 1.0
    def Y(a, b):  # map an L-door y-range to this door
        return (min(S * a, S * b), max(S * a, S * b))
    def B(lo, hi, band, **k):
        y0, y1 = Y(lo[1], hi[1]); p.box((lo[0], y0, lo[2]), (hi[0], y1, hi[2]), band, **k)
    # core slab, bevelled (stays 20 cm inside the outer plane)
    B((-300, -W2, -H2), (-80, W2, H2), 'GUNMETAL', bevel=6, side_bands={'x': 'GUNMETAL', 'other': 'GRILLE'})
    # outer armour plates: 2 columns x 3 rows, 20 cm gaps, chamfered; the seam column is narrower
    cols = [(-1230, -150), (-130, 930)]
    rows = [(-1550, -470), (-450, 650), (670, 1550)]
    for (y0, y1) in cols:
        for (z0, z1) in rows:
            B((-80, y0, z0), (-20, y1, z1), 'ARMOR', bevel=7, side_bands={'x': 'ARMOR', 'other': 'GUNMETAL'})
    # bevelled cap plates over the middle row (layered read, a step proud)
    for (y0, y1) in cols:
        B((-20, y0 + 120, -330), (-5, y1 - 120, 530), 'PANEL', bevel=4)
    # bottom hazard band and top crimson band
    B((-80, -1250, -1750), (-15, 950, -1570), 'HAZARD', bevel=4, side_bands={'x': 'HAZARD', 'other': 'GUNMETAL'})
    B((-80, -1250, 1570), (-15, 950, 1750), 'CRIMSON', bevel=4, side_bands={'x': 'CRIMSON', 'other': 'GUNMETAL'})
    # seam beam with a stepped lip and the red-orange edge strip (full height)
    B((-80, 950, -1750), (-10, 1250, 1750), 'GUNMETAL', bevel=6)
    B((-10, 1200, -1700), (0, 1240, 1700), 'STRIP')
    B((-10, 1000, -1700), (-2, 1180, 1700), 'STRIP_HOUSING', bevel=2)
    # half of the Dominion chevron (original mark: two nested downward chevrons split across the seam)
    for (top, depth, th) in ((1250, 950, 300), (700, 700, 240)):
        ys = [1250 - depth, 1250, 1250, 1250 - depth]
        poly = [(S * (1250 - depth), top), (S * 1250, top - depth), (S * 1250, top - depth - th), (S * (1250 - depth), top - th)]
        if mirror: poly = list(reversed(poly))
        p.prism(poly, -20, 0, 'CRIMSON', bevel=3, side_band='STRIP')
    # guide-rail shoes top and bottom (ride in the hull's door track)
    for z0, z1 in ((-1750, -1700), (1700, 1750)):
        for y in (-1000, 0, 800):
            B((-300, y - 80, z0), (-200, y + 80, z1), 'GUNMETAL', bevel=2)
    # 20 cm behind the socket plane: the hull's bow face is AT X 30000, so a face at 0 would z-fight when open
    bmesh.ops.translate(p.bm, verts=p.bm.verts, vec=(-20, 0, 0))
    return p.build()

def emitter():
    p = Part("SM_DominionBay_TractorEmitter")
    p.cyl(450, 0, 60, 'GUNMETAL', n=16, bevel=5, side_band='GRILLE')            # mount housing (16-gon)
    p.cyl(400, 60, 115, 'ARMOR', n=48, r_in=290, bevel=5, side_band='GUNMETAL')  # outer ring
    p.cyl(285, 60, 85, 'GRILLE', n=48)                                          # recessed throat
    p.cyl(250, 85, 100, 'STRIP', n=48)                                          # the lens (glow)
    p.cyl(330, 115, 125, 'STRIP', n=48, r_in=310)                               # glow ring on the face
    p.cyl(120, 100, 180, 'GUNMETAL', n=24, bevel=4)                             # central projector nose
    p.cyl(70, 180, 186, 'STRIP', n=24)
    for k in range(4):                                                          # four clamp fins at 45 deg
        a = math.radians(45 + 90 * k)
        poly = []
        for (r, w) in ((300, 60), (520, 40)):
            pass
        cy, cz = math.cos(a), math.sin(a)
        # a radial fin as a rotated box: build in local then rotate about X
        tmp = Part("tmp")
        tmp.box((40, 330, -45), (175, 470, 45), 'ARMOR', bevel=5, side_bands={'x': 'ARMOR', 'other': 'GUNMETAL'})
        bmesh.ops.rotate(tmp.bm, verts=tmp.bm.verts, cent=(0, 0, 0), matrix=Matrix.Rotation(a - math.pi / 2, 3, 'X'))
        me = bpy.data.meshes.new("t"); tmp.bm.to_mesh(me)
        before = set(p.bm.faces); p.bm.from_mesh(me); bpy.data.meshes.remove(me)
        for f in p.bm.faces:
            if f not in before: f[p.lay] = p.names.index('ARMOR')
    return p.build()

objs = [door("SM_DominionBay_Door_L", False), door("SM_DominionBay_Door_R", True), emitter()]
# collision: the slab is one convex box (outer plates and seam beam stay within X -320..-20); emitter = mount + nose
UCX = {}
for n in ("SM_DominionBay_Door_L", "SM_DominionBay_Door_R"):
    UCX[n] = [ucx(n, 0, box_pts((-320, -1250, -1750), (-20, 1250, 1750)))]
n = "SM_DominionBay_TractorEmitter"
UCX[n] = [ucx(n, 0, ring_pts(475, 0, 175, 16)), ucx(n, 1, ring_pts(125, 175, 186, 12))]

# --- report and export ----------------------------------------------------------------------------------------
dg = bpy.context.evaluated_depsgraph_get()
rep = {}
for o in objs:
    e = o.evaluated_get(dg); me = e.to_mesh()
    tris = sum(len(p.vertices) - 2 for p in me.polygons)
    xs = [v.co for v in me.vertices]
    lo = [min(v[i] for v in xs) * 100 for i in range(3)]; hi = [max(v[i] for v in xs) * 100 for i in range(3)]
    lo[1], hi[1] = -hi[1], -lo[1]     # back to Unreal Y
    rep[o.name] = dict(tris=tris, ue_bounds_cm=dict(min=[round(v, 1) for v in lo], max=[round(v, 1) for v in hi]),
                       size_cm=[round(h - l, 1) for l, h in zip(lo, hi)])
    e.to_mesh_clear()
    rep[o.name].update(bevel_edges=int(o["bevel_edges"]), bevel_width_cm=BEVEL_W[o.name] * 100, ucx=[u.name for u in UCX[o.name]])
    # export from a temp copy: modifiers applied, triangulated, tangents; the live object stays editable
    live = o.name
    t = o.copy(); t.data = o.data.copy(); sc.collection.objects.link(t)
    bpy.ops.object.select_all(action='DESELECT'); t.select_set(True); bpy.context.view_layer.objects.active = t
    for m in list(t.modifiers): bpy.ops.object.modifier_apply(modifier=m.name)
    bmt = bmesh.new(); bmt.from_mesh(t.data); bmesh.ops.triangulate(bmt, faces=bmt.faces, quad_method='BEAUTY', ngon_method='BEAUTY')
    bmt.to_mesh(t.data); bmt.free()
    o.name = live + "_LIVE_TMP"; t.name = live
    for u in UCX[live]: u.select_set(True)
    bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, live + ".fbx"), use_selection=True, object_types={'MESH'},
                             apply_unit_scale=True, apply_scale_options='FBX_SCALE_UNITS', global_scale=1.0,
                             axis_forward='-Z', axis_up='Y', use_mesh_modifiers=False, mesh_smooth_type='OFF',
                             use_tspace=True, use_custom_props=False, add_leaf_bones=False, bake_anim=False,
                             bake_space_transform=False, path_mode='STRIP', embed_textures=False)
    tm = t.data; bpy.data.objects.remove(t); bpy.data.meshes.remove(tm); o.name = live
# place for the .blend preview: closed state at SOCKET_Bay origin (Unreal (0, -/+1250, 0) -> Blender y flips)
bpy.data.objects["SM_DominionBay_Door_L"].location = (0, 12.5, 0)
bpy.data.objects["SM_DominionBay_Door_R"].location = (0, -12.5, 0)
bpy.data.objects["SM_DominionBay_TractorEmitter"].location = (12.0, 0, 34.0)
for o in objs:
    for extra in [bpy.data.objects["SRC_" + o.name]] + UCX[o.name]: extra.location = o.location
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, "DominionBay_Exterior.blend"), compress=True)
bpy.ops.file.make_paths_relative()
bpy.ops.wm.save_mainfile(compress=True)
json.dump(rep, open(os.path.join(OUT, "exterior_report.json"), "w"), indent=1)
print(json.dumps(rep, indent=1))
