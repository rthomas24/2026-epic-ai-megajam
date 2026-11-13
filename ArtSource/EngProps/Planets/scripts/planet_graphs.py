# Procedural planet surfaces, as Blender shader node graphs evaluated on the unit sphere.
# Each builder returns a dict of sockets: albedo (linear colour), emis (linear colour 0..1),
# height (0..1, 0 = sea level / flat), rough (0..1), cloud (0..1 coverage).
# All noise is evaluated on the normalised 3D object position, so the bake is seamless at the
# longitude wrap and has no pinching at the poles.
from pl_lib import hexc


def verdance(nb, seed=0.0):
    p = nb.objpos_normalized()
    px, py, pz = nb.sep(p)
    lat = nb.absv(pz)
    s = (seed, seed * 1.7, seed * 0.3)
    p0 = nb.vadd(p, s)

    # --- continents: domain-warped fBm, sea level at 0.5
    w1 = nb.noise(p0, 1.3, detail=3, rough=0.5, out="Color")
    q = nb.warp(p0, w1, 0.55)
    w2 = nb.noise(q, 3.2, detail=3, rough=0.5, out="Color")
    q2 = nb.warp(q, w2, 0.16)
    cont = nb.noise(q2, 1.05, detail=12, rough=0.60, lac=2.05)
    land = nb.smooth(cont, 0.497, 0.503)
    elev = nb.remap(cont, 0.5, 0.70)                     # 0 at the coast, 1 high inland
    depth = nb.remap(cont, 0.5, 0.36)                    # 0 at the coast, 1 deep ocean

    # mountain ridges (ridged noise, only on high ground)
    r1 = nb.ridge(nb.noise(nb.vadd(q2, (3.1, 1.2, 7.7)), 3.6, detail=7, rough=0.55))
    ridges = nb.mul(nb.powv(r1, 4.0), nb.smooth(elev, 0.12, 0.55))
    fine = nb.noise(q2, 38.0, detail=6, rough=0.6)
    coast_ramp = nb.smooth(elev, 0.0, 0.10)          # no cliff step at the shoreline
    height = nb.mul(land, nb.add(nb.add(nb.mul(elev, 0.45), nb.mul(ridges, 0.55)), nb.mul(nb.mul(fine, 0.10), coast_ramp)))

    # moisture / biome variation
    moist = nb.noise(nb.vadd(q, (11.0, 4.0, -3.0)), 2.4, detail=5, rough=0.55)
    jungle = nb.ramp(elev, [(0.0, "#5CF0B0"), (0.04, "#22C878"), (0.25, "#10A050"), (0.50, "#0A7E40"),
                            (0.70, "#178A58"), (0.86, "#6C4FE0"), (1.0, "#E6F0FF")])
    meadow = nb.ramp(elev, [(0.0, "#5CF5D0"), (0.20, "#2CE0A8"), (0.50, "#18B888"), (0.78, "#5E52E0"),
                            (1.0, "#E0ECFF")])
    land_col = nb.mix(nb.smooth(moist, 0.42, 0.62), meadow, jungle)
    # ridges catch a paler rock tone
    land_col = nb.mix(nb.mul(ridges, 0.30), land_col, "#7FE0B8")
    # violet highland bloom fields (palette secondary)
    vio = nb.mul(nb.smooth(nb.noise(nb.vadd(q, (-5.0, 9.0, 2.0)), 3.0, detail=4), 0.62, 0.72),
                 nb.smooth(elev, 0.2, 0.45))
    land_col = nb.mix(nb.mul(vio, 0.75), land_col, "#8A4CF0")

    ocean = nb.ramp(depth, [(0.0, "#5CF5E0"), (0.07, "#2CD2C8"), (0.18, "#14A0B0"), (0.40, "#0B6C8C"),
                            (1.0, "#06386A")])
    # ocean current streaks
    cur = nb.noise(nb.comb(nb.mul(px, 3.0), nb.mul(py, 3.0), nb.mul(pz, 9.0)), 2.5, detail=5, distortion=0.6)
    ocean = nb.mix(nb.mul(nb.smooth(cur, 0.55, 0.75), 0.18), ocean, "#1FB5C0")
    albedo = nb.mix(land, ocean, land_col)

    # polar ice
    ice_n = nb.noise(p0, 6.0, detail=6)
    ice = nb.smooth(nb.add(lat, nb.mul(ice_n, 0.12)), 0.90, 0.95)
    albedo = nb.mix(ice, albedo, "#E4F4FF")

    # --- bioluminescence: lowland jungle patches with vein networks + glowing shorelines
    low = nb.mul(nb.mul(land, nb.one_minus(nb.smooth(elev, 0.35, 0.65))), nb.one_minus(ice))
    patch = nb.smooth(nb.noise(nb.vadd(q2, (2.0, -6.0, 1.0)), 5.5, detail=5, rough=0.6), 0.47, 0.60)
    vein = nb.smooth(nb.ridge(nb.noise(nb.vadd(q2, (-8.0, 2.0, 5.0)), 10.0, detail=4, rough=0.5)), 0.80, 0.95)
    speck = nb.smooth(nb.noise(q2, 70.0, detail=2), 0.62, 0.74)
    bio = nb.mul(low, nb.add(nb.mul(patch, nb.add(0.30, nb.mul(vein, 0.70))), nb.mul(speck, nb.mul(patch, 0.4))))
    shore = nb.mul(nb.one_minus(nb.smooth(nb.absv(nb.sub(cont, 0.5)), 0.0, 0.012)),
                   nb.smooth(nb.noise(q2, 9.0, detail=3), 0.45, 0.6))
    shore = nb.mul(shore, nb.one_minus(ice))
    hue = nb.smooth(nb.noise(nb.vadd(q, (4.0, 4.0, 4.0)), 2.2, detail=3), 0.40, 0.62)
    bio_col = nb.mix(hue, "#3CFFE0", "#B45CFF")
    emis = nb.cadd(nb.cscale(bio_col, nb.clamp01(bio)), nb.cscale(hexc("#38F0FF"), nb.mul(shore, 0.8)))

    rough = nb.add(nb.mul(land, 0.62), 0.22)             # ocean 0.22, land 0.84
    rough = nb.add(rough, nb.mul(ice, -0.25))

    # --- clouds: weather systems (low-frequency mask) carrying detailed, swirled cloud texture,
    # organised in latitude bands
    cw1 = nb.noise(nb.vadd(p0, (20.0, 0.0, 0.0)), 1.4, detail=2, out="Color")
    cq = nb.warp(p0, cw1, 0.45)
    cw2 = nb.noise(cq, 3.5, detail=2, out="Color")
    cq = nb.warp(cq, cw2, 0.10)
    cx, cy, cz = nb.sep(cq)
    stretched = nb.comb(cx, cy, nb.mul(cz, 4.0))
    band = nb.add(nb.mul(nb.m("SINE", nb.add(nb.mul(pz, 10.0), nb.mul(nb.noise(cq, 1.5, detail=2), 5.0))), 0.5), 0.5)
    sysm = nb.noise(stretched, 1.6, detail=3, rough=0.5)
    sysm = nb.smooth(nb.add(nb.mul(sysm, 0.75), nb.mul(band, 0.25)), 0.50, 0.60)
    det = nb.noise(stretched, 5.5, detail=10, rough=0.62, distortion=0.15)
    det = nb.smooth(det, 0.38, 0.66)
    cloud = nb.mul(sysm, nb.add(0.25, nb.mul(det, 0.75)))
    # thin scattered cirrus outside the systems
    cirrus = nb.mul(nb.smooth(nb.noise(nb.comb(cx, cy, nb.mul(cz, 9.0)), 7.0, detail=8), 0.60, 0.75), 0.35)
    cloud = nb.clamp01(nb.add(cloud, cirrus))
    return dict(albedo=albedo, emis=emis, height=height, rough=rough, cloud=cloud)


