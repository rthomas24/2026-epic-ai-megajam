"""Measured orthographic plan set for the Dominion interior r1 (matplotlib, from interior_layout.json).
python3 make_plans.py <plans_r1_dir>
Writes plans/DI_Plan_Top.png, DI_Plan_Gameplay.png, DI_Section_Long.png, DI_Sections_Cross.png.
"""
import json, sys, os, math
import matplotlib; matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle, Polygon, Circle, Wedge
from matplotlib.lines import Line2D

PL = sys.argv[1]
L = json.load(open(os.path.join(PL, "interior_layout.json")))
OUT = os.path.join(PL, "plans"); os.makedirs(OUT, exist_ok=True)
plt.rcParams.update({"font.family": "DejaVu Sans", "font.size": 9})
BG, INK, GRIDC = "#f4f2ec", "#1b1d22", "#c9c6bd"
FIG = "#d08a00"; RED = "#b8321f"; BLUE = "#1f6fd1"; GREEN = "#1a8a3a"; PURP = "#7d3c98"
COL = {"SM_DI_Room_Floor": "#d9d6cc", "SM_DI_Room_Wall": "#2c2f36", "SM_DI_Hangar_Pad": "#e0b13a", "SM_DI_Walkway_Ring": "#6d7f99",
       "SM_DI_Console": "#2a9fb8", "SM_DI_Cover_Low": "#8a5a2b", "SM_DI_Doorway": RED, "SM_DI_Door": RED,
       "SM_DI_Corridor_Straight": "#b9bcc2", "SM_DI_Core": "#7a3b12", "SM_DI_Conduit": "#ff6a14", "SM_DI_Core_Shield": "#ffb070",
       "SM_DI_Core_Ring_A": "#555", "SM_DI_Core_Ring_B": "#555"}
BOX = {"SM_DI_Room_Floor": ((-200, -200), (200, 200)), "SM_DI_Room_Wall": ((-30, -200), (30, 200)),
       "SM_DI_Hangar_Pad": ((-600, -600), (600, 600)), "SM_DI_Walkway_Ring": ((-200, -150), (200, 150)),
       "SM_DI_Console": ((-30, -60), (30, 60)), "SM_DI_Cover_Low": ((-30, -100), (30, 100)),
       "SM_DI_Doorway": ((-30, -200), (30, 200)), "SM_DI_Door": ((-15, -150), (15, 150)),
       "SM_DI_Corridor_Straight": ((-200, -200), (200, 200))}
G = L["game"]; K = L["kit"]; REC = L["recommended"]; CORE = G["core"]
ROOM = {r["name"]: r for r in L["rooms"]}

def rect_xy(p, box=None):
    (x0, y0), (x1, y1) = box or BOX[p["mesh"]]
    a = math.radians(p["yaw"]); c, s = math.cos(a), math.sin(a)
    return [(p["x"] + c * u - s * v, p["y"] + s * u + c * v) for u, v in ((x0, y0), (x1, y0), (x1, y1), (x0, y1))]

def dim(ax, a, b, off, text, horiz=True, col=INK, fs=8):
    (x0, y0), (x1, y1) = a, b
    if horiz:
        y = y0 + off
        ax.annotate("", (x0, y), (x1, y), arrowprops=dict(arrowstyle="<->", color=col, lw=0.8, shrinkA=0, shrinkB=0))
        ax.plot([x0, x0], [y0, y], color=col, lw=0.4); ax.plot([x1, x1], [y1, y], color=col, lw=0.4)
        ax.text((x0 + x1) / 2, y, text, ha="center", va="bottom", fontsize=fs, color=col, bbox=dict(fc=BG, ec="none", pad=0.5), zorder=20)
    else:
        x = x0 + off
        ax.annotate("", (x, y0), (x, y1), arrowprops=dict(arrowstyle="<->", color=col, lw=0.8, shrinkA=0, shrinkB=0))
        ax.plot([x0, x], [y0, y0], color=col, lw=0.4); ax.plot([x1, x], [y1, y1], color=col, lw=0.4)
        ax.text(x, (y0 + y1) / 2, text, ha="center", va="center", rotation=90, fontsize=fs, color=col, bbox=dict(fc=BG, ec="none", pad=0.5), zorder=20)

def figure_side(ax, x, z=0, col=FIG, label=True, s=1.0):
    ax.add_patch(Rectangle((x - 18 * s, z), 36 * s, 86 * s, color=col, zorder=15)); ax.add_patch(Rectangle((x - 22 * s, z + 84 * s), 44 * s, 64 * s, color=col, zorder=15))
    ax.add_patch(Circle((x, z + 169 * s), 11 * s, color=col, zorder=15))
    if label: ax.text(x + 30 * s, z + 175 * s, "180 cm" if s == 1 else "%d cm" % (180 * s), fontsize=7, color=col, va="center", zorder=15)

def figure_plan(ax, x, y, label=None):
    ax.add_patch(Circle((x, y), 34, color=FIG, zorder=15)); ax.add_patch(Circle((x, y), 34, fc="none", ec="k", lw=0.4, zorder=15))
    if label: ax.text(x + 50, y + 40, label, fontsize=6.5, color="#8a5a00", zorder=15)

def frame(ax, title, sub=None):
    ax.set_facecolor(BG); ax.set_aspect("equal")
    ax.set_title(title, loc="left", fontsize=12.5, fontweight="bold", color=INK, pad=24 if sub is None else 46)
    if sub: ax.annotate(sub, (0, 1), xycoords="axes fraction", xytext=(0, 30), textcoords="offset points", fontsize=7.8, color="#444", va="bottom", annotation_clip=False, gid="sub")

