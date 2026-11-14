# SPDX-License-Identifier: MIT
# Kit, one modular set piece per space region (Ryan's map design, docs/eng/SPACE-DESIGN.md on eng/t6). Route C, the kit's
# hard-surface style and material set; seen from a ship at 300 m to 1.5 km. Non-walkable, one to three convex hulls,
# three LODs on import. Unreal cm, pivot at the centre of mass. Nothing carries a name, number or date.
#   Watchline       SM_KS_PicketRing        a broken picket-station ring segment, two dead sensor booms, a blinking amber light
#   Quiet Graveyard SM_KS_Hulk_Fore/_Mid/_Aft   three sections of one cut-up cruiser, dark, one cold-blue running light each
#   Survey Scar     SM_KS_ScarRig           a survey rig clamped across a split asteroid: gantry, drill arm, cable runs
#   Shattered Moons SM_KS_MoonShard_A/_B    two moon shards with exposed strata and a glowing fracture face
# Slots: Trim / Trim2 / Scorch (hull), Dom (the rig), Hazard is a trim region, Nav (amber blink), Cold (cold-blue light),
# Eye (white rig lights), Glow (fracture glow), Rock / Cut / Strata (stone).
import math
import random

from mathutils import Matrix

from kit_lib import Piece, rotz
from kit_pieces_space import _lathe_x, _rock, _ribs, hull_prof, loft
from kit_pieces_story import _tube


