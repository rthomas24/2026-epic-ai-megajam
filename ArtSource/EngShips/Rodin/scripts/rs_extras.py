# SPDX-License-Identifier: MIT
# Triad Drift - code-built additions to the Rodin squad ships (called by rs_prep.py before the export):
#   * Rift: a clean bevelled band around the portal ring (covers the ragged inner edge Rodin's disc removal left
#     and thickens the hoop) with magenta seam strips, and a slightly dished portal membrane filling the hoop;
#   * every squad ship: a glow core disc on the back wall of each engine nozzle (lit nozzles when landed).
# Each addition is its own object parented to the ship (the exporter joins them) with its own material slot:
#   M_<Ship>_Ring   -> MI_<Ship>_Ring   (M_TD_ShipTrim: dark metal; UV v > 0.85 = emissive seam)
#   M_<Ship>_Portal -> MI_<Ship>_Portal (M_TD_RiftPortal: UV = disc, centre 0.5 0.5, radius 0.5)
#   M_<Ship>_Glow   -> MI_<Ship>_Glow   (M_TD_ShipGlow: UV = disc, centre 0.5 0.5, radius 0.5)
# Units: Blender metres, nose +X, Z up (Blender Y = -Unreal Y; everything here is symmetric or mirrored).
import math

import bmesh
import bpy

# Rift's ring, measured on mesh_r1 by ray casts: axis along X, inner radius 69 cm (ragged), centre below.
RIFT_RING = dict(centre=(-1.60, 0.0, 0.44), r_in=0.615, r_out=0.785, half_depth=0.195, bevel=0.03)
# nozzle glow cores: (x of the disc, y, z, radius) in cm, 3 cm in front of each nozzle's back wall
NOZZLES = {
    "Rift": [(-233.0, 0.0, -34.0, 24.0), (-236.0, 64.0, -34.0, 11.5), (-236.0, -64.0, -34.0, 11.5)],
    "Lancer": [(-240.0, 0.0, -35.0, 16.5), (-201.5, 55.0, -38.0, 10.0), (-201.5, -55.0, -38.0, 10.0)],
    "Bastion": [(-236.0, 77.0, 0.0, 23.0), (-236.0, -77.0, 0.0, 23.0)],
}
GLOW = {"Rift": (1.0, 0.17, 0.84), "Lancer": (0.25, 0.91, 1.0), "Bastion": (1.0, 0.7, 0.12)}