def grid(ax, x0, x1, y0, y1, step=400):
    for x in range(int(x0) - int(x0) % step, int(x1) + 1, step): ax.axvline(x, color=GRIDC, lw=0.3, zorder=0)
    for y in range(int(y0) - int(y0) % step, int(y1) + 1, step): ax.axhline(y, color=GRIDC, lw=0.3, zorder=0)
    ax.set_xlim(x0, x1); ax.set_ylim(y0, y1); ax.tick_params(labelsize=7)

def draw_kit(ax, show_props=True):
    order = ["SM_DI_Room_Floor", "SM_DI_Corridor_Straight", "SM_DI_Hangar_Pad", "SM_DI_Walkway_Ring", "SM_DI_Room_Wall",
             "SM_DI_Doorway", "SM_DI_Door", "SM_DI_Cover_Low", "SM_DI_Console"]
    for m in order:
        for p in L["placements"]:
            if p["mesh"] != m or (m == "SM_DI_Room_Wall" and p["z"] > 0): continue
            if m == "SM_DI_Cover_Low" and not show_props: continue
            if m == "SM_DI_Corridor_Straight":
                ax.add_patch(Polygon(rect_xy(p), closed=True, fc=COL[m], ec="#777", lw=0.3, alpha=0.6, zorder=1))
                for sgn in (-1, 1):   # 30 cm walls inside the footprint, inner face |y| 170
                    ax.add_patch(Polygon(rect_xy(p, ((-200, 170 * sgn), (200, 200 * sgn))), closed=True, fc="#2c2f36", ec="none", zorder=3))
                    ax.add_patch(Polygon(rect_xy(p, ((-200, 160 * sgn), (200, 170 * sgn))), closed=True, fc="#6b6f78", ec="none", zorder=3))
                ax.plot([p["x"], p["x"]], [-156, 156], color="#555", lw=0.5, ls=":", zorder=3)   # portal rib
                continue
            alpha = 0.5 if m == "SM_DI_Room_Floor" else (0.35 if m == "SM_DI_Walkway_Ring" else 0.92)
            ax.add_patch(Polygon(rect_xy(p), closed=True, fc=COL[m], ec="#555" if m == "SM_DI_Room_Floor" else COL[m], lw=0.3, alpha=alpha,
                                 zorder=1 if m == "SM_DI_Room_Floor" else 4))
    # core pieces (circles)
    cx, cy = CORE["centre"]
    ax.add_patch(Circle((cx, cy), CORE["shield_radius"], fc=COL["SM_DI_Core_Shield"], alpha=0.18, ec="#e08030", lw=1, ls="--", zorder=4))
    ax.add_patch(Circle((cx, cy), CORE["ring_b_radius"], fc="none", ec="#555", lw=1.2, zorder=4))
    ax.add_patch(Circle((cx, cy), CORE["ring_a_radius"], fc="none", ec="#555", lw=1.2, zorder=4))
    ax.add_patch(Rectangle((cx - 400, cy - 400), 800, 800, fc=COL["SM_DI_Core"], alpha=0.85, ec="k", lw=0.6, zorder=5))
    for (x, y) in CORE["conduits"]:
        ax.add_patch(Rectangle((x - 60, y - 60), 120, 120, fc=COL["SM_DI_Conduit"], ec="k", lw=0.5, zorder=6))
    hx, hy = CORE["heart"]; ax.add_patch(Circle((hx, hy), CORE["heart_radius"], fc="#ffcf70", ec="#b85a00", lw=1, zorder=6))

def room_labels(ax, big=True):
    for r in L["rooms"]:
        x0, x1 = r["x"]; y0, y1 = r["y"]
        ax.text((x0 + x1) / 2, y1 - 260, "%s  %d x %d, clear h %d (%d storey%s)" % (r["name"].upper(), r["size"][0], r["size"][1], r["clear_height"],
                r["storeys"], "s" if r["storeys"] > 1 else ""), ha="center", fontsize=8.5 if big else 7.5, fontweight="bold", color=INK, zorder=21,
                bbox=dict(fc=BG, ec="none", alpha=0.8, pad=1))
    for c in L["corridors"]:
        ax.text((c["x"][0] + c["x"][1]) / 2, 330, "%s %d long, clear %d x %d" % (c["name"], c["length"], c["clear_width"], c["clear_height"]),
                ha="center", fontsize=7, color=INK, zorder=21, bbox=dict(fc=BG, ec="none", alpha=0.8, pad=0.5))
    for d in L["doors"]:
        ax.text(d["x"], -470, "%s\n300 x 400" % d["name"], ha="center", fontsize=6.3, color=RED, zorder=21)

def hull_axis(ax):
    sec = ax.secondary_xaxis("top", functions=(lambda x: 22800 - x, lambda h: 22800 - h)); sec.set_xlabel("hull X (Dominion local, cm)", fontsize=7.5)
    sec.tick_params(labelsize=6.5)

def clipall(fig):
    for ax in fig.axes:
        for t in ax.texts:
            if t.get_transform() != ax.transAxes and t.get_gid() != 'sub': t.set_clip_on(True); t.set_clip_box(ax.bbox)

SPLIT = [(-7700, 5600, -3000, 3000), (4800, 15300, -2300, 2300)]

