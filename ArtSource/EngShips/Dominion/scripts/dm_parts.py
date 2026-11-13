# SPDX-License-Identifier: MIT
"""
dm_parts.py - the Dominion's finale objectives (Triad Drift): shield generator pylon, its glowing core cell and the
trench reactor. Blender 5.2 headless, same conventions as dm_ships.py (parts authored in UNREAL metres, piece-local,
X forward, Z up, pivot = actor origin; converted with dm_ships.UE_TO_BL). See ../brief_parts.md for the contract.

Builders return a dm_ships.Kit (parts + UCX point lists). Materials are the kit's (triplanar panel/trim + flat glow)
plus two glows: GlowCyan (generator cell) and GlowReactor (reactor core).
"""

import math

import bmesh
from mathutils import Matrix, Vector

import dm_ships as S

# ------------------------------------------------------------------------------------------------ code contract (m)
GEN_R, GEN_HALF_H = 6.0, 10.0              # Cylinder x (12, 12, 20) centred on the actor
CORE_ATTACH_Z = 0.6                        # Core component relative location with an art pylon (scale 1)
CORE_WORLD_Z = 12.0                        # where the placeholder showed it (60 cm x pylon scale 20)
CORE_LOCAL_Z = CORE_WORLD_Z - CORE_ATTACH_Z  # 11.4 m: the core mesh's geometry centre in its own space
CORE_HALF = (5.4, 5.0)                     # Sphere x (0.9, 0.9, 0.5) under the pylon: 10.8 x 10.8 x 10 m
REACTOR_R = 12.5                           # Sphere x 25
TRENCH_FLOOR_Z = -15.0                     # hull deck Z 5000 - reactor actor Z 6500


def materials():
    m = S.materials()
    m["GlowCyan"] = S._glow_mat("M_Dominion_GlowCyan", (0.3, 0.8, 1.0), 2.2)
    m["GlowReactor"] = S._glow_mat("M_Dominion_GlowReactor", (1.0, 0.7, 0.15), 2.2)
    return m


# ------------------------------------------------------------------------------------------------ shapes

def lathe_z_bm(profile, segs, center=(0.0, 0.0, 0.0), phase=0.0, loop=False):
    """Revolve [(r, z), ...] around the UE Z axis. loop=True: the profile is a closed cross-section (a ring);
    otherwise its ends must lie on the axis (r = 0) and are closed there."""
    bm = bmesh.new()
    c = Vector(center)
    rings = []
    for i in range(segs):
        a = math.tau * i / segs + phase
        ca, sa = math.cos(a), math.sin(a)
        rings.append([bm.verts.new(c + Vector((r * ca, r * sa, z))) for r, z in profile])
    n = len(profile)
    spans = n if loop else n - 1
    for i in range(segs):
        ra, rb = rings[i], rings[(i + 1) % segs]
        for j in range(spans):
            k = (j + 1) % n
            q = [ra[j], ra[k], rb[k], rb[j]]
            u = []
            for v in q:
                if all((v.co - w.co).length > 1e-6 for w in u):
                    u.append(v)
            if len(u) >= 3:
                try:
                    bm.faces.new(u)
                except ValueError:
                    pass
    bmesh.ops.remove_doubles(bm, verts=list(bm.verts), dist=1e-5)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    return bm