def cinder(nb, seed=0.0):
    p = nb.objpos_normalized()
    px, py, pz = nb.sep(p)
    p0 = nb.vadd(p, (seed + 40.0, 3.0, -9.0))

    w1 = nb.noise(p0, 1.5, detail=3, out="Color")
    q = nb.warp(p0, w1, 0.5)
    w2 = nb.noise(q, 4.0, detail=3, out="Color")
    q2 = nb.warp(q, w2, 0.14)
    crust = nb.noise(q2, 1.6, detail=12, rough=0.62)
    elev = nb.remap(crust, 0.32, 0.72)
    r1 = nb.ridge(nb.noise(nb.vadd(q2, (1.0, 5.0, 3.0)), 4.2, detail=7, rough=0.55))
    ridges = nb.mul(nb.powv(r1, 3.0), nb.smooth(elev, 0.35, 0.7))
    fine = nb.noise(q2, 45.0, detail=6, rough=0.65)

    # --- lava rivers: thin ridge-lines of a warped noise, only in the lowlands
    lowland = nb.one_minus(nb.smooth(elev, 0.40, 0.62))
    belt = nb.smooth(nb.noise(nb.vadd(p0, (5.0, -2.0, 1.0)), 1.8, detail=2), 0.36, 0.50)   # volcanic regions
    rn = nb.noise(nb.vadd(q2, (-4.0, 2.0, 8.0)), 3.4, detail=2.5, rough=0.5)
    rline = nb.ridge(rn)
    lowland = nb.mul(lowland, belt)
    river_core = nb.mul(nb.smooth(rline, 0.972, 0.994), lowland)
    river_halo = nb.mul(nb.smooth(rline, 0.92, 0.985), lowland)
    # crack network (voronoi edges) glowing in patches
    ced = nb.voronoi(q2, 9.0, feature="DISTANCE_TO_EDGE")
    crack = nb.one_minus(nb.smooth(ced, 0.0, 0.03))
    cmask = nb.mul(nb.smooth(nb.noise(nb.vadd(q, (7.0, 7.0, 0.0)), 2.6, detail=3), 0.55, 0.68), belt)
    crack = nb.mul(nb.mul(crack, cmask), nb.one_minus(nb.smooth(elev, 0.55, 0.8)))
    # calderas: a few voronoi cells become lava lakes with raised rims
    vd = nb.voronoi(nb.vadd(p0, (2.0, 2.0, 2.0)), 4.0, feature="F1", randomness=0.8)
    vc = nb.voronoi(nb.vadd(p0, (2.0, 2.0, 2.0)), 4.0, feature="F1", randomness=0.8, out="Color")
    pick, _, _ = nb.sep(vc)
    chosen = nb.smooth(pick, 0.60, 0.64)
    pool = nb.mul(nb.one_minus(nb.smooth(vd, 0.07, 0.11)), chosen)
    pool_glow = nb.mul(nb.one_minus(nb.smooth(vd, 0.08, 0.22)), chosen)
    rim = nb.mul(nb.mul(nb.smooth(vd, 0.07, 0.10), nb.one_minus(nb.smooth(vd, 0.10, 0.20))), chosen)

    # amber crystal fields
    cf = nb.smooth(nb.noise(nb.vadd(p0, (-13.0, 6.0, 2.0)), 3.2, detail=4), 0.66, 0.72)
    cf = nb.mul(cf, nb.smooth(elev, 0.25, 0.45))
    cells = nb.voronoi(q2, 70.0, feature="F1", out="Color")
    cr, _, _ = nb.sep(cells)
    sparkle = nb.mul(cf, nb.add(0.35, nb.mul(nb.smooth(cr, 0.4, 0.8), 0.65)))

    lava = nb.clamp01(nb.add(nb.add(river_core, nb.mul(crack, 0.7)), pool))
    height = nb.add(nb.add(nb.mul(elev, 0.45), nb.mul(ridges, 0.5)), nb.mul(fine, 0.08))
    height = nb.add(height, nb.mul(rim, 0.25))
    height = nb.clamp01(nb.sub(height, nb.mul(nb.add(river_halo, pool_glow), 0.15)))

    rock = nb.ramp(elev, [(0.0, "#2E0E0A"), (0.25, "#24100E"), (0.45, "#1A1416"), (0.62, "#2A2325"),
                          (0.78, "#4A4446"), (1.0, "#77706F")])
    tint = nb.noise(nb.vadd(q, (3.0, -3.0, 3.0)), 3.5, detail=4)
    rock = nb.mix(nb.mul(nb.smooth(tint, 0.45, 0.65), 0.75), rock, "#5A1A10")
    rock = nb.mix(nb.mul(ridges, 0.35), rock, "#6E6466")
    # scorched ground around the lava
    rock = nb.mix(nb.mul(nb.add(river_halo, pool_glow), 0.7), rock, "#3A0E06")
    rock = nb.mix(nb.mul(cf, 0.9), rock, "#E0A040")
    albedo = nb.mix(lava, rock, "#FF6A1A")

    heat = nb.clamp01(nb.add(nb.add(nb.mul(river_core, 1.0), pool), nb.mul(crack, 0.5)))
    lava_col = nb.ramp(heat, [(0.0, "#C21E04"), (0.45, "#FF4A12"), (0.8, "#FF8A24"), (1.0, "#FFD27A")])
    halo = nb.add(nb.mul(river_halo, 0.22), nb.mul(pool_glow, 0.35))
    e_lava = nb.cscale(lava_col, nb.clamp01(nb.add(lava, halo)))
    emis = nb.cadd(e_lava, nb.cscale(hexc("#FFC247"), nb.mul(sparkle, 0.9)))

    rough = nb.add(0.88, nb.mul(lava, -0.35))
    rough = nb.add(rough, nb.mul(cf, -0.55))

    # --- ash plumes: sparse, wind-stretched, denser downwind of the volcanic belt
    cw = nb.noise(nb.vadd(p0, (9.0, 9.0, 9.0)), 1.4, detail=3, out="Color")
    cq = nb.warp(p0, cw, 1.1)
    cx, cy, cz = nb.sep(cq)
    st = nb.comb(cx, cy, nb.mul(cz, 2.4))
    c1 = nb.noise(st, 2.6, detail=10, rough=0.62, distortion=0.4)
    wisp = nb.noise(st, 11.0, detail=6, rough=0.6)
    cfield = nb.add(c1, nb.mul(nb.sub(wisp, 0.5), 0.2))
    cloud = nb.mul(nb.smooth(cfield, 0.50, 0.70), 0.8)
    return dict(albedo=albedo, emis=emis, height=height, rough=rough, cloud=cloud)
