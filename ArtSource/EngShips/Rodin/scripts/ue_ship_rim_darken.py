# SPDX-License-Identifier: MIT
# Triad Drift - add a fresnel darkening of base colour to M_TD_Ship IN PLACE (T1, Nov 14; the Lead's option (a) for the
# Lancer's cyan worst case). Headless:
#   powershell -File Tools\td-python.ps1 -Script ArtSource\EngShips\Rodin\scripts\ue_ship_rim_darken.py
#
# Base colour -> Lerp(base, RimDarkenColor, saturate(Fresnel(RimDarkenExponent) x RimDarken)) -> BaseColor.
# RimDarken defaults to 0, so every instance that does not set it renders exactly as before (the lerp's alpha is 0).
# Only the base colour input is rewired; every other node, the usage flags and the instances stay as they are.
# Idempotent: a second run finds its own Lerp (description "TD rim darken") and does nothing.
# ue_import_rodin_ships.py builds the same nodes when it rebuilds the master from scratch.

import unreal

MASTER = "/Game/TriadDrift/Gameplay/Ships/Materials/M_TD_Ship"
TAG = "TD rim darken"
mel = unreal.MaterialEditingLibrary
eas = unreal.EditorAssetLibrary
mp = unreal.MaterialProperty


def log(msg):
    unreal.log("TDPY: " + msg)


def main():
    mat = eas.load_asset(MASTER)
    if not mat:
        log("FAILED: no " + MASTER)
        return False
    base = mel.get_material_property_input_node(mat, mp.MP_BASE_COLOR)
    if not base:
        log("FAILED: nothing on BaseColor")
        return False
    if isinstance(base, unreal.MaterialExpressionLinearInterpolate) and base.get_editor_property("desc") == TAG:
        log("already has the rim darken lerp - nothing to do")
        return True
    out = mel.get_material_property_input_node_output_name(mat, mp.MP_BASE_COLOR)
    flags_before = {}
    for f in ("used_with_nanite", "used_with_instanced_static_meshes", "used_with_niagara_mesh_particles", "used_with_static_lighting"):
        try:
            flags_before[f] = mat.get_editor_property(f)
        except Exception:
            pass
    E = unreal
    x, y = -450, -520
    s = mel.create_material_expression(mat, E.MaterialExpressionScalarParameter, x - 500, y + 120)
    s.set_editor_property("parameter_name", "RimDarken")
    s.set_editor_property("default_value", 0.0)
    c = mel.create_material_expression(mat, E.MaterialExpressionVectorParameter, x - 500, y - 80)
    c.set_editor_property("parameter_name", "RimDarkenColor")
    c.set_editor_property("default_value", unreal.LinearColor(0.008, 0.01, 0.02, 1.0))
    ex = mel.create_material_expression(mat, E.MaterialExpressionScalarParameter, x - 650, y + 220)
    ex.set_editor_property("parameter_name", "RimDarkenExponent")
    ex.set_editor_property("default_value", 2.5)
    fres = mel.create_material_expression(mat, E.MaterialExpressionFresnel, x - 400, y + 220)
    fres.set_editor_property("base_reflect_fraction", 0.0)
    m = mel.create_material_expression(mat, E.MaterialExpressionMultiply, x - 250, y + 160)
    sat = mel.create_material_expression(mat, E.MaterialExpressionSaturate, x - 150, y + 160)
    lerp = mel.create_material_expression(mat, E.MaterialExpressionLinearInterpolate, x, y)
    lerp.set_editor_property("desc", TAG)
    ok = True
    ok &= mel.connect_material_expressions(ex, "", fres, "ExponentIn")
    ok &= mel.connect_material_expressions(fres, "", m, "A")
    ok &= mel.connect_material_expressions(s, "", m, "B")
    ok &= mel.connect_material_expressions(m, "", sat, "")
    ok &= mel.connect_material_expressions(base, out, lerp, "A")
    ok &= mel.connect_material_expressions(c, "RGB", lerp, "B")
    ok &= mel.connect_material_expressions(sat, "", lerp, "Alpha")
    ok &= mel.connect_material_property(lerp, "", mp.MP_BASE_COLOR)
    if not ok:
        log("FAILED: could not wire the rim darken nodes")
        return False
    flags_after = {f: mat.get_editor_property(f) for f in flags_before}
    log("usage flags before %s after %s" % (flags_before, flags_after))
    try:
        mel.recompile_material(mat)
    except Exception:
        log("note: no renderer in this headless run; the master compiles on first load")
    eas.save_asset(MASTER, only_if_is_dirty=False)
    log("rim darken added to " + MASTER + " (base was %s.%s)" % (base.get_name(), out or "-"))
    return True


if main():
    log("OK")