def shell_sector_bm(r_in, r_out, az0, az1, lat0, lat1, nu=8, nv=10):
    """Closed curved armour petal: a slice of a spherical shell (UE Z up), angles in degrees."""
    bm = bmesh.new()

    def p(r, u, v):
        az = math.radians(az0 + (az1 - az0) * u)
        la = math.radians(lat0 + (lat1 - lat0) * v)
        return Vector((r * math.cos(la) * math.cos(az), r * math.cos(la) * math.sin(az), r * math.sin(la)))
    G = {}
    for side, r in ((0, r_in), (1, r_out)):
        for i in range(nu + 1):
            for j in range(nv + 1):
                G[side, i, j] = bm.verts.new(p(r, i / nu, j / nv))
    for i in range(nu):
        for j in range(nv):
            bm.faces.new((G[1, i, j], G[1, i + 1, j], G[1, i + 1, j + 1], G[1, i, j + 1]))
            bm.faces.new((G[0, i, j + 1], G[0, i + 1, j + 1], G[0, i + 1, j], G[0, i, j]))
    for i in range(nu):
        bm.faces.new((G[0, i, 0], G[0, i + 1, 0], G[1, i + 1, 0], G[1, i, 0]))
        bm.faces.new((G[1, i, nv], G[1, i + 1, nv], G[0, i + 1, nv], G[0, i, nv]))
    for j in range(nv):
        bm.faces.new((G[1, 0, j], G[1, 0, j + 1], G[0, 0, j + 1], G[0, 0, j]))
        bm.faces.new((G[0, nu, j], G[0, nu, j + 1], G[1, nu, j + 1], G[1, nu, j]))
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    return bm


def radial_blade_bm(az_deg, pts_rz, thick):
    """A flat blade in the radial plane at azimuth az: polygon [(r, z), ...] extruded +/- thick/2 tangentially."""
    a = math.radians(az_deg)
    d = Vector((math.cos(a), math.sin(a), 0.0))
    t = Vector((-d.y, d.x, 0.0)) * (thick * 0.5)
    ring = [d * r + Vector((0.0, 0.0, z)) for r, z in pts_rz]
    return S.loft3_bm([[q - t for q in ring], [q + t for q in ring]])


def oriented_box_bm(center, az_deg, size_rtz, tilt_deg=0.0):
    """Box with its axes (radial, tangential, Z) at azimuth az, optionally tilted about the tangential axis."""
    bm = bmesh.new()
    m = (Matrix.Translation(Vector(center)) @ Matrix.Rotation(math.radians(az_deg), 4, "Z")
         @ Matrix.Rotation(math.radians(tilt_deg), 4, "Y") @ Matrix.Diagonal((*size_rtz, 1.0)))
    bmesh.ops.create_cube(bm, size=1.0, matrix=m)
    return bm


def polar(r, az_deg, z):
    a = math.radians(az_deg)
    return Vector((r * math.cos(a), r * math.sin(a), z))


def ring_pts(r, z0, z1, segs=16):
    return [polar(r, 360.0 * i / segs, z) for i in range(segs) for z in (z0, z1)]


def sphere_pts(r, center=(0.0, 0.0, 0.0), nu=16, nv=8):
    c = Vector(center)
    out = [c + Vector((0, 0, r)), c + Vector((0, 0, -r))]
    for j in range(1, nv):
        la = math.pi * (j / nv - 0.5)
        for i in range(nu):
            az = math.tau * i / nu
            out.append(c + Vector((r * math.cos(la) * math.cos(az), r * math.cos(la) * math.sin(az), r * math.sin(la))))
    return out


# ------------------------------------------------------------------------------------------------ generator pylon

# hex spire profile (r, z): keel spike, waist, shaft, heavy inverted-cone collar (widest) with a dished top for the cell
GEN_BODY = [(0.0, -10.0), (0.9, -9.3), (2.6, -6.6), (3.9, -4.6), (4.1, -1.5), (3.7, 0.0), (3.45, 4.0),
            (5.9, 6.75), (5.95, 7.7), (5.2, 8.45), (3.6, 8.6), (2.0, 8.9), (0.0, 8.5)]


def gen_body_r(z):
    for (ra, za), (rb, zb) in zip(GEN_BODY, GEN_BODY[1:]):
        if za <= z <= zb and zb > za:
            return ra + (rb - ra) * (z - za) / (zb - za)
    return 0.0


def _plate_quad(az, half, r_lo, z_lo, r_hi, z_hi):
    q = [polar(r_lo, az - half, z_lo), polar(r_lo, az + half, z_lo), polar(r_hi, az + half, z_hi),
         polar(r_hi, az - half, z_hi)]
    n = (q[1] - q[0]).cross(q[3] - q[0]).normalized()
    if n.dot(polar(1.0, az, 0.0)) < 0:
        n = -n
    return q, n


