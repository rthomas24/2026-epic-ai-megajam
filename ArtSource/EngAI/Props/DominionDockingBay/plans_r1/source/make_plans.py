"""Measured orthographic plan set for the Dominion docking bay (matplotlib, from dockbay_layout.json and hull slices).
python3 make_plans.py <plans_dir> <hull_slices_r2.json> <hull_measure_r1.json> <hull_measure_r2.json>
"""
import json, sys, os, math
import matplotlib; matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle, Polygon, Circle, FancyArrowPatch, Wedge
from matplotlib.lines import Line2D

PL = sys.argv[1]; SL = json.load(open(sys.argv[2])); M1 = json.load(open(sys.argv[3])); M2 = json.load(open(sys.argv[4]))
L = json.load(open(os.path.join(PL, "dockbay_layout.json")))
OUT = os.path.join(PL, "plans"); os.makedirs(OUT, exist_ok=True)
plt.rcParams.update({"font.family": "DejaVu Sans", "font.size": 9})
BG, INK, GRID = "#f4f2ec", "#1b1d22", "#c9c6bd"
COL = {"SM_DI_Room_Floor": "#d9d6cc", "SM_DI_Room_Wall": "#2c2f36", "SM_DI_Hangar_Pad": "#e0b13a", "SM_DI_Walkway_Ring": "#6d7f99",
       "SM_DI_Console": "#2a9fb8", "SM_DI_Cover_Low": "#8a5a2b", "SM_DI_Doorway": "#b8321f", "SM_DI_Door": "#b8321f",
       "SM_DI_Corridor_Straight": "#9a9fa8", "SM_DI_Room_Ceiling": "#d9d6cc"}
# footprint boxes from the kit README (min, max from the pivot, cm)
BOX = {"SM_DI_Room_Floor": ((-200, -200, -20), (200, 200, 0)), "SM_DI_Room_Ceiling": ((-200, -200, 600), (200, 200, 620)),
       "SM_DI_Room_Wall": ((-30, -200, 0), (30, 200, 600)), "SM_DI_Hangar_Pad": ((-600, -600, -40), (600, 600, 0)),
       "SM_DI_Walkway_Ring": ((-200, -150, -20), (200, 150, 110)), "SM_DI_Console": ((-30, -60, 0), (30, 60, 130)),
       "SM_DI_Cover_Low": ((-30, -100, 0), (30, 100, 110)), "SM_DI_Doorway": ((-30, -200, 0), (30, 200, 600)),
       "SM_DI_Door": ((-15, -150, 0), (15, 150, 400)), "SM_DI_Corridor_Straight": ((-200, -200, -20), (200, 200, 620))}

def rect_xy(p):
    (x0, y0, _), (x1, y1, _) = BOX[p["mesh"]]
    a = math.radians(p["yaw"]); c, s = math.cos(a), math.sin(a)
    pts = [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]
    return [(p["x"] + c * u - s * v, p["y"] + s * u + c * v) for u, v in pts]

def dim(ax, a, b, off, text, horiz=True, col=INK, fs=8):
    (x0, y0), (x1, y1) = a, b
    if horiz:
        y = y0 + off
        ax.annotate("", (x0, y), (x1, y), arrowprops=dict(arrowstyle="<->", color=col, lw=0.8, shrinkA=0, shrinkB=0))
        ax.plot([x0, x0], [y0, y], color=col, lw=0.4); ax.plot([x1, x1], [y1, y], color=col, lw=0.4)
        ax.text((x0 + x1) / 2, y, text, ha="center", va="bottom", fontsize=fs, color=col, bbox=dict(fc=BG, ec="none", pad=0.5))
    else:
        x = x0 + off
        ax.annotate("", (x, y0), (x, y1), arrowprops=dict(arrowstyle="<->", color=col, lw=0.8, shrinkA=0, shrinkB=0))
        ax.plot([x0, x], [y0, y0], color=col, lw=0.4); ax.plot([x1, x], [y1, y1], color=col, lw=0.4)
        ax.text(x, (y0 + y1) / 2, text, ha="center", va="center", rotation=90, fontsize=fs, color=col, bbox=dict(fc=BG, ec="none", pad=0.5))

