# Triad Drift - does the pilot's hilt or blade pass through a leg? Reads a grip_capture.py <tag>_hands.json (poses held
# through a clip) and reports, per pose, the gap between the hilt (pommel -8.5 cm .. emitter +19.3 cm, radius 2 cm) and
# the blade (to +114 cm, radius 2 cm) and each leg segment (thigh radius 10 cm, calf 7.5 cm: armour included).
# python blade_clearance.py <hands.json>          Animation builder (eng/anim), Nov 13.
import json
import math
import sys


def sub(a, b): return [a[i] - b[i] for i in range(3)]
def add(a, b): return [a[i] + b[i] for i in range(3)]
def mul(a, s): return [a[i] * s for i in range(3)]
def dot(a, b): return sum(a[i] * b[i] for i in range(3))


def seg_dist(p1, q1, p2, q2):
    """Closest distance between segments p1q1 and p2q2."""
    d1, d2, r = sub(q1, p1), sub(q2, p2), sub(p1, p2)
    a, e, f = dot(d1, d1), dot(d2, d2), dot(d2, r)
    c = dot(d1, r)
    b = dot(d1, d2)
    den = a * e - b * b
    s = min(1.0, max(0.0, (b * f - c * e) / den)) if den > 1e-9 else 0.0
    t = (b * s + f) / e if e > 1e-9 else 0.0
    if t < 0.0:
        t, s = 0.0, min(1.0, max(0.0, -c / a))
    elif t > 1.0:
        t, s = 1.0, min(1.0, max(0.0, (b - c) / a))
    v = sub(add(p1, mul(d1, s)), add(p2, mul(d2, t)))
    return math.sqrt(dot(v, v))


def main():
    data = json.load(open(sys.argv[1]))
    worst = {}
    for pose, r in data.items():
        b = r["bones"]
        if "thigh_r" not in b:
            print("no leg bones in", pose)
            continue
        h, x = b["hilt"]["loc"], b["hilt"]["x"]
        parts = {"hilt": (add(h, mul(x, -8.5)), add(h, mul(x, 19.3))), "blade": (add(h, mul(x, 19.3)), add(h, mul(x, 114.0)))}
        legs = {"thigh_r": ("thigh_r", "calf_r", 10.0), "calf_r": ("calf_r", "foot_r", 7.5), "thigh_l": ("thigh_l", "calf_l", 10.0), "calf_l": ("calf_l", "foot_l", 7.5)}
        line = []
        for pn, (p, q) in parts.items():
            gaps = {ln: seg_dist(p, q, b[a]["loc"], b[c]["loc"]) - rad - 2.0 for ln, (a, c, rad) in legs.items()}
            ln = min(gaps, key=gaps.get)
            line.append("%s %+.1f cm (%s)" % (pn, gaps[ln], ln))
            worst[pn] = min(worst.get(pn, 99.0), gaps[ln])
        print("%-10s %s" % (pose, " | ".join(line)))
    print("worst gap: " + ", ".join("%s %+.1f cm" % kv for kv in worst.items()) + "  (negative = inside the leg)")


if __name__ == "__main__":
    main()
