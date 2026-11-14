#!/usr/bin/env python3
"""Synthesize the shared 2K Outpost props trim/atlas (BC, DirectX N, ORM, E) and 1K delivery maps.

Atlas layout (UV 0-1, origin lower-left), designed so every prop shares one material
(M_OutpostProps / M_TD_PropTextured-compatible):

  gun        u 0-1     v 0.875-1.000   dark gunmetal panel A (tiles in U)
  gun2       u 0-1     v 0.750-0.875   gunmetal panel B, denser seams
  yellow     u 0-1     v 0.625-0.750   safety yellow / amber paint
  strap      u 0-1     v 0.500-0.625   dark blue-grey polymer / straps / frames
  hazard     u 0-1     v 0.4375-0.500  yellow/black diagonal hazard tape
  metal      u 0-1     v 0.375-0.4375  bare brushed metal
  vent       u 0-1     v 0.3125-0.375  louvered grille
  plastic    u 0-1     v 0.250-0.3125  pale console plastic
  screen_term  u 0.00-0.25  v 0.00-0.25  field-terminal cyan UI (unique)
  screen_kiosk u 0.25-0.50  v 0.00-0.25  data-kiosk radar UI (unique)
  keyboard     u 0.50-0.75  v 0.125-0.25 keypad / keyboard (unique)
  warn         u 0.50-0.75  v 0.00-0.125 lightning-bolt warning (unique)
  flood        u 0.75-0.875 v 0.125-0.25 white floodlamp (unique)
  bollard      u 0.75-0.875 v 0.00-0.125 orange bollard panel (unique)
  amber        u 0.875-1.00 v 0.125-0.25 amber LED / light bar (unique)
  led          u 0.875-1.00 v 0.00-0.125 thin cyan/crimson indicator (unique)

Albedo rule (audit B3c / E): gunmetal sits at sRGB 60-90, never crushed black.
Normals are authored in tangent space, OpenGL, then G is flipped for DirectX (255-G).
"""
from __future__ import annotations

import math
import os
import sys

import numpy as np
from PIL import Image

# --------------------------------------------------------------------------- regions (UV 0-1, origin lower-left)
ATLAS = {
    "gun":         {"u": (0.00, 1.00), "v": (0.875, 1.000), "tile": True},
    "gun2":        {"u": (0.00, 1.00), "v": (0.750, 0.875), "tile": True},
    "yellow":      {"u": (0.00, 1.00), "v": (0.625, 0.750), "tile": True},
    "strap":       {"u": (0.00, 1.00), "v": (0.500, 0.625), "tile": True},
    "hazard":      {"u": (0.00, 1.00), "v": (0.4375, 0.500), "tile": True},
    "metal":       {"u": (0.00, 1.00), "v": (0.375, 0.4375), "tile": True},
    "vent":        {"u": (0.00, 1.00), "v": (0.3125, 0.375), "tile": True},
    "plastic":     {"u": (0.00, 1.00), "v": (0.250, 0.3125), "tile": True},
    "screen_term": {"u": (0.00, 0.25), "v": (0.00, 0.25), "tile": False},
    "screen_kiosk":{"u": (0.25, 0.50), "v": (0.00, 0.25), "tile": False},
    "keyboard":    {"u": (0.50, 0.75), "v": (0.125, 0.25), "tile": False},
    "warn":        {"u": (0.50, 0.75), "v": (0.00, 0.125), "tile": False},
    "flood":       {"u": (0.75, 0.875), "v": (0.125, 0.25), "tile": False},
    "bollard":     {"u": (0.75, 0.875), "v": (0.00, 0.125), "tile": False},
    "amber":       {"u": (0.875, 1.00), "v": (0.125, 0.25), "tile": False},
    "led":         {"u": (0.875, 1.00), "v": (0.00, 0.125), "tile": False},
}

SEED = 4010


def srgb_to_lin(c):
    c = np.clip(c, 0.0, 1.0)
    return np.where(c <= 0.04045, c / 12.92, ((c + 0.055) / 1.055) ** 2.4)


def lin_to_srgb(c):
    c = np.clip(c, 0.0, 1.0)
    return np.where(c <= 0.0031308, c * 12.92, 1.055 * np.power(c, 1.0 / 2.4) - 0.055)


def rgb255(r, g, b):
    return np.array([r, g, b], dtype=np.float64) / 255.0