def figure_side(ax, x, z=0, col="#d08a00", label=True, facing=1):
    """180 cm figure in elevation."""
    ax.add_patch(Rectangle((x - 18, z), 36, 86, color=col)); ax.add_patch(Rectangle((x - 22, z + 84), 44, 64, color=col))
    ax.add_patch(Circle((x, z + 169), 11, color=col))
    if label: ax.text(x + 30, z + 175, "180 cm", fontsize=7, color=col, va="center")

def frame(ax, title, sub):
    ax.set_facecolor(BG); ax.set_aspect("equal")
    ax.set_title(title, loc="left", fontsize=13, fontweight="bold", color=INK, pad=30)
    ax.text(0, -0.075, sub, transform=ax.transAxes, fontsize=8, color="#444", va="top", wrap=True)

def grid(ax, x0, x1, y0, y1, step=400, labels=True, axis_names=("X", "Y")):
    for x in range(int(x0), int(x1) + 1, step):
        ax.axvline(x, color=GRID, lw=0.35, zorder=0)
    for y in range(int(y0), int(y1) + 1, step):
        ax.axhline(y, color=GRID, lw=0.35, zorder=0)
    ax.set_xlim(x0, x1); ax.set_ylim(y0, y1)
    ax.tick_params(labelsize=7)

H = L["hangar"]; P = L["pocket"]; MO = L["mouth"]; DO = L["doors"]; G = L["glide"]; PADS = L["pads"]
hull_x = lambda x: 22800 - x

# ==================================================================================================== 1. TOP PLAN
fig, ax = plt.subplots(figsize=(22, 12.5), dpi=110); fig.patch.set_facecolor(BG)
frame(ax, "DOMINION DOCKING BAY r1 · TOP PLAN (floor z = 0, ceilings removed)",
      "Unreal cm, T5 interior course-local frame (TDDominionInteriorCourse.inl): +X deeper into the ship. 400 cm kit grid "
      "(ArtSource/EngProps/DominionInterior). Hull X on the top axis = 22800 - x. Every rectangle is one kit piece placed by pivot at scale 1.")
grid(ax, -8400, 2400, -3400, 3400)
order = ["SM_DI_Room_Floor", "SM_DI_Corridor_Straight", "SM_DI_Hangar_Pad", "SM_DI_Walkway_Ring", "SM_DI_Room_Wall",
         "SM_DI_Doorway", "SM_DI_Door", "SM_DI_Cover_Low", "SM_DI_Console"]
for mname in order:
    for p in L["placements"]:
        if p["mesh"] != mname: continue
        if mname == "SM_DI_Room_Wall" and p["z"] > 0: continue
        alpha = 0.55 if mname in ("SM_DI_Room_Floor",) else (0.35 if mname == "SM_DI_Walkway_Ring" else 0.9)
        ax.add_patch(Polygon(rect_xy(p), closed=True, fc=COL[mname], ec="#555" if mname == "SM_DI_Room_Floor" else COL[mname],
                             lw=0.3, alpha=alpha, zorder=2 if mname != "SM_DI_Room_Floor" else 1))
# glide lane, force field, mouth
ax.add_patch(Rectangle((P["x"][0], -MO["width"] / 2), 6000, MO["width"], fc="none", ec="#1f6fd1", lw=1.2, ls="--", zorder=4))
ax.text(-6900, -2350, "glide pocket = hull pocket X 24000..30000, |Y| < 2500 (5000 wide)", color="#1f6fd1", fontsize=8, zorder=5)
ax.add_patch(Rectangle((-7200, -450), 6000, 900, fc="#1f6fd1", alpha=0.08, zorder=3))
ax.annotate("", (G["end_cpp"][0], 0), (G["start"][0], 0), arrowprops=dict(arrowstyle="-|>", color="#1f6fd1", lw=2), zorder=6)
ax.text(-5400, 120, "glide: GlideSeconds 2 s, ship locked + invulnerable", color="#1f6fd1", fontsize=8, zorder=6)
ax.plot([G["end_cpp"][0]] * 2, [-500, 500], color="#c0392b", lw=2, zorder=6)
ax.text(G["end_cpp"][0] + 40, 520, "glide end, C++ GlideDepth 6000\n(= pocket back wall; ship nose hits it)", color="#c0392b", fontsize=7, zorder=6)
ax.plot([G["end_recommended"][0]] * 2, [-500, 500], color="#1a8a3a", lw=2, zorder=6)
ax.text(G["end_recommended"][0] - 40, -760, "recommended GlideDepth 5000\n(nose 7.4 m clear of the bulkhead)", color="#1a8a3a", fontsize=7, ha="right", zorder=6)
ax.plot([L["force_field"]["x"]] * 2, [-2600, 2600], color="#3aa3ff", lw=3, alpha=0.6, zorder=6)
ax.text(-7150, 2700, "force field: BlockingVolume (pawn only) x = -7180", color="#1f6fd1", fontsize=8)
# mouth & doors (open and closed)
ax.plot([-7200, -7200], [-2500, 2500], color="#b8321f", lw=3, zorder=7)
for cy in DO["closed_centres_y"]:
    ax.add_patch(Rectangle((-7200 + 20, cy - 1250), 300, 2500, fc="none", ec="#b8321f", lw=0.8, ls=":", zorder=7))
