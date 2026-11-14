# SPDX-License-Identifier: MIT
# Crew readability at distance (Lead, Nov 13 night; materials only: no meshes, skeletons, anims or code).
# M_NullTrooper (the parent of the Grunt, Trooper, Heavy and Warden instances) was: Emissive = T_*_E x EmissiveStrength.
# Now:  Emissive = T_*_E x EmissiveStrength x lerp(1, FarBoost, far)  +  Fresnel^RimExponent x RimColor x RimStrength x lerp(RimNear, 1, far)
#   far = saturate((PixelDepth - FarStart) / (FarEnd - FarStart))
# so the eyes and the red-orange marks are as authored up close and up to FarBoost x brighter at 30 m (where they are a few
# pixels), and a thin dim red-orange edge light separates the silhouette from dark walls, mostly at range. Re-runnable: the
# graph is rebuilt from the three texture samples each time.
#   powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\Kit\scripts\ue_crew_read.py
import unreal

eas = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
P = "/Game/TriadDrift/Gameplay/Characters/"
MASTER = P + "NullTrooper/M_NullTrooper"
DEFAULTS = {"FarStart": 800.0, "FarEnd": 3000.0, "FarBoost": 5.0, "RimExponent": 3.0, "RimStrength": 0.6, "RimNear": 0.3}
RIM = (1.0, 0.28, 0.07)
# per instance: scalars and the rim colour (the Warden: bigger, brighter marks and a redder, stronger rim)
MIS = {
    "NullGrunt/MI_NullGrunt": ({"EmissiveStrength": 14.0}, None),
    "NullTrooper/MI_NullTrooper": ({"EmissiveStrength": 14.0}, None),
    "NullHeavy/MI_NullHeavy": ({"EmissiveStrength": 14.0, "RimStrength": 0.7}, None),
    "Warden/MI_Warden": ({"EmissiveStrength": 16.0, "FarBoost": 6.0, "RimStrength": 0.9}, (1.0, 0.12, 0.04)),
    "Warden/MI_Warden_GunArm": ({"EmissiveStrength": 12.0, "FarBoost": 6.0, "RimStrength": 0.9}, (1.0, 0.12, 0.04)),
    "Warden/MI_Warden_Shield": ({"EmissiveStrength": 3.0, "RimStrength": 0.3}, (1.0, 0.12, 0.04)),
}


def log(m):
    unreal.log("TDPY: " + m)


