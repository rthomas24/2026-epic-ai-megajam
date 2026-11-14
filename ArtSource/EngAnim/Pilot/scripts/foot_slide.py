# Triad Drift - foot slide from TD.Pilot.FootTrace log lines: per foot, the spans where it is planted (within 2.5 cm of
# its lowest height over the trace's local window and moving < 60 cm/s vertically), and how far it slides
# horizontally in world space while planted (cm, and cm per planted second). python foot_slide.py <log> [since HH.MM.SS]
# Animation builder (eng/anim), Nov 14.
import math
import re
import sys

PAT = re.compile(r"\[(\d{4}\.\d\d\.\d\d-(\d\d)\.(\d\d)\.(\d\d):\d+)\].*TD\.FootTrace t=([\d.]+) L=([-\d.,]+) R=([-\d.,]+) V=([-\d.,]+) yaw=[-\d.]+ loop=(\S+) rate=([\d.]+) legs=(\S+) upper=(\S+) shot=(\S+)")
rows = []
since = sys.argv[2] if len(sys.argv) > 2 else None
for line in open(sys.argv[1], errors="replace"):
    m = PAT.search(line)
    if not m:
        continue
    if since and "%s.%s.%s" % (m.group(2), m.group(3), m.group(4)) < since:
        continue
    f = lambda g: [float(x) for x in m.group(g).split(",")]
    rows.append({"t": float(m.group(5)), "L": f(6), "R": f(7), "V": f(8), "loop": m.group(9), "legs": m.group(11), "shot": m.group(13)})
# split into runs (a gap > 0.5 s game starts a new trace)
runs, cur = [], []
for r in rows:
    if cur and r["t"] - cur[-1]["t"] > 0.5:
        runs.append(cur); cur = []
    cur.append(r)
if cur:
    runs.append(cur)
for ri, run in enumerate(runs):
    print("trace %d: %.2f s, %d frames, speed %.0f..%.0f cm/s, loops %s, legs %s, shots %s" % (
        ri, run[-1]["t"] - run[0]["t"], len(run), min(math.hypot(*r["V"]) for r in run), max(math.hypot(*r["V"]) for r in run),
        sorted(set(r["loop"] for r in run)), sorted(set(r["legs"] for r in run)), sorted(set(r["shot"] for r in run))))
    for foot in ("L", "R"):
        total, planted_t, spans, worst = 0.0, 0.0, 0, (0.0, None)
        i = 0
        while i < len(run) - 1:
            # local floor: lowest z within +-0.4 s
            def low(k):
                t = run[k]["t"]
                return min(r[foot][2] for r in run if abs(r["t"] - t) <= 0.4)
            a, b = run[i], run[i + 1]
            dt = max(1e-4, b["t"] - a["t"])
            planted = a[foot][2] < low(i) + 2.5 and b[foot][2] < low(i + 1) + 2.5 and abs(b[foot][2] - a[foot][2]) / dt < 60
            if planted:
                d = math.hypot(b[foot][0] - a[foot][0], b[foot][1] - a[foot][1])
                total += d; planted_t += dt
                if d / dt > worst[0]:
                    worst = (d / dt, "t=%.2f loop=%s legs=%s shot=%s" % (a["t"], a["loop"], a["legs"], a["shot"]))
            i += 1
        print("  foot %s: planted %.2f s, slid %.1f cm (%.0f cm/s average while planted); worst %.0f cm/s at %s" % (
            foot, planted_t, total, total / planted_t if planted_t else 0, worst[0], worst[1]))
