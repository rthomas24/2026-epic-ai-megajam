"""T_DI_InteriorTrim_* (2K tiling trim sheet) and T_DI_Decals_* (2K decal atlas) for the Dominion interior.
Procedural, seeded, numpy only (route C, no AI image). Palette: Dominion gunmetal (BC median >= sRGB 55), crimson bands,
trident emblem, red-orange / amber / cyan / white emissive strips, hazard markings (R-042 queue item 3, R-043 trims).

TRIM SHEET: 2048 x 2048, every band is periodic in U over the full width (tiles horizontally). Texel density 5.12 px/cm
(2048 px = 400 cm = one kit cell), so a wall UV'd at 1 tile per cell lines up with the kit grid. Bands (V from the top, px):
""" 
import sys, os, json
import numpy as np
from PIL import Image

N = 2048
BANDS = [  # key, y0, y1, description
    ("WALL_PANEL", 0, 512, "armour wall panels, gunmetal, 4 plates per tile (100 cm each), grooves + bolt rows"),
    ("DECK_PLATE", 512, 768, "floor deck: diamond tread plate, matte, 2 plates per 100 cm"),
    ("CRIMSON_BAND", 768, 896, "Dominion crimson band with brass pinstripes + rivets (hull r2 band language)"),
    ("HAZARD", 896, 1024, "hazard chevrons orange #BF541A on black, 45 deg, period 256 px (50 cm)"),
    ("VENT", 1024, 1152, "ribbed vent / grille, 6.25 cm pitch"),
    ("KICK_PLATE", 1152, 1280, "kick plate / skirting, scuffed, worn top edge"),
    ("STRIP_RED", 1280, 1344, "emissive strip, red-orange (1.0, 0.14, 0.02) = MI_DI_GlowRed"),
    ("STRIP_AMBER", 1344, 1408, "emissive strip, amber (1.0, 0.55, 0.10) = MI_DI_Cell family"),
    ("STRIP_CYAN", 1408, 1472, "emissive strip, cyan (0.25, 0.85, 1.0) = MI_DI_Screen family"),
    ("STRIP_WHITE", 1472, 1536, "work-light strip, cold white (0.9, 0.95, 1.0) = MI_DI_GlowWhite"),
    ("CABLE_TRAY", 1536, 1792, "cable tray: 5 pipes / cable runs with clamps every 50 cm"),
    ("TRIM_CAP", 1792, 2048, "bevelled trim caps / door-jamb panels, 2 rows of small plates with bolts"),
]
out = sys.argv[1] if len(sys.argv) > 1 else "."
os.makedirs(out, exist_ok=True)

def srgb(h):
    h = h.lstrip('#'); return np.array([int(h[i:i + 2], 16) / 255.0 for i in (0, 2, 4)])

def tile_noise(h, w, scale, octaves=4, seed=0):
    r = np.random.default_rng(seed); acc = np.zeros((h, w)); amp = 1.0
    fy = np.fft.fftfreq(h)[:, None]; fx = np.fft.fftfreq(w)[None, :]; f = np.sqrt(fx ** 2 + fy ** 2)
    for o in range(octaves):
        n = np.real(np.fft.ifft2(np.fft.fft2(r.standard_normal((h, w))) * np.exp(-(f * scale / (2 ** o)) ** 2 * 8)))
        acc += (n - n.mean()) / (n.std() + 1e-9) * amp; amp *= 0.5
    return (acc - acc.min()) / (acc.max() - acc.min())

X = np.arange(N)[None, :].repeat(N, 0).astype(np.float64)
Y = np.arange(N)[:, None].repeat(N, 1).astype(np.float64)
def band(y0, y1): return (Y >= y0) & (Y < y1)
def bevel_box(y0, y1, x0, x1, b, h):
    xs = np.clip(np.minimum(X - x0, x1 - 1 - X) / b, 0, 1); ys = np.clip(np.minimum(Y - y0, y1 - 1 - Y) / b, 0, 1)
    return np.where((X >= x0) & (X < x1) & (Y >= y0) & (Y < y1), np.minimum(xs, ys) * h, 0)
def bolt(cx, cy, r, h):
    d = np.sqrt((X - cx) ** 2 + (Y - cy) ** 2); return np.where(d < r, h * np.sqrt(np.clip(1 - (d / r) ** 2, 0, 1)), 0)