def main():
    mat = eas.load_asset(MASTER)
    S = unreal.MaterialProperty
    tex = {}
    for e in [mel.get_material_property_input_node(mat, S.MP_BASE_COLOR), mel.get_material_property_input_node(mat, S.MP_ROUGHNESS),
              mel.get_material_property_input_node(mat, S.MP_NORMAL)]:
        tex[str(e.get_editor_property("parameter_name"))] = e.get_editor_property("texture")
    em = mel.get_material_property_input_node(mat, S.MP_EMISSIVE_COLOR)
    # find the emissive texture: the Multiply's input chain (rebuilt graphs keep the parameter names)
    etex = None
    for n in ("EmissiveTex",):
        try:
            etex = mel.get_material_default_texture_parameter_value(mat, n)
        except Exception:
            pass
    log("textures %s emissive %s" % (sorted(tex), etex.get_name() if etex else None))
    if etex is None or len(tex) != 3:
        log("FAILED: unexpected graph")
        return
    old = {"BaseColorTex": tex.get("BaseColorTex"), "ORMTex": tex.get("ORMTex"), "NormalTex": tex.get("NormalTex"), "EmissiveTex": etex}
    es = mel.get_material_default_scalar_parameter_value(mat, "EmissiveStrength")
    mel.delete_all_material_expressions(mat)

    def node(cls, x, y):
        return mel.create_material_expression(mat, cls, x, y)

    def tp(name, sampler, y):
        n = node(unreal.MaterialExpressionTextureSampleParameter2D, -1400, y)
        n.set_editor_property("parameter_name", name)
        n.set_editor_property("texture", old[name])
        n.set_editor_property("sampler_type", sampler)
        return n

    def sp(name, v, y):
        n = node(unreal.MaterialExpressionScalarParameter, -1400, y)
        n.set_editor_property("parameter_name", name)
        n.set_editor_property("default_value", v)
        return n
    T = unreal.MaterialSamplerType
    bc, orm, nm, e = tp("BaseColorTex", T.SAMPLERTYPE_COLOR, -800), tp("ORMTex", T.SAMPLERTYPE_MASKS, -500), \
        tp("NormalTex", T.SAMPLERTYPE_NORMAL, -200), tp("EmissiveTex", T.SAMPLERTYPE_COLOR, 100)
    est = sp("EmissiveStrength", es, 350)
    fs, fe, fb = sp("FarStart", DEFAULTS["FarStart"], 500), sp("FarEnd", DEFAULTS["FarEnd"], 600), sp("FarBoost", DEFAULTS["FarBoost"], 700)
    rx, rs, rn = sp("RimExponent", DEFAULTS["RimExponent"], 900), sp("RimStrength", DEFAULTS["RimStrength"], 1000), sp("RimNear", DEFAULTS["RimNear"], 1100)
    rc = node(unreal.MaterialExpressionVectorParameter, -1400, 1250)
    rc.set_editor_property("parameter_name", "RimColor")
    rc.set_editor_property("default_value", unreal.LinearColor(RIM[0], RIM[1], RIM[2], 1.0))
    depth = node(unreal.MaterialExpressionPixelDepth, -1150, 450)
    sub1, sub2 = node(unreal.MaterialExpressionSubtract, -1000, 480), node(unreal.MaterialExpressionSubtract, -1000, 580)
    div = node(unreal.MaterialExpressionDivide, -850, 520)
    far = node(unreal.MaterialExpressionSaturate, -720, 520)
    one = node(unreal.MaterialExpressionConstant, -720, 640)
    one.set_editor_property("r", 1.0)
    boost = node(unreal.MaterialExpressionLinearInterpolate, -580, 600)
    m1, m2 = node(unreal.MaterialExpressionMultiply, -900, 150), node(unreal.MaterialExpressionMultiply, -420, 200)
    fres = node(unreal.MaterialExpressionFresnel, -1000, 900)
    rimfar = node(unreal.MaterialExpressionLinearInterpolate, -580, 1050)
    r1, r2, r3 = node(unreal.MaterialExpressionMultiply, -850, 950), node(unreal.MaterialExpressionMultiply, -700, 1000), \
        node(unreal.MaterialExpressionMultiply, -420, 1000)
    add = node(unreal.MaterialExpressionAdd, -250, 400)
    c = mel.connect_material_expressions
    ok = True
    ok &= c(depth, "", sub1, "A") and c(fs, "", sub1, "B")
    ok &= c(fe, "", sub2, "A") and c(fs, "", sub2, "B")
    ok &= c(sub1, "", div, "A") and c(sub2, "", div, "B") and c(div, "", far, "")
    ok &= c(one, "", boost, "A") and c(fb, "", boost, "B") and c(far, "", boost, "Alpha")
    ok &= c(e, "RGB", m1, "A") and c(est, "", m1, "B")
    ok &= c(m1, "", m2, "A") and c(boost, "", m2, "B")
    ok &= c(rx, "", fres, "ExponentIn")
    ok &= c(fres, "", r1, "A") and c(rc, "", r1, "B")
    ok &= c(r1, "", r2, "A") and c(rs, "", r2, "B")
    ok &= c(rn, "", rimfar, "A") and c(one, "", rimfar, "B") and c(far, "", rimfar, "Alpha")
    ok &= c(r2, "", r3, "A") and c(rimfar, "", r3, "B")
    ok &= c(m2, "", add, "A") and c(r3, "", add, "B")
    P_ = mel.connect_material_property
    ok &= P_(bc, "RGB", S.MP_BASE_COLOR) and P_(orm, "G", S.MP_ROUGHNESS) and P_(orm, "B", S.MP_METALLIC)
    ok &= P_(nm, "RGB", S.MP_NORMAL) and P_(add, "", S.MP_EMISSIVE_COLOR)
    if not ok:
        log("FAILED: wiring")
        return
    try:
        mel.recompile_material(mat)
    except Exception:
        log("M_NullTrooper compiles on first load (no renderer here)")
    eas.save_asset(MASTER, only_if_is_dirty=False)
    for rel, (scal, rim) in MIS.items():
        mi = eas.load_asset(P + rel)
        if mi is None:
            log("missing " + rel)
            continue
        for k, v in scal.items():
            mel.set_material_instance_scalar_parameter_value(mi, k, v)
        if rim:
            mel.set_material_instance_vector_parameter_value(mi, "RimColor", unreal.LinearColor(rim[0], rim[1], rim[2], 1.0))
        eas.save_loaded_asset(mi)
        log("%s %s %s" % (rel, scal, rim))
    drone = eas.load_asset(P + "NullDrone/MI_NullDrone")           # its parent is the shared prop master: the strength only
    mel.set_material_instance_scalar_parameter_value(drone, "EmissiveStrength", 9.0)
    eas.save_loaded_asset(drone)
    log("crew read OK (master EmissiveStrength default %.1f)" % es)


main()