def _material(name, color, emission=0.0):
    mat = bpy.data.materials.new(name)
    b = next(n for n in mat.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
    b.inputs["Base Color"].default_value = (*color, 1.0)
    b.inputs["Emission Color"].default_value = (*color, 1.0)
    b.inputs["Emission Strength"].default_value = emission
    return mat


def _object(name, bm, mat, parent):
    me = bpy.data.meshes.new(name)
    bm.normal_update()
    bm.to_mesh(me)
    bm.free()
    for p in me.polygons:
        p.use_smooth = True
    me.materials.append(mat)
    ob = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(ob)
    ob.parent = parent
    return ob


def _disc(bm, uvl, x, y, z, radius, facing=-1.0, dish=0.0, segments=40, rings=5):
    """Disc in the YZ plane at x, normal along facing * X, optional dish (centre pushed by dish along -facing)."""
    rows = []
    centre = bm.verts.new((x - facing * dish, y, z))
    for i in range(1, rings + 1):
        t = i / rings
        rx = x - facing * dish * (1.0 - t * t)
        rows.append([bm.verts.new((rx, y + math.cos(2 * math.pi * k / segments) * radius * t,
                                   z + math.sin(2 * math.pi * k / segments) * radius * t)) for k in range(segments)])

    def uv(v):
        return (0.5 + (v.co.y - y) / (2.0 * radius), 0.5 + (v.co.z - z) / (2.0 * radius))

    def face(verts):
        f = bm.faces.new(verts)
        if f.normal.length == 0.0:
            f.normal_update()
        f.normal_update()
        if f.normal.x * facing < 0.0:
            f.normal_flip()
        for loop in f.loops:
            loop[uvl].uv = uv(loop.vert)

    for k in range(segments):
        face([centre, rows[0][k], rows[0][(k + 1) % segments]])
    for i in range(rings - 1):
        for k in range(segments):
            face([rows[i][k], rows[i + 1][k], rows[i + 1][(k + 1) % segments], rows[i][(k + 1) % segments]])


def rift_ring(parent):
    """The clean band: a lathed, bevelled rectangle that encloses the old thin hoop. Three narrow strips (front
    face, back face, inner face) are UV-mapped into v > 0.85 and glow as seams."""
    R = RIFT_RING
    cx, cy, cz = R["centre"]
    ri, ro, hd, bv = R["r_in"], R["r_out"], R["half_depth"], R["bevel"]
    rm = (ri + ro) / 2.0
    sw = 0.012          # seam half-width
    # profile (axial offset a, radius r, seam?) going round the section; seam strips are their own segments
    prof = [(-hd + bv, ri, 0), (-sw, ri, 0), (sw, ri, 1), (hd - bv, ri, 0),              # inner face (with seam)
            (hd, ri + bv, 0), (hd, rm - sw, 0), (hd, rm + sw, 1), (hd, ro - bv, 0),      # front face (with seam)
            (hd - bv, ro, 0), (-hd + bv, ro, 0),                                         # outer face
            (-hd, ro - bv, 0), (-hd, rm + sw, 0), (-hd, rm - sw, 1), (-hd, ri + bv, 0)]  # back face (with seam)
    seg = 96
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new("UVMap")
    rings = []
    for k in range(seg):
        a = 2 * math.pi * k / seg
        rings.append([bm.verts.new((cx + p[0], cy + math.cos(a) * p[1], cz + math.sin(a) * p[1])) for p in prof])
    n = len(prof)
    for k in range(seg):
        k2 = (k + 1) % seg
        for j in range(n):
            j2 = (j + 1) % n
            seam = prof[j2][2] == 1          # the segment that ENDS on a seam point is the strip
            f = bm.faces.new([rings[k][j], rings[k][j2], rings[k2][j2], rings[k2][j]])
            v0, v1 = (0.90, 0.98) if seam else (0.05 + 0.7 * j / n, 0.05 + 0.7 * (j + 1) / n)
            u0, u1 = 4.0 * k / seg, 4.0 * (k + 1) / seg
            for loop, (u, v) in zip(f.loops, ((u0, v0), (u0, v1), (u1, v1), (u1, v0))):
                loop[uvl].uv = (u, v)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return _object("RiftRingBand", bm, _material("M_Rift_Ring", (0.03, 0.03, 0.035)), parent)


def rift_portal(parent):
    R = RIFT_RING
    cx, cy, cz = R["centre"]
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new("UVMap")
    _disc(bm, uvl, cx, cy, cz, R["r_in"] + 0.012, facing=-1.0, dish=0.05, segments=64, rings=8)
    return _object("RiftPortalDisc", bm, _material("M_Rift_Portal", (0.8, 0.1, 1.0), 3.0), parent)


def nozzle_glow(ship, parent):
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new("UVMap")
    for x, y, z, r in NOZZLES[ship]:
        _disc(bm, uvl, x / 100.0, -y / 100.0, z / 100.0, r / 100.0, facing=-1.0, dish=0.02, segments=28, rings=3)
    return _object("%sNozzleGlow" % ship, bm, _material("M_%s_Glow" % ship, GLOW[ship], 4.0), parent)


def rift_clear_hoop(hull):
    """Remove the ragged fragments the disc removal left inside the hoop (faces of the hull that lie wholly inside
    the ring opening, in the ring's own depth, above the fuselage): they poked through the portal membrane."""
    R = RIFT_RING
    cx, cy, cz = R["centre"]
    bm = bmesh.new()
    bm.from_mesh(hull.data)

    def inside(v):
        return (abs(v.co.x - cx) < R["half_depth"] + 0.01 and v.co.z > 0.20
                and math.hypot(v.co.y - cy, v.co.z - cz) < R["r_in"] - 0.01)

    doomed = [f for f in bm.faces if all(inside(v) for v in f.verts)]
    n = len(doomed)
    bmesh.ops.delete(bm, geom=doomed, context="FACES")
    bm.to_mesh(hull.data)
    bm.free()
    hull.data.update()
    return n


def add(ship, parent):
    """Returns a list of (object name, tris) for the report."""
    made = []
    if ship not in NOZZLES:
        return made
    cleared = 0
    if ship == "Rift":
        cleared = rift_clear_hoop(parent)
        made += [rift_ring(parent), rift_portal(parent)]
    made.append(nozzle_glow(ship, parent))
    out = []
    for ob in made:
        ob.data.calc_loop_triangles()
        out.append((ob.name, len(ob.data.loop_triangles)))
    if cleared:
        out.append(("hoop fragments removed (faces)", cleared))
    return out
