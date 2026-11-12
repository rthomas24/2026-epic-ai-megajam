"""Build the offline M02 review page from measured reports; no Blender required."""
from pathlib import Path
import hashlib
import html
import json

HERE=Path(__file__).resolve().parent
OUT=HERE.parent/"review/M02"
qa=json.loads((OUT/"validation_summary.json").read_text())
renders=json.loads((OUT/"render_manifest.json").read_text())
titles=[("01_hero_front_3q","Front three-quarter"),("02_front","Front"),("03_rear","Rear"),
        ("04_side","Starboard"),("05_top","Top"),("06_rear_3q","Rear three-quarter"),
        ("07_underside","Underside & mounts"),("08_weapon_detail","Weapon modules"),("09_rocket_visual","Spike rocket visual")]
cards="".join(f'<article><a href="{name}.png"><img src="{name}.png" alt="{title} Lancer M02 render"></a><h3>{i:02d} / {title.upper()}</h3></article>' for i,(name,title) in enumerate(titles,1))
dimensions=" × ".join(f"{d/100:.2f}" for d in qa["dimensions_cm"])
status="Geometry checks passed" if qa["technical_pass"] else "Geometry checks need repair — see report"
body=f'''<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Lancer M02 / Production review</title><style>
*{{box-sizing:border-box}}body{{margin:0;background:#08131d;color:#dceaf1;font:16px/1.6 Segoe UI,Arial,sans-serif}}header,main,footer{{max-width:1500px;margin:auto;padding:28px 34px}}header{{border-bottom:1px solid #29404e}}h1{{font-size:36px;letter-spacing:4px;margin:5px 0}}.eyebrow{{color:#45d6e8;font-size:12px;letter-spacing:3px}}h2{{margin:28px 0 12px}}a{{color:#77ddeb}}.hero{{display:block;width:100%;border-radius:12px}}.metrics{{display:flex;gap:15px;flex-wrap:wrap;margin:22px 0}}.metrics div{{flex:1;min-width:180px;padding:16px;background:#132934;border:1px solid #2a4956;border-radius:9px}}.metrics b{{display:block;color:#baeff8;font-size:23px}}.metrics span{{font-size:12px;color:#9fb8c5}}.notice{{border-left:3px solid #5bcbdc;background:#102a36;padding:18px 24px;margin:25px 0}}.notice p{{margin:6px 0}}.grid{{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:22px}}article{{background:#112631;border:1px solid #294754;border-radius:10px;overflow:hidden}}article img{{width:100%;display:block}}article h3{{font-size:14px;letter-spacing:1px;margin:14px 20px}}.links{{display:flex;flex-wrap:wrap;gap:15px;margin:25px 0}}footer{{font-size:13px;color:#91adbd;border-top:1px solid #29404e}}@media(max-width:750px){{.grid{{grid-template-columns:1fr}}header,main,footer{{padding:20px}}h1{{font-size:28px}}}}
</style></head><body><header><div class="eyebrow">TRIAD DRIFT / SHIPS & WEAPONS</div><h1>LANCER / M02</h1><div>Production refinement · updated visual approval pending</div></header><main>
<a href="01_hero_front_3q.png"><img class="hero" src="01_hero_front_3q.png" alt="Lancer M02 front three-quarter"></a>
<div class="metrics"><div><b>{qa["assembly_triangles"]:,}</b><span>ASSEMBLED TRIANGLES</span></div><div><b>{dimensions} m</b><span>LENGTH × SPAN × HEIGHT</span></div><div><b>{qa["materials"]} / 33</b><span>MATERIALS / PRESERVED ATTACHMENTS</span></div></div>
<div class="notice"><p><strong>{html.escape(status)}</strong></p><p>General design and modular layout approved; this new level of polish still needs Nicholas's review. The existing M01 prototype, review evidence and attachment manifest are preserved. Nothing has been imported into Unreal.</p><p>Refined swept shoulders, airfoil wings, tapered weapon cradles, framed radiator banks, machined drive hardware and restrained cyan livery. Six PBR materials use panel tint variation; no external textures or downloaded models.</p><p>Engineering selected a +54 cm local visual offset for SpikeRockets; gameplay muzzle/collision stay unchanged. Static assembly uses the hull plus ten flattened visual parts.</p></div>
<h2>Multi-angle inspection</h2><p>Click any image for full resolution. Orthographic views are framed independently; see measured dimensions for scale.</p><div class="grid">{cards}</div>
<h2>Points for your decision</h2><p>Independent review confirms these angles are sufficient for inspection. Remaining art concerns are the abrupt wing-root transitions, simple weapon-support blocks, a less resolved underside, and strong canopy/gun gloss against uniform armour. This is a technically verified refinement candidate; final visual quality is not self-approved. The dark underside material view is supplemented by clay.</p>
<h2>Form and topology</h2><a href="Lancer_M02_clay_sheet.png"><img class="hero" src="Lancer_M02_clay_sheet.png" alt="Six-angle clay review"></a>
<div class="links"><a href="Lancer_M02_overview.png">One-image overview</a><a href="validation_summary.json">Measured QA</a><a href="geometry_qa.json">Full geometry checks</a><a href="../../production/README.md">Source and integration notes</a><a href="../../production/material_parameters.json">Material settings</a><a href="../../production/unreal_visual_parts.json">Derived visual assembly</a><a href="../index.html">Historical M01 review</a></div>
</main><footer>Ships & Weapons Agent · nick/ships-lancer · Original direction approved, final polish pending · No gameplay changes, Unreal import or main merge.</footer></body></html>'''
(OUT/"index.html").write_text(body,encoding="utf-8")
files=list(OUT.glob("*.png"))
manifest={"images":[dict(file=p.name,bytes=p.stat().st_size,sha256=hashlib.sha256(p.read_bytes()).hexdigest()) for p in files],
          "technical_pass":qa["technical_pass"],"visual_approval":"pending human review"}
(OUT/"delivery_manifest.json").write_text(json.dumps(manifest,indent=2))
print(json.dumps(dict(html=str(OUT/"index.html"),images=len(files),technical_pass=qa["technical_pass"])))