TALON = [(4.4, 6.8), (5.95, 6.9), (5.95, 8.4), (5.6, 10.5), (5.0, 13.4), (4.0, 15.6), (3.0, 17.0), (3.1, 16.0),
         (3.9, 14.4), (4.5, 12.4), (4.8, 10.4), (4.6, 8.6)]
TALON_INNER = [(4.6, 8.6), (4.8, 10.4), (4.5, 12.4), (3.9, 14.4), (3.1, 16.0)]


def _edge_strip(edge, width):
    """polygon (r, z) for a strip of the given width just outside a (r, z) polyline (radially outward)."""
    return list(edge) + [(r + width, z) for r, z in reversed(edge)]


def build_generator(asset, mats, stage):
    K = S.Kit(asset, mats)
    full = stage == "full"

    def bev(w):
        return w if full else None
    # hex vertices at 0, 60, ... ; faces centred at 30, 90, ...
    K.add(lathe_z_bm(GEN_BODY, 6), "Trim", bevel=bev(0.12), seg=2)
    # armoured skirt: six plates on the hex vertices, raked OUT at the top, hems tapering in toward the keel
    for k in range(6):
        az = 60.0 * k
        q, n = _plate_quad(az, 22.0, 3.7, -6.6, 5.1, 0.3)
        K.add(S.wedge_bm(q, n, 0.2, 0.5, 0.15), "Panel", bevel=bev(0.1), seg=2)
        if full:   # centre rib + two inset panel lines (dark trim) on each plate
            q2, n2 = _plate_quad(az, 2.2, 3.82, -6.0, 5.28, 0.0)
            K.add(S.slab_bm(q2, n2, 0.62, 0.0), "Trim", bevel=0.05, seg=1)
            for s in (-1, 1):
                q3, n3 = _plate_quad(az + s * 12.0, 7.0, 3.9, -4.4, 4.75, -1.4)
                K.add(S.slab_bm(q3, n3, 0.42, 0.0), "Trim", bevel=0.04, seg=1)
    # three blade vanes in the skirt gaps (face centres 30 / 150 / 270), rooted INTO the shaft, sweeping into the collar
    vane = [(2.8, -7.0), (4.8, -3.0), (5.55, 2.8), (5.9, 6.5), (3.0, 4.6), (2.8, 1.0)]
    for az in (30.0, 150.0, 270.0):
        K.add(radial_blade_bm(az, vane, 0.6), "Panel", bevel=bev(0.1), seg=2)
        if full:   # red seam on the leading edge, two bracing struts to the shaft
            K.add(radial_blade_bm(az, [(4.72, -2.8), (5.47, 2.8), (5.66, 2.8), (4.9, -2.8)], 0.3), "GlowRed")
            for z in (-1.8, 2.2):
                K.add(radial_blade_bm(az, [(2.9, z - 0.3), (5.0, z - 0.15), (5.0, z + 0.25), (2.9, z + 0.4)], 1.0),
                      "Trim", bevel=0.05, seg=1)
    # talons: rooted in the collar's side faces with a gusset, raked out, hooked in over the cell (bow-horn echo)
    for az in (90.0, 210.0, 330.0):
        K.add(radial_blade_bm(az, TALON, 0.9), "Trim", bevel=bev(0.12), seg=2)
        K.add(radial_blade_bm(az, [(3.4, 4.3), (5.95, 6.95), (4.2, 7.0)], 0.7), "Trim", bevel=bev(0.08), seg=1)
        if full:
            K.add(radial_blade_bm(az, _edge_strip(TALON_INNER, 0.2), 0.96), "GlowRed")
    if not full:
        return _gen_hulls(K)
    # talon armour caps (lighter panel) on the outer edge
    cap = [(5.55, 7.4), (5.98, 7.6), (5.98, 8.4), (5.65, 10.5), (5.1, 13.2), (4.8, 13.0), (5.25, 10.5), (5.55, 8.4)]
    for az in (90.0, 210.0, 330.0):
        K.add(radial_blade_bm(az, cap, 1.15), "Panel", bevel=0.06, seg=1)
    # red bands: under the collar cone and a 40 cm waist band
    K.add(lathe_z_bm([(3.2, 3.85), (3.62, 3.85), (3.62, 4.1), (3.2, 4.1)], 6, loop=True), "GlowRed")
    K.add(lathe_z_bm([(3.7, -1.2), (4.25, -1.2), (4.25, -0.8), (3.7, -0.8)], 6, loop=True), "GlowRed")
    r = gen_body_r(-2.2) + 0.15
    K.add(lathe_z_bm([(0.0, -2.5), (r, -2.5), (r, -1.9), (0.0, -1.9)], 6), "Panel", bevel=0.06, seg=1)
    # white eye-slit clusters (3 per face) on the shaft faces between the vanes, under a raked brow
    for az in (90.0, 210.0, 330.0):
        for z, w in ((3.05, 2.4), (2.6, 1.6), (2.18, 0.9)):
            ap = gen_body_r(z) * math.cos(math.radians(30.0))
            K.add(oriented_box_bm(polar(ap + 0.02, az, z), az, (0.3, w, 0.24)), "GlowWhite")
        ap = gen_body_r(3.5) * math.cos(math.radians(30.0))
        K.add(oriented_box_bm(polar(ap + 0.14, az, 3.5), az, (0.4, 3.0, 0.32), tilt_deg=-20.0), "Panel",
              bevel=0.05, seg=1)
    for az in (90.0, 210.0, 330.0):
        ap = gen_body_r(-7.4) * math.cos(math.radians(30.0))
        K.add(oriented_box_bm(polar(ap + 0.05, az, -7.4), az, (0.3, 0.8, 1.4)), "Panel", bevel=0.05, seg=1)
        K.add(oriented_box_bm(polar(ap + 0.14, az, -7.4), az, (0.12, 0.3, 0.9)), "GlowRed")
    return _gen_hulls(K)


