# SPDX-License-Identifier: MIT
# Kit, ART-18 story props (route C: code-built in the kit's style, the planet trim sheet, teal / ember Glow slot).
# Briefs: ../brief_art18.md. Unreal centimetres, +X forward, pivot on the ground at the footprint centre.
#   SM_Kit_SurveyTripod   - survey instrument 1: a 1.7 m tripod sensor head (Verdance's buried line)
#   SM_Kit_SurveyProbe    - survey instrument 2: a 1.1 m ground stake with a readout box
#   SM_Kit_SeismicPod     - survey instrument 3: a 0.3 m drum with a whip antenna
#   SM_Kit_Artefact       - the ancient-technology artefact (storyline section 6 clue): the same prop on Verdance,
#                           on Cinder and in the reactor chamber. Ancient stone (Stone slot) with teal glyph channels.
# Collision is walk-clear: one convex hull per prop, no thin parts the capsule could snag on.
import math

from kit_lib import Piece, rotz


def _leg(p, foot_r, hub_r, z_top, t=3.0, w=4.0):
    """A slanted leg in the XZ plane from (foot_r, 0, 0) up to (hub_r, 0, z_top), w wide in Y."""
    prof = [(foot_r - t, 0.0), (foot_r + t, 0.0), (hub_r + t, z_top), (hub_r - t, z_top)]
    p.prism(prof, "xz", -w, w, regs="frame", cap_reg="frame", fitu=True)


def survey_tripod():
    """Instrument 1: three legs to a hub at 1.3 m, a sensor head with a lens and a teal status strip facing +X."""
    p = Piece("SM_Kit_SurveyTripod", None)
    for a in (0.0, 120.0, 240.0):
        with p.at(rotz(a)):
            _leg(p, 55.0, 10.0, 132.0)
            p.box((51, -6, 0), (61, 6, 4), reg="sw_dark")                  # foot pad
    p.box((-13, -13, 128), (13, 13, 146), reg="frame")                     # hub
    p.box((-24, -17, 146), (24, 17, 172), regs={"x": "console", "X": "console", "y": "console", "Y": "console",
                                                  "Z": "frame", "z": "sw_dark"})
    with p.at(_lens_matrix()):
        p.lathe([(9.0, 0.0), (9.0, 6.0)], 12, regs="sw_gun")
        p.lathe([(9.0, 6.0), (0.0, 6.0)], 12, regs="sw_black", mat="Glow")
    p.box((24, -12, 166), (25, 12, 169), mat="Glow", faces="X")             # status strip
    feet = [(55.0 * math.cos(math.radians(a)), 55.0 * math.sin(math.radians(a)), 0.0) for a in (0.0, 120.0, 240.0)]
    # the hull rises to 200 (above the 180 cm auto-mantle) so the pilot is stopped, never climbs onto the sensor head
    p.chull(feet + [(x, y, z) for x in (-24, 24) for y in (-17, 17) for z in (146, 200)])
    return p


def _lens_matrix():
    from mathutils import Matrix
    return Matrix.Translation((24.0, 0.0, 159.0)) @ Matrix.Rotation(math.radians(90.0), 4, "Y")


def survey_probe():
    """Instrument 2: a stake driven into the ground (collar), a readout box at 0.7-1.0 m with a teal strip, a cap."""
    p = Piece("SM_Kit_SurveyProbe", ((-12, -14, 0), (12, 14, 110)))
    p.box((-12, -12, 0), (12, 12, 8), reg="sw_dark")
    p.box((-4, -4, 8), (4, 4, 110), reg="pillar", faces="xXyYZ",
          uvo={f: (("y" if f in "xX" else "x", -4, 4, 0.0, 0.05), ("z", 8, 110)) for f in "xXyY"})
    p.box((-10, -14, 70), (8, 14, 100), regs={"X": "console", "x": "frame", "y": "frame", "Y": "frame", "Z": "frame", "z": "sw_dark"})
    p.box((8, -10, 90), (9, 10, 94), mat="Glow", faces="X")
    p.cbox((-12, -14, 0), (12, 14, 110))
    return p


def seismic_pod():
    """Instrument 3: a squat drum pressed into the soil, a teal ring on the lid, a whip antenna."""
    p = Piece("SM_Kit_SeismicPod", ((-30, -30, 0), (30, 30, 90)))
    p.lathe([(30.0, 0.0), (30.0, 22.0), (24.0, 30.0)], 16, regs=["frame", "sw_gun"])
    p.lathe([(24.0, 30.0), (18.0, 30.0)], 16, regs="sw_dark")
    p.lathe([(18.0, 30.0), (13.0, 30.0)], 16, regs="sw_black", mat="Glow")
    p.lathe([(13.0, 30.0), (0.0, 30.0)], 16, regs="sw_dark")
    p.box((-1.5, -1.5, 30), (1.5, 1.5, 90), reg="sw_gun", faces="xXyYZ")
    p.cbox((-30, -30, 0), (30, 30, 30))
    return p


def artefact():
    """The ancient artefact: a hexagonal stone plinth and three leaning triangular stone blades locked around a
    core, with teal glyph channels up their outer edges and a triangular capstone - old, deliberate, not Null Armada
    and not Triad-made. About 2.6 m tall, 1.8 m across."""
    p = Piece("SM_Kit_Artefact", None)
    p.lathe([(92.0, 0.0), (92.0, 22.0), (84.0, 30.0)], 6, regs=["wall", "frame"], mat="Stone")
    p.lathe([(84.0, 30.0), (0.0, 30.0)], 6, regs="floor", mat="Stone")
    p.lathe([(26.0, 30.0), (26.0, 230.0), (0.0, 230.0)], 6, regs=["pillar", "frame"], mat="Stone")       # core
    for a in (0.0, 120.0, 240.0):
        with p.at(rotz(a)):
            # a blade: triangle section (point outward), leaning in from r 70 at the base to r 30 at the top
            prof = [(22.0, 30.0), (74.0, 30.0), (40.0, 250.0), (22.0, 250.0)]
            p.prism(prof, "xz", -14.0, 14.0, regs=["sw_dark", "wall", "frame", "frame"], cap_reg="wall", fitu=True, mat="Stone")
            # glyph channel down the blade's outer face, 2 cm proud, teal
            ga, gb = (73.0, 34.0), (40.5, 246.0)
            nx, nz = (gb[1] - ga[1]), -(gb[0] - ga[0])
            ln = math.hypot(nx, nz)
            ox, oz = nx / ln * 1.5, nz / ln * 1.5
            p.poly([(ga[0] + ox, -4.0, ga[1] + oz), (ga[0] + ox, 4.0, ga[1] + oz), (gb[0] + ox, 4.0, gb[1] + oz),
                    (gb[0] + ox, -4.0, gb[1] + oz)], "sw_black", mat="Glow", n=(ox, 0.0, oz), uv=[(0.5, 0.5)] * 4)
    # capstone: a flat triangle prism on the core top
    tri = [(46.0 * math.cos(math.radians(a)), 46.0 * math.sin(math.radians(a))) for a in (0.0, 120.0, 240.0)]
    p.prism(tri, "xy", 250.0, 262.0, regs="frame", cap_reg="floor", mat="Stone")
    hexa = [(92.0 * math.cos(math.radians(60 * k)), 92.0 * math.sin(math.radians(60 * k))) for k in range(6)]
    p.chull([(x, y, 0.0) for x, y in hexa] + [(x, y, 262.0) for x, y in tri] + [(x * 0.8, y * 0.8, 120.0) for x, y in hexa])
    return p


BUILDERS = [survey_tripod, survey_probe, seismic_pod, artefact]
