import numpy as np, sys
d = np.load(sys.argv[1]); xs, ys, zs = d["xs"], d["ys"], d["zs"]
def art(m, lo, hi, rows, cols, title, flip_rows=True, s=2):
    print(title)
    ch = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    r = range(0, m.shape[0], s)
    for i in (reversed(r) if flip_rows else r):
        line = ""
        for j in range(0, m.shape[1], s):
            v = m[i, j]
            line += "." if np.isnan(v) else ch[int(np.clip((v - lo) / (hi - lo) * 35, 0, 35))]
        print("%+5.0f %s" % (rows[i] * 100, line))
which = sys.argv[2:]
# top: rows = y (flip), cols = x  -> transpose
if "top" in which: art(d["top"].T, -1.13, 1.13, ys, xs, "TOP ztop (rows y cm, cols x -252..252 step 8cm; char = z, 0=-113 Z=+113, each char 6.5cm)")
if "bot" in which: art(d["bot"].T, -1.13, 1.13, ys, xs, "BOTTOM zbot")
if "rear" in which: art(d["rear"].T, -2.5, 0.5, zs, ys, "REAR x of first hit from behind (rows z, cols y -212..212; 0=-250 Z=+50, char 8.6cm)", s=1 if "fine" in which else 2)
if "nose" in which: art(d["nose"].T, -0.5, 2.5, zs, ys, "NOSE x first hit from front (0=-50, Z=250)")
if "side" in which: art(d["side"].T, 0, 2.1, zs, xs, "SIDE y of first hit from +Y (rows z, cols x; 0=0 Z=210cm, char 6cm)")
if "center" in which:
    j0 = int(np.argmin(abs(ys)))
    for i in range(0, len(xs), 3):
        row = "x%+5.0f top0 %6.1f bot0 %6.1f |" % (xs[i]*100, d["top"][i, j0]*100, d["bot"][i, j0]*100)
        for yy in (0.12, 0.24, 0.36, 0.48, 0.6, 0.72, 0.84, 1.0, 1.2, 1.4, 1.6, 1.8, 2.0):
            j = int(np.argmin(abs(ys - yy))); t, b = d["top"][i, j], d["bot"][i, j]
            row += " ----/----" if np.isnan(t) else " %4.0f/%4.0f" % (t*100, b*100)
        print(row)