def save(a, name, mode=None):
    im = Image.fromarray((np.clip(a, 0, 1) * 255 + 0.5).astype(np.uint8), mode); im.save(os.path.join(out, name), optimize=True)
def normal_dx(H, s):
    dx = (np.roll(H, -1, 1) - np.roll(H, 1, 1)) * s; dy = (np.roll(H, -1, 0) - np.roll(H, 1, 0)) * s
    nx, ny, nz = -dx, dy, np.ones_like(H); l = np.sqrt(nx ** 2 + ny ** 2 + nz ** 2)
    return np.stack([nx / l, -ny / l, nz / l], -1) * 0.5 + 0.5      # DirectX (Unreal) green

# ================================================================================================ TRIM SHEET
H = np.zeros((N, N)); BC = np.zeros((N, N, 3)); R = np.full((N, N), 0.5); M = np.zeros((N, N)); E = np.zeros((N, N, 3)); AO = np.ones((N, N))
grime = tile_noise(N, N, 60, 5, 11); fine = tile_noise(N, N, 6, 3, 12); wear = tile_noise(N, N, 20, 4, 13)
GUN, GUN_D, GUN_L = srgb('#565B63'), srgb('#4A4F57'), srgb('#646971')
# WALL_PANEL
for k in range(4):
    H += bevel_box(8, 504, k * 512 + 8, k * 512 + 504, 16, 0.8)
    H -= np.where((np.abs(Y - 256) < 3) & (X > k * 512 + 48) & (X < k * 512 + 464), 0.25, 0)         # inset line
    H += bevel_box(300, 470, k * 512 + 330, k * 512 + 470, 6, 0.12)                                  # access hatch
    for i in range(7):
        for cy in (30, 482): H += bolt(k * 512 + 48 + i * 69, cy, 8, 0.15)