# ======================================================================================== 1. TOP PLAN (kit pieces)
fig, axs = plt.subplots(2, 1, figsize=(24, 21), dpi=130, gridspec_kw=dict(height_ratios=[6000, 4600])); fig.patch.set_facecolor(BG)
for i, ax in enumerate(axs):
    x0, x1, y0, y1 = SPLIT[i]
    frame(ax, ["DOMINION INTERIOR r1 · TOP PLAN 1/2 · docking-bay pocket (PR #77) -> hangar -> deflect corridor  (floor z 0, ceilings off)",
               "TOP PLAN 2/2 · flight deck (arena) -> link -> Warden's gate -> link -> reactor core chamber"][i],
          "Unreal cm, T5 course-local frame (TDDominionInteriorCourse.inl): +X deeper into the ship, 400 cm kit grid (ArtSource/EngProps/DominionInterior), every rectangle = one kit piece by pivot at scale 1. All floors at z 0: the whole route is LEVEL (no stairs, no ramps).")
    grid(ax, x0, x1, y0, y1); draw_kit(ax); room_labels(ax); hull_axis(ax)
    ax.set_xlabel("interior X (cm)"); ax.set_ylabel("interior Y (cm)")
ax = axs[0]
ax.text(-4200, 2750, "GLIDE POCKET (PR #77, merged): 6000 x 5200, clear 3600, mouth 5000 x 3500 at x -7200 = SOCKET_Bay", ha="center", fontsize=8.5, fontweight="bold", zorder=21, bbox=dict(fc=BG, ec="none", alpha=0.85))
ax.add_patch(Rectangle((-7200, -2500), 400, 5000, fc="#d9661f", alpha=0.45, ec="k", lw=0.6, hatch="//", zorder=5))
ax.text(-7150, -2900, "bay threshold band (PR #77)", fontsize=7, color="#8a3a00")
ax.plot([-1200, -1200], [-1400, 1400], color=BLUE, lw=3, alpha=0.7, zorder=8)
ax.text(-1150, -1300, "HANGAR OPENING 2800 x 1200\n(no X0 wall: PR #77; C++ still builds one)", fontsize=7.5, color=BLUE, zorder=21, bbox=dict(fc=BG, ec=BLUE, lw=0.6, pad=1.5))
sl, sw, sh = L["ship_footprint"]["size"]
ax.add_patch(Rectangle((-200 - sl / 2, -sw / 2), sl, sw, fc="none", ec="k", lw=1.1, ls="--", zorder=9))
ax.text(-200, -sw / 2 - 150, "docked ship 524 x 488 x 230 on Pad A (-200, 0)", ha="center", fontsize=7, zorder=21)
figure_plan(ax, -600, 0, "pilot start (-600, 0)")
# console clear zones (150 cm in front)
for name, (cx, cy, yaw) in G["consoles"].items():
    for a in axs:
        ux = 1 if yaw == 180 else -1
        x0_ = cx + 30 if ux > 0 else cx - 30 - 150
        a.add_patch(Rectangle((x0_, cy - 60), 150, 120, fc="#2a9fb8", alpha=0.2, ec="#2a9fb8", lw=0.6, ls="--", zorder=8))
        a.text(cx, cy + (90 if cy > 0 else -150), name.replace("A4_Recording_", "console ") + " · 150 clear", ha="center", fontsize=6.5, color="#1d6f80", zorder=21)
# corridor cover gap call-out
cp = [p for p in L["placements"] if p["group"] == "cpp_cover" and p["x"] == 4500][0]
ax.add_patch(Polygon(rect_xy(dict(cp, y=REC["cover_corridor_y"])), closed=True, fc="none", ec=GREEN, lw=1.2, ls="--", zorder=9))
dim(ax, (4500, 30), (4500, 160), -260, "", horiz=False, col=RED, fs=7)
ax.text(3050, -900, "Troopers' cover (C++ y -70): gap to the kick plate only 130 cm\n(< 140 R-042). RECOMMENDED y -100 (green dashed): gap 160.", fontsize=7.5, color=RED, zorder=21, bbox=dict(fc=BG, ec=RED, lw=0.6, pad=1.5))
dim(ax, (-7200, 2600), (-1200, 2600), 150, "pocket 6000 (PR #77)")
dim(ax, (-1200, 1400), (1200, 1400), 500, "hangar 2400")
dim(ax, (1200, 200), (5200, 200), 900, "deflect corridor 4000 (10 x SM_DI_Corridor_Straight)")
dim(ax, (1500, -1400), (1500, 1400), 0, "hangar 2800", horiz=False)
dim(ax, (2000, -170), (2000, 170), 0, "340", horiz=False, fs=6.5)
dim(ax, (-7500, -2600), (-7500, 2600), 0, "pocket 5200", horiz=False)
ax.plot([-7400, -5400], [-2850, -2850], color="k", lw=3); ax.text(-6400, -2800, "20 m", ha="center", fontsize=8)
ax = axs[1]
dim(ax, (5200, 1400), (7600, 1400), 350, "arena 2400 (6 cells)")
dim(ax, (7600, 200), (8400, 200), 700, "link 800")
dim(ax, (8400, 1000), (10400, 1000), 350, "Warden 2000 (5)")
dim(ax, (10400, 200), (11200, 200), 700, "link 800")
dim(ax, (11200, 1800), (14800, 1800), 250, "core chamber 3600 (9 cells)")
dim(ax, (5000, -1400), (5000, 1400), 0, "2800", horiz=False)
dim(ax, (8300, -1000), (8300, 1000), 0, "2000", horiz=False)
dim(ax, (15050, -1800), (15050, 1800), 0, "3600", horiz=False)
cx, cy = CORE["centre"]
dim(ax, (cx, -1100), (cx + 778, -1100), -150, "conduits r 1100 @45°", fs=6.5)
ax.text(cx - 760, 140, "HEART\n(-760, 0)\nr 80", ha="center", fontsize=6.5, color="#8a4a00", zorder=21)
ax.text(cx, -720, "SM_DI_Core 800 x 800 x 1200 · Ring A r500 / Ring B r575 at z 600 · Shield r640 h1380", ha="center", fontsize=7, zorder=21, bbox=dict(fc=BG, ec="none", alpha=0.8))
figure_plan(ax, 9000, -300, "figure"); figure_plan(ax, 12000, 900, "figure")
ax.plot([5000, 7000], [-2150, -2150], color="k", lw=3); ax.text(6000, -2100, "20 m", ha="center", fontsize=8)
handles = [Line2D([], [], color=COL[k], lw=6, label=k) for k in ["SM_DI_Room_Floor", "SM_DI_Room_Wall", "SM_DI_Corridor_Straight", "SM_DI_Doorway",
           "SM_DI_Hangar_Pad", "SM_DI_Walkway_Ring", "SM_DI_Cover_Low", "SM_DI_Console", "SM_DI_Core", "SM_DI_Conduit", "SM_DI_Core_Shield"]]
