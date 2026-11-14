# SPDX-License-Identifier: MIT
# #149 (Nick's Lead: the intro's pilot shots show empty cockpit cavities; no seated pilots: intentional opaque /
# tinted glazing instead). Builds M_TD_CanopyTint (opaque, dark, glossy, a gradient reflection so it never reads as a
# flat black patch, a faint pilot-colour fresnel edge) and one instance per ship, then points each ship's canopy at it
# through its visuals asset's part override (the canopy meshes, MI_*_Canopy and M_TD_ShipGlass stay as they are: the
# old look comes back by clearing the override; docs/eng/ART-HISTORY.md).
#   powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\Kit\scripts\ue_canopy_tint.py
import unreal

eas, mel, tools = unreal.EditorAssetLibrary, unreal.MaterialEditingLibrary, unreal.AssetToolsHelpers.get_asset_tools()
MASTER = "/Game/TriadDrift/Gameplay/Ships/Materials/M_TD_CanopyTint"
SQUAD = "/Game/TriadDrift/Gameplay/Characters/Squad/DA_%s_Visuals"
EDGE = {"Rift": (1.0, 0.12, 0.75), "Lancer": (0.0, 0.8, 1.0), "Bastion": (1.0, 0.58, 0.06)}


def log(m):
    unreal.log("TDPY: " + m)


def master():
    folder, name = MASTER.rsplit("/", 1)
    mat = eas.load_asset(MASTER) if eas.does_asset_exist(MASTER) else tools.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    mat.set_editor_property("used_with_instanced_static_meshes", True)
    mel.delete_all_material_expressions(mat)

    def node(cls, x, y):
        return mel.create_material_expression(mat, cls, x, y)

    def sp(n, v, y):
        e = node(unreal.MaterialExpressionScalarParameter, -1100, y)
        e.set_editor_property("parameter_name", n)
        e.set_editor_property("default_value", v)
        return e

    def vp(n, c, y):
        e = node(unreal.MaterialExpressionVectorParameter, -1100, y)
        e.set_editor_property("parameter_name", n)
        e.set_editor_property("default_value", unreal.LinearColor(c[0], c[1], c[2], 1.0))
        return e
    tint = vp("Tint", (0.010, 0.012, 0.018), -600)
    edge = vp("EdgeTint", (0.0, 0.8, 1.0), -450)
    # round 2 (in-game frames: the first values read pale, like painted plastic, under the bright nebula):
    # darker reflections, softer gloss, a thinner edge
    sky = vp("ReflectTop", (0.22, 0.25, 0.32), -300)
    low = vp("ReflectLow", (0.02, 0.02, 0.03), -150)
    edge_glow, edge_exp = sp("EdgeGlow", 0.3, 0), sp("EdgeExponent", 5.0, 80)
    refl = sp("ReflectStrength", 0.08, 160)
    rough, spec, metal = sp("Roughness", 0.2, 240), sp("Specular", 0.35, 320), sp("Metallic", 0.0, 400)
    fres = node(unreal.MaterialExpressionFresnel, -800, -40)
    rv = node(unreal.MaterialExpressionReflectionVectorWS, -900, -260)
    rz = node(unreal.MaterialExpressionComponentMask, -760, -260)
    rz.set_editor_property("r", False)
    rz.set_editor_property("g", False)
    rz.set_editor_property("b", True)
    half = node(unreal.MaterialExpressionConstantBiasScale, -640, -260)       # z -1..1 -> 0..1
    half.set_editor_property("bias", 1.0)
    half.set_editor_property("scale", 0.5)
    grad = node(unreal.MaterialExpressionLinearInterpolate, -500, -220)
    gref = node(unreal.MaterialExpressionMultiply, -380, -220)
    eg = node(unreal.MaterialExpressionMultiply, -500, -420)
    eg2 = node(unreal.MaterialExpressionMultiply, -380, -420)
    emis = node(unreal.MaterialExpressionAdd, -240, -320)
    ok = True
    ok &= mel.connect_material_expressions(edge_exp, "", fres, "ExponentIn")
    ok &= mel.connect_material_expressions(rv, "", rz, "")
    ok &= mel.connect_material_expressions(rz, "", half, "")
    ok &= mel.connect_material_expressions(low, "", grad, "A")
    ok &= mel.connect_material_expressions(sky, "", grad, "B")
    ok &= mel.connect_material_expressions(half, "", grad, "Alpha")
    ok &= mel.connect_material_expressions(grad, "", gref, "A")
    ok &= mel.connect_material_expressions(refl, "", gref, "B")
    ok &= mel.connect_material_expressions(edge, "", eg, "A")
    ok &= mel.connect_material_expressions(fres, "", eg, "B")
    ok &= mel.connect_material_expressions(eg, "", eg2, "A")
    ok &= mel.connect_material_expressions(edge_glow, "", eg2, "B")
    ok &= mel.connect_material_expressions(eg2, "", emis, "A")
    ok &= mel.connect_material_expressions(gref, "", emis, "B")
    ok &= mel.connect_material_property(tint, "", unreal.MaterialProperty.MP_BASE_COLOR)
    ok &= mel.connect_material_property(emis, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    ok &= mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    ok &= mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
    ok &= mel.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)
    if not ok:
        log("FAILED wiring " + MASTER)
        return None
    try:
        mel.recompile_material(mat)
    except Exception:
        pass
    eas.save_asset(MASTER, only_if_is_dirty=False)
    return mat


