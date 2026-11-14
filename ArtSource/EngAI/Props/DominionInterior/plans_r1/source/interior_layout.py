"""Dominion interior r1: the measured layout of the whole on-foot route, single source of truth for the plans and the
Blender blockout (R-042 queue item 3, R-043: builders build the walkable interior from the DominionInterior kit).

Coordinates: Unreal cm, the interior COURSE-LOCAL frame of T5's ATDPlanetCourse::BuildInterior
(Source/unrealGameJam/TriadDrift/Ground/Interior/TDDominionInteriorCourse.inl, main 188acc50): +X deeper into the ship,
floor top z = 0, 400 cm grid, 600 cm storeys. Kit pieces (ArtSource/EngProps/DominionInterior) are placed BY PIVOT at
scale 1, exactly as the course's Kit()/Room()/CorridorRun()/Doorway() helpers do. Every gameplay number below is copied
from that file (line numbers in comments) so the plan and the game agree; RECOMMENDED changes are kept separate.

The docking-bay glide pocket and the hangar with its X0 end wall removed come unchanged from PR #77
(ArtSource/EngAI/Props/DominionDockingBay/plans_r1/source/dockbay_layout.py, merged), imported below.
Hull mapping (Dominion local, hull r2): hull_x = 22800 - x, hull_y = -y, hull_z = z - 3750.

Run: python3 interior_layout.py <out.json> [<repo_root>]
"""
import json, sys, os, math, importlib.util

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(sys.argv[2]) if len(sys.argv) > 2 else os.path.abspath(os.path.join(HERE, "../../../../../.."))
spec = importlib.util.spec_from_file_location("dockbay_layout", os.path.join(REPO, "ArtSource/EngAI/Props/DominionDockingBay/plans_r1/source/dockbay_layout.py"))
DB = importlib.util.module_from_spec(spec); spec.loader.exec_module(DB)

GRID, STOREY = 400.0, 600.0
# --- TDDominionInteriorCourse.inl lines 205-213 ----------------------------------------------------------------
HX0, HX1, HY = -1200.0, 1200.0, 1400.0          # hangar 24 x 28 m, 2 storeys
CX1 = 5200.0                                     # deflect corridor 1200..5200 (40 m, clear 340)
AX1, AY = 7600.0, 1400.0                         # arena / flight deck 5200..7600 (24 x 28 m), 1 storey
L1X1 = 8400.0                                    # link 7600..8400
WX1, WY = 10400.0, 1000.0                        # Warden room 8400..10400 (20 x 20 m), 1 storey
L2X1 = 11200.0                                   # link 10400..11200
KX1, KY = 14800.0, 1800.0                        # core chamber 11200..14800 (36 x 36 m), 3 storeys
CY = 170.0                                       # corridor clear half width
CORE = ((L2X1 + KX1) / 2, 0.0)                   # (13000, 0)

ROOMS = [  # name, x0, x1, half_y, storeys, door_x0, door_x1, role (story-0708-actions.md reduced form)
    ("Hangar", HX0, HX1, HY, 2, False, True, "hangar; pilot start; console A4_Recording_Hangar; opens onto the PR #77 glide pocket"),
    ("Arena", CX1, AX1, AY, 1, True, True, "flight deck = 'barracks' (3 waves); consoles _Corridor (entrance) and _Barracks (exit)"),
    ("Warden", L1X1, WX1, WY, 1, True, True, "Warden's gate = 'security checkpoint' (door shows ACCESS GRANTED text)"),
    ("Core", L2X1, KX1, KY, 3, True, False, "reactor core chamber; console A4_Recording_Core; future-crew trio (N-052)"),
]
CORRIDORS = [("Deflect corridor", HX1, CX1), ("Link 1", AX1, L1X1), ("Link 2", WX1, L2X1)]
DOORS = [("HangarDoor", HX1), ("ArenaIn", CX1), ("ArenaOut", AX1), ("WardenIn", L1X1), ("WardenOut", WX1), ("CoreIn", L2X1)]

P = []
def put(mesh, x, y, z=0.0, yaw=0.0, group="", note=""):
    P.append(dict(mesh=mesh, x=round(x, 3), y=round(y, 3), z=round(z, 3), yaw=yaw, group=group, note=note))
def cells(a, b):
    v = a + GRID / 2
    while v < b:
        yield v; v += GRID

# 1) PR #77 pocket + hangar (its X0 wall already removed), minus its 2-cell corridor stub (we lay the full corridor)
for p in DB.P:
    if p["group"] == "corridor": continue
    g = p["group"]
    P.append(dict(p, group=("bay_" + g) if g in ("pocket", "gantry", "pads", "dressing") else g))