for cy in DO["open_centres_y"]:
    ax.add_patch(Rectangle((-7200 + 20, cy - 1250), 300, 2500, fc="#b8321f", alpha=0.25, ec="#b8321f", lw=0.8, zorder=7))
ax.text(-6800, 3300, "doors OPEN: slabs at |y| 2500..5000\n(ATDDockingBay: ±DoorHalfWidth·(0.5+Open01))", color="#b8321f", fontsize=7, va="top")
ax.text(-8350, -2900, "MOUTH 5000 x 3500\n= SOCKET_Bay (30000, 0, -2000)\nfacing hull +X (here -X)", color="#b8321f", fontsize=8)
# BAY THRESHOLD marking (Lead N-061)
ax.add_patch(Rectangle((-7200, -2500), 400, 5000, fc="#d9661f", alpha=0.55, ec="k", lw=0.8, hatch="//", zorder=7.5))
ax.plot([-6800, -6800], [-2500, 2500], color="#ff3b0a", lw=2.5, zorder=7.6)
ax.text(-6760, -1900, "BAY THRESHOLD (N-061): 400 cm hazard-chevron floor band\nx -7200..-6800 (= hull X 30000..29600), full 5000 width,\nT_DominionBay_Trim HAZARD band; emissive edge strip at x -6800;\nemissive jamb + header strips frame the 5000 x 3500 mouth",
        fontsize=7, color="#8a3a00", zorder=9, bbox=dict(fc=BG, ec="#d9661f", lw=0.8, pad=2))
# pads
for k in ("A", "B", "C"):
    x, y = PADS[k]
    ax.text(x, y + 640, "Pad_%s (%d, %d)" % (k, x, y) + (" · T5, the ship sits here" if k == "A" else " · dressing"), ha="center", fontsize=7.5, color="#6b4d00", zorder=8)
# ship footprint and pilot start, console clearances
sl, sw, sh = L["ship_footprint"]["size"]
ax.add_patch(Rectangle((-200 - sl / 2, -sw / 2), sl, sw, fc="none", ec="k", lw=1, ls="--", zorder=8))
ax.annotate("", (-200 + sl / 2 + 160, 0), (-200 + sl / 2, 0), arrowprops=dict(arrowstyle="-|>", color="k"), zorder=8)
ax.text(-200, -sw / 2 - 120, "docked ship footprint 524 x 488 (nose +X)", ha="center", fontsize=7, zorder=8)
sx, sy, _ = L["start"]
ax.add_patch(Circle((sx, sy), 30, color="#d08a00", zorder=9)); ax.text(sx - 40, sy + 60, "pilot start\n(-600, 0)", fontsize=7, ha="right", color="#8a5a00", zorder=9)
for (cx, cy, user_dx) in ((-900, 900, 1), (-1500, -1800, -1), (-1500, 1800, -1)):
    x0 = cx + 30 if user_dx > 0 else cx - 30 - 150
    ax.add_patch(Rectangle((x0, cy - 60), 150, 120, fc="#2a9fb8", alpha=0.18, ec="#2a9fb8", lw=0.6, ls="--", zorder=8))
