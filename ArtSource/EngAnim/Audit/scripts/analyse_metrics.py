# Reads Saved/EngAnim/metrics.json and prints, per clip: additive setup, loop pop, right-hand peak speed time and
# furthest forward reach time (hit-frame estimates), and for loops the natural ground speed from the planted foot.
import json, math, sys
rows = json.load(open(sys.argv[1] if len(sys.argv) > 1 else "Saved/EngAnim/metrics.json"))
for r in rows:
    n = r["path"].split("/")[-1]; L = r["len"]; nf = r["frames"]; dt = L / nf if nf else 0
    pos = r["pos"]
    def sub(a, b): return [a[i] - b[i] for i in range(3)]
    def ln(a): return math.sqrt(sum(c * c for c in a))
    out = "%-28s len %.3f f%3d add=%s/%s ref=%s rm=%d pop %.1f(%s)" % (n, L, nf, r["additive"].split(".")[-1].split(":")[0].replace("AAT_", ""),
        r["ref_pose_type"].split(".")[-1].split(":")[0].replace("ABPT_", ""), r["ref_pose_seq"], r["rootmotion"], r["loop_pop_cm"], r["loop_pop_bone"])
    if "hand_r" in pos and nf > 1:
        for hand in ("hand_r", "hand_l"):
            rel = [sub(h, p) for h, p in zip(pos[hand], pos["root"])]
            sp = [ln(sub(rel[i + 1], rel[i])) / dt for i in range(nf)]
            i = max(range(nf), key=lambda k: sp[k])
            reach = [ln(sub(h, p)[:2] + [0]) for h, p in zip(pos[hand], pos["pelvis"])]
            j = max(range(nf + 1), key=lambda k: reach[k])
            out += " | %s vmax %4.0f cm/s @%.2fs reach %3.0f @%.2fs" % (hand[-1], sp[i], (i + 0.5) * dt, reach[j], j * dt)
        # ground speed: speed of the lower foot relative to the root, horizontally
        gs = []
        for i in range(nf):
            fl, fr = pos["foot_l"], pos["foot_r"]
            f = fl if fl[i][2] <= fr[i][2] else fr
            gs.append(ln(sub(f[i + 1], f[i])[:2] + [0]) / dt)
        gs.sort()
        out += " | stance foot speed median %4.0f" % gs[len(gs) // 2]
        pz = [p[2] for p in pos["pelvis"]]
        out += " pelvis z %.0f..%.0f" % (min(pz), max(pz))
        rt = [ln(sub(p, pos["root"][0])) for p in pos["root"]]
        out += " root travel %.0f" % max(rt)
    print(out)
