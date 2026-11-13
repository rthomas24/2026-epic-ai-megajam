# SPDX-License-Identifier: MIT
# Triad Drift - import the DominionInterior kit (ArtSource/EngProps/DominionInterior/export/SM_DI_*.fbx) into Unreal.
# Headless, from the repo root:
#   powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\DominionInterior\scripts\ue_import_di_kit.py
#
# Writes ONLY under /Game/TriadDrift/Gameplay/Props/DominionInterior:
#   Textures/T_DI_Atlas_BC|N|ORM      the kit's one shared 2K atlas
#   MI_DI_Atlas                        instance of the engineering master /Game/TriadDrift/Gameplay/Materials/M_TD_PropTextured
#   MI_DI_GlowRed, MI_DI_GlowWhite, MI_DI_Screen, MI_DI_Cell, MI_DI_CoreGlow
#                                      instances of /Game/TriadDrift/Gameplay/Props/Dominion/M_TD_DominionGlow
#                                      (GlowColor, GlowStrength, PulseAmount, PulseRate)
#   Materials/M_DI_Shield + MI_DI_Shield   translucent unlit shell (ShieldColor, ShieldStrength, ShieldOpacity)
#   Pipelines/PL_DI_StaticMesh         Interchange pipeline (UCX collision by name, no Nanite, no lightmap UVs)
#   SM_DI_*                            the meshes; slots mapped by name to the instances above
# Reads (never modifies) the two masters above. Idempotent. Verifies each mesh against review/size_check_r3.json
# (bounds min / max, so the pivot too), triangles, slots and collision, and prints TDPY: OK only if all pass.
import glob
import json
import os

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
REPO = os.path.dirname(os.path.dirname(os.path.dirname(KIT)))
DEST = "/Game/TriadDrift/Gameplay/Props/DominionInterior"
TEX_DEST = DEST + "/Textures"
PIPELINE = DEST + "/Pipelines/PL_DI_StaticMesh"
SHIELD = DEST + "/Materials/M_DI_Shield"
MASTER_TEX = "/Game/TriadDrift/Gameplay/Materials/M_TD_PropTextured"
MASTER_GLOW = "/Game/TriadDrift/Gameplay/Props/Dominion/M_TD_DominionGlow"
SIZE_CHECK = os.path.join(KIT, "review", "size_check_r3.json")

GLOWS = {   # slot key: (scalars, colour)
    "GlowRed": ({"GlowStrength": 8.0, "PulseAmount": 0.0}, (1.0, 0.14, 0.02)),
    "GlowWhite": ({"GlowStrength": 14.0, "PulseAmount": 0.0}, (0.9, 0.95, 1.0)),
    "Screen": ({"GlowStrength": 0.0, "PulseAmount": 0.0}, (0.25, 0.85, 1.0)),      # dark until gameplay lights it
    "Cell": ({"GlowStrength": 14.0, "PulseAmount": 0.15, "PulseRate": 2.6}, (1.0, 0.42, 0.08)),
    "CoreGlow": ({"GlowStrength": 30.0, "PulseAmount": 0.1, "PulseRate": 1.6}, (1.0, 0.86, 0.7)),
}
NO_COLLISION = ("SM_DI_Core_Ring_A", "SM_DI_Core_Ring_B")

eas = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary


def log(msg):
    unreal.log("TDPY: " + msg)


def _set(obj, prop, value, notes):
    try:
        obj.set_editor_property(prop, value)
    except Exception as error:
        notes.append("could not set %s.%s (%s)" % (type(obj).__name__, prop, error))


def ensure_dir(path):
    if not eas.does_directory_exist(path):
        eas.make_directory(path)