ax.text(-900, 1060, "console, 150 cm clear", fontsize=6.5, color="#1d6f80", ha="center")
ax.text(-1650, -2000, "dock-control console", fontsize=6.5, color="#1d6f80", ha="center")
ax.text(1200, 260, "HangarDoor\n300 x 400 clear", fontsize=7, color="#b8321f")
ax.text(1700, -380, "→ deflect corridor (T5, to x 5200)", fontsize=7)
ax.text(-4300, -2160, "gantry (SM_DI_Walkway_Ring at z 1200, dressing)", fontsize=7, color="#3d4a5c")
ax.text(-4300, 2050, "gantry (rail toward the pocket)", fontsize=7, color="#3d4a5c")
# figure in plan
ax.add_patch(Circle((-6200, 600), 25, color="#d08a00", zorder=9)); ax.text(-6160, 640, "figure", fontsize=6.5, color="#8a5a00")
# dimensions
dim(ax, (-7200, 2600), (-1200, 2600), 450, "pocket 6000 (15 cells) = GlideDepth in C++")
dim(ax, (-1200, 1400), (1200, 1400), 1050, "hangar 2400 (6 cells, T5)")
dim(ax, (-7800, -2600), (-7800, 2600), 0, "pocket 5200 (13 cells; clear 5140)", horiz=False)
dim(ax, (1900, -1400), (1900, 1400), 0, "hangar 2800 (7)", horiz=False)
dim(ax, (-8150, -2500), (-8150, 2500), 0, "mouth 5000", horiz=False, col="#b8321f")
dim(ax, (-3200, -2200), (-2000, -2200), -380, "pad 1200")
# hull axis on top
sec = ax.secondary_xaxis("top", functions=(lambda x: 22800 - x, lambda h: 22800 - h)); sec.set_xlabel("hull X (Dominion local, cm)", fontsize=8)
sec.tick_params(labelsize=7)
ax.set_xlabel("interior X (cm)"); ax.set_ylabel("interior Y (cm)   [hull Y = -y]")
handles = [Line2D([], [], color=COL[k], lw=6, label=k) for k in order]
ax.legend(handles=handles, loc="lower right", fontsize=7, framealpha=0.9, ncol=3)
# scale bar
ax.plot([-8200, -6200], [-3250, -3250], color="k", lw=3); ax.text(-7200, -3200, "20 m", ha="center", fontsize=8)
fig.tight_layout(); fig.savefig(os.path.join(OUT, "DB_Plan_Top.png"), facecolor=BG); plt.close(fig)

