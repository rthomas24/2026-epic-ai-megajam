"""manifest.json for DominionInterior/plans_r1 (sha256 of every shipped file, key numbers, mismatches)."""
import os, sys, json, hashlib
P = sys.argv[1]
L = json.load(open(os.path.join(P, "interior_layout.json"))); T = json.load(open(os.path.join(P, "textures", "trim_layout.json")))
files = []
for root, _, fs in os.walk(P):
    for f in sorted(fs):
        if f == "manifest.json" or f.endswith((".pyc", ".blend1")) or "__pycache__" in root: continue
        p = os.path.join(root, f); files.append(dict(path=os.path.relpath(p, P), bytes=os.path.getsize(p), sha256=hashlib.sha256(open(p, "rb").read()).hexdigest()))
files.sort(key=lambda d: d["path"])
m = dict(asset="DominionInterior", revision="plans_r1", board=["R-042 queue item 3", "R-043"], follows="#54 docking bay (PR #77)",
         route="C procedural (Blender 4.5.3 + matplotlib + numpy), no AI mesh or image, no fal spend",
         units="Unreal cm", frame=L["frame"], source=L["source"], grid_cm=L["grid"], storey_cm=L["storey"],
         rooms=L["rooms"], corridors=L["corridors"], doors=L["doors"], pocket=L["pocket"], hangar_opening=L["hangar_opening"],
         ship_footprint=L["ship_footprint"], game=L["game"], kit=L["kit"], recommended=L["recommended"],
         placements=len(L["placements"]), figure_cm=180,
         textures=dict(trim=dict(name="T_DI_InteriorTrim", maps=["BC (sRGB)", "N (DirectX)", "ORM (AO, rough, metal)", "E (sRGB colour)"], size=2048, px_per_cm=5.12, bands=T["trim"]["bands"]),
                       decals=dict(name="T_DI_Decals", maps=["BC (sRGB RGBA, alpha = opacity)", "N (DirectX)", "ORM", "E"], size=2048, rects=T["decals"]),
                       stats="review/tex_stats.txt (/tmp/audit-scripts/tex_stats.py)"),
         sightlines=json.load(open(os.path.join(P, "review", "sightlines.json"))),
         mismatches=json.load(open(os.path.join(P, "mismatches.json"))), files=files)
json.dump(m, open(os.path.join(P, "manifest.json"), "w"), indent=1); print("manifest", len(files), "files")