def instance(ship, parent):
    path = "/Game/TriadDrift/Gameplay/Ships/%s/MI_%s_CanopyTint" % (ship, ship)
    folder, name = path.rsplit("/", 1)
    mi = eas.load_asset(path) if eas.does_asset_exist(path) else tools.create_asset(name, folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, parent)
    c = EDGE[ship]
    mel.set_material_instance_vector_parameter_value(mi, "EdgeTint", unreal.LinearColor(c[0], c[1], c[2], 1.0))
    if ship == "Bastion":
        # r4 (in game the hull-hugging shell read as a flat dark patch): more sheen and a stronger amber edge, so the
        # wide glazing shows its curvature
        # r5 (the Lead: it read light): as dark and glossy as the Lancer's, amber only as a thin edge
        for k, v in (("ReflectStrength", 0.04), ("EdgeGlow", 0.35), ("EdgeExponent", 6.0), ("Roughness", 0.2), ("Specular", 0.35)):
            mel.set_material_instance_scalar_parameter_value(mi, k, v)
    eas.save_asset(path, only_if_is_dirty=False)
    return mi


def _copy(part, overrides=None):
    q = unreal.TDShipVisualPart()
    for k in ("part_name", "mesh", "parent_socket", "relative_transform", "material_overrides"):
        q.set_editor_property(k, part.get_editor_property(k))
    if overrides is not None:
        q.set_editor_property("material_overrides", overrides)
    return q


def override_canopy(ship, mi):
    """Rebuild the parts list (the getter hands back copies) with the canopy part pointing at the tint instance."""
    da = eas.load_asset(SQUAD % ship)
    out, hit = [], False
    for part in da.get_editor_property("parts"):
        m = part.get_editor_property("mesh")
        is_canopy = bool(m) and m.get_name() == "SM_%s_Canopy" % ship
        hit |= is_canopy
        out.append(_copy(part, [mi] if is_canopy else None))
    if not hit:
        log("%s: no canopy part" % ship)
        return
    da.set_editor_property("parts", out)
    eas.save_asset(SQUAD % ship, only_if_is_dirty=False)
    got = [[o.get_path_name() for o in p.get_editor_property("material_overrides")] for p in eas.load_asset(SQUAD % ship).get_editor_property("parts")]
    log("%s parts overrides now %s" % (ship, got))


def main():
    m = master()
    if not m:
        return
    for ship in ("Rift", "Lancer", "Bastion"):
        mi = instance(ship, m)
        if ship != "Bastion":
            override_canopy(ship, mi)
    log("done")


main()


# ---- the Bastion: a fitted canopy shell (ArtSource/EngShips/BastionCanopy) as a visuals part at the root
import os  # noqa: E402
import sys  # noqa: E402

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))


def bastion_shell():
    import ue_import_kit as K
    notes = []
    dest = "/Game/TriadDrift/Gameplay/Ships/Bastion"
    K.PIPELINE = dest + "/Pipelines/PL_Bastion_Canopy"
    pipe = K.ensure_pipeline(notes)
    fbx = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))), "EngShips", "BastionCanopy", "SM_Bastion_Canopy.fbx")
    params = unreal.ImportAssetParameters()
    params.is_automated = True
    params.replace_existing = True
    params.override_pipelines.append(pipe)
    mgr = unreal.InterchangeManager.get_interchange_manager_scripted()
    mgr.import_asset(dest, unreal.InterchangeManager.create_source_data(fbx), params)
    mesh = eas.load_asset(dest + "/SM_Bastion_Canopy")
    if not mesh:
        log("FAILED: SM_Bastion_Canopy import %s" % notes)
        return
    mi = eas.load_asset("/Game/TriadDrift/Gameplay/Ships/Bastion/MI_Bastion_CanopyTint")
    mesh.set_material(0, mi)
    sme = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    try:
        sme.remove_collisions(mesh)
    except Exception:
        pass
    ns = mesh.get_editor_property("nanite_settings")
    ns.enabled = False
    mesh.set_editor_property("nanite_settings", ns)
    eas.save_loaded_asset(mesh)
    bb = mesh.get_bounding_box()
    log("SM_Bastion_Canopy bounds %s .. %s tris %d" % (bb.min, bb.max, mesh.get_num_triangles(0)))
    da = eas.load_asset(SQUAD % "Bastion")
    parts = [_copy(p) for p in da.get_editor_property("parts")]
    if not any(p.get_editor_property("mesh") and p.get_editor_property("mesh").get_name() == "SM_Bastion_Canopy" for p in parts):
        part = unreal.TDShipVisualPart()
        part.set_editor_property("part_name", "Canopy")
        part.set_editor_property("mesh", mesh)
        # on the hull's Cockpit socket (hull-local (120, 0, 20), no rotation) so it follows the hull mesh exactly;
        # the shell's own pivot is the hull origin
        sock = eas.load_asset(dest + "/SM_Bastion").find_socket("Cockpit")
        loc = sock.get_editor_property("relative_location") if sock else unreal.Vector(120.0, 0.0, 20.0)
        part.set_editor_property("parent_socket", "Cockpit" if sock else "None")
        part.set_editor_property("relative_transform", unreal.Transform(location=unreal.Vector(-loc.x, -loc.y, -loc.z)) if sock else unreal.Transform())
        part.set_editor_property("material_overrides", [mi])
        parts.append(part)
        da.set_editor_property("parts", parts)
        eas.save_asset(SQUAD % "Bastion", only_if_is_dirty=False)
    log("Bastion parts now %s" % [(str(p.get_editor_property("part_name")), str(p.get_editor_property("parent_socket"))) for p in eas.load_asset(SQUAD % "Bastion").get_editor_property("parts")])


bastion_shell()