def _gen_hulls(K):
    K.hull_pts(ring_pts(GEN_R, -GEN_HALF_H, GEN_HALF_H, 16))     # the placeholder cylinder (damage target)
    K.hull_pts(ring_pts(5.6, 7.0, 17.0, 12))                     # the cradle + cell, so shots at the weak point hit
    return K


# ------------------------------------------------------------------------------------------------ generator core

CELL = [(0.0, -4.9), (1.2, -4.3), (2.9, -3.0), (3.5, -1.4), (3.5, 1.4), (2.9, 3.0), (1.2, 4.3), (0.0, 4.9)]


def cell_r(z):
    for (ra, za), (rb, zb) in zip(CELL, CELL[1:]):
        if za <= z <= zb:
            return ra + (rb - ra) * (z - za) / (zb - za)
    return 0.0


def build_generator_core(asset, mats, stage):
    K = S.Kit(asset, mats)
    c = Vector((0.0, 0.0, CORE_LOCAL_Z))
    K.add(lathe_z_bm(CELL, 12, center=c), "GlowCyan")          # faceted crystal capsule (flat facets)
    if stage != "full":
        return K
    # dark containment frame: 6 ribs on facet edges standing 25 cm proud (facets read as inset cyan panels)
    prof = [z * 0.25 for z in range(-16, 17)]
    for k in range(6):
        az = 60.0 * k
        outer = [(cell_r(z) + 0.25, z + CORE_LOCAL_Z) for z in prof]
        inner = [(max(0.05, cell_r(z) - 0.1), z + CORE_LOCAL_Z) for z in reversed(prof)]
        K.add(radial_blade_bm(az, outer + inner, 0.2), "Trim")
    for z0, z1 in ((-2.35, -1.85), (1.85, 2.35)):
        ri = min(cell_r(z0), cell_r(z1)) - 0.1
        ro = max(cell_r(z0), cell_r(z1)) + 0.5
        K.add(lathe_z_bm([(ri, z0 + CORE_LOCAL_Z), (ro, z0 + CORE_LOCAL_Z), (ro, z1 + CORE_LOCAL_Z),
                          (ri, z1 + CORE_LOCAL_Z)], 12, loop=True), "Trim", bevel=0.05, seg=1)
    # pointed hex end caps
    for s in (1, -1):
        prof_c = [(0.0, s * 5.05), (1.45, s * 4.3), (1.3, s * 3.95), (0.0, s * 4.2)]
        if s < 0:
            prof_c = list(reversed(prof_c))
        K.add(lathe_z_bm([(r, z + CORE_LOCAL_Z) for r, z in prof_c], 6), "Trim", bevel=0.05, seg=1)
    return K