handles += [Line2D([], [], color="#6b6f78", lw=3, label="corridor kick plate |y| 160..170"), Line2D([], [], color="#555", lw=1, ls=":", label="portal rib |y| <= 156, z 586"),
            Line2D([], [], color="#2a9fb8", lw=6, alpha=0.3, label="console 150 cm clear zone (R-042)")]
ax.legend(handles=handles, loc="lower right", fontsize=7, framealpha=0.92, ncol=3)
clipall(fig); fig.subplots_adjust(left=0.05, right=0.96, top=0.93, bottom=0.05, hspace=0.28, wspace=0.18); fig.savefig(os.path.join(OUT, "DI_Plan_Top.png"), facecolor=BG); plt.close(fig)

# ======================================================================================== 2. GAMEPLAY PLAN
def blocked(p0, p1, c, r):
    (x0, y0), (x1, y1) = p0, p1; dx, dy = x1 - x0, y1 - y0
    t = max(0, min(1, ((c[0] - x0) * dx + (c[1] - y0) * dy) / (dx * dx + dy * dy)))
    return math.hypot(x0 + t * dx - c[0], y0 + t * dy - c[1]) < r
SIGHT = []
fig, axs = plt.subplots(2, 1, figsize=(24, 21), dpi=130, gridspec_kw=dict(height_ratios=[6000, 4600])); fig.patch.set_facecolor(BG)
for i, ax in enumerate(axs):
    x0, x1, y0, y1 = SPLIT[i]
    frame(ax, ["GAMEPLAY OVERLAY 1/2 · volumes, spawns, checkpoints, sightlines (all from TDDominionInteriorCourse.inl)", "GAMEPLAY OVERLAY 2/2"][i],
          "Boxes = C++ encounter / trigger volumes (centre ± extent). ▲ entry (Continue start), ◆ checkpoint, ★ trailhead, ✕ spawn. Dashed green = sightline (eye 170 cm). Escape: debris lane |y| <= 150 runs x -1200..14800.")
    grid(ax, x0, x1, y0, y1); draw_kit(ax); hull_axis(ax)
    for name, v in G["volumes"].items():
        (cx, cy, cz), (ex, ey, ez) = v["c"], v["e"]
        col = PURP if "Escape" in name else ("#c0392b" if "encounter" in name.lower() else BLUE)
        ax.add_patch(Rectangle((cx - ex, cy - ey), 2 * ex, 2 * ey, fc=col, alpha=0.07, ec=col, lw=1.1, ls="-." if "Escape" in name else "-", zorder=7))
        if x0 < cx < x1:
            ax.text(cx, cy + ey + 40 if "debris" not in name else -250, name, ha="center", fontsize=7, color=col, zorder=21, bbox=dict(fc=BG, ec="none", alpha=0.7, pad=0.3))
    for name, (x, y) in G["entries"].items():
        ax.plot(x, y, marker="^", color=GREEN, ms=9, zorder=12); ax.text(x, y - 170, "entry " + name, fontsize=6.3, color=GREEN, ha="center", zorder=21)
    for name, (x, y) in G["checkpoints"].items():
        ax.plot(x, y + 60, marker="D", color=BLUE, ms=6, zorder=12); ax.text(x, y + 150, "ckpt " + name, fontsize=6.3, color=BLUE, ha="center", zorder=21)
    for name, (x, y) in G["trailheads"].items():
        ax.plot(x, y - 60, marker="*", color="#d4a017", ms=11, zorder=12); ax.text(x, y - 280, name, fontsize=6.3, color="#8a6a00", ha="center", zorder=21)
    for name, (x, y) in G["spawns"].items():
        ax.plot(x, y, marker="X", color="#c0392b", ms=9, zorder=12); ax.text(x + 60, y + (60 if y >= 0 else -120), name, fontsize=6.5, color="#c0392b", zorder=21)
    for (pos, rad, col) in G["lights"]:
        if x0 < pos[0] < x1: ax.add_patch(Circle(pos[:2], 45, fc="#fff3b0", ec="#a08000", lw=0.6, zorder=12))
ax = axs[0]
# sightlines in the corridor: hangar door -> troopers
for tgt in G["spawns"].values():
    if tgt[0] < 6000:
        ax.plot([1200, tgt[0]], [0, tgt[1]], color=GREEN, ls="--", lw=1, zorder=11)
