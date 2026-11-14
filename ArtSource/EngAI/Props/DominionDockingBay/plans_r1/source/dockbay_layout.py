"""Dominion docking bay r1: the measured layout, single source of truth for the plans and the Blender blockout.

Coordinates: Unreal cm, the interior COURSE-LOCAL frame of T5's ATDPlanetCourse::BuildInterior
(Source/unrealGameJam/TriadDrift/Ground/Interior/TDDominionInteriorCourse.inl): +X deeper into the ship, floor top
z = 0, 400 cm grid, 600 cm storeys. Kit pieces (ArtSource/EngProps/DominionInterior) are placed BY PIVOT at scale 1,
exactly as the course's Kit()/Room() helpers do.

Hull mapping (Dominion local, hull r1/r2 SOCKET_Bay at (30000, 0, -2000), +X out of the mouth):
    hull_x = 22800 - x,  hull_y = -y,  hull_z = z - 3750      (a 180 deg yaw plus an offset)
so the mouth plane x = -7200 is hull X 30000, the pocket floor z = 0 is hull Z -3750 (the mouth sill), and the
pocket back wall x = -1200 is hull X 24000 (the r1/r2 aft bulkhead).

Run: python3 dockbay_layout.py <out.json>
"""
import json, sys

GRID, STOREY = 400.0, 600.0

# --- T5's hangar, as built on main (unchanged footprint, so gameplay keeps every coordinate) -------------------
HX0, HX1, HY, H_STOREYS = -1200.0, 1200.0, 1400.0, 2
# --- The glide pocket (new, walkable floor, kit only): the hull's 60 m pocket on the 400 grid ---------------------
PX0, PX1, PY, P_STOREYS = -7200.0, -1200.0, 2600.0, 6        # 6000 x 5200 x 3600 (hull pocket 6000 x 5000 x 3500)
# --- The mouth (hull r1/r2 and ATDDockingBay) -------------------------------------------------------------------
MOUTH_W, MOUTH_H = 5000.0, 3500.0                              # DoorHalfWidth 2500 x2; door height 2 x 0.7 x 2500
MOUTH_C = (PX0, 0.0, MOUTH_H / 2)                              # = SOCKET_Bay (30000, 0, -2000) in hull space
DOOR_HALF = 2500.0
DOOR_DEPTH = 300.0
GLIDE_DEPTH_CPP = 6000.0
GLIDE_DEPTH_RECOMMENDED = 5000.0
# --- Corridor stub after the hangar door (T5: deflect corridor to x 5200) ---------------------------------------
CX1_STUB = 2000.0

def hull(x, y, z):
    return (22800.0 - x, -y, z - 3750.0)

P = []   # placements
def put(mesh, x, y, z=0.0, yaw=0.0, group="", note=""):
    P.append(dict(mesh=mesh, x=round(x, 3), y=round(y, 3), z=round(z, 3), yaw=yaw, group=group, note=note))

def cells(a, b):
    v = a + GRID / 2
    while v < b:
        yield v
        v += GRID

# Hangar (mirrors Room(HX0, HX1, HY, 2, bDoorAtX0=false, bDoorAtX1=true) but with the X0 end wall REMOVED: the
# hangar opens onto the pocket. That is the only change to T5's hangar.)
for x in cells(HX0, HX1):
    for y in cells(-HY, HY):
        put("SM_DI_Room_Floor", x, y, 0, 0, "hangar")
        put("SM_DI_Room_Ceiling", x, y, (H_STOREYS - 1) * STOREY, 0, "hangar")
for s in range(H_STOREYS):
    z = s * STOREY
    for x in cells(HX0, HX1):
        put("SM_DI_Room_Wall", x, -HY, z, 90, "hangar")
        put("SM_DI_Room_Wall", x, HY, z, 90, "hangar")
    for y in cells(-HY, HY):
        if not (s == 0 and abs(y) < 1):
            put("SM_DI_Room_Wall", HX1, y, z, 0, "hangar")
put("SM_DI_Doorway", HX1, 0, 0, 0, "hangar", "T5 HangarDoor: 300 x 400 clear, recognises the pilot")
put("SM_DI_Door", HX1, 0, 0, 0, "hangar", "slides +Z 400")
put("SM_DI_Hangar_Pad", -200, 0, 0.5, 0, "pads", "Pad_A (T5): the ship sits here")
put("SM_DI_Console", -900, 900, 0, 180, "hangar", "T5 StoryConsole A4_Recording_Hangar")

# Pocket
for x in cells(PX0, PX1):
    for y in cells(-PY, PY):
        put("SM_DI_Room_Floor", x, y, 0, 0, "pocket")
        put("SM_DI_Room_Ceiling", x, y, (P_STOREYS - 1) * STOREY, 0, "pocket")
