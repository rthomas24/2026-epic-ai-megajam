"""T_DominionBay_Trim_*: one 2K tiling trim sheet for the non-walkable bay-mouth pieces (doors, emitter).
Periodic in U over the full width (every band tiles horizontally); bands are stacked in V. Seeded, numpy only.
Outputs (2048 x 2048, 8-bit PNG): _BC (sRGB), _N (DirectX / Unreal green), _ORM (R AO, G roughness, B metallic),
_E (emissive mask, sRGB white = glow; the material tints it).
Bands, V from the top in px (UV v = 1 - px / 2048):
  0-640     armour panel: dark gunmetal #4C5058 plates (lifted from #16171B so BC median >= sRGB 55), 4 per tile, panel lines, bolt rows
  640-896   hazard chevrons #BF541A on #2E2F33, worn = the BAY THRESHOLD marking (N-061)
  896-1152  ribbed grille / vent
  1152-1408 gunmetal #62676F plate with rivets
  1408-1536 crimson band #6E1420 (Dominion maroon) with a thin gold-ish pinstripe
  1536-1664 emissive strip: black housing with a 40 px glowing core (E mask)
  1664-2048 small panels #50545C / trim cap with edge wear
Run: python3 make_trim_textures.py <outdir>
"""
import sys, os
import numpy as np
from PIL import Image

N = 2048
rng = np.random.default_rng(54)
out = sys.argv[1] if len(sys.argv) > 1 else "."
os.makedirs(out, exist_ok=True)

def srgb(h):
    h = h.lstrip('#'); return np.array([int(h[i:i+2], 16) / 255.0 for i in (0, 2, 4)])

def tile_noise(h, w, scale, octaves=4, seed=0):
    """Periodic value noise via FFT-filtered white noise (tiles in both axes)."""
    r = np.random.default_rng(seed)
    acc = np.zeros((h, w))
    amp = 1.0
    for o in range(octaves):
        wn = r.standard_normal((h, w))
        fy = np.fft.fftfreq(h)[:, None]; fx = np.fft.fftfreq(w)[None, :]
        f = np.sqrt(fx ** 2 + fy ** 2)
        s = scale / (2 ** o)
        filt = np.exp(-(f * s) ** 2 * 8)
        n = np.real(np.fft.ifft2(np.fft.fft2(wn) * filt))
        n = (n - n.mean()) / (n.std() + 1e-9)
        acc += n * amp; amp *= 0.5
    acc = (acc - acc.min()) / (acc.max() - acc.min())
    return acc

H = np.zeros((N, N))          # height 0..1
BC = np.zeros((N, N, 3))
R = np.full((N, N), 0.5)      # roughness
M = np.zeros((N, N))          # metallic
E = np.zeros((N, N))          # emissive mask
AO = np.ones((N, N))
X = np.arange(N)[None, :].repeat(N, 0)
Y = np.arange(N)[:, None].repeat(N, 1)
grime = tile_noise(N, N, 60, 5, 1)
fine = tile_noise(N, N, 6, 3, 2)
wear = tile_noise(N, N, 20, 4, 3)

def band(y0, y1):
    return (Y >= y0) & (Y < y1)

def bevel_box(mask_y0, mask_y1, x0, x1, b, height):
    """A chamfered raised rectangle (height map) inside [x0,x1) x [y0,y1)."""
    xs = np.clip(np.minimum(X - x0, x1 - 1 - X) / b, 0, 1)
    ys = np.clip(np.minimum(Y - mask_y0, mask_y1 - 1 - Y) / b, 0, 1)
    inside = (X >= x0) & (X < x1) & (Y >= mask_y0) & (Y < mask_y1)
    return np.where(inside, np.minimum(xs, ys) * height, 0)

# 0) armour panels 0-640: 4 plates of 512 wide with 12 px grooves; a bolt row near top and bottom
b = band(0, 640)
for k in range(4):
    H += bevel_box(10, 630, k * 512 + 8, k * 512 + 504, 14, 0.8)
    for i in range(8):
        cx = k * 512 + 40 + i * 62
        for cy in (40, 600):
            d = np.sqrt((X - cx) ** 2 + (Y - cy) ** 2)
            H += np.where(d < 9, 0.15 * np.sqrt(np.clip(1 - (d / 9) ** 2, 0, 1)), 0)
# a secondary inset panel line in each plate
for k in range(4):
    groove = ((np.abs(Y - 320) < 3) & (X > k * 512 + 60) & (X < k * 512 + 452))
    H -= np.where(groove, 0.25, 0)
BC[b] = srgb('#4C5058'); R[b] = 0.32; M[b] = 0.6

