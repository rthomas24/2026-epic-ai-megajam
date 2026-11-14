#!/usr/bin/env python3
"""Dominion Mothership mesh_r2 texture synthesis (deterministic, seed-locked).

Authors the hero atlas at 4096x4096 (BC, N[DirectX], ORM, E) and downsamples with Lanczos to 2048x2048.
Also authors the tileable detail layer T_DominionMothership_Detail_{N,ORM} (2048x2048, wraps seamlessly).
Atlas regions MUST match ATLAS in build_dominion_mothership.py.

usage: python3 make_textures.py OUT_4K_DIR OUT_2K_DIR
"""
import sys, os, math
import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage as ndi

S = 4096
# name: (u0, v0, u1, v1) in UV space (v up). Row 0 of the image is v = 1.
ATLAS = {
    "ARMOR":      (0.0,   0.5,    0.5,   1.0),
    "ARMOR_DARK": (0.5,   0.5,    1.0,   1.0),
    "FLOOR":      (0.0,   0.25,   0.5,   0.5),
    "WALL":       (0.0,   0.0,    0.5,   0.25),
    "BANNER":     (0.5,   0.0,    0.625, 0.5),
    "TRIM_RED":   (0.625, 0.0,    0.75,  0.25),
    "METAL":      (0.75,  0.0,    1.0,   0.25),
    "AMBER":      (0.625, 0.25,   0.6875, 0.3125),
    "WHITE":      (0.6875, 0.25,  0.75,  0.3125),
    "RECESS":     (0.625, 0.3125, 0.75,  0.375),
    "ENGINE":     (0.625, 0.375,  0.75,  0.5),
    "HANGAR":     (0.75,  0.25,   1.0,   0.5),
}

def px(name, size=S):
    u0, v0, u1, v1 = ATLAS[name]
    return (int(round((1 - v1) * size)), int(round(u0 * size)), int(round((1 - v0) * size)), int(round(u1 * size)))

def hexrgb(h):
    h = h.lstrip('#'); return np.array([int(h[i:i+2], 16) for i in (0, 2, 4)], np.float32) / 255.0

def fbm(h, w, rng, octaves=5, base=8, wrap=False):
    """Smooth value noise in [0,1]. wrap=True -> exactly periodic (FFT band-limited)."""
    if wrap:
        out = np.zeros((h, w), np.float32); amp = 1.0; tot = 0
        for o in range(octaves):
            f = base * (2 ** o)
            n = rng.standard_normal((h, w)).astype(np.float32)
            F = np.fft.fft2(n)
            fy = np.fft.fftfreq(h)[:, None] * h; fx = np.fft.fftfreq(w)[None, :] * w
            r = np.sqrt(fx ** 2 + fy ** 2)
            F *= np.exp(-(r / f) ** 2)
            layer = np.real(np.fft.ifft2(F)); layer /= (layer.std() + 1e-6)
            out += amp * layer; tot += amp; amp *= 0.5
        out /= tot
    else:
        out = np.zeros((h, w), np.float32); amp = 1.0; tot = 0
        for o in range(octaves):
            f = base * (2 ** o)
            g = rng.standard_normal((f + 3, int(f * w / h) + 3)).astype(np.float32)
            layer = ndi.zoom(g, (h / (f + 3) * 1.0001, w / (g.shape[1]) * 1.0001), order=3)[:h, :w]
            if layer.shape != (h, w):
                layer = np.pad(layer, ((0, h - layer.shape[0]), (0, w - layer.shape[1])), mode='edge')
            out += amp * layer / (layer.std() + 1e-6); tot += amp; amp *= 0.5
        out /= tot
    return np.clip(out * 0.18 + 0.5, 0, 1)

