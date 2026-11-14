"""manifest.json for plans_r1 (sizes + sha256 of every shipped file, key numbers, C++ mismatches)."""
import os, sys, json, hashlib
P = sys.argv[1]
L = json.load(open(os.path.join(P, "dockbay_layout.json"))); E = json.load(open(os.path.join(P, "mesh", "exterior_report.json")))
files = []
for root, _, fs in os.walk(P):
    for f in sorted(fs):
        if f == "manifest.json" or f.endswith((".pyc", ".blend1")): continue
        p = os.path.join(root, f); files.append(dict(path=os.path.relpath(p, P), bytes=os.path.getsize(p),
                                                     sha256=hashlib.sha256(open(p, "rb").read()).hexdigest()))
files.sort(key=lambda d: d["path"])
m = dict(
    asset="DominionDockingBay", revision="plans_r1", issue=54, board="R-043", route="C procedural (Blender 4.5.3 + matplotlib), no AI mesh",
    units="Unreal cm; FBX UnitScaleFactor 100, import scale 1.0",
    frame=L["frame"], grid_cm=L["grid"], storey_cm=L["storey"],
    hull=dict(revision="r2 (merged PR #72)", socket_bay=[30000, 0, -2000], mouth_cm=[5000, 3500], facing="+X", pocket_hull="X 24000..30000, |Y| 2500, Z -3750..-250"),
    interior=dict(mouth=L["mouth"], pocket=L["pocket"], hangar=L["hangar"], pads=L["pads"], force_field=L["force_field"],
                  threshold=dict(floor_band_x=[-7200, -6800], width=5000, glow_strip_x=-6800, jambs_y=[2460, 2500], header_z=[3460, 3500],
                                 texture="T_DominionBay_Trim_BC hazard band (px 640-896)", note="Lead N-061"),
                  kit="ArtSource/EngProps/DominionInterior", placements=len(L["placements"]), figure_cm=180),
    exterior_pieces=E,
    cpp=dict(DoorHalfWidth=2500, door_height=3500, door_depth=300, ConeDepth=300000, ConeHalfAngleDeg=35, GlideDepth=6000, GlideSeconds=2,
             MouthDistance=30000, MouthRadius=6000),
    mismatches=[
        "ATDDominion::BeginDockingFinale spawns the bay at local (Bounds.Max.X, 0, lerp(Min.Z, Center.Z, 0.6)) = hull r2 (33400, 0, -2933): 34 m in front of SOCKET_Bay (30000, 0, -2000) and 9.3 m low; GetEscapeStart uses the same formula. Use SOCKET_Bay.",
        "GlideDepth 6000 from SOCKET_Bay ends on the aft bulkhead (hull X 24000): ship nose in the wall. Recommend 5000.",
        "Placeholder Frame (150 x 6500 x 4550) should be hidden with the art hull (hull r2 models the mouth frame).",
        "Kit pocket 5200 x 3600 vs hull pocket 5000 x 3500 (grid rounding, hidden behind the hull frame).",
        "DoorHalfWidth 2500 / door height 3500 / depth 300 / open travel |Y| 5000 agree with hull r2.",
        "Reference board opening 28 x 12 m and module 36 x 18 x 20 m are overridden by hull r2 + C++ (50 x 35 m mouth).",
    ],
    files=files)
json.dump(m, open(os.path.join(P, "manifest.json"), "w"), indent=1)
print("manifest", len(files), "files")