# ==================================================================================================== 2. SIDE SECTION
fig, ax = plt.subplots(figsize=(22, 9.5), dpi=110); fig.patch.set_facecolor(BG)
frame(ax, "SIDE SECTION A-A (y = 0, looking +Y)", "Interior z = 0 is the hull mouth sill (hull Z -3750). Kit storeys 600 cm; floor slabs -20..0, ceiling slabs at storey top. Right axis: hull Z = z - 3750.")
grid(ax, -8400, 2400, -400, 4400)
def slab(x0, x1, z0, z1, c="#2c2f36", a=0.9): ax.add_patch(Rectangle((x0, z0), x1 - x0, z1 - z0, fc=c, alpha=a, ec="none"))
slab(-7200, 1200, -20, 0, "#8d8a82")                                  # floors
slab(1200, 2000, -20, 0, "#9a9fa8")
slab(-7200, -1200, 3600, 3620); slab(-1200, 1200, 1200, 1220); slab(1200, 2000, 600, 620, "#9a9fa8")
slab(-1230, -1170, 1200, 3600)                                       # back wall above the hangar opening
for s in range(2): slab(1170, 1230, s * 600 + (400 if s == 0 else 0), (s + 1) * 600)   # hangar X1 wall above the door
ax.add_patch(Rectangle((1170, 0), 60, 400, fc="none", ec="#b8321f", lw=1.2))
ax.text(1260, 200, "HangarDoor 300 x 400", fontsize=7, color="#b8321f")
for z in range(0, 3600, 600): ax.plot([-7200, -1200], [z, z], color=GRID, lw=0.3)
# mouth, door, force field
ax.plot([-7200, -7200], [0, 3500], color="#b8321f", lw=4); ax.plot([-7200, -7200], [3500, 3600], color="#2c2f36", lw=4)
ax.add_patch(Rectangle((-7200 + 20, 0), 300, 3500, fc="none", ec="#b8321f", lw=0.8, ls=":"))
ax.text(-7150, 3700, "mouth 3500 high (closed doors dotted, 300 deep)", fontsize=7.5, color="#b8321f")
ax.plot([-7180] * 2, [0, 3600], color="#3aa3ff", lw=2, alpha=0.6)
ax.add_patch(Rectangle((-7200, 0), 400, 25, fc="#d9661f", ec="k", lw=0.6, hatch="//", zorder=6))
ax.plot([-7190] * 2, [0, 3500], color="#ff3b0a", lw=2); ax.plot([-6800] * 2, [0, 40], color="#ff3b0a", lw=3)
ax.text(-6760, 90, "threshold band 400 (hazard) + glow strip; glow jamb strips up the mouth edge", fontsize=7, color="#8a3a00")
# glide path
ax.annotate("", (-1200, 1750), (-8300, 1750), arrowprops=dict(arrowstyle="-|>", color="#1f6fd1", lw=2))
ax.plot([-7200], [1750], "o", color="#1f6fd1"); ax.text(-8300, 1830, "approach axis z 1750\n= SOCKET_Bay (hull Z -2000)", color="#1f6fd1", fontsize=7.5)
ax.plot([-1200] * 2, [1550, 1950], color="#c0392b", lw=2); ax.plot([-2200] * 2, [1550, 1950], color="#1a8a3a", lw=2)
ax.text(-1180, 2000, "C++ glide end (6000)", color="#c0392b", fontsize=7); ax.text(-2180, 2250, "recommended (5000)", color="#1a8a3a", fontsize=7, ha="right")
# r1 / r2 C++ placement as built
for (lab, z, c) in (("r1", M1["cpp_bay_actor"][2] + 3750, "#7d3c98"), ("r2", M2["cpp_bay_actor"][2] + 3750, "#a569bd")):
    ax.plot([-7200 - 3400], [z], "x", color=c)
    ax.text(-8350, z - 160, "C++ today (%s): actor at hull (33400, 0, %.0f) = 34 m out, z %.0f here" % (lab, z - 3750, z), color=c, fontsize=7)
# ship, pads, figures
for (x, w) in ((-200, 1200), (-2600, 1200)):
    slab(x - w / 2, x + w / 2, -40 + 0.5, 0.5, "#e0b13a")
ax.add_patch(Rectangle((-200 - 262, 0.5), 524, 230, fc="none", ec="k", lw=1, ls="--")); ax.text(-200, 260, "ship 524 x 230", ha="center", fontsize=7)
figure_side(ax, -600); figure_side(ax, -6200); figure_side(ax, -2900, col="#d08a00", label=False)
# gantry
for x in range(-7200, -1200, 400): slab(x, x + 400, 1180, 1200, "#6d7f99", 0.6)
ax.text(-4300, 1260, "gantry at z 1200 (behind the section plane, |y| 2420)", fontsize=7, color="#3d4a5c")
dim(ax, (-7200, 3620), (-1200, 3620), 280, "glide pocket 6000")
dim(ax, (-1200, 1220), (1200, 1220), 200, "hangar 2400")
dim(ax, (-7700, 0), (-7700, 3600), 0, "pocket clear 3600 (6 storeys)", horiz=False)
dim(ax, (-7450, 0), (-7450, 3500), 0, "mouth 3500", horiz=False, col="#b8321f")
dim(ax, (-1000, 0), (-1000, 1200), 0, "hangar clear 1200 (2 storeys)", horiz=False)
dim(ax, (1500, 0), (1500, 600), 0, "corridor 600", horiz=False)
sec = ax.secondary_yaxis("right", functions=(lambda z: z - 3750, lambda h: h + 3750)); sec.set_ylabel("hull Z (cm)", fontsize=8)
sec2 = ax.secondary_xaxis("top", functions=(lambda x: 22800 - x, lambda h: 22800 - h)); sec2.set_xlabel("hull X (cm)", fontsize=8)
ax.set_xlabel("interior X (cm)"); ax.set_ylabel("interior z (cm)")
ax.plot([-8200, -6200], [-300, -300], color="k", lw=3); ax.text(-7200, -260, "20 m", ha="center", fontsize=8)
fig.tight_layout(); fig.savefig(os.path.join(OUT, "DB_Section_Side.png"), facecolor=BG); plt.close(fig)