SIGHT.append(("Hangar door (1200,0) -> troopers (4750,±100)", 3551, "clear (cover blocks y <= 30 only past x 4470)"))
ax.text(2300, 600, "sightline 35.5 m down the lane: troopers see the door the moment it opens", fontsize=7.5, color=GREEN, zorder=21)
ax.text(-1100, 1150, "Pad A ship trigger 1000 x 1000 = escape goal", fontsize=7, color=PURP, zorder=21)
ax.plot([-600, -200], [0, 0], color=GREEN, ls="--", lw=1); 
ax = axs[1]
cx, cy = CORE["centre"]
eye = (11500, 0)
for (x, y) in CORE["conduits"]:
    b = blocked(eye, (x, y), (cx, cy), 400 * math.sqrt(2) * 0.9)
    ax.plot([eye[0], x], [eye[1], y], color="#c0392b" if b else GREEN, ls="--", lw=1, zorder=11)
    SIGHT.append(("Core door ckpt (11500,0) -> conduit (%d,%d)" % (x, y), round(math.hypot(x - eye[0], y - eye[1])), "BLOCKED by the core body" if b else "clear"))
ax.text(11600, 1450, "from the core door: the two near conduits are in view;\nthe two far ones hide behind the core (red) -> walk the ring", fontsize=7.5, color=INK, zorder=21, bbox=dict(fc=BG, ec="none", alpha=0.85))
ax.plot([5200, 7600], [0, 0], color=GREEN, ls="--", lw=1); SIGHT.append(("Arena door to door", 2400, "clear (covers at |y| 500 off-axis)"))
ax.plot([8400, 9900], [0, 0], color=GREEN, ls="--", lw=1.4); SIGHT.append(("Warden door (8400) -> Warden spawn (9900)", 1500, "clear"))
ax.text(8450, 120, "15 m to the Warden", fontsize=7, color=GREEN, zorder=21)
for i, r in enumerate(G["arena_spawn_radius"]):
    ax.add_patch(Circle((6400, 0), r, fc="none", ec="#c0392b", lw=0.6, ls=":", zorder=8))
ax.text(6400, -980, "wave spawn radius 800 / 900 / 900 (Grunt x3; Trooper x2 + Heavy; Drone x2 + Grunt)", ha="center", fontsize=6.5, color="#c0392b", zorder=21)
ax.text(cx, 1300, "N-052: the future-crew trio defends here (not in C++ yet).\nKeep the ring between r 700 and r 1500 clear for that fight.", ha="center", fontsize=7.5, color=PURP, zorder=21, bbox=dict(fc=BG, ec=PURP, lw=0.6, pad=1.5))
ax.add_patch(Wedge((cx, cy), 1500, 0, 360, width=800, fc=PURP, alpha=0.05, zorder=2))
clipall(fig); fig.subplots_adjust(left=0.05, right=0.96, top=0.93, bottom=0.05, hspace=0.28, wspace=0.18); fig.savefig(os.path.join(OUT, "DI_Plan_Gameplay.png"), facecolor=BG); plt.close(fig)
json.dump(SIGHT, open(os.path.join(PL, "review", "sightlines.json"), "w"), indent=1)

# ======================================================================================== 3. LONG SECTION A-A (y = 0)
fig, axs = plt.subplots(2, 1, figsize=(24, 15), dpi=130); fig.patch.set_facecolor(BG)
def slab(ax, x0, x1, z0, z1, c="#2c2f36", a=0.9, z=3): ax.add_patch(Rectangle((x0, z0), x1 - x0, z1 - z0, fc=c, alpha=a, ec="none", zorder=z))
def door_cut(ax, x, storeys):
    slab(ax, x - 30, x + 30, 400, 600)                             # lintel
    for s in range(1, storeys): slab(ax, x - 30, x + 30, s * 600, (s + 1) * 600)
    ax.add_patch(Rectangle((x - 15, 0), 30, 400, fc="none", ec=RED, lw=1, ls=":", zorder=6))
    slab(ax, x - 15, x + 15, 400, 800, RED, 0.35, 6)            # door slid up +400
for i, ax in enumerate(axs):
    x0, x1 = SPLIT[i][:2]
    frame(ax, ["SECTION A-A 1/2 (y = 0, looking +Y) · pocket -> hangar -> deflect corridor", "SECTION A-A 2/2 · arena -> Warden -> core"][i],
          "Floor top z 0 everywhere (floor slabs -20..0); ceiling slab undersides at 600 / 1200 / 1800 / 3600. Doors 300 x 400 clear slide +Z 400 into the lintel (shown open, red). Right axis: hull Z = z - 3750.")
    for x in range(int(x0) - int(x0) % 400, int(x1) + 1, 400): ax.axvline(x, color=GRIDC, lw=0.3, zorder=0)
    for z in range(-400, 4001, 200): ax.axhline(z, color=GRIDC, lw=0.25 if z % 600 else 0.6, zorder=0)
    ax.set_xlim(x0, x1); ax.set_ylim(-400, 4000); ax.set_aspect("equal"); ax.tick_params(labelsize=7)
    hull_axis(ax)
    sec = ax.secondary_yaxis("right", functions=(lambda z: z - 3750, lambda h: h + 3750)); sec.set_ylabel("hull Z", fontsize=7.5); sec.tick_params(labelsize=6.5)
    ax.set_xlabel("interior X (cm)"); ax.set_ylabel("z (cm)")
    slab(ax, -7200, 14800, -20, 0, "#8d8a82")
    for r in [dict(x=[-7200, -1200], clear_height=3600)] + L["rooms"]:
        slab(ax, r["x"][0], r["x"][1], r["clear_height"], r["clear_height"] + 20)
    for c in L["corridors"]:
        slab(ax, c["x"][0], c["x"][1], 600, 620, "#6b6f78")
        for x in range(int(c["x"][0]) + 200, int(c["x"][1]), 400): slab(ax, x - 7, x + 7, 586, 600, "#555", 0.9, 4)
    for d in L["doors"]:
        st = 2 if d["x"] == 1200 else (3 if d["x"] == 11200 else 1)
        door_cut(ax, d["x"], st)
    slab(ax, -1230, -1170, 1200, 3600)
    slab(ax, 14770, 14830, 0, 1800); slab(ax, 7570, 7630, 600, 600)