# ------------------------------------------------------------------------------------------------ Watchline
def picket_ring():
    """A broken picket-station ring segment (about 130 m across): a 140-degree arc of a 60 m ring, 12 x 8 m in section,
    lit window bands along the intact arc, a dark scorched torn end, two dead sensor booms (one bent) and a blinking
    amber light - the patrol line's station, ours, broken at First Contact. Pivot = the ring centre (the arc is 60 m
    out along +X)."""
    p = Piece("SM_KS_PicketRing", None)
    R_, A0, A1, N = 6000.0, -70.0, 70.0, 28
    T, Hh = 600.0, 400.0                                      # half radial thickness, half height
    step = (A1 - A0) / N
    seg = R_ * math.radians(step) * 0.5 + 8.0
    for k in range(N):
        a_ = A0 + step * (k + 0.5)
        with p.at(rotz(a_)):
            torn = k >= N - 2
            p.box((R_ - T, -seg, -Hh), (R_ + T, seg if k != N - 1 else seg * 0.2, Hh),
                  regs={"X": "wall" if k % 3 else "wallB", "x": "wallB", "Z": "roof", "z": "sw_dark", "y": "frame", "Y": "frame"},
                  mat="Scorch" if torn else ("Trim2" if k % 4 == 1 else "Trim"))
            if k % 2 == 0:
                p.box((R_ + T, -seg * 0.6, -120.0), (R_ + T + 50.0, seg * 0.6, 120.0), reg="frame")
            if not torn and k % 5 != 2:                          # two lit window bands on the outer face (one dark section)
                for zc in (-150.0, 150.0):
                    p.poly([(R_ + T + 2.0, -seg * 0.8, zc - 35.0), (R_ + T + 2.0, seg * 0.8, zc - 35.0),
                            (R_ + T + 2.0, seg * 0.8, zc + 35.0), (R_ + T + 2.0, -seg * 0.8, zc + 35.0)], mat="Light", n=(1, 0, 0))
            if not torn:                                         # the squad's cyan stripe and a steady cyan running glow
                p.poly([(R_ + T + 3.0, -seg, -300.0), (R_ + T + 3.0, seg, -300.0), (R_ + T + 3.0, seg, -230.0), (R_ + T + 3.0, -seg, -230.0)],
                       "frame", mat="Stripe", n=(1, 0, 0), uv=[(0, 0), (1, 0), (1, 1), (0, 1)])
                p.poly([(R_ + T + 4.0, -seg, -330.0), (R_ + T + 4.0, seg, -330.0), (R_ + T + 4.0, seg, -310.0), (R_ + T + 4.0, -seg, -310.0)],
                       mat="CyanRun", n=(1, 0, 0))
            if k == N - 1:
                for (z0, z1, ext) in ((240.0, 400.0, 1000.0), (-400.0, -240.0, 650.0), (-80.0, 80.0, 380.0)):
                    p.box((R_ - T, seg * 0.2, z0), (R_ + T, seg * 0.2 + ext, z1), reg="wall", mat="Scorch")
                # the breach: the open end glows cold, bent ribs stick out of it
                p.poly([(R_ - T + 20.0, seg * 0.2 + 2.0, -Hh + 20.0), (R_ + T - 20.0, seg * 0.2 + 2.0, -Hh + 20.0),
                        (R_ + T - 20.0, seg * 0.2 + 2.0, Hh - 20.0), (R_ - T + 20.0, seg * 0.2 + 2.0, Hh - 20.0)], mat="Breach", n=(0, 1, 0))
                rnd_ = random.Random(3)
                for j in range(6):
                    z = -Hh + 120.0 + j * (2 * Hh - 240.0) / 5.0
                    xr = R_ - T + 80.0 + rnd_.random() * (2 * T - 160.0)
                    mid = (xr + rnd_.uniform(-200, 200), seg * 0.2 + 500.0 + rnd_.uniform(0, 400), z + rnd_.uniform(-150, 150))
                    tip = (mid[0] + rnd_.uniform(-500, 500), mid[1] + rnd_.uniform(300, 800), mid[2] + rnd_.uniform(-400, 400))
                    _tube(p, (xr, seg * 0.2, z), mid, 30.0, reg="frame", mat="Scorch")
                    _tube(p, mid, tip, 22.0, reg="frame", mat="Scorch")
    for (a_, L, bend) in ((-35.0, 4500.0, 0.0), (30.0, 3800.0, 35.0)):
        with p.at(rotz(a_)):
            end = (R_ + T + L * math.cos(math.radians(bend)), L * math.sin(math.radians(bend)) * 0.4, 0.0)
            for (dy, dz) in ((-110.0, -110.0), (110.0, -110.0), (0.0, 130.0)):
                _tube(p, (R_ + T, dy, dz), (end[0], end[1] + dy * 0.4, end[2] + dz * 0.4), 28.0, reg="sw_gun")
            p.box((end[0] - 250.0, end[1] - 250.0, -250.0), (end[0] + 400.0, end[1] + 250.0, 250.0), regs={"X": "sw_black"}, reg="frame")
    with p.at(rotz(5.0)):
        p.box((R_ - 60.0, -60.0, Hh), (R_ + 60.0, 60.0, 1400.0), reg="pillar")
        p.box((R_ - 160.0, -160.0, 1400.0), (R_ + 160.0, 160.0, 1720.0), mat="Nav")
        # r4: the distance cue - an 8 m amber panel and a squad-cyan panel on the outer face
        p.box((R_ + T, -400.0, -350.0), (R_ + T + 40.0, 400.0, 50.0), mat="Nav")
        p.box((R_ + T, -400.0, 80.0), (R_ + T + 40.0, 400.0, 330.0), mat="CyanRun")
    with p.at(rotz(52.0)):                                       # the failing light: a flickering lamp on a bracket
        p.box((R_ + T, -40.0, 300.0), (R_ + T + 300.0, 40.0, 360.0), reg="frame")
        p.box((R_ + T + 220.0, -90.0, 360.0), (R_ + T + 380.0, 90.0, 520.0), mat="Flicker")
    pts = []
    for a_ in (A0, 0.0, A1):
        for rr in (R_ - T, R_ + T + 4500.0 * (abs(a_) < 40)):
            for z in (-Hh, 1720.0 if a_ == 0.0 else Hh):
                pts.append((rr * math.cos(math.radians(a_)), rr * math.sin(math.radians(a_)), z))
    p.chull(pts)
    return p


# ------------------------------------------------------------------------------------------------ Quiet Graveyard
W2, H2 = 2400.0, 1600.0


def _torn_end(p, x, sign, w=W2, h=H2):
    """Staggered plates and frames where a section was cut from the next."""
    for (y0, y1, z0, z1, ext) in ((-w * 0.36, w * 0.36, h * 0.38, h * 0.46, 700.0), (-w / 2, -w * 0.4, -h * 0.1, h * 0.2, 420.0),
                                  (w * 0.4, w / 2, -h * 0.1, h * 0.25, 560.0), (-w * 0.28, w * 0.28, -h * 0.56, -h * 0.47, 250.0)):
        xa, xb = sorted((x, x + sign * ext))
        p.box((xa, y0, z0), (xb, y1, z1), reg="wall", mat="Scorch")
    _ribs(p, min(x, x + sign * 500.0), max(x, x + sign * 500.0), 250.0, hull_prof(w * 0.92, h * 0.92), t=60.0)


