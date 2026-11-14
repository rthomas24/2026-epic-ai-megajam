# SPDX-License-Identifier: MIT
# Rebuild M_TD_PlanetAtmosphere (the additive rim shell around each galaxy globe) with a camera-distance fade, so the
# planet's ground and clouds show as the ship dives in instead of a flat teal wall (T4's finding on the Verdance dive).
# Same graph as ArtSource/EngProps/Planets/scripts/ue_import_planets.py build_atmo_master(), plus:
#   d    = distance(camera, shell centre) / shell radius        (1 = on the shell)
#   fade = lerp(NearKeep, 1, saturate((d - FadeNearR) / FadeRangeR))
#   and the rim glow is limited to the silhouette band when close (RimPower raised towards CloseRimPower).
# The instances MI_PlanetAtmo_Verdance / _Cinder keep their own values; this sets the new parameters on both.
#   powershell -File ArtSource\EngShips\Dominion\scripts\run_ue_rendered.ps1 -Script ArtSource\EngEnvironment\scripts\ue_atmo_fade_build.py
# Old look: the asset at git 5af18d02 (ART-HISTORY row "Planet atmosphere shell").
import os
import sys

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(REPO, "ArtSource", "EngProps", "Planets", "scripts"))
import ue_mat_builder  # noqa: E402

DEST = "/Game/TriadDrift/Gameplay/Props/Planets"
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
MP = unreal.MaterialProperty
problems = []
SUN_DIR = (0.779, 0.546, 0.309)   # ue_planet_surface.SUN_DIR (code sets it per level)

# name: (default, Verdance, Cinder)
NEW = {
    "FadeNearR": (1.05, 1.05, 1.05),
    "FadeRangeR": (2.2, 2.4, 1.8),
    "NearKeep": (0.06, 0.04, 0.12),
    "CloseRimPower": (9.0, 10.0, 7.0),
}


def log(m):
    unreal.log("TDPY: " + str(m))


def main():
    path = DEST + "/M_TD_PlanetAtmosphere"
    mat = eal.load_asset(path)
    if mat is None:
        log("FAILED (no %s)" % path)
        return
    mel.delete_all_material_expressions(mat)
    b = ue_mat_builder.MB(mat, problems)
    n = b.e(unreal.MaterialExpressionVertexNormalWS)
    v = b.e(unreal.MaterialExpressionCameraVectorWS)
    x = b.sat(b.dot(n, v))
    # camera distance in shell radii
    cam = b.e(unreal.MaterialExpressionCameraPositionWS)
    obj = b.e(unreal.MaterialExpressionObjectPositionWS)
    rad = b.e(unreal.MaterialExpressionObjectRadius)
    dist = b.e(unreal.MaterialExpressionDistance)
    b.c(cam, "", dist, "A")
    b.c(obj, "", dist, "B")
    d = b.div(dist, rad)
    t = b.sat(b.div(b.sub(d, b.scalar("FadeNearR", NEW["FadeNearR"][0])), b.scalar("FadeRangeR", NEW["FadeRangeR"][0])))
    fade = b.lerp(b.scalar("NearKeep", NEW["NearKeep"][0]), b.const(1.0), t)
    rim_p = b.lerp(b.scalar("CloseRimPower", NEW["CloseRimPower"][0]), b.scalar("RimPower", 4.0), t)
    inner = b.pow(b.one_minus(x), rim_p)
    edge = b.pow(b.sat(b.mul(x, b.scalar("EdgeSoftness", 4.5))), b.const(3.0))
    sun = b.vector("SunDirection", SUN_DIR)
    ndl = b.dot(n, b.norm(b.mask(sun, True, True, True)))
    day = b.sat(b.add(b.mul(ndl, b.const(0.8)), b.const(0.45)))
    day = b.lerp(b.const(1.0), day, b.scalar("SunMaskAmount", 0.7))
    glow = b.mul(b.mul(b.mul(inner, edge), day), fade)
    col = b.mul(b.vector("AtmoColor", (0.2, 0.9, 0.8)), b.scalar("AtmoIntensity", 3.0))
    b.out(b.mul(col, glow), "", MP.MP_EMISSIVE_COLOR)
    mel.recompile_material(mat)
    mel.layout_material_expressions(mat)
    eal.save_asset(path, only_if_is_dirty=False)
    for i, p in ((1, "Verdance"), (2, "Cinder")):
        ipath = "%s/MI_PlanetAtmo_%s" % (DEST, p)
        inst = eal.load_asset(ipath)
        for k, vals in NEW.items():
            if not mel.set_material_instance_scalar_parameter_value(inst, k, float(vals[i])):
                problems.append("%s: %s" % (p, k))
        mel.update_material_instance(inst)
        eal.save_asset(ipath, only_if_is_dirty=False)
    for p in problems:
        log("PROBLEM: " + p)
    log("FAILED (%d problems)" % len(problems) if problems else "OK")


main()
