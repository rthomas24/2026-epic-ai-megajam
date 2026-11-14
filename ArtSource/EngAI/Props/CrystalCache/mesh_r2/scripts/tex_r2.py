#!/usr/bin/env python3
"""Post-process the raw crystal bakes into the shipped 2K maps (numpy/PIL).
BC = Col bake, E = Emi bake (noise floor cut), ORM = (AO, rough, metal), N = OpenGL bake with green flipped
(DirectX for Unreal). Every map gets its empty UV space filled from the nearest covered texel (EDT dilation)
so mips never pull in black."""
import sys
import numpy as np
from PIL import Image
from scipy import ndimage
from pathlib import Path

tex = Path(sys.argv[1]); out = Path(sys.argv[2])
P = "T_CrystalCacheCrystal"
ld = lambda n, m="RGB": np.asarray(Image.open(tex / n).convert(m)).astype(np.float32) / 255.0
mask = ld(P + "_MASK_raw.png", "L") > 0.5
bc, emi, rm = ld(P + "_BC_raw.png"), ld(P + "_E_raw.png"), ld(P + "_RM_raw.png")
ao = ld(P + "_AO_raw.png", "L")
nrm = ld(P + "_N_ogl.png")
print("coverage", round(float(mask.mean()), 3))
_, (iy, ix) = ndimage.distance_transform_edt(~mask, return_indices=True)
fill = lambda a: a[iy, ix]

# AO: faces buried inside neighbouring shards / the nest bake black and are never seen; keep a floor
ao = np.clip(ao, 0.0, 1.0)
ao_cov = ao[mask]
print("AO covered median", round(float(np.median(ao_cov)), 3))
ao = np.where(mask, np.maximum(ao, 0.08), ao)
emi[np.max(emi, axis=-1) <= 10 / 255] = 0
orm = np.stack([ao, np.clip(rm[..., 0], 0.06, 0.90), np.clip(rm[..., 1], 0.0, 0.25)], -1)
n2 = nrm.copy(); n2[..., 1] = 1.0 - nrm[..., 1]
save = lambda a, n: Image.fromarray((np.clip(fill(a), 0, 1) * 255 + 0.5).astype(np.uint8)).save(out / n, optimize=True)
save(bc, P + "_BC.png"); save(emi, P + "_E.png"); save(orm, P + "_ORM.png"); save(n2, P + "_N.png")
lum = 0.2126 * bc[..., 0] + 0.7152 * bc[..., 1] + 0.0722 * bc[..., 2]
print("BC covered median sRGB", round(float(np.median(lum[mask])) * 255, 1))
print("textures written")