def _slits(p, x0, x1, k=1.0):
    """Rows of dead window slits along both flanks (dark, a few still faintly lit)."""
    rnd = random.Random(int(x0))
    for s_ in (-1.0, 1.0):
        y = s_ * (W2 / 2 * k + 4.0)
        for zc in (H2 * 0.18, H2 * 0.02):
            x = x0
            while x < x1:
                w = rnd.uniform(180.0, 320.0)
                lit = rnd.random() < 0.13
                p.poly([(x, y, zc - 22.0), (x + w, y, zc - 22.0), (x + w, y, zc + 22.0), (x, y, zc + 22.0)],
                       "sw_black", mat="Light" if lit else "Trim", n=(0, s_, 0), uv=[(0.5, 0.5)] * 4)
                x += w + rnd.uniform(60.0, 160.0)


def _hulk_cues(p, x0, x1, ztop):
    """r4: a 7 m cold blue-white beacon panel on the dorsal (the 1.5 km cue) and rim-lit edge trim along both upper edges."""
    xm = (x0 + x1) * 0.5
    p.box((xm - 350.0, -60.0, ztop), (xm + 350.0, 60.0, ztop + 350.0), mat="Cold")
    for s_ in (-1.0, 1.0):
        y = s_ * (W2 * 0.36 + 8.0)
        p.poly([(x0, y, H2 * 0.43), (x1, y, H2 * 0.43), (x1, y, H2 * 0.45), (x0, y, H2 * 0.45)], mat="RimLight", n=(0, s_, 0.5))


def hulk_fore():
    """Cruiser bow section (about 75 m): the tapering bow and a dark command blister, cut off behind."""
    p = Piece("SM_KS_Hulk_Fore", None)
    loft(p, 0.0, hull_prof(W2, H2), 4000.0, hull_prof(W2, H2, 0.6), reg="wall", caps=(False, False))
    loft(p, 4000.0, hull_prof(W2, H2, 0.6), 6500.0, hull_prof(W2, H2, 0.08), reg="wallB", mat="Trim2", caps=(False, True))
    p.box((1500.0, -500.0, H2 * 0.45), (3500.0, 500.0, H2 * 0.45 + 450.0), regs={"Z": "roof", "X": "sw_black"}, reg="wall", mat="Trim2")
    _torn_end(p, 0.0, -1.0)
    _slits(p, 300.0, 3800.0, 1.0)
    p.box((6300.0, -80.0, -80.0), (6460.0, 80.0, 80.0), mat="Cold")
    p.box((2400.0, -120.0, H2 * 0.45 + 450.0), (2640.0, 120.0, H2 * 0.45 + 690.0), mat="Nav")
    _hulk_cues(p, 600.0, 3600.0, H2 * 0.45 + 450.0)
    p.chull([(x, y, z) for x in (-700.0, 6500.0) for y, z in hull_prof(W2, H2, 1.0 if x < 0 else 0.1)] +
            [(x, y, H2 * 0.45 + 450.0) for x in (1500.0, 3500.0) for y in (-500.0, 500.0)])
    return p


def hulk_mid():
    """Cruiser mid section (about 50 m): a long hull block cut at both ends, a row of dead hangar doors and a broken
    dorsal spine."""
    p = Piece("SM_KS_Hulk_Mid", None)
    prof = hull_prof(W2, H2)
    p.prism([(y, z) for y, z in prof][::-1], "yz", -2200.0, 2200.0, regs="wall", cap_reg="sw_dark")
    for k in range(4):
        x = -1700.0 + k * 1100.0
        p.box((x, -W2 / 2 - 8.0, -H2 * 0.05), (x + 700.0, -W2 / 2 + 4.0, H2 * 0.3), reg="hatch", mat="Trim2")
    p.box((-2200.0, -150.0, H2 * 0.45), (900.0, 150.0, H2 * 0.45 + 300.0), reg="frame")
    _torn_end(p, -2200.0, -1.0)
    _torn_end(p, 2200.0, 1.0)
    _slits(p, -2000.0, 2000.0, 1.0)
    # the hull breach: a torn hole on the +Y flank glowing cold, scorch round it
    p.poly([(-600.0, W2 / 2 + 12.0, -450.0), (600.0, W2 / 2 + 12.0, -400.0), (450.0, W2 / 2 + 12.0, 400.0), (-500.0, W2 / 2 + 12.0, 380.0)],
           mat="Breach", n=(0, 1, 0))
    p.poly([(-1000.0, W2 / 2 + 6.0, -600.0), (900.0, W2 / 2 + 6.0, -550.0), (750.0, W2 / 2 + 6.0, 600.0), (-850.0, W2 / 2 + 6.0, 550.0)],
           "wall", mat="Scorch", n=(0, 1, 0), uv=[(0, 0), (1, 0), (1, 1), (0, 1)])
    p.box((-80.0, W2 / 2 - 10.0, H2 * 0.2), (80.0, W2 / 2 + 150.0, H2 * 0.2 + 160.0), mat="Cold")
    p.box((-200.0, -120.0, H2 * 0.45 + 300.0), (40.0, 120.0, H2 * 0.45 + 540.0), mat="Nav")
    _hulk_cues(p, -2000.0, 2000.0, H2 * 0.45 + 300.0)
    p.chull([(x, y, z) for x in (-2900.0, 2900.0) for y, z in prof] + [(x, 0.0, H2 * 0.45 + 300.0) for x in (-2200.0, 900.0)])
    return p