ax = axs[0]
ax.plot([-7200, -7200], [0, 3500], color=RED, lw=4); ax.text(-7150, 3700, "mouth 3500 (PR #77)", color=RED, fontsize=7.5)
slab(ax, -800, 400, -40 + 0.5, 0.5, "#e0b13a", 1, 5); ax.add_patch(Rectangle((-462, 0.5), 524, 230, fc="none", ec="k", lw=1, ls="--", zorder=6))
ax.text(-200, 260, "ship 230 h", ha="center", fontsize=7)
figure_side(ax, -600); figure_side(ax, 3000); figure_side(ax, -4000); figure_side(ax, 4300, col="#c0392b", label=False)
ax.add_patch(Rectangle((4470, 0), 60, 110, fc=COL["SM_DI_Cover_Low"], zorder=7)); ax.text(4400, 130, "cover 110 h", fontsize=7, ha="right")
dim(ax, (-1000, 0), (-1000, 1200), 0, "hangar clear 1200", horiz=False)
dim(ax, (2500, 0), (2500, 586), 0, "586 under ribs / 600", horiz=False, fs=7)
dim(ax, (1300, 0), (1300, 400), 0, "door 400", horiz=False, col=RED, fs=7)
dim(ax, (-7500, 0), (-7500, 3600), 0, "pocket clear 3600", horiz=False)
ax.text(-3500, 150, "level floor z 0 from the mouth to the core: no stairs, no ramps (Pad tops +0.5 cm, flush)", fontsize=8, color=GREEN, zorder=21)
ax.plot([-7400, -5400], [-300, -300], color="k", lw=3); ax.text(-6400, -260, "20 m", ha="center", fontsize=8)
ax = axs[1]
cx = CORE["centre"][0]
slab(ax, cx - 400, cx + 400, 0, 1200, COL["SM_DI_Core"], 0.85, 5)
ax.add_patch(Rectangle((cx - 640, 0), 1280, 1380, fc="#ffb070", alpha=0.15, ec="#e08030", lw=1, ls="--", zorder=4))
slab(ax, cx - 575, cx + 575, 576, 624, "#555", 0.9, 6)
for (x, y) in CORE["conduits"]:
    if y < 0: slab(ax, x - 60, x + 60, 0, 300, COL["SM_DI_Conduit"], 0.9, 6)
ax.add_patch(Circle((cx - 760, 160), 80, fc="#ffcf70", ec="#b85a00", zorder=7))
ax.text(cx, 1430, "shield 1380 h (r 640); core 1200; rings z 600", ha="center", fontsize=7.5)
figure_side(ax, 11600); figure_side(ax, 6400); figure_side(ax, 9000)
# Warden 260 tall
ax.add_patch(Rectangle((9900 - 40, 30), 80, 230, fc="#5b2333", zorder=7)); ax.add_patch(Circle((9900, 270), 22, fc="#5b2333", zorder=7))
ax.text(9960, 280, "Warden ~260 (capsule half-height 130)", fontsize=7)
for (x, rot) in ((5900, 0), (6900, 1)): ax.add_patch(Rectangle((x - (100 if rot else 30), 0), 200 if rot else 60, 110, fc=COL["SM_DI_Cover_Low"], zorder=7))
dim(ax, (14500, 0), (14500, 1800), 0, "core chamber clear 1800 (3 storeys)", horiz=False)
dim(ax, (7000, 0), (7000, 600), 0, "arena 600", horiz=False, fs=7)
dim(ax, (10100, 0), (10100, 600), 0, "600", horiz=False, fs=7)
ax.plot([5000, 7000], [-300, -300], color="k", lw=3); ax.text(6000, -260, "20 m", ha="center", fontsize=8)
clipall(fig); fig.subplots_adjust(left=0.05, right=0.96, top=0.93, bottom=0.05, hspace=0.28, wspace=0.18); fig.savefig(os.path.join(OUT, "DI_Section_Long.png"), facecolor=BG); plt.close(fig)

# ======================================================================================== 4. CROSS SECTIONS + standards
fig, axs = plt.subplots(2, 3, figsize=(24, 14), dpi=130); fig.patch.set_facecolor(BG)
def xs_setup(ax, title, y0, y1, z1, sub=None):
    frame(ax, title, sub)
    for y in range(int(y0) - int(y0) % 100, int(y1) + 1, 100): ax.axvline(y, color=GRIDC, lw=0.25 if y % 400 else 0.6, zorder=0)
    for z in range(-100, int(z1) + 1, 100): ax.axhline(z, color=GRIDC, lw=0.25 if z % 600 else 0.6, zorder=0)
    ax.set_xlim(y0, y1); ax.set_ylim(-60, z1); ax.tick_params(labelsize=7); ax.set_xlabel("interior Y (cm), looking +X"); ax.set_ylabel("z (cm)")
