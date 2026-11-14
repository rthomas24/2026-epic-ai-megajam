# SPDX-License-Identifier: MIT
# Triad Drift - import Fab intake assets with one material instance per slot, headless:
#   $env:TD_FAB_ONLY = "PipeBeam"; powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\Fab\scripts\ue_import_fab_multi.py
# Same conventions as ue_import_fab.py: Interchange FBX with PL_Ship_StaticMesh (UCX_ by name, no lightmap UVs, Nanite),
# textures T_Fab_<Asset>_<Part>_BC|N|ORM|E (N DirectX, ORM masks), MI_Fab_<Asset>_<Part> on M_TD_PropTextured.
# TEXTURE sets (no mesh): T_Fab_<Set>_BC|N|ORM and one MI_Fab_<Set>, not assigned anywhere.
import json
import os

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
FAB = os.path.dirname(HERE)
PIPELINE = "/Game/TriadDrift/Gameplay/Ships/Pipelines/PL_Ship_StaticMesh"
MASTER = "/Game/TriadDrift/Gameplay/Materials/M_TD_PropTextured"
MESHES = {
    # asset: (export dir, destination, {slot material name: (texture part, EmissiveStrength)})
    "PipeBeam": (os.path.join(FAB, "PipeBeam", "export"), "/Game/TriadDrift/Gameplay/Props/Fab/PipeBeam",
                 {"M_Fab_PipeBeam_Truss": ("Truss", 6.0), "M_Fab_PipeBeam_Cable": ("Cable", 0.0)}),
}
TEXSETS = {
    # set: (export dir, destination, max size)
    "ForestNordicMoss": (os.path.join(FAB, "Textures", "ForestNordicMoss", "export"), "/Game/TriadDrift/Gameplay/Props/Fab/Textures/ForestNordicMoss", 4096),
    "ConcreteRubble": (os.path.join(FAB, "Textures", "ConcreteRubble", "export"), "/Game/TriadDrift/Gameplay/Props/Fab/Textures/ConcreteRubble", 4096),
    "StoneTileFloor": (os.path.join(FAB, "Textures", "StoneTileFloor", "export"), "/Game/TriadDrift/Gameplay/Props/Fab/Textures/StoneTileFloor", 4096),
}
eas = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
FAILS = []


def log(m):
    unreal.log("TDPY: " + m)


def save(a):
    if not eas.save_loaded_asset(a, False):
        FAILS.append(a.get_path_name())


def textures(src, dest, prefix, max_size):
    if not eas.does_directory_exist(dest):
        eas.make_directory(dest)
    tasks = []
    for suffix in ("BC", "N", "ORM", "E"):
        p = os.path.join(src, "%s_%s.png" % (prefix, suffix))
        if os.path.isfile(p):
            t = unreal.AssetImportTask()
            for k, v in (("filename", p), ("destination_path", dest), ("automated", True), ("replace_existing", True), ("save", False)):
                t.set_editor_property(k, v)
            tasks.append((suffix, t))
    tools.import_asset_tasks([t for _, t in tasks])
    out = {}
    tcs = unreal.TextureCompressionSettings
    for suffix, _ in tasks:
        t = eas.load_asset("%s/%s_%s" % (dest, prefix, suffix))
        if t is None:
            FAILS.append("%s_%s" % (prefix, suffix))
            continue
        if suffix == "N":
            t.set_editor_property("compression_settings", tcs.TC_NORMALMAP)
            t.set_editor_property("srgb", False)
            t.set_editor_property("flip_green_channel", False)
        elif suffix == "ORM":
            t.set_editor_property("compression_settings", tcs.TC_MASKS)
            t.set_editor_property("srgb", False)
        else:
            t.set_editor_property("compression_settings", tcs.TC_DEFAULT)
            t.set_editor_property("srgb", True)
        t.set_editor_property("max_texture_size", max_size)
        save(t)
        out[suffix] = t
    return out


def make_mi(dest, name, tex, emissive):
    path = "%s/%s" % (dest, name)
    mi = eas.load_asset(path) if eas.does_asset_exist(path) else tools.create_asset(
        name, dest, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, eas.load_asset(MASTER))
    for p, s in (("BaseColorTex", "BC"), ("ORMTex", "ORM"), ("NormalTex", "N"), ("EmissiveTex", "E")):
        if s in tex:
            mel.set_material_instance_texture_parameter_value(mi, p, tex[s])
    mel.set_material_instance_scalar_parameter_value(mi, "EmissiveStrength", emissive if "E" in tex else 0.0)
    save(mi)
    return mi


def mesh_asset(name, cfg):
    src, dest, slots = cfg
    mis = {}
    for slot, (part, em) in slots.items():
        tex = textures(src, dest, "T_Fab_%s_%s" % (name, part), 2048)
        mis[slot] = make_mi(dest, "MI_Fab_%s_%s" % (name, part), tex, em)
    params = unreal.ImportAssetParameters()
    params.is_automated = True
    params.replace_existing = True
    params.override_pipelines.append(unreal.SoftObjectPath("%s.%s" % (PIPELINE, PIPELINE.rsplit("/", 1)[-1])))
    mgr = unreal.InterchangeManager.get_interchange_manager_scripted()
    mgr.import_asset(dest, unreal.InterchangeManager.create_source_data(os.path.join(src, "SM_Fab_%s.fbx" % name)), params)
    mesh = eas.load_asset("%s/SM_Fab_%s" % (dest, name))
    if mesh is None:
        FAILS.append("mesh " + name)
        return None
    sme = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    sme.set_generate_lightmap_uv(mesh, False)
    ns = mesh.get_editor_property("nanite_settings")
    ns.enabled = True
    mesh.set_editor_property("nanite_settings", ns)
    slot_names = []
    for i, sm in enumerate(mesh.static_materials):
        sn = str(sm.get_editor_property("material_slot_name"))
        slot_names.append(sn)
        mi = mis.get(sn) or next((v for k, v in mis.items() if k.endswith(sn.rsplit("_", 1)[-1])), None)
        if mi:
            mesh.set_material(i, mi)
        else:
            FAILS.append("no MI for slot " + sn)
    save(mesh)
    b = mesh.get_bounding_box()
    return {"mesh": mesh.get_path_name(), "slots": slot_names, "hulls": sme.get_convex_collision_count(mesh),
            "bounds_cm": [[round(b.min.x), round(b.min.y), round(b.min.z)], [round(b.max.x), round(b.max.y), round(b.max.z)]],
            "mis": [m.get_path_name() for m in mis.values()]}


def tex_set(name, cfg):
    src, dest, size = cfg
    tex = textures(src, dest, "T_Fab_" + name, size)
    mi = make_mi(dest, "MI_Fab_" + name, tex, 0.0)
    return {"set": name, "textures": sorted(tex), "mi": mi.get_path_name()}


only = [s for s in os.environ.get("TD_FAB_ONLY", "").split(",") if s]
rows = []
for n, c in MESHES.items():
    if not only or n in only:
        rows.append(mesh_asset(n, c))
for n, c in TEXSETS.items():
    if only and n in only:
        rows.append(tex_set(n, c))
for r in rows:
    log(json.dumps(r))
log("OK" if rows and all(rows) and not FAILS else "FAILED %s" % FAILS)