def value_noise(h, w, period, rng):
    """Cheap tileable value noise. period is the cell size in pixels (divides w,h ideally)."""
    ny = max(2, int(round(h / period)))
    nx = max(2, int(round(w / period)))
    grid = rng.random((ny, nx))
    # wrap
    grid = np.pad(grid, ((0, 1), (0, 1)), mode="wrap")
    ys = np.linspace(0, ny, h, endpoint=False)
    xs = np.linspace(0, nx, w, endpoint=False)
    yi = np.floor(ys).astype(np.int32)
    xi = np.floor(xs).astype(np.int32)
    fy = (ys - yi)[:, None]
    fx = (xs - xi)[None, :]
    # smoothstep
    fy = fy * fy * (3 - 2 * fy)
    fx = fx * fx * (3 - 2 * fx)
    g00 = grid[yi[:, None], xi[None, :]]
    g10 = grid[yi[:, None] + 1, xi[None, :]]
    g01 = grid[yi[:, None], xi[None, :] + 1]
    g11 = grid[yi[:, None] + 1, xi[None, :] + 1]
    return (g00 * (1 - fy) + g10 * fy) * (1 - fx) + (g01 * (1 - fy) + g11 * fy) * fx


def fbm(h, w, periods, rng, weights=None):
    acc = np.zeros((h, w), dtype=np.float64)
    weights = weights or [0.5 ** i for i in range(len(periods))]
    s = 0.0
    for p, wt in zip(periods, weights):
        acc += wt * value_noise(h, w, p, rng)
        s += wt
    return acc / s


def slice_px(res, u0, u1, v0, v1):
    x0 = int(round(u0 * res))
    x1 = int(round(u1 * res))
    y0 = int(round(v0 * res))  # lower-left origin in UV; numpy row 0 is top, so flip later
    y1 = int(round(v1 * res))
    return x0, x1, y0, y1


def put(arr, res, name, tile):
    """Return a view of arr (H,W,...) in UV lower-left coords mapped to numpy (top=1)."""
    r = ATLAS[name]
    x0, x1, y0, y1 = slice_px(res, r["u"][0], r["u"][1], r["v"][0], r["v"][1])
    # numpy row 0 is v=1 (top). v=0 is row res. So rows [res-y1 : res-y0]
    ra, rb = res - y1, res - y0
    return arr[ra:rb, x0:x1], (x0, x1, y0, y1)


def bevel_height(mask, radius):
    """Approximate distance-to-edge height: 1 inside, 0 at / outside edge, bevel of `radius` px."""
    from numpy.lib.stride_tricks import sliding_window_view
    # cheap iterative min/erode for a rounded falloff
    h, w = mask.shape
    d = np.where(mask > 0.5, 0.0, 1e6)
    # 4-pass chamfer distance (coarse but sharp enough for trim)
    for _ in range(int(radius) + 2):
        up = np.pad(d[:-1, :], ((1, 0), (0, 0)), mode="edge") + 1
        dn = np.pad(d[1:, :], ((0, 1), (0, 0)), mode="edge") + 1
        lf = np.pad(d[:, :-1], ((0, 0), (1, 0)), mode="edge") + 1
        rt = np.pad(d[:, 1:], ((0, 0), (0, 1)), mode="edge") + 1
        d = np.minimum(d, np.minimum(np.minimum(up, dn), np.minimum(lf, rt)))
    height = np.clip(d / max(radius, 1e-6), 0.0, 1.0)
    # smoothstep the bevel
    t = height
    return t * t * (3 - 2 * t)


def height_to_normal(height, strength=1.0):
    """OpenGL tangent normal from a height field (0-1). wrap-around gradients."""
    dx = (np.roll(height, -1, axis=1) - np.roll(height, 1, axis=1)) * 0.5
    dy = (np.roll(height, 1, axis=0) - np.roll(height, -1, axis=0)) * 0.5  # flip because row 0 is top
    nx = -dx * strength
    ny = -dy * strength
    nz = np.ones_like(height)
    n = np.stack([nx, ny, nz], axis=-1)
    n /= np.linalg.norm(n, axis=-1, keepdims=True) + 1e-8
    return n


def rivets(h, w, spacing, radius, rng, phase=0.0):
    yy, xx = np.indices((h, w))
    cx = (xx + phase) % spacing
    cy = (yy + phase * 0.7) % spacing
    dx = np.minimum(cx, spacing - cx)
    dy = np.minimum(cy, spacing - cy)
    # only at lattice intersections (near 0,0 of the cell)
    on = (dx < radius * 3) & (dy < radius * 3)
    dist = np.sqrt(dx * dx + dy * dy)
    riv = np.clip(1.0 - dist / radius, 0.0, 1.0)
    riv = riv * riv * (3 - 2 * riv)
    # indent (negative height) with a tiny lip
    height = -0.7 * riv + 0.15 * np.clip(1.0 - np.abs(dist - radius * 0.85) / (radius * 0.35), 0, 1)
    return np.where(on, height, 0.0)