# ==================================================================================================== 3. FRONT SECTION
fig, ax = plt.subplots(figsize=(16, 9.5), dpi=110); fig.patch.set_facecolor(BG)
frame(ax, "FRONT SECTION B-B (x = -6800, looking +X into the bay)", "Hull mouth 5000 x 3500 (red) inside the kit pocket 5200 x 3600; hangar opening 2800 x 1200 on the back wall; doors shown open (solid) and closed (dotted).")
grid(ax, -5600, 5600, -400, 4400)
slab(-2600, 2600, -20, 0, "#8d8a82"); slab(-2600, 2600, 3600, 3620)
slab(-2630, -2570, 0, 3600); slab(2570, 2630, 0, 3600)
ax.add_patch(Rectangle((-1400, 0), 2800, 1200, fc="#f0e6c8", ec="k", lw=1)); ax.text(0, 1250, "hangar opening 2800 x 1200 (T5 hangar, X0 wall removed)", ha="center", fontsize=8)
slab(-2600, -1400, 0, 3600, "#3b3f47", 0.25); slab(1400, 2600, 0, 3600, "#3b3f47", 0.25); slab(-1400, 1400, 1200, 3600, "#3b3f47", 0.25)
ax.add_patch(Rectangle((-2500, 0), 5000, 3500, fc="none", ec="#b8321f", lw=2.2))
for cy in DO["closed_centres_y"]: ax.add_patch(Rectangle((cy - 1250, 0), 2500, 3500, fc="none", ec="#b8321f", lw=0.8, ls=":"))
for cy in DO["open_centres_y"]: ax.add_patch(Rectangle((cy - 1250, 0), 2500, 3500, fc="#b8321f", alpha=0.18, ec="#b8321f", lw=0.8))
ax.text(-5550, 3550, "door slab (open)\n|y| 2500..5000,\nhidden in the hull", fontsize=7.5, color="#b8321f", va="top")
for s in (-1, 1):
    slab(s * 2570 - (300 if s > 0 else 0), s * 2570 + (0 if s > 0 else 300), 1180, 1200, "#6d7f99")
    ax.plot([s * 2270] * 2, [1200, 1310], color="#6d7f99", lw=2)
    slab(PADS["B"][1] - 600 if s < 0 else PADS["C"][1] - 600, (PADS["B"][1] if s < 0 else PADS["C"][1]) + 600, -40, 0.5, "#e0b13a")
ax.text(-1600, -300, "Pad_B", ha="center", fontsize=7.5); ax.text(1600, -300, "Pad_C", ha="center", fontsize=7.5)
ax.add_patch(Rectangle((-244, 0.5), 488, 230, fc="none", ec="k", ls="--")); ax.text(0, 260, "ship 488 wide", ha="center", fontsize=7)
ax.add_patch(Circle((0, 1750), 60, fc="none", ec="#1f6fd1", lw=1.5)); ax.text(80, 1800, "approach axis (SOCKET_Bay)", color="#1f6fd1", fontsize=7.5)
# threshold frame seen from inside: glow jambs at |y| 2460..2500, header 3460..3500, hazard band on the sill
for sg in (-1, 1): ax.add_patch(Rectangle((min(sg * 2460, sg * 2500), 0), 40, 3500, fc="#ff3b0a", ec="none", zorder=7))
ax.add_patch(Rectangle((-2500, 3460), 5000, 40, fc="#ff3b0a", ec="none", zorder=7))
ax.add_patch(Rectangle((-2500, 0), 5000, 30, fc="#d9661f", ec="k", lw=0.5, hatch="//", zorder=7))
ax.text(-2400, 3300, "BAY THRESHOLD frame (N-061): glow jambs + header, hazard sill band", fontsize=7, color="#8a3a00")
figure_side(ax, 900); figure_side(ax, -2000, label=False)
dim(ax, (-2500, 3620), (2500, 3620), 330, "mouth 5000 = 2 x DoorHalfWidth 2500", col="#b8321f")
dim(ax, (-2600, 3620), (2600, 3620), 600, "pocket 5200 (clear 5140)")
dim(ax, (2900, 0), (2900, 3500), 0, "3500 = 2 x 0.7 x DoorHalfWidth", horiz=False, col="#b8321f")
dim(ax, (3300, 0), (3300, 3600), 0, "3600", horiz=False)
dim(ax, (-2570, 1300), (-2270, 1300), 260, "gantry 300", fs=7)
dim(ax, (-2500, -200), (-5000, -200), -0, "door travel 2500", col="#b8321f", fs=7)
sec = ax.secondary_yaxis("right", functions=(lambda z: z - 3750, lambda h: h + 3750)); sec.set_ylabel("hull Z (cm)", fontsize=8)
ax.set_xlabel("interior Y (cm)   [hull Y = -y]"); ax.set_ylabel("interior z (cm)")
fig.tight_layout(); fig.savefig(os.path.join(OUT, "DB_Section_Front.png"), facecolor=BG); plt.close(fig)

