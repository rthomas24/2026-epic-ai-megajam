# SPDX-License-Identifier: MIT
# Triad Drift - RiftHero hybrid: clean-up of the kept Rodin texture set (same UV layout, 2K in, 2K out).
#   base colour  : white and magenta paint flattened to two clean values through masks; panel lines, chips and
#                  edge wear (the high-frequency part) are kept, the blotches (the low-frequency part) go
#   ORM          : roughness evened per material, metallic removed from paint (it was noise), AO untouched
#   normal (DX)  : broad lumps removed from paint (high-pass) and the remainder softened, most on the magenta
#   emissive     : low-level noise gated out, lights kept
# Values are handled in the files' own encoding (sRGB for BC / E, linear for ORM / N).
import os

import bpy
import numpy as np

WHITE = np.array([0.86, 0.865, 0.885], np.float32)        # sRGB-encoded
MAGENTA = np.array([0.90, 0.045, 0.56], np.float32)
ACCENT_TARGET = {"magenta": MAGENTA, "cyan": np.array([0.17, 0.76, 0.93], np.float32),
                 "amber": np.array([0.95, 0.62, 0.12], np.float32)}


def load(path):
    im = bpy.data.images.load(path, check_existing=False)
    w, h = im.size
    a = np.empty(w * h * 4, np.float32)
    im.pixels.foreach_get(a)
    bpy.data.images.remove(im)
    return a.reshape(h, w, 4)[..., :3].copy()


def save(arr, path, srgb):
    h, w = arr.shape[:2]
    im = bpy.data.images.new("__tex__", w, h, alpha=False)
    im.colorspace_settings.name = "sRGB" if srgb else "Non-Color"
    px = np.ones((h, w, 4), np.float32)
    px[..., :3] = np.clip(arr, 0.0, 1.0)
    im.pixels.foreach_set(px.ravel())
    im.filepath_raw = path
    im.file_format = "PNG"
    im.save()
    bpy.data.images.remove(im)
    return path


def box(a, r):
    """Box blur, radius r pixels, wrap-free (edge clamp), any trailing channels."""
    def one(x, axis):
        n = x.shape[axis]
        pad = [(0, 0)] * x.ndim
        pad[axis] = (r + 1, r)
        c = np.cumsum(np.pad(x, pad, mode="edge"), axis=axis, dtype=np.float64)
        hi = np.take(c, np.arange(2 * r + 1, 2 * r + 1 + n), axis=axis)
        lo = np.take(c, np.arange(0, n), axis=axis)
        return ((hi - lo) / (2 * r + 1)).astype(np.float32)
    return one(one(a, 0), 1)


def blur(a, r):
    return box(box(a, r), r)


def sstep(x, a, b):
    t = np.clip((x - a) / (b - a), 0.0, 1.0)
    return t * t * (3 - 2 * t)


def _accent_other(bc, accent):
    r, g, b = bc[..., 0], bc[..., 1], bc[..., 2]
    mx, mn = bc.max(-1), bc.min(-1)
    sat = (mx - mn) / (mx + 1e-5)
    if accent == "cyan":
        return sstep(sat, 0.30, 0.50) * sstep(b - r, 0.15, 0.30) * sstep(g - r, 0.10, 0.25) * sstep(mx, 0.30, 0.50)
    return sstep(sat, 0.35, 0.55) * sstep(r - b, 0.20, 0.35) * sstep(g - b, 0.06, 0.18) * sstep(r - g, 0.04, 0.14) \
        * sstep(mx, 0.30, 0.50)                                   # amber


def masks(bc, accent="magenta"):
    """(white paint, accent paint) soft masks. The accent mask is called mag: the Rift's magenta came first."""
    r, g, b = bc[..., 0], bc[..., 1], bc[..., 2]
    mx, mn = bc.max(-1), bc.min(-1)
    sat = (mx - mn) / (mx + 1e-5)
    if accent != "magenta":
        mag = _accent_other(bc, accent)
    else:
        mag = sstep(sat, 0.30, 0.50) * sstep(r - g, 0.12, 0.28) * sstep(b - g, 0.02, 0.12) * sstep(mx, 0.30, 0.50) \
            * sstep(r - 0.85 * b, 0.0, 0.08)
    wht = (1.0 - sstep(sat, 0.14, 0.26)) * sstep(mx, 0.42, 0.58)
    # soften the mask edges a little so the flattening has no hard border
    mag = np.clip(blur(mag, 1), 0, 1)
    wht = np.clip(blur(wht, 1), 0, 1)
    return wht, mag