# ------------------------------------------------------------------------------------------------ reactor

PED = [(0.0, -15.6), (10.0, -15.6), (10.0, -14.7), (6.2, -12.6), (5.1, -10.4), (7.1, -7.1), (6.3, -6.7),
       (0.0, -8.2)]
PETAL_HALF = 20.0
CORE_R = 8.0


def _sph(r, az, lat):
    a, la = math.radians(az), math.radians(lat)
    return Vector((r * math.cos(la) * math.cos(a), r * math.cos(la) * math.sin(a), r * math.sin(la)))


def build_reactor(asset, mats, stage):
    K = S.Kit(asset, mats)
    full = stage == "full"

    def bev(w):
        return w if full else None
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=40, v_segments=20, radius=CORE_R)
    K.add(bm, "GlowReactor", smooth=80.0)
    # white-hot heart under the oculus
    K.add(lathe_z_bm([(0.0, 8.35), (2.6, 8.05), (3.5, 7.55), (0.0, 7.2)], 24), "GlowWhite", smooth=50.0)
    # six faceted armour petals centred at 30 + 60k (flat facets: 1 x 3 chords), gaps face +/-X and the diagonals
    for k in range(6):
        az = 60.0 * k + 30.0
        K.add(shell_sector_bm(9.7, 10.2, az - PETAL_HALF, az + PETAL_HALF, -50.0, 40.0, nu=1, nv=3), "Panel",
              bevel=bev(0.12), seg=2, angle=20.0)
        # petal tip raked OUT above the band into a crown tooth (hull chevron language)
        b0, b1 = _sph(10.2, az - PETAL_HALF, 40.0), _sph(10.2, az + PETAL_HALF, 40.0)
        c0, c1 = _sph(9.7, az - PETAL_HALF, 40.0), _sph(9.7, az + PETAL_HALF, 40.0)
        t_out = [polar(7.75, az - 3.5, 11.2), polar(7.75, az + 3.5, 11.2)]
        t_in = [polar(7.3, az - 3.5, 11.0), polar(7.3, az + 3.5, 11.0)]
        K.add(S.loft3_bm([[c0, c1, t_in[1], t_in[0]], [b0, b1, t_out[1], t_out[0]]]), "Panel", bevel=bev(0.1), seg=2)
        if full:
            # gap reveals (inner walls give the glowing stripes depth)
            for e in (az - PETAL_HALF, az + PETAL_HALF):
                K.add(shell_sector_bm(8.0, 9.75, e - 0.8, e + 0.8, -46.0, 38.0, nu=1, nv=3), "Trim", bevel=0.04,
                      seg=1, angle=20.0)
            # spine on each tooth
            K.add(S.loft3_bm([[_sph(9.5, az - 1.6, 30.0), _sph(9.5, az + 1.6, 30.0), polar(7.45, az + 0.8, 11.05),
                                polar(7.45, az - 0.8, 11.05)],
                               [_sph(9.95, az - 1.6, 30.0), _sph(9.95, az + 1.6, 30.0), polar(7.95, az + 0.8, 11.25),
                                polar(7.95, az - 0.8, 11.25)]]), "Trim", bevel=0.04, seg=1)
    # equatorial clamp band
    K.add(lathe_z_bm([(9.0, -1.15), (11.2, -1.15), (11.45, -0.8), (11.45, 0.8), (11.2, 1.15), (9.0, 1.15)], 36,
                     loop=True), "Trim", bevel=bev(0.06), seg=1, angle=40.0)
    # oculus ring with a hot lip on its inner face
    K.add(lathe_z_bm([(5.4, 6.4), (7.4, 6.4), (7.6, 7.3), (7.0, 8.0), (5.8, 8.3)], 24, loop=True), "Trim",
          bevel=bev(0.08), seg=1, angle=40.0)
    K.add(lathe_z_bm([(4.9, 6.9), (5.5, 6.9), (5.7, 8.25), (5.1, 8.45)], 24, loop=True), "GlowReactor")
    # pedestal cup down to the trench floor, four buttresses (box sections)
    K.add(lathe_z_bm(PED, 6, phase=math.radians(30.0)), "Trim", bevel=bev(0.12), seg=2)
    for k in range(4):
        az = 90.0 * k + 45.0
        K.add(radial_blade_bm(az, [(6.0, -15.6), (12.6, -15.6), (12.4, -14.2), (11.5, -3.0), (10.4, -1.2),
                                   (9.4, -6.2), (7.0, -11.0)], 1.5), "Trim", bevel=bev(0.15), seg=2)
    if not full:
        return _reactor_hulls(K)
    for k in range(4):
        az = 90.0 * k + 45.0
        web = [(8.2, -12.6), (10.9, -12.6), (10.6, -6.0), (9.9, -6.0)]
        K.add(radial_blade_bm(az, web, 1.8), "Panel", bevel=0.06, seg=1)
        K.add(radial_blade_bm(az, [(10.85, -12.0), (11.05, -12.0), (10.75, -6.6), (10.55, -6.6)], 1.86), "GlowRed")
        K.add(oriented_box_bm(polar(11.9, az, -15.2), az, (3.0, 2.8, 0.9)), "Trim", bevel=0.08, seg=1)  # foot pad
    # clamp blocks on the petal centres (never over a gap) with white eye slits; red seams on band and cup lip
    for k in range(6):
        az = 60.0 * k + 30.0
        K.add(oriented_box_bm(polar(11.55, az, 0.0), az, (1.9, 3.4, 3.8)), "Trim", bevel=0.1, seg=2)
        K.add(oriented_box_bm(polar(12.5, az, 0.95), az, (0.12, 2.2, 0.3)), "GlowWhite")
        K.add(oriented_box_bm(polar(12.5, az, 0.4), az, (0.12, 1.3, 0.24)), "GlowWhite")
        K.add(oriented_box_bm(polar(12.0, az, -1.5), az, (1.0, 2.2, 0.6)), "Panel", bevel=0.04, seg=1)
    K.add(lathe_z_bm([(11.4, -0.18), (11.56, -0.18), (11.56, 0.18), (11.4, 0.18)], 36, loop=True), "GlowRed")
    K.add(lathe_z_bm([(6.6, -7.25), (7.25, -7.25), (7.25, -6.95), (6.6, -6.95)], 6, loop=True,
                     phase=math.radians(30.0)), "GlowRed")
    for k in range(6):
        az = 60.0 * k
        ap = 6.0 * math.cos(math.radians(30.0))
        K.add(oriented_box_bm(polar(ap - 0.05, az, -12.4), az, (0.2, 0.5, 2.6), tilt_deg=-25.0), "GlowRed")
    return _reactor_hulls(K)


def _reactor_hulls(K):
    K.hull_pts(sphere_pts(REACTOR_R))                       # the placeholder sphere (damage target)
    K.hull_pts(ring_pts(10.0, -15.6, -8.0, 12))             # pedestal to the trench floor
    return K


BUILDERS = {
    # asset: (builder, tri budget (lo, hi))
    "SM_Dominion_Generator": (build_generator, (3000, 40000)),
    "SM_Dominion_GeneratorCore": (build_generator_core, (300, 10000)),
    "SM_Dominion_Reactor": (build_reactor, (8000, 60000)),
}