def hulk_aft():
    """Cruiser engine section (about 40 m): the aft hull with three dead engine bells, cut off in front."""
    p = Piece("SM_KS_Hulk_Aft", None)
    loft(p, -2000.0, hull_prof(W2, H2, 0.85), 1500.0, hull_prof(W2, H2), reg="wall", caps=(True, False))
    _torn_end(p, 1500.0, 1.0)
    _slits(p, -1600.0, 1200.0, 1.0)
    for (y, z) in ((-650.0, -150.0), (650.0, -150.0), (0.0, 450.0)):
        _lathe_x(p, -2000.0, y, z, [(330.0, 0.0), (420.0, -400.0), (650.0, -1200.0), (600.0, -1200.0), (360.0, -400.0), (260.0, 0.0)],
                 14, regs=["sw_gun", "frame", "sw_dark", "sw_black", "sw_black"], mat="Trim2")
    p.box((-2100.0, -80.0, H2 * 0.4), (-1940.0, 80.0, H2 * 0.4 + 160.0), mat="Cold")
    p.box((0.0, -120.0, H2 * 0.5), (240.0, 120.0, H2 * 0.5 + 240.0), mat="Nav")
    _hulk_cues(p, -1600.0, 1200.0, H2 * 0.45)
    p.chull([(x, y, z) for x in (-3300.0, 2200.0) for y in (-W2 / 2, W2 / 2) for z in (-H2 * 0.6, H2 * 0.5)])
    return p