# ==================================================================================================== 4. HULL MOUTH / C++ METRICS
fig = plt.figure(figsize=(20, 17), dpi=110); fig.patch.set_facecolor(BG)
gs = fig.add_gridspec(2, 2, height_ratios=[1, 1.25], hspace=0.35)
ax1 = fig.add_subplot(gs[0, 0]); ax2 = fig.add_subplot(gs[0, 1]); ax3 = fig.add_subplot(gs[1, :])
def segs(ax, key, i, j, col="#444", lw=0.5):
    for a, b in SL[key]: ax.plot([a[i], b[i]], [a[j], b[j]], color=col, lw=lw)
frame(ax1, "HULL r2 · SIDE SECTION y = 0 (bow)", "Dominion local cm (+X bow). Slice of PR #72's SM_DominionMothership.fbx; r1 bay identical.")
segs(ax1, "side_y0", 0, 2); ax1.set_xlim(22000, 38500); ax1.set_ylim(-6000, 5500); ax1.grid(True, color=GRID, lw=0.3)
ax1.add_patch(Rectangle((24000, -3750), 6000, 3500, fc="#1f6fd1", alpha=0.10, ec="#1f6fd1", ls="--"))
ax1.plot(30000, -2000, "o", color="#b8321f", ms=8); ax1.text(30100, -1900, "SOCKET_Bay\n(30000, 0, -2000)", color="#b8321f", fontsize=8)
ax1.plot(33000, -2000, "s", color="#1a8a3a"); ax1.text(33100, -2300, "HangarPoint / launch\n(33000, 0, -2000)", color="#1a8a3a", fontsize=7.5)
for (lab, M, c) in (("r1", M1, "#7d3c98"), ("r2", M2, "#a569bd")):
    x, _, z = M["cpp_bay_actor"]; ax1.plot(x, z, "x", color=c, ms=10, mew=2)
    ax1.text(x + 150, z - 120 if lab == "r1" else z + 60, "C++ actor %s (%.0f, 0, %.0f)" % (lab, x, z), color=c, fontsize=7.5)
    ex = M["cpp_escape_start"]; ax1.plot(ex[0], ex[2], "^", color=c)
ax1.text(37000, -2700, "escape start\n(B.Max.X + 4000)", fontsize=7, color="#7d3c98")
ax1.annotate("", (24000, -2000), (30000, -2000), arrowprops=dict(arrowstyle="-|>", color="#1f6fd1", lw=1.5))
ax1.text(24100, -1800, "glide 6000 (C++ GlideDepth)", color="#1f6fd1", fontsize=7.5)
ax1.add_patch(Rectangle((31200, 1000), 186, 800, fc="#2a9fb8", alpha=0.7)); ax1.text(31450, 1300, "tractor emitter\n(31200, 0, 1400)", fontsize=7.5, color="#1d6f80")
ax1.add_patch(Rectangle((29680, -3750), 300, 3500, fc="#b8321f", alpha=0.35)); ax1.text(29000, -4400, "doors X 29680..29980", color="#b8321f", fontsize=7.5)
figure_side(ax1, 29400, -3750)
ax1.set_xlabel("hull X"); ax1.set_ylabel("hull Z")

