# SPDX-License-Identifier: MIT
# Triad Drift - RiftHero close-range pass (ship builder, Nov 13 evening; Ryan in the Dominion hangar at 3-4 m: "not
# sharp", "melted, lumpy surfaces"). On top of the hybrid (Rift_hybrid_v05.blend):
#   1. the Rodin hull body is denoised with feature-preserving bilateral normal filtering (rh_denoise.py): the lumps on
#      large panels go, creases stay, silhouette / UVs / topology unchanged (every baked map still fits);
#   2. the hull normal map's broad band (the AI lumps baked into it) is attenuated, its fine band (panel lines) kept:
#      N = fine + k x broad, with a normalized (gutter-aware) Gaussian so empty texels do not bleed in;
#   3. the result is saved as Rift_hybrid_<tag>.blend and exported over export/SM_Rift.fbx + T_Rift_N.png (the v05
#      versions stay in git), with the v05 manifest's sockets / slots / portal entries carried over.
#   blender --background ArtSource/EngShips/RiftHero/Rift_hybrid_v05.blend --python rh_closerange.py -- --tag v06
#        [--sigma-r 0.28 --normal-iters 30 --vertex-iters 30 --sigma-s-k 2.2 --n-broad 0.4 --n-sigma 12] [--no-export]
import json, os, shutil, sys
import bpy
import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rh_common as C
import rh_denoise as D

ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []


def arg(name, default, cast=float):
    return cast(ARGS[ARGS.index(name) + 1]) if name in ARGS else default


TAG = arg("--tag", "v06", str)


def load_linear(path):
    im = bpy.data.images.load(path, check_existing=False)
    im.colorspace_settings.name = "Non-Color"
    w, h = im.size
    a = np.empty(w * h * 4, np.float32)
    im.pixels.foreach_get(a)
    bpy.data.images.remove(im)
    return a.reshape(h, w, 4)[..., :3].astype(np.float64)


def save_linear(arr, path):
    h, w = arr.shape[:2]
    im = bpy.data.images.new("__n__", w, h, alpha=False)
    im.colorspace_settings.name = "Non-Color"
    px = np.ones((h, w, 4), np.float32)
    px[..., :3] = np.clip(arr, 0.0, 1.0)
    im.pixels.foreach_set(px.ravel())
    im.filepath_raw = path
    im.file_format = "PNG"
    im.save()
    bpy.data.images.remove(im)


def gauss(img, sigma):
    """Separable Gaussian via FFT, wrap-free (padded)."""
    pad = int(sigma * 3)
    out = img
    for axis in (0, 1):
        n = out.shape[axis] + 2 * pad
        x = np.arange(n) - n // 2
        k = np.exp(-(x * x) / (2 * sigma * sigma))
        k /= k.sum()
        K = np.fft.rfft(np.fft.ifftshift(k))
        widths = [(0, 0)] * out.ndim
        widths[axis] = (pad, pad)
        p = np.pad(out, widths, mode="constant")
        f = np.fft.irfft(np.fft.rfft(p, axis=axis) * K.reshape([-1 if a == axis else 1 for a in range(out.ndim)]), n=n, axis=axis)
        out = np.take(f, np.arange(pad, pad + out.shape[axis]), axis=axis)
    return out


def soften_normal(path_in, path_out, broad_keep, sigma):
    n = load_linear(path_in)
    v = n * 2.0 - 1.0                                          # DirectX tangent normal, xy carry the slope
    mask = (np.abs(v[..., 0]) + np.abs(v[..., 1]) > 1e-4) | (v[..., 2] < 0.999)   # texels with content (not flat gutter)
    m = mask.astype(np.float64)[..., None]
    low = gauss(v[..., :2] * m, sigma) / np.maximum(gauss(m, sigma), 1e-6)
    xy = v[..., :2] - (1.0 - broad_keep) * low * m
    z = np.sqrt(np.clip(1.0 - (xy ** 2).sum(-1), 0.0, 1.0))
    out = np.concatenate([xy, z[..., None]], -1) * 0.5 + 0.5
    save_linear(out, path_out)
    return {"texels_with_content": round(float(mask.mean()), 3), "mean_slope_before": round(float(np.abs(v[..., :2])[mask].mean()), 4),
            "mean_slope_after": round(float(np.abs(xy)[mask].mean()), 4)}


def main():
    res = {"tag": TAG}
    body = bpy.data.objects["SM_Rift"]
    co, tris, pairs, me = D._arrays(body)
    res["denoise"] = D.denoise(body, sigma_s=arg("--sigma-s-k", 2.2) * me, sigma_r=arg("--sigma-r", 0.28),
                               normal_iters=int(arg("--normal-iters", 30)), vertex_iters=int(arg("--vertex-iters", 30)))
    n_src = os.path.join(C.EXPORT, "T_Rift_N.png")
    keep = os.path.join(C.ANALYSIS, "T_Rift_N_v05.png")
    if not os.path.isfile(keep):
        shutil.copyfile(n_src, keep)                            # the v05 map, the input of every later run
    n_out = os.path.join(C.ANALYSIS, "T_Rift_N_%s.png" % TAG)
    res["normal"] = soften_normal(keep, n_out, arg("--n-broad", 0.4), arg("--n-sigma", 12.0))
    # point the preview material at the softened map (review renders show what Unreal gets)
    for mat in body.data.materials:
        for nd in (mat.node_tree.nodes if mat and mat.use_nodes else []):
            if nd.type == "TEX_IMAGE" and nd.image and "T_Rift_N" in os.path.basename(nd.image.filepath):
                nd.image = bpy.data.images.load(n_out, check_existing=False)
                nd.image.colorspace_settings.name = "Non-Color"
    blend = os.path.join(C.ROOT, "Rift_hybrid_%s.blend" % TAG)
    bpy.ops.wm.save_as_mainfile(filepath=blend, compress=True)
    res["blend"] = blend
    if "--no-export" not in ARGS:
        man_path = os.path.join(C.EXPORT, "SM_Rift.manifest.json")
        with open(man_path, encoding="utf-8") as fh:
            old = json.load(fh)
        for stale in ("SM_Rift.fbx", "SM_Rift.manifest.json"):
            if os.path.isfile(os.path.join(C.EXPORT, stale)):
                os.remove(os.path.join(C.EXPORT, stale))
        ex = C.skill("blender-unreal-export/scripts/export_for_unreal.py")
        try:
            ex["main"](root=body.name, out_dir=C.EXPORT, front_axis="+X", collision="auto")
        except AttributeError as error:
            res["export_note"] = "exporter cleanup: %s" % error
        with open(man_path, encoding="utf-8") as fh:
            man = json.load(fh)
        for k in ("ue_sockets", "slots", "portal_disc"):
            if k in old:
                man[k] = old[k]
        man["closerange"] = {"tag": TAG, "denoise": res["denoise"], "normal": res["normal"]}
        with open(man_path, "w", encoding="utf-8") as fh:
            json.dump(man, fh, indent=2)
        shutil.copyfile(n_out, n_src)
        res["export"] = {k: man.get(k) for k in ("tris", "size_cm", "materials", "collision", "errors", "warnings")}
    C.out(res)


main()