b = band(0, 512); BC[b] = GUN; R[b] = 0.34; M[b] = 0.65
BC[band(300, 470) & (((X % 512) > 330) & ((X % 512) < 470))] = GUN_D
# DECK_PLATE: diamond tread
b = band(512, 768)
H += bevel_box(516, 764, 0, N, 6, 0.3) * b
for k in range(8): H -= np.where(b & (np.abs(X - k * 256) < 3), 0.2, 0)
cu = (X % 32) / 32.0 - 0.5; cv = ((Y - 512) % 32) / 32.0 - 0.5
par = ((X // 32) + ((Y - 512) // 32)) % 2 == 0
ra, rb = np.where(par, cu + cv, cu - cv) / 1.4142, np.where(par, cu - cv, cu + cv) / 1.4142
tread = np.sqrt(np.clip(1 - (ra / 0.34) ** 2 - (rb / 0.09) ** 2, 0, 1))      # raised diamond lugs, alternating 45 deg
H += np.where(b, tread * 0.18, 0)
BC[b] = srgb('#5C5F64'); R[b] = 0.62; M[b] = 0.55
# CRIMSON_BAND
b = band(768, 896); H[b] += 0.5
BC[b] = srgb('#7A1626'); R[b] = 0.36; M[b] = 0.25
pin = band(780, 786) | band(878, 884); BC[pin] = srgb('#9A7A44'); M[pin] = 0.9; R[pin] = 0.3; H[pin] += 0.05
for cx in range(32, N, 128): H += bolt(cx, 832, 7, 0.15)
# HAZARD
b = band(896, 1024)
stripe = (((X + (Y - 896)) // 128) % 2 == 0)
BC[b & stripe] = srgb('#BF541A'); BC[b & ~stripe] = srgb('#26272B'); R[b] = 0.58; M[b] = 0.1; H[b] += 0.35
H[band(896, 904) | band(1016, 1024)] = 0.2
# VENT
b = band(1024, 1152); ph = (X % 32) / 32.0; rib = 0.5 + 0.5 * np.cos(2 * np.pi * (ph - 0.5))
H[b] = rib[b] * 0.6 + 0.1; H[band(1024, 1036) | band(1140, 1152)] = 0.75
BC[b] = GUN_D * (0.55 + 0.45 * rib[..., None][b]); R[b] = 0.45; M[b] = 0.8; AO[b] = 0.55 + 0.45 * rib[b]
# KICK_PLATE
b = band(1152, 1280); H += bevel_box(1156, 1276, 0, N, 10, 0.5) * b
for cx in range(64, N, 256): H += bolt(cx, 1216, 8, 0.15)
BC[b] = GUN_L; R[b] = 0.45; M[b] = 0.85
scuff = (tile_noise(N, N, 3, 3, 21) > 0.62) & band(1200, 1276)
BC[scuff] = srgb('#8A8D90'); R[scuff] = 0.3
# STRIPS
for (y0, col) in ((1280, (1.0, 0.14, 0.02)), (1344, (1.0, 0.55, 0.10)), (1408, (0.25, 0.85, 1.0)), (1472, (0.9, 0.95, 1.0))):
    b = band(y0, y0 + 64); H[b] = 0.3; H[band(y0 + 4, y0 + 18) | band(y0 + 46, y0 + 60)] = 0.6
    core = band(y0 + 20, y0 + 44)
    BC[b] = srgb('#2E3035'); R[b] = 0.3; M[b] = 0.7
    BC[core] = 0.6 + 0.4 * np.array(col); R[core] = 0.15; M[core] = 0.0; E[core] = np.array(col); H[core] = 0.25
    for cx in range(0, N, 256): E[core & (np.abs(X - cx) < 3)] *= 0.3     # diffuser segment seams
# CABLE_TRAY
b = band(1536, 1792); H[b] = 0.05; BC[b] = srgb('#3A3D42'); R[b] = 0.5; M[b] = 0.6
for (cy, r, col, met) in ((1570, 22, '#5A5E66', 0.9), (1622, 26, '#2A2B2E', 0.0), (1676, 22, '#7A1626', 0.1), (1724, 18, '#2A2B2E', 0.0), (1762, 20, '#6A6E75', 0.9)):
    d = np.abs(Y - cy); m = b & (d < r)
    H[m] = 0.2 + 0.6 * np.sqrt(np.clip(1 - (d[m] / r) ** 2, 0, 1)); BC[m] = srgb(col); M[m] = met; R[m] = 0.3 if met > 0.5 else 0.6
for cx in range(128, N, 256):
    m = b & (np.abs(X - cx) < 14); H[m] = 0.9; BC[m] = GUN_L; M[m] = 0.9; R[m] = 0.35
# TRIM_CAP
b = band(1792, 2048)
for k in range(8):
    H += bevel_box(1798, 1916, k * 256 + 6, k * 256 + 250, 10, 0.5)
    H += bevel_box(1924, 2042, k * 256 + 6, k * 256 + 122, 10, 0.45); H += bevel_box(1924, 2042, k * 256 + 134, k * 256 + 250, 10, 0.45)
    for cx in (k * 256 + 24, k * 256 + 232): H += bolt(cx, 1816, 6, 0.12); H += bolt(cx, 1898, 6, 0.12)
BC[b] = GUN; R[b] = 0.38; M[b] = 0.7
# wear, grime, normal
edge = np.clip((np.abs(np.gradient(H, axis=0)) + np.abs(np.gradient(H, axis=1))) * 8, 0, 1) * (wear > 0.55)
emis = (E.sum(-1) > 0)[..., None]
BC = np.where(emis, BC, BC * (0.88 + 0.16 * grime[..., None]) * (0.95 + 0.1 * fine[..., None]))
BC = BC + edge[..., None] * (srgb('#8C8F93') - BC) * 0.4 * (~emis)
R = np.clip(R + (grime - 0.5) * 0.18 + edge * 0.12, 0.08, 1)
AO = AO * (1 - 0.35 * np.clip(-np.gradient(H, axis=0) * 6, 0, 1))
H = H + (fine - 0.5) * 0.02
save(BC, "T_DI_InteriorTrim_BC.png"); save(normal_dx(H, 6.0), "T_DI_InteriorTrim_N.png")
save(np.stack([AO, R, M], -1), "T_DI_InteriorTrim_ORM.png"); save(E, "T_DI_InteriorTrim_E.png")

# ================================================================================================ DECAL ATLAS
# 4 quadrants of 1024 px. BC is RGBA (alpha = decal opacity). Suggested world size in DECALS below.
DECALS = [
    ("TRIDENT", (0, 0, 1024, 1024), "Dominion trident emblem plate (original): crimson disc, gunmetal ring, pale trident; 160 cm on door heads / bulkheads"),
    ("TRIAD_MARK", (1024, 0, 2048, 1024), "three-chevron Triad unit marking + bar-and-dot bay mark, worn stencil (brief: on lintels, half-covered); 120 cm"),
    ("HAZARD_FRAME", (0, 1024, 1024, 2048), "hazard-striped keep-clear frame for door thresholds / pad edges (no text); 400 cm floor"),
    ("ROUTE_ARROW", (1024, 1024, 2048, 2048), "squadron route stencil: three amber emissive chevrons pointing +U with dot/bar marks; 200 cm floor"),
]
D = 1024
A = np.zeros((N, N)); DBC = np.zeros((N, N, 3)); DH = np.zeros((N, N)); DR = np.full((N, N), 0.5); DM = np.zeros((N, N)); DE = np.zeros((N, N, 3))
dw = tile_noise(N, N, 14, 4, 31)
def q(ix, iy):  # local coords in a quadrant, centred, -1..1
    x0, y0 = ix * D, iy * D; m = (X >= x0) & (X < x0 + D) & (Y >= y0) & (Y < y0 + D)
    return m, (X - x0 - D / 2) / (D / 2), (Y - y0 - D / 2) / (D / 2)
def poly_mask(px, py, pts):
    """even-odd point-in-polygon on arrays (pts in -1..1 local, y down)."""
    inside = np.zeros(px.shape, bool); n = len(pts)
    for i in range(n):
        x1, y1 = pts[i]; x2, y2 = pts[(i + 1) % n]
        cond = ((y1 > py) != (y2 > py)) & (px < (x2 - x1) * (py - y1) / (y2 - y1 + 1e-12) + x1)
        inside ^= cond
    return inside
# TRIDENT
m, u, v = q(0, 0); r = np.sqrt(u ** 2 + v ** 2)
disc = m & (r < 0.92); ring = m & (r >= 0.80) & (r < 0.92)
A[disc] = 1; DBC[disc] = srgb('#7A1626'); DR[disc] = 0.38; DM[disc] = 0.2; DH[disc] = 0.4
DBC[ring] = srgb('#5E636B'); DM[ring] = 0.85; DR[ring] = 0.3; DH[ring] = 0.7
for k in range(12):
    a = k * np.pi / 6; bx, by = 0.86 * np.cos(a), 0.86 * np.sin(a); bm = m & ((u - bx) ** 2 + (v - by) ** 2 < 0.03 ** 2); DH[bm] = 0.9
tri = (poly_mask(u, v, [(-0.06, 0.62), (0.06, 0.62), (0.06, -0.30), (-0.06, -0.30)]) |                       # shaft
       poly_mask(u, v, [(-0.42, -0.18), (0.42, -0.18), (0.42, -0.06), (-0.42, -0.06)]) |                       # crossbar
       poly_mask(u, v, [(-0.42, -0.06), (-0.30, -0.06), (-0.30, -0.50), (-0.36, -0.62), (-0.42, -0.50)]) |    # left prong
       poly_mask(u, v, [(0.30, -0.06), (0.42, -0.06), (0.42, -0.50), (0.36, -0.62), (0.30, -0.50)]) |         # right prong
       poly_mask(u, v, [(-0.07, -0.30), (0.07, -0.30), (0.07, -0.62), (0.0, -0.74), (-0.07, -0.62)]) |        # centre prong
       poly_mask(u, v, [(-0.20, 0.62), (0.20, 0.62), (0.0, 0.74)]))                                           # pommel
tri &= m
DBC[tri] = srgb('#C9C6BB'); DH[tri] = 0.55; DR[tri] = 0.45; DM[tri] = 0.3
for s in (-1, 1):  # chevrons flanking the shaft
    ch = m & poly_mask(u, v, [(s * 0.16, 0.10), (s * 0.30, 0.22), (s * 0.30, 0.32), (s * 0.16, 0.20)]); DBC[ch] = srgb('#BF541A'); DH[ch] = 0.5
# TRIAD_MARK
m, u, v = q(1, 0)
for k in range(3):
    cy = -0.45 + k * 0.28
    ch = m & poly_mask(u, v, [(-0.55, cy + 0.10), (0.0, cy - 0.18), (0.55, cy + 0.10), (0.55, cy + 0.24), (0.0, cy - 0.04), (-0.55, cy + 0.24)])
    A[ch] = 1; DBC[ch] = srgb('#B9BCB4'); DH[ch] = 0.2; DR[ch] = 0.55
bars = m & (np.abs(v - 0.62) < 0.05) & (((u + 0.6) % 0.32) < 0.22) & (np.abs(u) < 0.6)
dots = m & (np.sqrt((u - 0.0) ** 2 + (v - 0.80) ** 2) < 0.05) | m & (np.sqrt((u - 0.18) ** 2 + (v - 0.80) ** 2) < 0.05) | m & (np.sqrt((u + 0.18) ** 2 + (v - 0.80) ** 2) < 0.05)
for mm in (bars, dots): A[mm] = 1; DBC[mm] = srgb('#B9BCB4'); DR[mm] = 0.55
mq = m & (A > 0); A[mq] = np.clip(A[mq] * (dw[mq] * 1.6 - 0.15), 0, 1)          # worn stencil
# HAZARD_FRAME
m, u, v = q(0, 1); fr = m & (np.maximum(np.abs(u), np.abs(v)) > 0.70) & (np.maximum(np.abs(u), np.abs(v)) < 0.96)
st = (((X + Y) // 64) % 2 == 0)
A[fr] = 1; DBC[fr & st] = srgb('#BF541A'); DBC[fr & ~st] = srgb('#1E1F22'); DR[fr] = 0.6; DH[fr] = 0.15
inner = m & (np.maximum(np.abs(u), np.abs(v)) > 0.66) & (np.maximum(np.abs(u), np.abs(v)) <= 0.70); A[inner] = 1; DBC[inner] = srgb('#C9C6BB')
A[fr] *= np.clip(0.55 + dw[fr], 0, 1)
# ROUTE_ARROW
m, u, v = q(1, 1)
for k in range(3):
    cx = -0.55 + k * 0.42
    ch = m & poly_mask(u, v, [(cx - 0.10, -0.45), (cx + 0.20, 0.0), (cx - 0.10, 0.45), (cx - 0.24, 0.45), (cx + 0.06, 0.0), (cx - 0.24, -0.45)])
    A[ch] = 1; DBC[ch] = srgb('#E0A040'); DE[ch] = (1.0, 0.55, 0.10); DR[ch] = 0.3; DH[ch] = 0.1
for k in range(5):
    dm = m & (np.sqrt((u - (-0.6 + k * 0.3)) ** 2 + (v - 0.70) ** 2) < 0.045); A[dm] = 1; DBC[dm] = srgb('#B9BCB4')
bm = m & (np.abs(v + 0.70) < 0.035) & (np.abs(u) < 0.6); A[bm] = 1; DBC[bm] = srgb('#B9BCB4')
DBC = DBC * (0.9 + 0.12 * dw[..., None])
Image.fromarray(np.concatenate([(np.clip(DBC, 0, 1) * 255 + .5).astype(np.uint8), (np.clip(A, 0, 1) * 255 + .5).astype(np.uint8)[..., None]], -1), "RGBA").save(os.path.join(out, "T_DI_Decals_BC.png"), optimize=True)
save(normal_dx(DH, 4.0), "T_DI_Decals_N.png"); save(np.stack([np.ones((N, N)) * 0.9 + 0.1 * (A == 0), DR, DM], -1), "T_DI_Decals_ORM.png"); save(DE, "T_DI_Decals_E.png")
json.dump(dict(trim=dict(size=N, px_per_cm=N / 400.0, tiles_u_cm=400, bands=[dict(key=k, v_px=[a, b_], height_px=b_ - a, height_cm=(b_ - a) / 5.12,
               uv_v=[round(1 - b_ / N, 5), round(1 - a / N, 5)], desc=d) for (k, a, b_, d) in BANDS]),
               decals=[dict(key=k, px=list(r), uv=[r[0] / N, 1 - r[3] / N, r[2] / N, 1 - r[1] / N], desc=d) for (k, r, d) in DECALS]),
          open(os.path.join(out, "trim_layout.json"), "w"), indent=1)
print("textures ok")
