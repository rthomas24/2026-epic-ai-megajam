"""Fill SOURCE.md from SOURCE_template.md + the json outputs. python3 make_source_md.py <plans_r1_dir>"""
import os, sys, json
P = sys.argv[1]; t = open(os.path.join(P, "source", "SOURCE_template.md")).read()
T = json.load(open(os.path.join(P, "textures", "trim_layout.json")))
t = t.replace("SIGHTLINES", "\n".join("- %s: %.1f m, %s" % (a, b / 100, c) for a, b, c in json.load(open(os.path.join(P, "review", "sightlines.json")))))
t = t.replace("MISMATCHES", "\n".join("%d. %s" % (i + 1, m) for i, m in enumerate(json.load(open(os.path.join(P, "mismatches.json"))))))
t = t.replace("BANDS", "\n".join("| %s | %d-%d | %.4f..%.4f | %d px = %.1f cm | %s |" % (b["key"], *b["v_px"], *b["uv_v"], b["height_px"], b["height_cm"], b["desc"]) for b in T["trim"]["bands"]))
t = t.replace("DECALS", "\n".join("- **%s** px %s, UV (%.2f, %.2f)-(%.2f, %.2f): %s" % (d["key"], d["px"], *d["uv"], d["desc"]) for d in T["decals"]))
t = t.replace("TEXSTATS", open(os.path.join(P, "review", "tex_stats.txt")).read().strip())
open(os.path.join(P, "SOURCE.md"), "w").write(t); print("SOURCE.md ok")