def split_rects(y0, x0, y1, x1, rng, min_px, max_px, out, depth=0):
    h, w = y1 - y0, x1 - x0
    if (h <= max_px and w <= max_px and rng.random() < 0.35 + 0.1 * depth) or (h < 2 * min_px and w < 2 * min_px):
        out.append((y0, x0, y1, x1)); return
    if (w > h and w >= 2 * min_px) or h < 2 * min_px:
        c = int(x0 + w * rng.uniform(0.3, 0.7)); split_rects(y0, x0, y1, c, rng, min_px, max_px, out, depth + 1); split_rects(y0, c, y1, x1, rng, min_px, max_px, out, depth + 1)
    else:
        c = int(y0 + h * rng.uniform(0.3, 0.7)); split_rects(y0, x0, c, x1, rng, min_px, max_px, out, depth + 1); split_rects(c, x0, y1, x1, rng, min_px, max_px, out, depth + 1)

def panel_field(h, w, rng, min_px, max_px, seam_px=5, bevel_px=14, rivets=True, vents=0.15, greebles=0.12, wrap=False):
    """Returns height (0 seam .. ~1.3), tone (per-panel jitter -1..1), seam mask, emissive slit mask (empty)."""
    rects = []
    split_rects(0, 0, h, w, rng, min_px, max_px, rects)
    H = np.zeros((h, w), np.float32); T = np.zeros((h, w), np.float32); kind = np.zeros((h, w), np.uint8)
    for (y0, x0, y1, x1) in rects:
        yy = np.arange(y0, y1)[:, None]; xx = np.arange(x0, x1)[None, :]
        d = np.minimum(np.minimum(yy - y0, y1 - 1 - yy), np.minimum(xx - x0, x1 - 1 - xx)).astype(np.float32)
        plate = np.clip((d - seam_px) / bevel_px, 0, 1)
        plate = np.where(d < seam_px, 0, 0.55 + 0.45 * plate)
        H[y0:y1, x0:x1] = plate * (0.9 + 0.2 * rng.random())
        T[y0:y1, x0:x1] = rng.uniform(-1, 1)
        ph, pw = y1 - y0, x1 - x0
        r = rng.random()
        if r < vents and ph > 60 and pw > 60:   # vent grille: row of recessed slots
            n = int(rng.integers(4, 10)); horiz = pw > ph
            iy0, ix0, iy1, ix1 = y0 + ph // 5, x0 + pw // 5, y1 - ph // 5, x1 - pw // 5
            for k in range(n):
                if horiz:
                    sx = int(ix0 + (ix1 - ix0) * (k + 0.2) / n); ex = int(ix0 + (ix1 - ix0) * (k + 0.7) / n)
                    H[iy0:iy1, sx:ex] = 0.25
                else:
                    sy = int(iy0 + (iy1 - iy0) * (k + 0.2) / n); ey = int(iy0 + (iy1 - iy0) * (k + 0.7) / n)
                    H[sy:ey, ix0:ix1] = 0.25
            kind[iy0:iy1, ix0:ix1] = 1
        elif r < vents + greebles and ph > 50 and pw > 50:   # raised inset box
            gy0 = y0 + int(ph * rng.uniform(0.2, 0.4)); gy1 = y1 - int(ph * rng.uniform(0.2, 0.4))
            gx0 = x0 + int(pw * rng.uniform(0.2, 0.4)); gx1 = x1 - int(pw * rng.uniform(0.2, 0.4))
            H[gy0:gy1, gx0:gx1] += 0.25
            H[gy0:gy0 + 3, gx0:gx1] -= 0.2; H[gy1 - 3:gy1, gx0:gx1] -= 0.2
            H[gy0:gy1, gx0:gx0 + 3] -= 0.2; H[gy0:gy1, gx1 - 3:gx1] -= 0.2
            kind[gy0:gy1, gx0:gx1] = 2
        if rivets and ph > 40 and pw > 40:
            rr = max(2, seam_px // 2 + 1); off = seam_px + bevel_px // 2 + 3
            for cy in (y0 + off, y1 - off):
                for cx in np.linspace(x0 + off, x1 - off, max(2, pw // 90)).astype(int):
                    H[max(cy - rr, 0):cy + rr, max(cx - rr, 0):cx + rr] += 0.12
    seam = (H < 0.05).astype(np.float32)
    return H, T, seam, kind

def normal_dx(H, strength, wrap=False):
    mode = 'wrap' if wrap else 'nearest'
    dx = ndi.sobel(H, axis=1, mode=mode) / 8.0
    drow = ndi.sobel(H, axis=0, mode=mode) / 8.0
    nx = -dx * strength
    ny_gl = drow * strength          # OpenGL: Ny = -dh/dv = +dh/drow
    nz = np.ones_like(H)
    l = np.sqrt(nx ** 2 + ny_gl ** 2 + nz ** 2)
    nx, ny_gl, nz = nx / l, ny_gl / l, nz / l
    ny_dx = -ny_gl                    # DirectX: green inverted
    return np.stack([nx, ny_dx, nz], -1) * 0.5 + 0.5

def cavity_ao(H, wrap=False, r=6.0, k=1.6):
    mode = 'wrap' if wrap else 'nearest'
    b = ndi.gaussian_filter(H, r, mode=mode)
    return np.clip(1.0 - k * np.clip(b - H, 0, 1), 0.25, 1.0)

def lerp3(a, b, t): return a[None, None, :] * (1 - t[..., None]) + b[None, None, :] * t[..., None]

def draw_trident(w, h):
    """Original stylized Dominion trident (white on transparent), drawn at w x h. Returns float mask."""
    im = Image.new('L', (w, h), 0); d = ImageDraw.Draw(im)
    cx = w / 2.0
    def P(pts): d.polygon([(cx + x * w, y * h) for (x, y) in pts], fill=255)
    # central spear: diamond tip + shaft
    P([(0, 0.05), (0.075, 0.17), (0.03, 0.22), (0.03, 0.62), (0.0, 0.70), (-0.03, 0.62), (-0.03, 0.22), (-0.075, 0.17)])
    # side prongs: swept blades curving up and out
    for s in (-1, 1):
        P([(s * 0.03, 0.36), (s * 0.20, 0.31), (s * 0.29, 0.21), (s * 0.33, 0.05), (s * 0.40, 0.22), (s * 0.37, 0.36), (s * 0.03, 0.47)])
    # crossbar chevron
    P([(-0.30, 0.44), (0.0, 0.53), (0.30, 0.44), (0.30, 0.49), (0.0, 0.59), (-0.30, 0.49)])
    # pommel bar
    P([(-0.12, 0.72), (0.12, 0.72), (0.08, 0.76), (-0.08, 0.76)])
    return np.asarray(im, np.float32) / 255.0

def build_atlas(seed=4101):
    rng = np.random.default_rng(seed)
    BC = np.zeros((S, S, 3), np.float32); HGT = np.zeros((S, S), np.float32)
    R = np.full((S, S), 0.55, np.float32); M = np.full((S, S), 0.7, np.float32); AO = np.ones((S, S), np.float32)
    E = np.zeros((S, S, 3), np.float32); NS = np.full((S, S), 6.0, np.float32)
    seamc = hexrgb('#16181c')

    def armor(name, lo, hi, min_px, max_px, metal, rough, slits=0.0):
        y0, x0, y1, x1 = px(name); h, w = y1 - y0, x1 - x0
        Hh, T, seam, kind = panel_field(h, w, rng, min_px, max_px)
        g = fbm(h, w, rng, 5, 6)
        t = np.clip(0.5 + 0.35 * T + (g - 0.5) * 0.9, 0, 1)
        col = lerp3(hexrgb(lo), hexrgb(hi), t)
        edge = np.clip(Hh - ndi.gaussian_filter(Hh, 3), 0, 1)
        col = col + edge[..., None] * 0.16                                    # worn bevel highlights
        col = col * (0.92 + 0.16 * fbm(h, w, rng, 4, 24))[..., None]           # grime breakup
        col[kind == 1] *= 0.55
        col = np.where(seam[..., None] > 0, seamc[None, None, :], col)
        BC[y0:y1, x0:x1] = col; HGT[y0:y1, x0:x1] = Hh
        R[y0:y1, x0:x1] = np.clip(rough + (g - 0.5) * 0.25 + seam * 0.2 + (kind == 1) * 0.1, 0.2, 0.95)
        M[y0:y1, x0:x1] = np.where(seam > 0, 0.3, metal); AO[y0:y1, x0:x1] = cavity_ao(Hh)
        if slits > 0:   # sparse tiny amber light slits in some seams
            for _ in range(int(slits)):
                sy = int(rng.integers(20, h - 20)); sx = int(rng.integers(20, w - 120)); L = int(rng.integers(40, 110))
                E[y0 + sy:y0 + sy + 5, x0 + sx:x0 + sx + L] = hexrgb('#ff5a14') * 1.0
                BC[y0 + sy:y0 + sy + 5, x0 + sx:x0 + sx + L] = hexrgb('#ff8a40')
        return Hh

    armor("ARMOR", '#4c525c', '#646b76', 110, 520, 0.45, 0.5)
    armor("ARMOR_DARK", '#3a3f48', '#4e545e', 90, 420, 0.5, 0.55, slits=6)
    armor("TRIM_RED", '#9a1620', '#d42834', 120, 400, 0.35, 0.5)
    armor("METAL", '#2a2d33', '#373a41', 50, 200, 0.6, 0.45)
    armor("RECESS", '#141619', '#1c1f23', 80, 260, 0.6, 0.7)

    # --- trench floor: plating along u (= ship X), conduit lanes, grates, beacon marks (painted, not emissive)
    y0, x0, y1, x1 = px("FLOOR"); h, w = y1 - y0, x1 - x0
    Hh, T, seam, kind = panel_field(h, w, rng, 120, 380, vents=0.3, greebles=0.15)
    yy = np.arange(h)[:, None]; xx = np.arange(w)[None, :]
    for lane in (0.2, 0.8):   # longitudinal conduit grooves with pipes
        c = int(lane * h); Hh[c - 22:c + 22, :] = 0.15
        for p in (-12, 0, 12): Hh[c + p - 4:c + p + 4, :] = 0.45
    rib = ((xx % 256) < 18)                     # transverse ribs every ~5.6 m
    Hh = np.where(rib & (Hh > 0.2), Hh + 0.25, Hh)
    g = fbm(h, w, rng, 5, 6)
    col = lerp3(hexrgb('#363b44'), hexrgb('#4a5059'), np.clip(0.5 + 0.3 * T + (g - .5), 0, 1))
    col = np.where((Hh < 0.2)[..., None], seamc[None, None, :], col)
    chevron = (((xx + (np.abs(yy - h / 2) * 0.8).astype(int)) % 512) < 26) & (np.abs(yy - h / 2) < h * 0.12)
    col = np.where(chevron[..., None], hexrgb('#5a3a22')[None, None, :], col)   # faded amber lane chevrons (paint)
    BC[y0:y1, x0:x1] = col; HGT[y0:y1, x0:x1] = Hh
    R[y0:y1, x0:x1] = np.clip(0.6 + (g - .5) * .3, .2, .95); M[y0:y1, x0:x1] = 0.5; AO[y0:y1, x0:x1] = cavity_ao(Hh)

    # --- trench wall: horizontal armour courses, buttress pilasters, pipes, small amber slits
    y0, x0, y1, x1 = px("WALL"); h, w = y1 - y0, x1 - x0
    Hh, T, seam, kind = panel_field(h, w, rng, 100, 340, vents=0.2)
    yy = np.arange(h)[:, None]; xx = np.arange(w)[None, :]
    course = (yy % 170) < 14; Hh = np.where(course, 0.1, Hh)
    pil = (xx % 512) < 60; Hh = np.where(pil, np.maximum(Hh, 1.2) - 0.2 * ((xx % 512) < 6), Hh)
    for p in (int(h * 0.62), int(h * 0.68)): Hh[p - 7:p + 7, :] = np.maximum(Hh[p - 7:p + 7, :], 0.9)
    g = fbm(h, w, rng, 5, 6)
    col = lerp3(hexrgb('#3a3f48'), hexrgb('#50565f'), np.clip(0.5 + 0.3 * T + (g - .5), 0, 1))
    col = np.where((Hh < 0.15)[..., None], seamc[None, None, :], col)
    col = np.where(pil[..., None], col * 0.85, col)
    BC[y0:y1, x0:x1] = col; HGT[y0:y1, x0:x1] = Hh
    R[y0:y1, x0:x1] = np.clip(0.55 + (g - .5) * .3, .2, .95); M[y0:y1, x0:x1] = 0.5; AO[y0:y1, x0:x1] = cavity_ao(Hh)
    for k in range(0, w, 512):   # amber slit lights on each pilaster (sparse)
        E[y0 + int(h * 0.30):y0 + int(h * 0.30) + 60, x0 + k + 22:x0 + k + 38] = hexrgb('#ff5a14')

    # --- banner: deep red panel, darker border, white trident
    y0, x0, y1, x1 = px("BANNER"); h, w = y1 - y0, x1 - x0
    Hh, T, seam, kind = panel_field(h, w, rng, 200, 700, rivets=False, vents=0, greebles=0, seam_px=4)
    g = fbm(h, w, rng, 5, 6)
    col = lerp3(hexrgb('#9a1620'), hexrgb('#d42834'), np.clip(0.55 + (g - .5) * 1.2 + 0.1 * T, 0, 1))
    yy = np.arange(h)[:, None]; xx = np.arange(w)[None, :]
    border = (xx < 34) | (xx > w - 35) | (yy < 34) | (yy > h - 35)
    col = np.where(border[..., None], hexrgb('#5a1014')[None, None, :], col)
    band = ((yy > h * 0.06) & (yy < h * 0.075)) | ((yy > h * 0.925) & (yy < h * 0.94))
    col = np.where(band[..., None], hexrgb('#d8d4cc')[None, None, :], col)
    tw = int(w * 0.86); th = int(tw * 1.3)
    tri = draw_trident(tw, th); ty = int(h * 0.16); tx = (w - tw) // 2
    m = np.zeros((h, w), np.float32); m[ty:ty + th, tx:tx + tw] = tri
    col = col * (1 - m[..., None]) + hexrgb('#f2f0ea')[None, None, :] * m[..., None]
    Hh = Hh * 0.6 + m * 0.35 + border * 0.2
    BC[y0:y1, x0:x1] = col; HGT[y0:y1, x0:x1] = Hh
    R[y0:y1, x0:x1] = 0.5 + (g - .5) * .2; M[y0:y1, x0:x1] = 0.25; AO[y0:y1, x0:x1] = cavity_ao(Hh)

    # --- emissive solids
    for name, c, bc in (("AMBER", '#ff5a14', '#ff7a30'), ("WHITE", '#fff4e6', '#ffffff')):
        y0, x0, y1, x1 = px(name)
        BC[y0:y1, x0:x1] = hexrgb(bc); E[y0:y1, x0:x1] = hexrgb(c); R[y0:y1, x0:x1] = 0.3; M[y0:y1, x0:x1] = 0.0; HGT[y0:y1, x0:x1] = 0.8
    y0, x0, y1, x1 = px("ENGINE"); h, w = y1 - y0, x1 - x0
    yy, xx = np.mgrid[0:h, 0:w]; r = np.sqrt(((yy - h / 2 + .5) / (h / 2)) ** 2 + ((xx - w / 2 + .5) / (w / 2)) ** 2)
    core = hexrgb('#fff3d0'); mid = hexrgb('#ff8a2a'); rim = hexrgb('#ff4a12')
    t1 = np.clip(r / 0.45, 0, 1); t2 = np.clip((r - 0.45) / 0.5, 0, 1)
    ec = lerp3(core, mid, t1); ec = np.where((r > 0.45)[..., None], lerp3(mid, rim, t2), ec)
    ring = (np.abs(r - 0.7) < 0.03) | (np.abs(r - 0.88) < 0.02)
    ec = np.where(ring[..., None], ec * 0.55, ec)
    ec = np.where((r > 0.97)[..., None], 0.0, ec)
    E[y0:y1, x0:x1] = ec; BC[y0:y1, x0:x1] = np.clip(ec, 0.05, 1); R[y0:y1, x0:x1] = 0.3; M[y0:y1, x0:x1] = 0.0

    # --- hangar interior bulkheads: dark panels, amber light bars
    y0, x0, y1, x1 = px("HANGAR"); h, w = y1 - y0, x1 - x0
    Hh, T, seam, kind = panel_field(h, w, rng, 90, 300, vents=0.25)
    g = fbm(h, w, rng, 5, 6)
    col = lerp3(hexrgb('#2e333b'), hexrgb('#424751'), np.clip(0.5 + 0.3 * T + (g - .5), 0, 1))
    col = np.where(seam[..., None] > 0, seamc[None, None, :], col)
    BC[y0:y1, x0:x1] = col; HGT[y0:y1, x0:x1] = Hh
    R[y0:y1, x0:x1] = 0.55; M[y0:y1, x0:x1] = 0.5; AO[y0:y1, x0:x1] = cavity_ao(Hh)
    for k in range(1, 4):
        cy = y0 + int(h * k / 4)
        E[cy - 6:cy + 6, x0 + 30:x1 - 30] = hexrgb('#ff5a14'); BC[cy - 6:cy + 6, x0 + 30:x1 - 30] = hexrgb('#ff7a30')

    NS[:] = 8.5
    N = normal_dx(HGT, 8.5)
    ORM = np.stack([AO, R, M], -1)
    return BC, N, ORM, E

def build_detail(size=2048, seed=4141):
    """Tileable detail: panel seams, rivets, vents, fine noise. Exactly periodic in both axes."""
    rng = np.random.default_rng(seed)
    Hh, T, seam, kind = panel_field(size, size, rng, 160, 700, seam_px=4, bevel_px=10, vents=0.18, greebles=0.15, wrap=True)
    # seams on the tile border are split half/half so the wrap seam keeps the interior width
    Hh = np.roll(Hh, (311, 517), axis=(0, 1))   # move the layout seams off the tile border (field is periodic)
    fine = fbm(size, size, rng, 4, 64, wrap=True)
    Hh = Hh + (fine - 0.5) * 0.08
    N = normal_dx(Hh, 5.5, wrap=True)
    ao = cavity_ao(Hh, wrap=True, r=5)
    rough = np.clip(0.5 + (fbm(size, size, rng, 5, 8, wrap=True) - 0.5) * 0.8 + (Hh < 0.1) * 0.15, 0, 1)
    ORM = np.stack([ao, rough, np.zeros_like(ao)], -1)
    return N, ORM

def save(arr, path, mode='RGB'):
    Image.fromarray(np.clip(np.round(arr * 255), 0, 255).astype(np.uint8), mode).save(path, optimize=True)

def down(path4k, path2k, kind):
    im = Image.open(path4k).convert('RGB').resize((2048, 2048), Image.LANCZOS)
    a = np.asarray(im).astype(np.float32) / 255.0
    if kind == 'N':
        v = a * 2 - 1; v[..., 2] = np.clip(v[..., 2], 0.01, 1); v /= np.linalg.norm(v, axis=-1, keepdims=True); a = v * 0.5 + 0.5
    if kind == 'E':
        b = np.round(a * 255); b[b.max(-1) <= 18] = 0; a = b / 255.0
    save(a, path2k)

if __name__ == '__main__':
    o4, o2 = sys.argv[1], sys.argv[2]; os.makedirs(o4, exist_ok=True); os.makedirs(o2, exist_ok=True)
    BC, N, ORM, E = build_atlas()
    E8 = np.round(np.clip(E, 0, 1) * 255); E8[E8.max(-1) <= 18] = 0; E = E8 / 255.0
    for k, a in (("BC", BC), ("N", N), ("ORM", ORM), ("E", E)):
        p4 = os.path.join(o4, f"T_DominionMothership_{k}_4K.png"); save(a, p4)
        down(p4, os.path.join(o2, f"T_DominionMothership_{k}.png"), k); print("wrote", k)
    DN, DORM = build_detail()
    save(DN, os.path.join(o2, "T_DominionMothership_Detail_N.png")); save(DORM, os.path.join(o2, "T_DominionMothership_Detail_ORM.png"))
    print("detail done")