def panel_lines(h, w, step_x, step_y, width, phase_x=0, phase_y=0):
    yy, xx = np.indices((h, w))
    dx = np.abs(((xx + phase_x + 0.5) % step_x) - 0.5)
    dy = np.abs(((yy + phase_y + 0.5) % step_y) - 0.5)
    lx = np.clip(1.0 - dx / width, 0, 1)
    ly = np.clip(1.0 - dy / width, 0, 1)
    line = np.maximum(lx, ly)
    return line * line * (3 - 2 * line)


def fill_panel(bc, orm, height, rng, base_srgb, metal, rough, seam_dark=0.82,
               step=(128, 96), seam_w=2.2, rivet_sp=64, rivet_r=3.5, noise=0.04,
               wear_light=0.08, ao=0.10):
    h, w = bc.shape[:2]
    base = np.array(base_srgb, dtype=np.float64)
    n1 = fbm(h, w, [64, 32, 16, 8], rng)
    n2 = fbm(h, w, [128, 48], rng)
    variation = (n1 - 0.5) * noise
    # wear slightly lifts edges of panels
    lines = panel_lines(h, w, step[0], step[1], seam_w)
    riv = rivets(h, w, rivet_sp, rivet_r, rng)
    # height: panel slightly proud, seams recessed, rivets indented
    height[:] += 0.08 - 0.55 * lines + 0.55 * riv
    # albedo in sRGB
    col = base[None, None, :] * (1.0 + variation[:, :, None])
    col *= (1.0 - (1.0 - seam_dark) * lines[:, :, None])
    # dirt in seams
    col *= (1.0 - ao * (n2 * 0.6 + 0.4) * lines)[:, :, None]
    # edge wear (lighter, slightly warmer) near seams
    wear = lines * n1 * wear_light
    col += wear[:, :, None] * np.array([1.05, 0.98, 0.85]) * 0.25
    # rivet heads slightly darker metal
    col *= (1.0 + riv * 0.08)[:, :, None]
    bc[:] = np.clip(col, 0, 1)
    # ORM
    ao_tex = np.clip(1.0 - 0.35 * lines - 0.15 * np.clip(-riv, 0, 1) - 0.08 * n2, 0.4, 1.0)
    rough_tex = np.clip(rough + (n1 - 0.5) * 0.10 + lines * 0.06, 0.25, 0.85)
    metal_tex = np.clip(metal + riv * 0.08 - lines * 0.05, 0.0, 1.0)
    orm[:, :, 0] = ao_tex
    orm[:, :, 1] = rough_tex
    orm[:, :, 2] = metal_tex


def draw_rect(img, x0, y0, x1, y1, color, thickness=0):
    """thickness=0 fills, else stroke. coords in this tile's pixels, y down."""
    h, w = img.shape[:2]
    x0, x1 = int(max(0, x0)), int(min(w, x1))
    y0, y1 = int(max(0, y0)), int(min(h, y1))
    if x1 <= x0 or y1 <= y0:
        return
    if thickness <= 0:
        img[y0:y1, x0:x1] = color
    else:
        t = thickness
        img[y0:y0 + t, x0:x1] = color
        img[y1 - t:y1, x0:x1] = color
        img[y0:y1, x0:x0 + t] = color
        img[y0:y1, x1 - t:x1] = color


def draw_circle(img, cx, cy, r, color, thickness=0):
    h, w = img.shape[:2]
    yy, xx = np.ogrid[:h, :w]
    dist = np.sqrt((xx - cx) ** 2 + (yy - cy) ** 2)
    if thickness <= 0:
        img[dist <= r] = color
    else:
        img[(dist <= r) & (dist >= r - thickness)] = color