# 1) hazard chevrons 640-896 (period 256 px so they tile)
b = band(640, 896)
H += bevel_box(648, 888, 0, N, 6, 0.4) * 0  # flat band
stripe = (((X + (Y - 640)) // 128) % 2 == 0)
BC[b & stripe] = srgb('#BF541A'); BC[b & ~stripe] = srgb('#2E2F33')
R[b] = 0.55; M[b] = 0.1
H[b] += 0.35
H[(Y >= 640) & (Y < 652)] = 0.2; H[(Y >= 884) & (Y < 896)] = 0.2

# 2) ribbed grille 896-1152: 32 px period ribs
b = band(896, 1152)
ph = (X % 32) / 32.0
rib = 0.5 + 0.5 * np.cos(2 * np.pi * (ph - 0.5))
H[b] = (rib[b] * 0.6 + 0.1)
H[band(896, 912) | band(1136, 1152)] = 0.75
BC[b] = srgb('#5A5E66') * (0.6 + 0.4 * rib[..., None][b]); R[b] = 0.45; M[b] = 0.8
AO[b] = 0.55 + 0.45 * rib[b]

# 3) gunmetal plate 1152-1408 with rivets every 64 px
b = band(1152, 1408)
H += bevel_box(1156, 1404, 0, N, 10, 0.55) * b
for cy in (1180, 1380):
    for cx in range(32, N, 64):
        d = np.sqrt((X - cx) ** 2 + (Y - cy) ** 2)
        H += np.where(d < 7, 0.18 * np.sqrt(np.clip(1 - (d / 7) ** 2, 0, 1)), 0)
BC[b] = srgb('#62676F'); R[b] = 0.42; M[b] = 0.9

# 4) crimson band 1408-1536
b = band(1408, 1536)
H[b] = 0.5
BC[b] = srgb('#6E1420'); R[b] = 0.35; M[b] = 0.3
pin = band(1418, 1424) | band(1520, 1526)
BC[pin] = srgb('#8A6A3A'); M[pin] = 0.9; R[pin] = 0.3

# 5) emissive strip 1536-1664
b = band(1536, 1664)
H[b] = 0.3
H[band(1540, 1580) | band(1620, 1660)] = 0.6
core = band(1580, 1620)
BC[b] = srgb('#34363B'); R[b] = 0.3; M[b] = 0.7
BC[core] = np.array([0.9, 0.9, 0.9]); R[core] = 0.15; M[core] = 0.0; E[core] = 1.0
H[core] = 0.25

# 6) small panels 1664-2048: 8 per tile, 256 wide, with a lip
b = band(1664, 2048)
for k in range(8):
    H += bevel_box(1672, 1856, k * 256 + 6, k * 256 + 250, 8, 0.5)
    H += bevel_box(1864, 2040, k * 256 + 6, k * 256 + 120, 8, 0.45); H += bevel_box(1864, 2040, k * 256 + 134, k * 256 + 250, 8, 0.45)
BC[b] = srgb('#50545C'); R[b] = 0.38; M[b] = 0.7

# wear, grime, fine breakup (multiplied, so bands keep their read)
edge = np.clip(np.abs(np.gradient(H, axis=0)) + np.abs(np.gradient(H, axis=1)), 0, 1) * 8
edge = np.clip(edge, 0, 1) * (wear > 0.55)
BC = BC * (0.88 + 0.16 * grime[..., None]) * (0.95 + 0.1 * fine[..., None])
BC = BC + edge[..., None] * (srgb('#7A7D82') - BC) * 0.35 * (E[..., None] == 0)
R = np.clip(R + (grime - 0.5) * 0.18 + edge * 0.15, 0.08, 1)
AO = AO * (1 - 0.35 * np.clip(-np.gradient(H, axis=0) * 6, 0, 1))
H = H + (fine - 0.5) * 0.02

# normal from height (tileable gradient with np.roll), DirectX convention (green = -dy)
s = 6.0
dx = (np.roll(H, -1, 1) - np.roll(H, 1, 1)) * s
dy = (np.roll(H, -1, 0) - np.roll(H, 1, 0)) * s
nx, ny, nz = -dx, dy, np.ones_like(H)   # image rows go down: +dy in rows = -v; OpenGL green = -dy_rows... DX flips it
l = np.sqrt(nx ** 2 + ny ** 2 + nz ** 2)
nx, ny, nz = nx / l, ny / l, nz / l
ny_dx = -ny
Nimg = np.stack([nx, ny_dx, nz], -1) * 0.5 + 0.5

def save(a, name):
    Image.fromarray((np.clip(a, 0, 1) * 255 + 0.5).astype(np.uint8)).save(os.path.join(out, name), optimize=True)
save(BC, "T_DominionBay_Trim_BC.png")
save(Nimg, "T_DominionBay_Trim_N.png")
save(np.stack([AO, R, M], -1), "T_DominionBay_Trim_ORM.png")
save(E, "T_DominionBay_Trim_E.png")
print("ok")