# ------------------------------------------------------------------------------------------------ Survey Scar
def scar_rig():
    """A Null survey rig clamped across a split asteroid (about 90 m): the rock broken in two with a clean 14 m canyon
    (its two machined faces dark, their edges lit by work-light strips), a heavy truss gantry bridging it with
    hazard-yellow work lights, a drill arm down into the split with a slow white strobe, cable runs down both halves.
    Rock halves: the asteroid material on hard-shaded facets; the rig: the Dominion trim."""
    p = Piece("SM_KS_ScarRig", None)
    R = (3600.0, 3800.0, 3200.0)
    GAP = 700.0
    for (seed, yaw, dx) in ((31, 0.0, -GAP), (37, 180.0, GAP)):
        with p.at(Matrix.Translation((dx, 0.0, 0.0)) @ rotz(yaw)):
            _rock(p, R, seed, XC=0.0, BR=1.0, BD=0.0, NU=56, NV=30)
            # lit rim strips round the machined face (on the face plane, just proud)
            for (y0, y1, z0, z1) in ((-2600.0, 2600.0, 2300.0, 2400.0), (-2600.0, 2600.0, -2400.0, -2300.0),
                                     (-2720.0, -2620.0, -2200.0, 2200.0), (2620.0, 2720.0, -2200.0, 2200.0)):
                p.poly([(5.0, y0, z0), (5.0, y1, z0), (5.0, y1, z1), (5.0, y0, z1)], mat="Glow", n=(1, 0, 0))     # red edge seams
    p.faces = [(v, u, "DomRaw" if m == "Cut" else m) for v, u, m in p.faces]        # the canyon walls: Null black trim
    p.smooth_deg = 22.0
    G = 3000.0
    for y in (-800.0, 800.0):
        for z in (G, G + 650.0):
            _tube(p, (-2700.0, y, z), (2700.0, y, z), 90.0, reg="hazard" if z == G else "frame", mat="Dom")
        for x in range(-2700, 2701, 900):
            _tube(p, (float(x), y, G), (float(x) + 450.0, y, G + 650.0), 45.0, reg="sw_gun", mat="Dom")
            _tube(p, (float(x), -800.0, G + 650.0), (float(x), 800.0, G + 650.0), 45.0, reg="sw_gun", mat="Dom")
        for x in (-2200.0, -900.0, 900.0, 2200.0):                  # hazard-yellow work lights along the gantry
            p.box((x - 210.0, y - 210.0, G - 420.0), (x + 210.0, y + 210.0, G - 90.0), mat="Work")
    for (x, y) in ((-2700.0, -800.0), (2700.0, -800.0), (-2700.0, 800.0), (2700.0, 800.0)):
        _tube(p, (x, y, G), (x * 1.05, y * 1.4, G - 1500.0), 90.0, reg="frame", mat="Dom")
        p.box((x * 1.05 - 220.0, y * 1.4 - 220.0, G - 1750.0), (x * 1.05 + 220.0, y * 1.4 + 220.0, G - 1450.0), reg="hazard", mat="Dom")
    p.box((-600.0, -600.0, G + 650.0), (600.0, 600.0, G + 1550.0), regs={"Z": "roof"}, reg="wallB", mat="Dom")
    p.box((-620.0, -620.0, G + 1150.0), (620.0, 620.0, G + 1230.0), reg="hazard", mat="Dom")
    for (nx, ny) in ((1, 0), (-1, 0), (0, 1), (0, -1)):            # the white eye ring on the drill head
        for t in (-300.0, 0.0, 300.0):
            cx_, cy_ = nx * 602.0 + (0 if nx else t), ny * 602.0 + (0 if ny else t)
            ex, ey = (0.0, 120.0) if nx else (120.0, 0.0)           # r4: 3x the eye area
            p.poly([(cx_ - ex, cy_ - ey, G + 1260.0), (cx_ + ex, cy_ + ey, G + 1260.0), (cx_ + ex, cy_ + ey, G + 1480.0), (cx_ - ex, cy_ - ey, G + 1480.0)],
                   mat="Eye", n=(nx, ny, 0))
    # r4: the distance cue - a Null red lamp cluster (8 m) on a mast over the drill head
    p.box((-80.0, -80.0, G + 1550.0), (80.0, 80.0, G + 2200.0), reg="frame", mat="Dom")
    p.box((-400.0, -400.0, G + 2200.0), (400.0, 400.0, G + 2600.0), mat="Beacon")
    _tube(p, (0.0, 0.0, G), (0.0, 0.0, -1800.0), 260.0, reg="frame", mat="Dom")
    p.box((-400.0, -400.0, -2200.0), (400.0, 400.0, -1800.0), reg="hazard", mat="Dom")
    p.box((-140.0, -140.0, 200.0), (140.0, 140.0, 480.0), mat="Strobe")          # the slow white strobe on the drill arm
    rnd = random.Random(6)
    for s_ in (-1.0, 1.0):
        prev = (s_ * 1900.0, -350.0, G)
        for k in range(5):
            nxt = (s_ * (2100.0 + 400.0 * k), -350.0 + rnd.uniform(-200, 200), G - 700.0 * (k + 1))
            _tube(p, prev, nxt, 45.0, reg="sw_black", mat="Dom")
            prev = nxt
    p.chull([(x, y, z) for x in (-4300.0, 4300.0) for y in (-3800.0, 3800.0) for z in (-3200.0, 4600.0)])
    return p