# 2) Rooms as Room() builds them (lines 94-119); the hangar is already in from PR #77
def room(name, x0, x1, hy, storeys, d0, d1):
    g = name.lower()
    for x in cells(x0, x1):
        for y in cells(-hy, hy):
            put("SM_DI_Room_Floor", x, y, 0, 0, g)
            put("SM_DI_Room_Ceiling", x, y, (storeys - 1) * STOREY, 0, g)
    for s in range(storeys):
        z = s * STOREY
        for x in cells(x0, x1):
            put("SM_DI_Room_Wall", x, -hy, z, 90, g); put("SM_DI_Room_Wall", x, hy, z, 90, g)
        for y in cells(-hy, hy):
            mid = s == 0 and abs(y) < 1
            if not (mid and d0): put("SM_DI_Room_Wall", x0, y, z, 0, g)
            if not (mid and d1): put("SM_DI_Room_Wall", x1, y, z, 0, g)
for (n, x0, x1, hy, st, d0, d1, _) in ROOMS[1:]:
    room(n, x0, x1, hy, st, d0, d1)
# 3) Corridors (CorridorRun, lines 121-137)
for (n, x0, x1) in CORRIDORS:
    for x in cells(x0, x1):
        put("SM_DI_Corridor_Straight", x, 0, 0, 0, "corridor", n)
# 4) Doorways + doors (lines 139-163; the hangar door is already in from PR #77)
for (n, x) in DOORS[1:]:
    put("SM_DI_Doorway", x, 0, 0, 0, "doors", n + ": 300 x 400 clear, recognises the pilot")
    put("SM_DI_Door", x, 0, 0, 0, "doors", "slides +Z 400")
# 5) Props exactly as the C++ places them
put("SM_DI_Cover_Low", CX1 - 700, -70, 0, 0, "cpp_cover", "C++ line 287: troopers' cover (gap on +Y = 130 cm to the kick plate)")
for i in range(4):
    put("SM_DI_Cover_Low", CX1 + 700 + (i % 2) * 1000, (-1 if i < 2 else 1) * 500, 0, 90.0 * (i % 2), "cpp_cover", "arena cover (line 316)")
put("SM_DI_Console", CX1 + 250, -AY + 300, 0, 180, "consoles", "A4_Recording_Corridor (line 303)")
put("SM_DI_Console", AX1 - 300, AY - 300, 0, 0, "consoles", "A4_Recording_Barracks (line 324)")
put("SM_DI_Console", L2X1 + 400, -KY + 400, 0, 180, "consoles", "A4_Recording_Core (line 395)")
put("SM_DI_Core", CORE[0], 0, 0, 0, "core", "line 263")
put("SM_DI_Core_Ring_A", CORE[0], 0, 600, 0, "core", "spins 14 deg/s")
put("SM_DI_Core_Ring_B", CORE[0], 0, 600, 0, "core", "spins -9 deg/s")
put("SM_DI_Core_Shield", CORE[0], 0, 0, 0, "core", "drops when the 4 conduits break")
CONDUITS = []
for i in range(4):
    a = math.pi * 0.25 + i * math.pi * 0.5
    cx, cy = CORE[0] + math.cos(a) * 1100, math.sin(a) * 1100
    CONDUITS.append((round(cx, 1), round(cy, 1)))
    put("SM_DI_Conduit", cx, cy, 0, 0, "core", "shield conduit %d (ATDShatterCrystal, 80 HP)" % i)
HEART = (CORE[0] - 760, 0.0)

