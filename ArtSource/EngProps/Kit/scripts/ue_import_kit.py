# SPDX-License-Identifier: MIT
# Triad Drift - import the planet-outpost Kit (ArtSource/EngProps/Kit/export/SM_Kit_*.fbx) into Unreal. Forked from
# DominionInterior/scripts/ue_import_di_kit.py. Headless, from the repo root (or through ue_kit_all.py):
#   powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\Kit\scripts\ue_import_kit.py
#
# Writes ONLY under /Game/TriadDrift/Gameplay/Props/Kit:
#   Textures/T_Kit_Trim_BC|N|ORM   the shared 2K trim sheet
#   MI_Kit_Trim                     instance of /Game/TriadDrift/Gameplay/Materials/M_TD_PropTextured (read only)
#   MI_Kit_Glow_Verdance (teal, the mesh default), MI_Kit_Glow_Cinder (ember), MI_Kit_Screen
#                                   instances of /Game/TriadDrift/Gameplay/Props/Dominion/M_TD_DominionGlow (read only)
#   Pipelines/PL_Kit_StaticMesh, SM_Kit_*
import glob
import json
import os

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
REPO = os.path.dirname(os.path.dirname(os.path.dirname(KIT)))
DEST = "/Game/TriadDrift/Gameplay/Props/Kit"
TEX_DEST = DEST + "/Textures"
PIPELINE = DEST + "/Pipelines/PL_Kit_StaticMesh"
MASTER_TEX = "/Game/TriadDrift/Gameplay/Materials/M_TD_PropTextured"
MASTER_GLOW = "/Game/TriadDrift/Gameplay/Props/Dominion/M_TD_DominionGlow"
SIZE_CHECK = os.path.join(KIT, "review", "size_check_r3.json")

GLOWS = {   # MI name: (slot key it fills by default, scalars, colour)
    "MI_Kit_Glow_Verdance": ("Glow", {"GlowStrength": 8.0, "PulseAmount": 0.0}, (0.05, 0.85, 0.75)),
    "MI_Kit_Glow_Cinder": (None, {"GlowStrength": 8.0, "PulseAmount": 0.0}, (1.0, 0.36, 0.06)),
    "MI_Kit_Screen": ("Screen", {"GlowStrength": 2.5, "PulseAmount": 0.0}, (0.1, 0.6, 0.7)),
}
NO_COLLISION = ("SM_Kit_LightStrip_200",)

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
    for p in sorted(glob.glob(os.path.join(KIT, "textures", "T_Kit_Trim_*.png"))):
        t = unreal.AssetImportTask()
        t.set_editor_property("filename", p)
        t.set_editor_property("destination_path", TEX_DEST)
        t.set_editor_property("automated", True)
        t.set_editor_property("replace_existing", True)
        t.set_editor_property("save", False)
        tasks.append(t)
    tools.import_asset_tasks(tasks)
    for suffix in ("BC", "N", "ORM"):
        tex = eas.load_asset("%s/T_Kit_Trim_%s" % (TEX_DEST, suffix))
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
    atlas = instance("MI_Kit_Trim", master_tex)
    mel.set_material_instance_texture_parameter_value(atlas, "BaseColorTex", tex["BC"])
    mel.set_material_instance_texture_parameter_value(atlas, "ORMTex", tex["ORM"])
    mel.set_material_instance_texture_parameter_value(atlas, "NormalTex", tex["N"])
    mel.set_material_instance_scalar_parameter_value(atlas, "EmissiveStrength", 0.0)
    eas.save_loaded_asset(atlas)
    mis["Trim"] = atlas
    for name, (key, scalars, rgb) in GLOWS.items():
        mi = instance(name, master_glow)
        for k, v in scalars.items():
            mel.set_material_instance_scalar_parameter_value(mi, k, v)
        mel.set_material_instance_vector_parameter_value(mi, "GlowColor", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
        eas.save_loaded_asset(mi)
        if key:
            mis[key] = mi
    pipe = ensure_pipeline(notes)
    sme = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    mgr = unreal.InterchangeManager.get_interchange_manager_scripted()
    rows, bad = [], 0
    for man_path in sorted(glob.glob(os.path.join(KIT, "export", "SM_Kit_*.manifest.json"))):
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
        if asset == "SM_Kit_Railing_400":
            # handedness check on an asymmetric piece: the rail (walkway) / the junction column (corner) must be on +Y
            try:
                desc = mesh.get_static_mesh_description(0)
                ys = []
                for i in range(desc.get_vertex_count()):
                    v = desc.get_vertex_position(unreal.VertexID(i))
                    if v.z > 50.0:
                        ys.append(v.y)
                side = "+Y" if ys and min(ys) > -0.01 else ("-Y" if ys and max(ys) < 0 else "mixed %d" % len(ys))
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


if __name__ == "__main__":
    main()