# ------------------------------------------------------------------------------------------------ Shattered Moons
def _shard(name, R, seed, seams, band=1100.0):
    """A moon shard: a hard-faceted rock broken by an uneven stepped fracture (+X): 5 offset plates 4-10 m apart round
    an impact point, so the break has shadow; subtle strata bands (Strata / Strata2, about 25 % apart); a pale rim band
    along the break's edge (Rim); and a crack network radiating from the impact point - wide and hot orange-white at the
    centre (CrackHot), orange further out (Seam), thinning to dark-red hairlines at the edges (CrackDim), each crack laid
    over a wider dim underglow (CrackDim) for a soft falloff. Rock UVs tile 16 x 8."""
    p = Piece(name, None)
    XC0 = R[0] * 0.35
    rnd = random.Random(seed + 1)
    iy, iz = R[1] * 0.08, -R[2] * 0.06                      # the impact point on the face
    steps = [0.0, -450.0, -900.0, -300.0, -700.0, -1000.0]

    def plate(y, z):
        a_ = (math.degrees(math.atan2(z - iz, y - iy)) + 360.0) % 360.0
        r_ = math.hypot(y - iy, z - iz)
        if r_ < R[1] * 0.12:
            return 0
        return 1 + int(a_ // 72.0)

    def xc(y, z):
        return XC0 + steps[plate(y, z)]
    _rock(p, R, seed, XC=xc, BR=1.0, BD=0.0, NU=96, NV=60, rim=True)
    out = []
    for v, u, m in p.faces:
        if m == "Cut":
            zc = sum(q.z for q in v) / len(v)
            m = "Strata" if int((zc + 100000.0) // band) % 2 == 0 else "Strata2"
            u = [(q.y / 3000.0, q.z / 1200.0) for q in v]
        elif m in ("Rock", "Rim"):
            u = [(a_ * 4.0, b_ * 4.0) for a_, b_ in u]
        out.append((v, u, m))
    p.faces = out
    p.smooth_deg = 18.0

    def crack(y0, z0, ang, length, w0, w1, depth):
        """One crack from (y0, z0): short segments that follow the stepped face, tapering width, hot to dim."""
        y, z, d = y0, z0, 0.0
        n = max(4, int(length / 350.0))
        for i in range(n):
            t0, t1 = i / n, (i + 1) / n
            ang += rnd.uniform(-0.35, 0.35)
            y1, z1 = y + math.cos(ang) * length / n, z + math.sin(ang) * length / n
            for (wmul, mat, lift) in ((2.6, "CrackDim", 15.0), (1.0, "CrackHot" if t0 < 0.25 and depth == 0 else ("Seam" if t0 < 0.6 else "CrackDim"), 30.0)):
                wa, wb = (w0 + (w1 - w0) * t0) * wmul / 2.0, (w0 + (w1 - w0) * t1) * wmul / 2.0
                nx_, nz_ = -(z1 - z), (y1 - y)
                ln = math.hypot(nx_, nz_) or 1.0
                nx_, nz_ = nx_ / ln, nz_ / ln
                pts = [(y - nx_ * wa, z - nz_ * wa), (y1 - nx_ * wb, z1 - nz_ * wb), (y1 + nx_ * wb, z1 + nz_ * wb), (y + nx_ * wa, z + nz_ * wa)]
                p.poly([(xc(py, pz) + lift, py, pz) for py, pz in pts], mat=mat, n=(1, 0, 0))
            if depth < 2 and rnd.random() < 0.18:
                crack(y1, z1, ang + rnd.choice((-1, 1)) * rnd.uniform(0.5, 0.9), length * (1.0 - t1) * 0.7, (w0 + (w1 - w0) * t1) * 0.6, w1 * 0.5, depth + 1)
            y, z = y1, z1
    for k in range(seams):
        ang = 2 * math.pi * k / seams + rnd.uniform(-0.3, 0.3)
        crack(iy, iz, ang, min(R[1], R[2]) * rnd.uniform(0.7, 0.95), 520.0, 40.0, 0)
    # the hot heart of the impact
    hs = 1400.0                                               # r4: twice the hot heart
    p.poly([(xc(iy, iz) + 35.0, iy - hs, iz - hs), (xc(iy, iz) + 35.0, iy + hs, iz - hs), (xc(iy, iz) + 35.0, iy + hs, iz + hs),
            (xc(iy, iz) + 35.0, iy - hs, iz + hs)], mat="CrackHot", n=(1, 0, 0))
    return p, XC0


def moon_shard_a():
    """Moon shard A (about 300 m): a huge broken slab of moon, its fracture face flat and banded with exposed strata,
    ember seams glowing in the cracks - whatever stood near the Dominion was broken and swept aside."""
    p, XC = _shard("SM_KS_MoonShard_A", (18000.0, 15000.0, 9000.0), 41, 7)
    p.chull([(-18000.0, y, z) for y in (-15000.0, 15000.0) for z in (-9000.0, 9000.0)] +
            [(XC, y, z) for y in (-15000.0, 15000.0) for z in (-9000.0, 9000.0)])
    return p


def moon_shard_b():
    """Moon shard B (about 180 m): a smaller, chunkier shard from the same moon, the same strata and glowing fracture."""
    p, XC = _shard("SM_KS_MoonShard_B", (10000.0, 8000.0, 7000.0), 47, 6)
    p.chull([(-10000.0, y, z) for y in (-8000.0, 8000.0) for z in (-7000.0, 7000.0)] +
            [(XC, y, z) for y in (-8000.0, 8000.0) for z in (-7000.0, 7000.0)])
    return p


BUILDERS = [picket_ring, hulk_fore, hulk_mid, hulk_aft, scar_rig, moon_shard_a, moon_shard_b]