# --- gameplay volumes and points, copied from the C++ ------------------------------------------------------------
GAME = dict(
    pilot_start=(-600, 0, 140), pilot_capsule=dict(radius=34, half_height=90, note="TDFirstPersonCharacter.cpp:47"),
    crew_capsule=dict(radius=40, half_height=90, note="TDGroundActors.cpp:325"),
    entries={"Hangar": (-600, 0), "Arena": (CX1 + 400, 0), "Warden": (L1X1 - 300, 0), "Core": (L2X1 - 300, 0), "Escape (yaw 180)": (L2X1 + 300, 0)},
    checkpoints={"Corridor": (HX1 - 300, 0), "Arena 1/2": (CX1 + 400, 0), "Warden": (L1X1 + 300, 0), "Core": (L2X1 + 300, 0), "Escape (yaw 180)": (L2X1 + 300, 0)},
    trailheads={"THE HANGAR DOOR": (600, 0), "THE FLIGHT DECK": (CX1 + 900, 0), "THE GATE": (L1X1 + 400, 0), "THE CORE": (L2X1 + 500, 0)},
    volumes={
        "Lane encounter (2 troopers)": dict(c=((HX1 + CX1) / 2, 0, 200), e=((CX1 - HX1) / 2, CY, 400)),
        "IntoArena trigger": dict(c=(CX1 + 500, 0, 0), e=(300, 600, 400)),
        "Arena encounters (3 waves)": dict(c=((CX1 + AX1) / 2, 0, 200), e=((AX1 - CX1) / 2, AY, 500)),
        "Warden encounter": dict(c=((L1X1 + WX1) / 2, 0, 200), e=((WX1 - L1X1) / 2, WY, 500)),
        "Escape: ship trigger": dict(c=(-200, 0, 0), e=(500, 500, 400)),
        "Escape: collapse debris lane": dict(c=((HX0 + KX1) / 2, 0, 0), e=((KX1 - HX0) / 2, 150, 0)),
    },
    spawns={"Trooper A": (CX1 - 450, -100), "Trooper B": (CX1 - 450, 100), "Warden (capsule half-height 130)": (WX1 - 500, 0)},
    arena_spawn_radius=[800, 900, 900],
    core=dict(centre=CORE, conduits=CONDUITS, conduit_radius=1100, heart=HEART, heart_radius=80, shield_radius=640,
              ring_a_radius=500, ring_b_radius=575, core_radius=400, core_height=1200, shield_height=1380, rings_z=600),
    lights=[((0, 0, 1000), 3200, "white"), *[((x, 0, 520), 700, "red") for x in range(int(HX1 + 600), int(CX1), 1200)],
            (((CX1 + AX1) / 2, 0, 520), 2600, "white"), (((L1X1 + WX1) / 2, 0, 520), 2200, "warm red"), ((CORE[0], 0, 1500), 4200, "core orange")],
    consoles={"A4_Recording_Hangar": (-900, 900, 180), "A4_Recording_Corridor": (CX1 + 250, -AY + 300, 180),
              "A4_Recording_Barracks": (AX1 - 300, AY - 300, 0), "A4_Recording_Core": (L2X1 + 400, -KY + 400, 180)},
)
# --- kit facts used by the clearance checks (EngProps/DominionInterior/README.md) ----------------------------------
KIT = dict(corridor_inner_wall=170, corridor_kick_plate=160, corridor_rib=156, rib_underside=586, room_clear=600,
           door_clear=(300, 400), doorway_frame=(60, 400, 600), console=(60, 120, 130), cover=(60, 200, 110))
REC = dict(
    cover_corridor_y=-100,      # recommended: gap 0..160 = 160 cm (>= 140, R-042); C++ -70 gives 130
    ramp_max_deg=20, stair_max_riser=20, ramp_for_storey_cells=5,  # 600 cm over 2000 cm = 16.7 deg
)

def hull(x, y, z): return (22800.0 - x, -y, z - 3750.0)

LAYOUT = dict(
    frame="Unreal cm, T5 interior course-local (+X deeper in, floor z 0); hull = (22800 - x, -y, z - 3750)",
    source="TDDominionInteriorCourse.inl at main 188acc50; pocket + hangar from PR #77 dockbay_layout.py",
    grid=GRID, storey=STOREY,
    rooms=[dict(name=n, x=[x0, x1], y=[-hy, hy], size=[x1 - x0, 2 * hy], storeys=st, clear_height=st * STOREY, role=r)
           for (n, x0, x1, hy, st, d0, d1, r) in ROOMS],
    corridors=[dict(name=n, x=[x0, x1], length=x1 - x0, clear_width=2 * CY, clear_height=600) for (n, x0, x1) in CORRIDORS],
    doors=[dict(name=n, x=x, clear=[300, 400], frame=[60, 400, 600]) for (n, x) in DOORS],
    pocket=DB.LAYOUT["pocket"], mouth=DB.LAYOUT["mouth"], hangar_opening=DB.LAYOUT["hangar_opening"], pads=DB.LAYOUT["pads"],
    ship_footprint=DB.LAYOUT["ship_footprint"], glide=DB.LAYOUT["glide"],
    game=GAME, kit=KIT, recommended=REC, figure_cm=180, placements=P,
    route_length_cm=dict(start_to_core=CORE[0] - (-600), mouth_to_core=CORE[0] - (-7200)),
)

if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else "interior_layout.json"
    json.dump(LAYOUT, open(out, "w"), indent=1)
    from collections import Counter
    c = Counter(p["mesh"] for p in P)
    print(len(P), "placements"); [print(" ", k, v) for k, v in sorted(c.items())]