frame(ax2, "HULL r2 · TOP SECTION z = -2000", "Bow mouth, pocket and door travel; mandibles to X 33400 set the bounds C++ uses.")
segs(ax2, "top_zm2000", 0, 1); ax2.set_xlim(21000, 38500); ax2.set_ylim(-8000, 8000); ax2.grid(True, color=GRID, lw=0.3)
ax2.add_patch(Rectangle((24000, -2500), 6000, 5000, fc="#1f6fd1", alpha=0.10, ec="#1f6fd1", ls="--"))
for cy in (-3750, 3750): ax2.add_patch(Rectangle((29680, cy - 1250), 300, 2500, fc="#b8321f", alpha=0.35))
for cy in (-1250, 1250): ax2.add_patch(Rectangle((29680, cy - 1250), 300, 2500, fc="none", ec="#b8321f", ls=":"))
ax2.plot(30000, 0, "o", color="#b8321f", ms=8); ax2.plot(33400, 0, "x", color="#7d3c98", ms=10, mew=2)
ax2.text(33500, 250, "C++ actor X 33400\n(bounds Max.X: brow / mandibles)", color="#7d3c98", fontsize=7.5)
ax2.add_patch(Rectangle((30000, -6000), 30000, 12000, fc="none", ec="#c0392b", lw=1, ls="-."))
ax2.text(30200, 6200, "lock zone: MouthDistance 30000 x MouthRadius 6000 (box shown to 38500)", color="#c0392b", fontsize=7.5)
ax2.text(24200, 2700, "pocket 6000 x 5000", color="#1f6fd1", fontsize=8)
ax2.text(28200, -7400, "open doors |Y| 2500..5000 (in the hull)", color="#b8321f", fontsize=7.5)
ax2.set_xlabel("hull X"); ax2.set_ylabel("hull Y")

frame(ax3, "APPROACH CONE (ATDDockingBay, as in C++) · top view, hull frame", "ConeHalfAngleDeg 35, ConeDepth 300000 (3 km; first 60 m and 1 km shown), TractorSpeed 1500, MinSteeringScale 0.35, HeadingPull 18 deg/s, Lineup 0.6/s, MouthDistance 30000, MouthRadius 6000, GlideSeconds 2, GlideDepth 6000, DoorOpenSeconds 2.5, DoorHalfWidth 2500.")
segs(ax3, "top_zm2000", 0, 1, lw=0.3); ax3.set_xlim(18000, 108000); ax3.set_ylim(-26000, 26000); ax3.set_aspect("equal")
ax3.grid(True, color=GRID, lw=0.3)
t = math.tan(math.radians(35))
for x0, c, lab in ((30000, "#1a8a3a", "cone from SOCKET_Bay (recommended)"), (33400, "#7d3c98", "cone from the C++ actor today (X 33400)")):
    ax3.add_patch(Polygon([(x0, 0), (x0 + 100000, 100000 * t), (x0 + 100000, -100000 * t)], closed=True, fc=c, alpha=0.06, ec=c, lw=1))
    ax3.text(x0 + 45000, (-45000 * t) + (2500 if c == "#7d3c98" else -2500), lab, color=c, fontsize=8, rotation=-35 * 0.0)
ax3.add_patch(Rectangle((33400, -6000), 30000, 12000, fc="#c0392b", alpha=0.08, ec="#c0392b", ls="-."))
ax3.text(34000, -8500, "glide lock-in zone today: |lateral| <= 6000 within 300 m of X 33400 — a straight 2 s lerp from its corner\n(e.g. 1 m out, 60 m off-axis) to the glide end passes through the mandibles (|Y| >= 3000, X <= 33400)", color="#c0392b", fontsize=8)
for i in range(1, 13):
    x = i * 300000 / 13
    if x < 105000: ax3.plot([30000 + x] * 2, [-(2500 + 0.08 * x), 2500 + 0.08 * x], "o", color="#888", ms=2)
ax3.text(60000, 9000, "guide lights: 12 pairs every ConeDepth/13 = 230.8 m at ±(DoorHalfWidth + 0.08·X), z = mouth sill", fontsize=7.5, color="#555")
ax3.set_xlabel("hull X (cm)"); ax3.set_ylabel("hull Y (cm)")
fig.tight_layout(); fig.savefig(os.path.join(OUT, "DB_Hull_Mouth_Cone.png"), facecolor=BG); plt.close(fig)
print("plans ok")