def import_textures(notes):
    out = {}
    tasks = []
    for p in sorted(glob.glob(os.path.join(KIT, "textures", "T_DI_Atlas_*.png"))):
        t = unreal.AssetImportTask()
        t.set_editor_property("filename", p)
        t.set_editor_property("destination_path", TEX_DEST)
        t.set_editor_property("automated", True)
        t.set_editor_property("replace_existing", True)
        t.set_editor_property("save", False)
        tasks.append(t)
    tools.import_asset_tasks(tasks)
    for suffix in ("BC", "N", "ORM"):
        tex = eas.load_asset("%s/T_DI_Atlas_%s" % (TEX_DEST, suffix))
        if tex is None:
            notes.append("texture missing: " + suffix)
            continue
        if suffix == "N":
            _set(tex, "compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP, notes)
            _set(tex, "srgb", False, notes)
            _set(tex, "flip_green_channel", False, notes)
        elif suffix == "ORM":
            _set(tex, "compression_settings", unreal.TextureCompressionSettings.TC_MASKS, notes)
            _set(tex, "srgb", False, notes)
        else:
            _set(tex, "compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT, notes)
            _set(tex, "srgb", True, notes)
        eas.save_loaded_asset(tex)
        out[suffix] = tex
    return out


def instance(name, parent):
    path = "%s/%s" % (DEST, name)
    mi = eas.load_asset(path) if eas.does_asset_exist(path) else tools.create_asset(
        name, DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, parent)
    return mi


def ensure_shield():
    ensure_dir(DEST + "/Materials")
    if eas.does_asset_exist(SHIELD):
        return eas.load_asset(SHIELD)
    folder, name = SHIELD.rsplit("/", 1)
    mat = tools.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("two_sided", True)

    def node(cls, x, y):
        return mel.create_material_expression(mat, cls, x, y)
    col = node(unreal.MaterialExpressionVectorParameter, -900, -200)
    col.set_editor_property("parameter_name", "ShieldColor")
    col.set_editor_property("default_value", unreal.LinearColor(1.0, 0.2, 0.05, 1.0))
    strength = node(unreal.MaterialExpressionScalarParameter, -900, 0)
    strength.set_editor_property("parameter_name", "ShieldStrength")
    strength.set_editor_property("default_value", 6.0)
    opacity = node(unreal.MaterialExpressionScalarParameter, -900, 300)
    opacity.set_editor_property("parameter_name", "ShieldOpacity")
    opacity.set_editor_property("default_value", 0.35)
    fres = node(unreal.MaterialExpressionFresnel, -900, 150)
    e_mul = node(unreal.MaterialExpressionMultiply, -600, -100)
    e_mul2 = node(unreal.MaterialExpressionMultiply, -400, -100)
    base = node(unreal.MaterialExpressionConstant, -900, 420)
    base.set_editor_property("r", 0.25)
    f_add = node(unreal.MaterialExpressionAdd, -650, 200)
    o_mul = node(unreal.MaterialExpressionMultiply, -400, 250)
    ok = True
    ok &= mel.connect_material_expressions(col, "", e_mul, "A")
    ok &= mel.connect_material_expressions(strength, "", e_mul, "B")
    ok &= mel.connect_material_expressions(fres, "", f_add, "A")
    ok &= mel.connect_material_expressions(base, "", f_add, "B")
    ok &= mel.connect_material_expressions(e_mul, "", e_mul2, "A")
    ok &= mel.connect_material_expressions(f_add, "", e_mul2, "B")
    ok &= mel.connect_material_expressions(f_add, "", o_mul, "A")
    ok &= mel.connect_material_expressions(opacity, "", o_mul, "B")
    ok &= mel.connect_material_property(e_mul2, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    ok &= mel.connect_material_property(o_mul, "", unreal.MaterialProperty.MP_OPACITY)
    if not ok:
        log("FAILED: could not wire " + SHIELD)
        return None
    try:
        mel.recompile_material(mat)
    except Exception:
        log("note: no renderer in this headless run; M_DI_Shield compiles on first load in an editor")
    eas.save_asset(SHIELD, only_if_is_dirty=False)
    return mat


def ensure_pipeline(notes):
    ensure_dir(DEST + "/Pipelines")
    if not eas.does_asset_exist(PIPELINE):
        eas.duplicate_asset("/Interchange/Pipelines/DefaultAssetsPipeline", PIPELINE)
    pipe = eas.load_asset(PIPELINE)
    _set(pipe, "asset_type_sub_folders", False, notes)
    mesh = pipe.get_editor_property("mesh_pipeline")
    for k, v in {"import_static_meshes": True, "import_skeletal_meshes": False,
                 "collision": True, "import_collision_according_to_mesh_name": True, "one_convex_hull_per_ucx": True,
                 "generate_lightmap_u_vs": False, "build_nanite": False}.items():
        _set(mesh, k, v, notes)
    try:
        sp = pipe.get_editor_property("material_pipeline")
        _set(sp, "import_materials", False, notes)
        try:
            _set(sp.get_editor_property("texture_pipeline"), "import_textures", False, notes)
        except Exception:
            pass
    except Exception as error:
        notes.append("pipeline material_pipeline: %s" % error)
    eas.save_loaded_asset(pipe)
    return unreal.SoftObjectPath("%s.%s" % (PIPELINE, PIPELINE.rsplit("/", 1)[-1]))


def main():
    notes = []
    with open(SIZE_CHECK, encoding="utf-8") as fh:
        expect = {r["asset"]: r for r in json.load(fh)}
    ensure_dir(DEST)
    ensure_dir(TEX_DEST)
    master_tex, master_glow = eas.load_asset(MASTER_TEX), eas.load_asset(MASTER_GLOW)
    if master_tex is None or master_glow is None:
        log("FAILED: missing master %s or %s" % (MASTER_TEX, MASTER_GLOW))
        return
    tex = import_textures(notes)
    if len(tex) != 3:
        log("FAILED: atlas textures %s %s" % (sorted(tex), notes))
        return
    mis = {}
    atlas = instance("MI_DI_Atlas", master_tex)
    mel.set_material_instance_texture_parameter_value(atlas, "BaseColorTex", tex["BC"])
    mel.set_material_instance_texture_parameter_value(atlas, "ORMTex", tex["ORM"])
    mel.set_material_instance_texture_parameter_value(atlas, "NormalTex", tex["N"])
    mel.set_material_instance_scalar_parameter_value(atlas, "EmissiveStrength", 0.0)
    eas.save_loaded_asset(atlas)
    mis["Atlas"] = atlas
    for key, (scalars, rgb) in GLOWS.items():
        mi = instance("MI_DI_" + key, master_glow)
        for k, v in scalars.items():
            mel.set_material_instance_scalar_parameter_value(mi, k, v)
        mel.set_material_instance_vector_parameter_value(mi, "GlowColor", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
        eas.save_loaded_asset(mi)
        mis[key] = mi
    shield = ensure_shield()
    if shield is None:
        return
    smi = instance("MI_DI_Shield", shield)
    eas.save_loaded_asset(smi)
    mis["Shield"] = smi
    pipe = ensure_pipeline(notes)
    sme = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    mgr = unreal.InterchangeManager.get_interchange_manager_scripted()
    rows, bad = [], 0
    for man_path in sorted(glob.glob(os.path.join(KIT, "export", "SM_DI_*.manifest.json"))):
        with open(man_path, encoding="utf-8") as fh:
            man = json.load(fh)
        asset = man["asset"]
        fbx = os.path.join(KIT, "export", asset + ".fbx")
        n = []
        params = unreal.ImportAssetParameters()
        params.is_automated = True
        params.replace_existing = True
        params.override_pipelines.append(pipe)
        mgr.import_asset(DEST, unreal.InterchangeManager.create_source_data(fbx), params)
        mesh = eas.load_asset("%s/%s" % (DEST, asset))
        if mesh is None:
            log("FAILED %s: not found after import" % asset)
            bad += 1
            continue
        try:
            sme.set_generate_lightmap_uv(mesh, False)
        except Exception as error:
            n.append("lightmap: %s" % error)
        try:
            ns = mesh.get_editor_property("nanite_settings")
            ns.enabled = False
            mesh.set_editor_property("nanite_settings", ns)
        except Exception as error:
            n.append("nanite: %s" % error)
        slots = []
        for i, sm in enumerate(mesh.static_materials):
            slot = str(sm.material_slot_name)
            key = next((k for k in mis if slot.endswith("_" + k) or slot == k), None)
            if key is None:
                n.append("slot %s unmapped" % slot)
                continue
            mesh.set_material(i, mis[key])
            slots.append("%d:%s" % (i, key))
        if asset in NO_COLLISION:
            try:
                sme.remove_collisions(mesh)
            except Exception as error:
                n.append("remove_collisions: %s" % error)
        eas.save_loaded_asset(mesh)
        bb = mesh.get_bounding_box()
        mn = [round(bb.min.x, 2), round(bb.min.y, 2), round(bb.min.z, 2)]
        mx = [round(bb.max.x, 2), round(bb.max.y, 2), round(bb.max.z, 2)]
        try:
            tris = int(mesh.get_num_triangles(0))
        except Exception:
            tris = -1
        # hull extents are not readable from Python in 5.9: count them (they are authored inside the stated box)
        try:
            hulls = int(sme.get_convex_collision_count(mesh))
        except Exception as error:
            hulls = -1
            n.append("hull count: %s" % error)
        side = None
        if asset in ("SM_DI_Walkway_Ring", "SM_DI_Corridor_Corner"):
            # handedness check on an asymmetric piece: the rail (walkway) / the junction column (corner) must be on +Y
            try:
                desc = mesh.get_static_mesh_description(0)
                ys = []
                for i in range(desc.get_vertex_count()):
                    v = desc.get_vertex_position(unreal.VertexID(i))
                    if (asset == "SM_DI_Walkway_Ring" and v.z > 50.0) or (asset == "SM_DI_Corridor_Corner" and abs(v.x + 120.0) < 0.1 and 50.0 < v.z < 150.0):
                        ys.append(v.y)
                side = "+Y" if ys and min(ys) > 0 else ("-Y" if ys and max(ys) < 0 else "mixed %d" % len(ys))
            except Exception as error:
                n.append("vertex read: %s" % error)
        e = expect.get(asset)
        # Unreal's Y is Blender's -Y through FBX: compare against the mirrored box too and say which one matched
        same = e and all(abs(mn[i] - e["min_cm"][i]) < 0.6 and abs(mx[i] - e["max_cm"][i]) < 0.6 for i in range(3))
        mirrored = e and all(abs(mn[i] - (e["min_cm"][i] if i != 1 else -e["max_cm"][1])) < 0.6
                             and abs(mx[i] - (e["max_cm"][i] if i != 1 else -e["min_cm"][1])) < 0.6 for i in range(3))
        want_col = asset not in NO_COLLISION
        ok = bool(same or mirrored) and tris == man["tris"] and len(slots) == len(mesh.static_materials) \
            and hulls == (len(man.get('collision', [])) if want_col else 0) and side in (None, '+Y')
        if not ok:
            bad += 1
        row = {"asset": asset, "ok": ok, "min_cm": mn, "max_cm": mx, "size_cm": [round(mx[i] - mn[i], 2) for i in range(3)],
               "bounds_match": "same" if same else ("y-mirrored" if mirrored else "NO"), "tris": tris, "slots": slots,
               "hulls": hulls, "asymmetric_side": side, "notes": n}
        rows.append(row)
        log(json.dumps(row))
    out = os.path.join(KIT, "review", "ue_import_report.json")
    with open(out, "w", encoding="utf-8") as fh:
        json.dump(rows, fh, indent=1)
    log("report: %s; notes %s" % (out, notes))
    log("imported %d mesh(es), %d failed" % (len(rows), bad))
    if rows and not bad:
        log("OK")
    else:
        log("FAILED: see the lines above")


main()