# B-B corridor at the troopers' cover
ax = axs[0, 0]; xs_setup(ax, "B-B · deflect corridor at x 4500 (troopers' cover)", -320, 320, 700, "SM_DI_Corridor_Straight · walls inner face |y| 170, kick plates to |y| 160, rib |y| 156 up to z 586")
slab(ax, -200, 200, -20, 0, "#8d8a82"); slab(ax, -200, 200, 600, 620); slab(ax, -200, -170, 0, 600); slab(ax, 170, 200, 0, 600)
slab(ax, -170, -160, 0, 120, "#6b6f78"); slab(ax, 160, 170, 0, 120, "#6b6f78")
ax.add_patch(Rectangle((-156, 0), 312, 586, fc="none", ec="#555", lw=0.8, ls=":"))
ax.add_patch(Rectangle((-170, 0), 200, 110, fc=COL["SM_DI_Cover_Low"], alpha=0.9, zorder=6)); ax.text(-70, 50, "cover C++\ny -170..30", ha="center", fontsize=7, color="w", zorder=7)
ax.add_patch(Rectangle((-200, 0), 200, 110, fc="none", ec=GREEN, lw=1.5, ls="--", zorder=7)); ax.text(-100, 125, "recommended y -200..0", ha="center", fontsize=7, color=GREEN)
figure_side(ax, 95)
ax.add_patch(Rectangle((95 - 34, 0), 68, 180, fc="none", ec="k", lw=0.7, ls="--", zorder=16)); ax.text(95, 195, "capsule Ø68", ha="center", fontsize=6.5)
dim(ax, (30, 300), (160, 300), 0, "130 < 140", col=RED, fs=7.5); dim(ax, (0, 360), (160, 360), 0, "160 (rec.)", col=GREEN, fs=7.5)
dim(ax, (-170, 640), (170, 640), 0, "clear 340"); dim(ax, (-280, 0), (-280, 600), 0, "600", horiz=False)
# C-C doorway
ax = axs[0, 1]; xs_setup(ax, "C-C · SM_DI_Doorway + SM_DI_Door (all 6 doors)", -320, 320, 1000, "frame 60 x 400 x 600 on the cell seam; clear 300 x 400 (R-042 min 140 x 240); door slides +Z 400")
slab(ax, -200, -150, 0, 600, "#3a3d44"); slab(ax, 150, 200, 0, 600, "#3a3d44"); slab(ax, -150, 150, 400, 600, "#3a3d44")
ax.add_patch(Rectangle((-150, 0), 300, 400, fc="none", ec=RED, lw=1, ls=":")); slab(ax, -150, 150, 400, 800, RED, 0.3, 4)
ax.text(0, 700, "door open (+400)", ha="center", fontsize=7, color=RED)
ax.add_patch(Rectangle((-70, 0), 140, 240, fc="none", ec=GREEN, lw=1, ls="--")); ax.text(0, 250, "R-042 min 140 x 240", ha="center", fontsize=6.5, color=GREEN)
figure_side(ax, 0); dim(ax, (-150, 440), (150, 440), -60, "300"); dim(ax, (240, 0), (240, 400), 0, "400", horiz=False)
ax.text(0, 900, "lintel: kit three-chevron mark\n+ T_DI_Decals trident above", ha="center", fontsize=7)
# D-D arena
ax = axs[0, 2]; xs_setup(ax, "D-D · flight deck (arena) at x 6400", -1500, 1500, 800, "2800 wide x 600 clear · 4 x SM_DI_Cover_Low 110 h at |y| 500 · Warden room E-E is the same at 2000 wide")
slab(ax, -1400, 1400, -20, 0, "#8d8a82"); slab(ax, -1400, 1400, 600, 620); slab(ax, -1430, -1370, 0, 600); slab(ax, 1370, 1430, 0, 600)
for y in (-500, 500): ax.add_patch(Rectangle((y - 100, 0), 200, 110, fc=COL["SM_DI_Cover_Low"], zorder=6))
figure_side(ax, 0); figure_side(ax, -800, col="#c0392b", label=False); ax.text(-800, 200, "crew 180", ha="center", fontsize=6.5, color="#c0392b")
ax.add_patch(Rectangle((-1000, 0), 2000, 600, fc="none", ec=INK, lw=0.6, ls="-."))
ax.text(0, 640, "Warden room: |y| 1000 (dash-dot), Warden ~260 tall", ha="center", fontsize=7)
dim(ax, (-1370, 700), (1370, 700), 0, "clear 2740 (walls 60 on the boundary)")
# F-F core
ax = axs[1, 0]; xs_setup(ax, "F-F · reactor core chamber at x 13000", -2000, 2000, 2000, "3600 wide x 1800 clear · core 800 x 1200 · rings r500/r575 at z 600 · shield r640 x 1380 · conduits 120 x 300 at r1100 (45°)")
slab(ax, -1800, 1800, -20, 0, "#8d8a82"); slab(ax, -1800, 1800, 1800, 1820); slab(ax, -1830, -1770, 0, 1800); slab(ax, 1770, 1830, 0, 1800)
slab(ax, -400, 400, 0, 1200, COL["SM_DI_Core"], 0.85, 5); slab(ax, -575, 575, 576, 624, "#555", 0.9, 6)
ax.add_patch(Rectangle((-640, 0), 1280, 1380, fc="#ffb070", alpha=0.15, ec="#e08030", lw=1, ls="--", zorder=4))
for y in (-778, 778): slab(ax, y - 60, y + 60, 0, 300, COL["SM_DI_Conduit"], 0.9, 6)
figure_side(ax, -1200); figure_side(ax, 1200, label=False)
dim(ax, (-1770, 1900), (1770, 1900), 0, "clear 3540"); dim(ax, (-1900, 0), (-1900, 1800), 0, "1800", horiz=False)
ax.text(0, 1500, "core light z 1500", ha="center", fontsize=7)
# G hangar cross
ax = axs[1, 1]; xs_setup(ax, "H-H · hangar at x -200 (Pad A) + opening to the pocket", -1600, 1600, 1400, "2800 x 1200 clear (2 storeys); Pad A 1200 x 1200 x 40, top flush (+0.5); ship 524 x 488 x 230; opening at x -1200 is the full 2800 x 1200")
slab(ax, -1400, 1400, -20, 0, "#8d8a82"); slab(ax, -1400, 1400, 1200, 1220); slab(ax, -1430, -1370, 0, 1200); slab(ax, 1370, 1430, 0, 1200)
slab(ax, -600, 600, -39.5, 0.5, "#e0b13a", 1, 5); ax.add_patch(Rectangle((-244, 0.5), 488, 230, fc="none", ec="k", lw=1, ls="--", zorder=6))
ax.text(0, 250, "ship 488 w x 230 h", ha="center", fontsize=7); figure_side(ax, -800); figure_side(ax, 900, label=False)
dim(ax, (244, 120), (1370, 120), 0, "1126 wing-to-wall", fs=7); dim(ax, (0, 230), (0, 1200), 0, "970 over the ship", horiz=False, fs=7)
# standards panel
ax = axs[1, 2]; ax.set_facecolor(BG); ax.axis("off")
ax.set_title("WALKABILITY STANDARD (R-042 / R-043) · checked against this layout", loc="left", fontsize=12.5, fontweight="bold", color=INK)
rows = [("Rule (Lead R-042)", "Required", "This layout", "OK"),
        ("Doorway clear", ">= 140 x 240", "300 x 400 (all 6)", "yes"),
        ("Ceiling clear", ">= 280", "586 (corridor ribs) .. 3600", "yes"),
        ("Floors", "flat", "z 0 everywhere, slabs -20..0", "yes"),
        ("Ramps", "<= 20°", "none on the route", "n/a"),
        ("Console clear front", ">= 150", "4 consoles, all >= 150", "yes"),
        ("Corridor clear width", "(>= 140)", "340; 312 at ribs", "yes"),
        ("Gap past corridor cover", ">= 140", "130 (C++) -> 160 rec.", "NO"),
        ("Ship on Pad A", "clear", "970 over, 1126 to walls", "yes"),
        ("180 cm figure", "every sheet", "plans, sections, renders", "yes")]