def screen_ui(bc, orm, emi, height, kind, rng):
    h, w = bc.shape[:2]
    # dark cyan glass body
    bg = rgb255(28, 72, 92) if kind == "term" else rgb255(18, 48, 70)
    glow = rgb255(40, 210, 230)
    hi = rgb255(180, 245, 255)
    grid_c = rgb255(30, 140, 165)
    bc[:] = bg
    emi[:] = 0.05 * glow
    # bezel already handled by mesh; draw inset frame
    draw_rect(bc, 8, 8, w - 8, h - 8, glow * 0.35, thickness=3)
    draw_rect(emi, 8, 8, w - 8, h - 8, glow * 0.55, thickness=3)
    # inner glass
    inset = 14
    yy, xx = np.indices((h, w))
    glass = (xx >= inset) & (xx < w - inset) & (yy >= inset) & (yy < h - inset)
    # subtle scanlines
    scan = 0.08 * (0.5 + 0.5 * np.sin(yy / 2.2))
    bc[glass] = (bg * (1.0 + 0.15 * (rng.random((h, w, 1))[glass] - 0.5).ravel().reshape(-1, 1).repeat(1, axis=1)[:bc[glass].shape[0]]
                        if False else 1.0))
    # redo glass cleanly
    noise = (rng.random((h, w)) - 0.5) * 0.04
    for c in range(3):
        chan = np.where(glass, bg[c] * (1.0 + noise) - scan * 0.04, bc[:, :, c])
        bc[:, :, c] = chan

    if kind == "term":
        # left: circular radar / wireframe sphere
        cx, cy, R = int(w * 0.30), int(h * 0.50), int(min(w, h) * 0.28)
        draw_circle(bc, cx, cy, R, glow * 0.55, thickness=2)
        draw_circle(emi, cx, cy, R, glow * 0.9, thickness=2)
        for k in (0.33, 0.66, 1.0):
            draw_circle(bc, cx, cy, R * k, glow * 0.35, thickness=1)
            draw_circle(emi, cx, cy, R * k, glow * 0.55, thickness=1)
        # meridians
        for ang in range(0, 180, 30):
            a = math.radians(ang)
            x0 = cx + int(R * math.cos(a)); y0 = cy + int(R * math.sin(a))
            x1 = cx - int(R * math.cos(a)); y1 = cy - int(R * math.sin(a))
            n = max(abs(x1 - x0), abs(y1 - y0), 1)
            xs = np.linspace(x0, x1, n).astype(int)
            ys = np.linspace(y0, y1, n).astype(int)
            m = (xs >= 0) & (xs < w) & (ys >= 0) & (ys < h)
            bc[ys[m], xs[m]] = glow * 0.5
            emi[ys[m], xs[m]] = glow * 0.8
        # right: two stacked readouts
        x0, x1 = int(w * 0.58), int(w * 0.90)
        y0, y1 = int(h * 0.18), int(h * 0.48)
        y2, y3 = int(h * 0.55), int(h * 0.85)
        draw_rect(bc, x0, y0, x1, y1, glow * 0.25, thickness=2)
        draw_rect(emi, x0, y0, x1, y1, glow * 0.5, thickness=2)
        draw_rect(bc, x0, y2, x1, y3, glow * 0.25, thickness=2)
        draw_rect(emi, x0, y2, x1, y3, glow * 0.5, thickness=2)
        # bar graph / map blobs
        for i, bh in enumerate([0.35, 0.7, 0.5, 0.9, 0.4, 0.6]):
            bx0 = x0 + 8 + i * int((x1 - x0 - 16) / 6)
            bx1 = bx0 + int((x1 - x0) / 9)
            by1 = y1 - 6
            by0 = by1 - int((y1 - y0 - 14) * bh)
            draw_rect(bc, bx0, by0, bx1, by1, hi * 0.8)
            draw_rect(emi, bx0, by0, bx1, by1, hi)
        # contour-like blobs in lower right
        for (px, py, pr) in ((0.74, 0.70, 0.07), (0.82, 0.76, 0.05), (0.70, 0.78, 0.04)):
            draw_circle(bc, int(w * px), int(h * py), int(min(w, h) * pr), glow * 0.7, thickness=2)
            draw_circle(emi, int(w * px), int(h * py), int(min(w, h) * pr), glow, thickness=2)
    else:
        # kiosk: big radar with crosshair and a bright core
        cx, cy, R = int(w * 0.50), int(h * 0.50), int(min(w, h) * 0.36)
        for k in (0.25, 0.5, 0.75, 1.0):
            draw_circle(bc, cx, cy, R * k, glow * 0.45, thickness=2)
            draw_circle(emi, cx, cy, R * k, glow * 0.7, thickness=2)
        # crosshair
        draw_rect(bc, cx - 1, cy - R, cx + 2, cy + R, glow * 0.7)
        draw_rect(bc, cx - R, cy - 1, cx + R, cy + 2, glow * 0.7)
        draw_rect(emi, cx - 1, cy - R, cx + 2, cy + R, hi)
        draw_rect(emi, cx - R, cy - 1, cx + R, cy + 2, hi)
        # ticks
        for ang in range(0, 360, 15):
            a = math.radians(ang)
            r0, r1 = R * 0.92, R
            if ang % 45 == 0:
                r0 = R * 0.84
            x0 = cx + int(r0 * math.cos(a)); y0 = cy + int(r0 * math.sin(a))
            x1 = cx + int(r1 * math.cos(a)); y1 = cy + int(r1 * math.sin(a))
            n = max(abs(x1 - x0), abs(y1 - y0), 1)
            xs = np.linspace(x0, x1, n).astype(int)
            ys = np.linspace(y0, y1, n).astype(int)
            m = (xs >= 0) & (xs < w) & (ys >= 0) & (ys < h)
            bc[ys[m], xs[m]] = glow
            emi[ys[m], xs[m]] = hi
        # bright core
        draw_circle(bc, cx, cy, max(3, R // 10), hi)
        draw_circle(emi, cx, cy, max(3, R // 10), np.array([1.0, 1.0, 1.0]))
        # sweep arm
        a = math.radians(-35)
        x1 = cx + int(R * 0.9 * math.cos(a)); y1 = cy + int(R * 0.9 * math.sin(a))
        n = max(abs(x1 - cx), abs(y1 - cy), 1)
        xs = np.linspace(cx, x1, n).astype(int)
        ys = np.linspace(cy, y1, n).astype(int)
        m = (xs >= 0) & (xs < w) & (ys >= 0) & (ys < h)
        bc[ys[m], xs[m]] = hi
        emi[ys[m], xs[m]] = np.array([1.0, 1.0, 1.0])

    # raise emission on anything already glowing, keep bezel dim
    orm[:, :, 0] = 1.0
    orm[:, :, 1] = 0.28
    orm[:, :, 2] = 0.05
    height[:] = 0.0
    # slight inset of the screen (concave)
    yy, xx = np.indices((h, w))
    edge = np.minimum(np.minimum(xx, w - 1 - xx), np.minimum(yy, h - 1 - yy))
    height[:] = -0.15 * np.clip(1.0 - edge / 10.0, 0, 1)


def make_keyboard(bc, orm, emi, height, rng):
    h, w = bc.shape[:2]
    base = rgb255(42, 48, 56)
    key = rgb255(58, 66, 76)
    gap = rgb255(32, 36, 42)
    glow = rgb255(40, 180, 210)
    bc[:] = gap
    emi[:] = 0
    rows, cols = 5, 14
    mgn_x, mgn_y = 8, 8
    gw, gh = (w - 2 * mgn_x) / cols, (h - 2 * mgn_y) / rows
    yy, xx = np.indices((h, w))
    ht = np.zeros((h, w), dtype=np.float64)
    for r in range(rows):
        for c in range(cols):
            x0 = int(mgn_x + c * gw + 1.5)
            y0 = int(mgn_y + r * gh + 1.5)
            x1 = int(mgn_x + (c + 1) * gw - 1.5)
            y1 = int(mgn_y + (r + 1) * gh - 1.5)
            if x1 <= x0 or y1 <= y0:
                continue
            draw_rect(bc, x0, y0, x1, y1, key)
            # a few keys lit
            if (r + 3 * c + r * c) % 11 == 0:
                draw_rect(bc, x0 + 2, y0 + 2, x1 - 2, y1 - 2, glow * 0.55)
                draw_rect(emi, x0 + 2, y0 + 2, x1 - 2, y1 - 2, glow * 0.85)
            # rounded height for keycaps
            mask = np.zeros((h, w), dtype=bool)
            mask[y0:y1, x0:x1] = True
            dist = np.minimum(np.minimum(xx - x0, x1 - 1 - xx), np.minimum(yy - y0, y1 - 1 - yy))
            cap = np.clip(dist / 3.0, 0, 1)
            ht += np.where(mask, 0.15 + 0.55 * cap * cap, 0)
    height[:] = ht
    orm[:, :, 0] = 0.9
    orm[:, :, 1] = 0.45
    orm[:, :, 2] = 0.12


def make_warn(bc, orm, emi, height):
    h, w = bc.shape[:2]
    bg = rgb255(70, 76, 84)
    yel = rgb255(230, 190, 40)
    blk = rgb255(36, 34, 30)
    bc[:] = bg
    emi[:] = 0
    # triangle
    cy = int(h * 0.58)
    cx = w // 2
    side = int(min(w, h) * 0.72)
    yy, xx = np.indices((h, w))
    # equilateral pointing up
    y_top = cy - int(side * 0.58)
    y_bot = cy + int(side * 0.32)
    # barycentric
    p0 = np.array([cx, y_top])
    p1 = np.array([cx - side // 2, y_bot])
    p2 = np.array([cx + side // 2, y_bot])

    def edge(a, b, x, y):
        return (x - a[0]) * (b[1] - a[1]) - (y - a[1]) * (b[0] - a[0])

    e01 = edge(p0, p1, xx, yy)
    e12 = edge(p1, p2, xx, yy)
    e20 = edge(p2, p0, xx, yy)
    tri = (e01 >= 0) & (e12 >= 0) & (e20 >= 0)
    # inner black triangle for the border
    scale = 0.78
    c = (p0 + p1 + p2) / 3.0
    q0, q1, q2 = c + (p0 - c) * scale, c + (p1 - c) * scale, c + (p2 - c) * scale
    i01 = edge(q0, q1, xx, yy)
    i12 = edge(q1, q2, xx, yy)
    i20 = edge(q2, q0, xx, yy)
    inner = (i01 >= 0) & (i12 >= 0) & (i20 >= 0)
    bc[tri] = yel
    bc[inner] = bg * 0.5 + yel * 0.05
    # bolt
    bolt = [
        (cx + int(side * 0.02), y_top + int(side * 0.28)),
        (cx - int(side * 0.04), cy - int(side * 0.02)),
        (cx + int(side * 0.06), cy - int(side * 0.02)),
        (cx - int(side * 0.02), y_bot - int(side * 0.22)),
        (cx + int(side * 0.00), cy + int(side * 0.04)),
        (cx - int(side * 0.08), cy + int(side * 0.04)),
    ]
    # rasterize convex? it's a chevron - two triangles
    def fill_poly(pts, color):
        # bounding box scan
        xs = [p[0] for p in pts]; ys = [p[1] for p in pts]
        for y in range(max(0, min(ys)), min(h, max(ys) + 1)):
            # ray crossings
            nodes = []
            for i in range(len(pts)):
                x1, y1 = pts[i]
                x2, y2 = pts[(i + 1) % len(pts)]
                if (y1 <= y < y2) or (y2 <= y < y1):
                    if y2 != y1:
                        nodes.append(x1 + (y - y1) * (x2 - x1) / (y2 - y1))
            nodes.sort()
            for i in range(0, len(nodes) - 1, 2):
                a, b = int(nodes[i]), int(nodes[i + 1])
                a, b = max(0, a), min(w, b + 1)
                if b > a:
                    bc[y, a:b] = color
    fill_poly(bolt, yel)
    orm[:, :, 0] = 0.95
    orm[:, :, 1] = 0.42
    orm[:, :, 2] = 0.18
    height[:] = np.where(tri, 0.2, 0.0)


def make_lamp(bc, orm, emi, height, color, emit):
    h, w = bc.shape[:2]
    yy, xx = np.indices((h, w))
    # soft rectangular lamp with a brighter center and a thin dark bezel already in the mesh
    cx, cy = (w - 1) / 2, (h - 1) / 2
    nx = np.abs((xx - cx) / (w * 0.48))
    ny = np.abs((yy - cy) / (h * 0.48))
    # superellipse fill
    inside = (nx ** 6 + ny ** 6) < 1.0
    fall = np.clip(1.0 - (nx ** 4 + ny ** 4), 0, 1)
    bc[:] = color * 0.25
    emi[:] = 0
    for i in range(3):
        bc[:, :, i] = np.where(inside, color[i] * (0.75 + 0.25 * fall), bc[:, :, i])
        emi[:, :, i] = np.where(inside, emit[i] * (0.65 + 0.35 * fall), 0)
    orm[:, :, 0] = 1.0
    orm[:, :, 1] = np.where(inside, 0.32, 0.5)
    orm[:, :, 2] = np.where(inside, 0.05, 0.2)
    height[:] = np.where(inside, -0.08 * (1 - fall), 0.0)


def make_led_strip(bc, orm, emi, height):
    h, w = bc.shape[:2]
    # left half cyan indicators, right half crimson
    cyan = rgb255(40, 210, 230)
    crimson = rgb255(210, 40, 55)
    base = rgb255(50, 54, 60)
    bc[:] = base
    emi[:] = 0
    mid = w // 2
    # three pills each side
    for i, col, emit in ((0, cyan, cyan), (1, cyan, cyan), (2, crimson, crimson), (3, crimson, crimson)):
        x0 = int(w * (0.06 + i * 0.23))
        x1 = int(w * (0.06 + i * 0.23 + 0.18))
        y0, y1 = int(h * 0.28), int(h * 0.72)
        draw_rect(bc, x0, y0, x1, y1, col * 0.7)
        draw_rect(emi, x0, y0, x1, y1, emit)
    orm[:, :, 0] = 1.0
    orm[:, :, 1] = 0.35
    orm[:, :, 2] = 0.15
    height[:] = 0.05


def make_hazard(bc, orm, height, rng):
    h, w = bc.shape[:2]
    yel = rgb255(220, 175, 35)
    blk = rgb255(58, 56, 52)  # lifted "black" — not crushed
    yy, xx = np.indices((h, w))
    stripe = ((xx + yy) // max(1, h // 2)) % 2 == 0
    n = fbm(h, w, [32, 16], rng)
    bc[:] = np.where(stripe[:, :, None], yel, blk) * (1.0 + (n[:, :, None] - 0.5) * 0.06)
    # slight wear
    orm[:, :, 0] = 0.92 - 0.06 * n
    orm[:, :, 1] = np.where(stripe, 0.40, 0.48) + (n - 0.5) * 0.05
    orm[:, :, 2] = np.where(stripe, 0.22, 0.35)
    height[:] = 0.04 * n


def make_vent(bc, orm, height, rng):
    h, w = bc.shape[:2]
    base = rgb255(68, 74, 82)
    dark = rgb255(50, 54, 60)
    n = fbm(h, w, [32, 12], rng)
    bc[:] = base * (1.0 + (n[:, :, None] - 0.5) * 0.05)
    yy, xx = np.indices((h, w))
    # horizontal louvers
    period = max(8, h // 6)
    slat = ((yy + 2) % period) < (period * 0.55)
    gap = ~slat
    # slat front face slightly lighter, gap recessed dark
    bc[gap] = dark
    # height: slats proud, rounded
    phase = ((yy + 2) % period).astype(np.float64)
    slat_h = np.where(slat, 0.45 * np.sin(np.pi * np.clip(phase / (period * 0.55), 0, 1)), -0.35)
    height[:] = slat_h
    orm[:, :, 0] = np.where(gap, 0.55, 0.9)
    orm[:, :, 1] = np.where(gap, 0.55, 0.40)
    orm[:, :, 2] = np.where(gap, 0.55, 0.70)


def build(res, out_dir, prefix="T_OutpostProps"):
    rng = np.random.default_rng(SEED)
    bc = np.zeros((res, res, 3), dtype=np.float64)
    orm = np.zeros((res, res, 3), dtype=np.float64)
    emi = np.zeros((res, res, 3), dtype=np.float64)
    height = np.zeros((res, res), dtype=np.float64)
    # default fill (shouldn't show)
    bc[:] = rgb255(80, 84, 90)
    orm[:] = (0.9, 0.45, 0.3)

    def region(name):
        b, _ = put(bc, res, name, True)
        o, _ = put(orm, res, name, True)
        e, _ = put(emi, res, name, True)
        h, _ = put(height, res, name, True)
        return b, o, e, h

    # --- tiling strips
    b, o, e, h = region("gun")
    fill_panel(b, o, h, rng, rgb255(76, 82, 90), metal=0.72, rough=0.42,
               step=(res // 8, res // 12), seam_w=max(2.0, res / 1024 * 2.2),
               rivet_sp=res // 16, rivet_r=max(2.5, res / 1024 * 3.4), noise=0.045)

    b, o, e, h = region("gun2")
    fill_panel(b, o, h, rng, rgb255(70, 76, 84), metal=0.68, rough=0.46,
               step=(res // 12, res // 10), seam_w=max(1.8, res / 1024 * 2.0),
               rivet_sp=res // 20, rivet_r=max(2.2, res / 1024 * 3.0), noise=0.05, seam_dark=0.80)

    b, o, e, h = region("yellow")
    fill_panel(b, o, h, rng, rgb255(214, 158, 38), metal=0.22, rough=0.48,
               step=(res // 7, res // 11), seam_w=max(2.0, res / 1024 * 2.4),
               rivet_sp=res // 14, rivet_r=max(2.4, res / 1024 * 3.2), noise=0.05,
               wear_light=0.14, seam_dark=0.78, ao=0.14)

    b, o, e, h = region("strap")
    fill_panel(b, o, h, rng, rgb255(58, 64, 74), metal=0.28, rough=0.52,
               step=(res // 5, res // 9), seam_w=max(2.5, res / 1024 * 3.0),
               rivet_sp=res // 12, rivet_r=max(2.6, res / 1024 * 3.6), noise=0.04,
               wear_light=0.06, seam_dark=0.85)

    b, o, e, h = region("hazard")
    make_hazard(b, o, h, rng)

    b, o, e, h = region("metal")
    fill_panel(b, o, h, rng, rgb255(132, 136, 142), metal=0.88, rough=0.34,
               step=(res // 4, res // 8), seam_w=max(1.5, res / 1024 * 1.6),
               rivet_sp=res // 10, rivet_r=max(2.0, res / 1024 * 2.8), noise=0.035,
               wear_light=0.05, seam_dark=0.90)
    # brushed streaks
    b2, _, _, h2 = region("metal")
    hh, ww = b2.shape[:2]
    brush = fbm(hh, 8, [4, 2], rng)
    brush = np.repeat(brush, ww // 8 + 4, axis=1)[:, :ww]
    b2 *= 1.0 + (brush[:, :, None] - 0.5) * 0.07
    h2 += (brush - 0.5) * 0.04

    b, o, e, h = region("vent")
    make_vent(b, o, h, rng)

    b, o, e, h = region("plastic")
    fill_panel(b, o, h, rng, rgb255(92, 98, 108), metal=0.16, rough=0.44,
               step=(res // 9, res // 14), seam_w=max(1.6, res / 1024 * 1.8),
               rivet_sp=res // 16, rivet_r=max(2.0, res / 1024 * 2.6), noise=0.03,
               wear_light=0.05, seam_dark=0.88)

    # --- unique islands
    b, o, e, h = region("screen_term")
    screen_ui(b, o, e, h, "term", rng)
    b, o, e, h = region("screen_kiosk")
    screen_ui(b, o, e, h, "kiosk", rng)
    b, o, e, h = region("keyboard")
    make_keyboard(b, o, e, h, rng)
    b, o, e, h = region("warn")
    make_warn(b, o, e, h)
    b, o, e, h = region("flood")
    make_lamp(b, o, e, h, rgb255(240, 236, 220), rgb255(255, 250, 235))
    b, o, e, h = region("bollard")
    make_lamp(b, o, e, h, rgb255(255, 186, 90), rgb255(255, 170, 60))
    b, o, e, h = region("amber")
    make_lamp(b, o, e, h, rgb255(255, 200, 110), rgb255(255, 180, 70))
    b, o, e, h = region("led")
    make_led_strip(b, o, e, h)

    # normals from height (OpenGL), then DirectX flip
    # slightly stronger on seams
    n_gl = height_to_normal(height, strength=6.0 * (res / 1024.0))
    n_dx = n_gl.copy()
    n_dx[:, :, 1] *= -1.0

    def to_u8_srgb(a):
        return np.clip(np.round(lin_to_srgb(srgb_to_lin(a)) * 255.0), 0, 255).astype(np.uint8)

    def to_u8_lin(a):
        return np.clip(np.round(a * 255.0), 0, 255).astype(np.uint8)

    def n_to_u8(n):
        return np.clip(np.round((n * 0.5 + 0.5) * 255.0), 0, 255).astype(np.uint8)

    # Keep albedo in sRGB (we authored in sRGB already)
    bc_u8 = np.clip(np.round(bc * 255.0), 0, 255).astype(np.uint8)
    orm_u8 = to_u8_lin(orm)
    emi_u8 = np.clip(np.round(emi * 255.0), 0, 255).astype(np.uint8)
    n_u8 = n_to_u8(n_dx)
    n_gl_u8 = n_to_u8(n_gl)

    os.makedirs(out_dir, exist_ok=True)
    paths = {}
    for suffix, arr in (("BC", bc_u8), ("N", n_u8), ("ORM", orm_u8), ("E", emi_u8)):
        p = os.path.join(out_dir, f"{prefix}_{suffix}.png")
        Image.fromarray(arr, mode="RGB").save(p, optimize=True)
        paths[suffix] = p
        print("wrote", p, arr.shape)
    # OpenGL preview companion (not shipped in the 1K set unless asked)
    pgl = os.path.join(out_dir, f"{prefix}_N_OpenGL.png")
    Image.fromarray(n_gl_u8, mode="RGB").save(pgl, optimize=True)
    paths["N_OpenGL"] = pgl
    return paths


def downsample(src_dir, dst_dir, size=1024, prefix="T_OutpostProps"):
    os.makedirs(dst_dir, exist_ok=True)
    out = {}
    for suffix in ("BC", "N", "ORM", "E", "N_OpenGL"):
        src = os.path.join(src_dir, f"{prefix}_{suffix}.png")
        im = Image.open(src)
        im = im.resize((size, size), Image.Resampling.LANCZOS)
        dst = os.path.join(dst_dir, f"{prefix}_{suffix}.png")
        im.save(dst, optimize=True)
        out[suffix] = dst
        print("wrote", dst)
    return out


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    mesh = os.path.dirname(here)
    master = os.path.join(here, "textures_2k")
    delivery = mesh
    if len(sys.argv) >= 3:
        master, delivery = sys.argv[1], sys.argv[2]
    build(2048, master)
    downsample(master, delivery, 1024)
    print("done")


if __name__ == "__main__":
    main()