for s in range(P_STOREYS):
    z = s * STOREY
    for x in cells(PX0, PX1):
        put("SM_DI_Room_Wall", x, -PY, z, 90, "pocket")
        put("SM_DI_Room_Wall", x, PY, z, 90, "pocket")
    for y in cells(-PY, PY):                     # back wall at x = -1200 around the hangar opening (2800 x 1200)
        if abs(y) > HY or s >= H_STOREYS:
            put("SM_DI_Room_Wall", PX1, y, z, 0, "pocket")
# Service gantry along both pocket walls at storey 2 (dressing, unreachable: the rail faces the pocket)
for x in cells(PX0, PX1):
    put("SM_DI_Walkway_Ring", x, -(PY - 30 - 150), 2 * STOREY, 0, "gantry", "rail +Y, toward the pocket")
    put("SM_DI_Walkway_Ring", x, (PY - 30 - 150), 2 * STOREY, 180, "gantry", "rail toward the pocket")
# Pads B and C (dressing: a parked Null fighter on each; the pawn collides with parked ships, T1)
put("SM_DI_Hangar_Pad", -2600, -1600, 0.5, 0, "pads", "Pad_B (dressing)")
put("SM_DI_Hangar_Pad", -2600, 1600, 0.5, 0, "pads", "Pad_C (dressing)")
# Deck-crew cover crates beside the pads, outside the glide lane (|y| < 900 kept clear)
for (x, y) in ((-3700, -2150), (-3700, 2150), (-1600, -2150), (-1600, 2150)):
    put("SM_DI_Cover_Low", x, y, 0, 90, "dressing")
# Pocket consoles at the hangar opening (dock control), 150 cm clear in front (screen faces -X: user stands at -X)
put("SM_DI_Console", -1500, -1800, 0, 0, "dressing", "dock control port; user at x <= -1530")
put("SM_DI_Console", -1500, 1800, 0, 0, "dressing", "dock control starboard")
# Corridor stub after the hangar door (T5's deflect corridor continues to x 5200)
for x in cells(HX1, CX1_STUB):
    put("SM_DI_Corridor_Straight", x, 0, 0, 0, "corridor")

LAYOUT = dict(
    frame="Unreal cm, T5 interior course-local (+X deeper in, floor z 0); hull = (22800 - x, -y, z - 3750)",
    grid=GRID, storey=STOREY,
    hangar=dict(x=[HX0, HX1], y=[-HY, HY], clear_height=H_STOREYS * STOREY, storeys=H_STOREYS,
                note="T5 as built; only change: no X0 end wall (opens onto the pocket)"),
    pocket=dict(x=[PX0, PX1], y=[-PY, PY], clear_height=P_STOREYS * STOREY, storeys=P_STOREYS,
                inner_wall_y=PY - 30, hull_pocket="X 24000..30000, |Y| 2500, Z -3750..-250"),
    hangar_opening=dict(y=[-HY, HY], z=[0, H_STOREYS * STOREY], width=2 * HY, height=H_STOREYS * STOREY),
    mouth=dict(centre=MOUTH_C, width=MOUTH_W, height=MOUTH_H, hull_centre=hull(*MOUTH_C), facing="-X here, +X hull"),
    doors=dict(half_width=DOOR_HALF, height=MOUTH_H, depth=DOOR_DEPTH,
               closed_centres_y=[-DOOR_HALF / 2, DOOR_HALF / 2], open_centres_y=[-1.5 * DOOR_HALF, 1.5 * DOOR_HALF],
               open_span_y=[DOOR_HALF, 2 * DOOR_HALF]),
    glide=dict(start=MOUTH_C, end_cpp=(PX0 + GLIDE_DEPTH_CPP, 0, MOUTH_H / 2),
               end_recommended=(PX0 + GLIDE_DEPTH_RECOMMENDED, 0, MOUTH_H / 2)),
    pads=dict(A=(-200, 0), B=(-2600, -1600), C=(-2600, 1600), size=1200),
    force_field=dict(x=PX0 + 20, y=[-PY, PY], z=[0, P_STOREYS * STOREY],
                     note="BlockingVolume (pawn only; ships and camera pass) just inside the mouth"),
    start=(-600, 0, 40), console=(-900, 900), hangar_door=(HX1, 0),
    ship_footprint=dict(at=(-200, 0), size=(524, 488, 230), note="largest squad ship (Lancer/Bastion 524 long, Bastion 488 wide)"),
    figure_cm=180,
    placements=P,
)

if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else "dockbay_layout.json"
    json.dump(LAYOUT, open(out, "w"), indent=1)
    from collections import Counter
    c = Counter(p["mesh"] for p in P)
    print(len(P), "placements"); [print(" ", k, v) for k, v in sorted(c.items())]