for i, r in enumerate(rows):
    y = 0.95 - i * 0.045
    for j, (t, xx) in enumerate(zip(r, (0.0, 0.33, 0.55, 0.92))):
        ax.text(xx, y, t, fontsize=8.5 if i else 8.8, fontweight="bold" if i == 0 else None, transform=ax.transAxes,
                color=(RED if t == "NO" else GREEN if t == "yes" else INK))
ax.text(0.0, 0.40, "IF A LEVEL CHANGE IS EVER ADDED\n(gallery, pit, raised pad):", fontsize=9, fontweight="bold", transform=ax.transAxes)
ax.text(0.0, 0.05, "· Ramp: <= 20° (R-042 wins over the 30° in the task brief). One 600 storey over\n"
        "  5 cells = 2000 cm run -> 16.7°; 4 cells (1600) = 20.6° -> too steep.\n"
        "· Stairs: risers <= 20 cm, never the 32-38 cm kit-storey splits; give the pawn a\n"
        "  ramp-proxy collision under them; that proxy must be <= 20°, so treads >= 55 cm\n"
        "  (20/55 = 20°), e.g. 30 risers x 20 = 600 over 30 treads x 55 = 1650.\n"
        "· Landings 400 x 400 (one cell) top and bottom; handrail 110 (walkway rail).\n"
        "· Collision = visual floor (R-043); the pawn's default step-up is 45 cm, so a\n"
        "  20 cm riser is never a wall, and nothing over 45 cm is climbable.", fontsize=8.3, transform=ax.transAxes, va="bottom")
# ramp diagram
ins = ax.inset_axes([0.5, 0.36, 0.5, 0.13]); ins.set_facecolor(BG); ins.set_aspect("equal")
ins.add_patch(Polygon([(0, 0), (2000, 600), (2000, 0)], fc="#c9c6bd", ec=INK)); ins.text(900, 380, "16.7°", fontsize=7)
for k in range(30): ins.add_patch(Rectangle((k * 55 + 2200, 0), 55, (k + 1) * 20, fc="#8d8a82", ec="none"))
ins.text(2900, 640, "30 x 20/55 stairs", fontsize=6.5); ins.text(300, 640, "ramp 2000 / 600", fontsize=6.5)
figure_side(ins, 1000 * 0 + 120, 0, label=False); ins.set_xlim(-50, 3950); ins.set_ylim(-20, 760); ins.axis("off")
clipall(fig); fig.subplots_adjust(left=0.05, right=0.96, top=0.93, bottom=0.05, hspace=0.28, wspace=0.18); fig.savefig(os.path.join(OUT, "DI_Sections_Cross.png"), facecolor=BG); plt.close(fig)
print("plans ok", SIGHT)