def clean(src_dir, out_dir, stem="Rift", accent="magenta"):
    bc = load(os.path.join(src_dir, "T_%s_BC.png" % stem))
    orm = load(os.path.join(src_dir, "T_%s_ORM.png" % stem))
    nrm = load(os.path.join(src_dir, "T_%s_N.png" % stem))
    em = load(os.path.join(src_dir, "T_%s_E.png" % stem))
    wht, mag = masks(bc, accent)
    stats = {"white_frac": float(wht.mean()), "magenta_frac": float(mag.mean())}

    # ---- base colour
    lum = bc.max(-1)
    out = bc.copy()
    stats["magenta_mean_before"] = [round(float(x), 3) for x in bc[mag > 0.8].mean(0)]
    for m, target, keep, lo_clip, power in ((wht, WHITE, 0.10, 0.50, 0.85), (mag, ACCENT_TARGET[accent], 0.05, 0.72, 0.45)):
        low = blur(lum * m, 28) / (blur(m, 28) + 1e-4)
        detail = np.clip(lum / (low + 1e-4), lo_clip, 1.06) ** power        # panel lines / edge wear stay (fainter on magenta)
        flat = target[None, None, :] * detail[..., None]
        k = (m * (1.0 - keep))[..., None]
        out = out * (1 - k) + flat * k
    stats["bc_white_std_before"] = float(lum[wht > 0.8].std()) if (wht > 0.8).any() else 0.0
    stats["bc_white_std_after"] = float(out.max(-1)[wht > 0.8].std()) if (wht > 0.8).any() else 0.0
    save(out, os.path.join(out_dir, "T_%s_BC.png" % stem), True)

    # ---- ORM (R AO, G roughness, B metallic)
    paint = np.clip(wht + mag, 0, 1)
    rough, metal = orm[..., 1], orm[..., 2]
    r_low = blur(rough, 20)
    r_hp = rough - r_low
    r_paint = (0.50 * wht + 0.44 * mag) / (paint + 1e-4) + 0.15 * r_hp
    r_other = np.maximum(0.5 * rough + 0.5 * r_low, 0.34)
    rough2 = np.clip(r_paint * paint + r_other * (1 - paint), 0.12, 0.92)
    m_low = blur(metal, 12)
    metal2 = np.clip((0.5 * metal + 0.5 * m_low) * (1 - paint), 0, 1)             # paint is not metal
    stats["rough_paint_std_before"] = float(rough[paint > 0.8].std())
    stats["rough_paint_std_after"] = float(rough2[paint > 0.8].std())
    stats["metal_paint_mean_before"] = float(metal[paint > 0.8].mean())
    save(np.stack([orm[..., 0], rough2, metal2], -1), os.path.join(out_dir, "T_%s_ORM.png" % stem), False)

    # ---- normal (DirectX, tangent space): remove broad lumps on paint, soften the rest
    n = nrm * 2.0 - 1.0
    xy = n[..., :2]
    low = blur(xy, 16)
    k_low = (0.85 * wht + 0.95 * mag)[..., None]                           # how much of the lumps to remove
    gain = (1.0 - 0.30 * wht - 0.55 * mag - 0.10 * (1 - paint))[..., None]  # remaining detail strength
    xy2 = (xy - low * k_low) * gain
    z = np.sqrt(np.clip(1.0 - (xy2 ** 2).sum(-1), 0.05, 1.0))
    n2 = np.concatenate([xy2, z[..., None]], -1)
    n2 /= np.linalg.norm(n2, axis=-1, keepdims=True)
    stats["normal_xy_rms_before"] = float(np.sqrt((xy[paint > 0.8] ** 2).mean()))
    stats["normal_xy_rms_after"] = float(np.sqrt((xy2[paint > 0.8] ** 2).mean()))
    save(n2 * 0.5 + 0.5, os.path.join(out_dir, "T_%s_N.png" % stem), False)

    # ---- emissive: gate the noise floor, keep the lights
    el = em.max(-1)
    gate = sstep(el, 0.10, 0.32)
    save(np.clip(em * gate[..., None] * 1.1, 0, 1), os.path.join(out_dir, "T_%s_E.png" % stem), True)
    stats["emissive_frac_before"] = float((el > 0.05).mean())
    stats["emissive_frac_after"] = float(((el * gate) > 0.05).mean())
    return stats
