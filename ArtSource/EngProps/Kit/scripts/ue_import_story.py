# SPDX-License-Identifier: MIT
# Story props for the planets -> /Game/TriadDrift/Gameplay/Props/Kit/Story: imports export_story/*.fbx with the kit's
# pipeline (UCX hull on pieces over 1 m, none on stakes and decals), assigns instances by slot, imports the two decal
# texture sets.   powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\Kit\scripts\ue_import_story.py
import glob
import json
import os
import sys

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import ue_import_kit as K  # noqa: E402

KIT = os.path.dirname(HERE)
DEST = "/Game/TriadDrift/Gameplay/Props/Kit/Story"
K.DEST = DEST
K.TEX_DEST = DEST + "/Textures"
K.PIPELINE = DEST + "/Pipelines/PL_Story_StaticMesh"
eas, mel, tools = unreal.EditorAssetLibrary, unreal.MaterialEditingLibrary, unreal.AssetToolsHelpers.get_asset_tools()
KT = "/Game/TriadDrift/Gameplay/Props/Kit/"
NO_COL = ("SM_Kit_SurveyStakeFlag", "SM_Kit_DragMark", "SM_Kit_ScorchLines", "SM_Kit_LightPool", "SM_Kit_InsigniaHull", "SM_Kit_InsigniaPanel")


def log(m):
    unreal.log("TDPY: " + m)


def glow(name, rgb, strength, pulse=0.0, rate=1.0):
    mi = K.instance(name, eas.load_asset(K.MASTER_GLOW))
    mel.set_material_instance_scalar_parameter_value(mi, "GlowStrength", strength)
    mel.set_material_instance_scalar_parameter_value(mi, "PulseAmount", pulse)
    mel.set_material_instance_scalar_parameter_value(mi, "PulseRate", rate)
    mel.set_material_instance_vector_parameter_value(mi, "GlowColor", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    eas.save_loaded_asset(mi)
    return mi


def tinted(name, tint, grime, self_illum=0.0):
    mi = K.instance(name, eas.load_asset(KT + "Materials/M_KitD_Trim"))
    for p_, t in (("BaseColorTex", "T_Kit_Trim_BC"), ("ORMTex", "T_Kit_Trim_ORM"), ("NormalTex", "T_Kit_Trim_N")):
        mel.set_material_instance_texture_parameter_value(mi, p_, eas.load_asset(KT + "Textures/" + t))
    mel.set_material_instance_scalar_parameter_value(mi, "GrimeAmount", grime)
    mel.set_material_instance_vector_parameter_value(mi, "BaseTint", unreal.LinearColor(tint[0], tint[1], tint[2], 1.0))
    mel.set_material_instance_scalar_parameter_value(mi, "SelfIllum", self_illum)
    eas.save_loaded_asset(mi)
    return mi


def decal_mi(name, stem, notes):
    K.ensure_dir(K.TEX_DEST)
    tasks = []
    for k in ("BC", "N", "ORM", "E"):
        t = unreal.AssetImportTask()
        t.set_editor_property("filename", os.path.join(KIT, "textures", "%s_%s.png" % (stem, k)))
        t.set_editor_property("destination_path", K.TEX_DEST)
        t.set_editor_property("automated", True)
        t.set_editor_property("replace_existing", True)
        t.set_editor_property("save", False)
        tasks.append(t)
    tools.import_asset_tasks(tasks)
    mi = K.instance(name, eas.load_asset(K.DECAL_MASTER))
    for k, pname in (("BC", "DecalBC"), ("N", "DecalN"), ("ORM", "DecalORM"), ("E", "DecalE")):
        t = eas.load_asset("%s/%s_%s" % (K.TEX_DEST, stem, k))
        if k == "N":
            K._set(t, "compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP, notes)
            K._set(t, "srgb", False, notes)
        elif k == "ORM":
            K._set(t, "compression_settings", unreal.TextureCompressionSettings.TC_MASKS, notes)
            K._set(t, "srgb", False, notes)
        eas.save_loaded_asset(t)
        mel.set_material_instance_texture_parameter_value(mi, pname, t)
    mel.set_material_instance_scalar_parameter_value(mi, "DecalEmissive", 0.0)
    eas.save_loaded_asset(mi)
    return mi


def main():
    notes = []
    K.ensure_dir(DEST)
    mis = {"Trim": eas.load_asset(KT + "MI_Kit_Trim"), "Glow": eas.load_asset(KT + "MI_Kit_Glow_Cinder"),
           "Beacon": eas.load_asset(KT + "MI_Kit_Beacon_Cinder"),
           "Canopy": tinted("MI_Kit_Canopy", (0.62, 0.56, 0.42), 0.7), "Flag": tinted("MI_Kit_Flag", (1.0, 0.42, 0.08), 0.5),
           "Decal": decal_mi("MI_Kit_DragMark", "T_Kit_DragMark", notes), "Decal2": decal_mi("MI_Kit_ScorchLines", "T_Kit_ScorchLines", notes),
           # r2: Cinder's camp in lighter worn steel, a little self-lit (Cinder's lighting is dark); the landmarks' lights
           "Worn": tinted("MI_Kit_StoryWorn", (1.0, 0.95, 0.86), 0.35, 0.12), "Light": eas.load_asset(KT + "MI_Kit_Light"),
           "Dom": eas.load_asset(KT + "Dominion/MI_KitD_Trim"),
           "Nav": glow("MI_Kit_RelayNav", (0.55, 0.9, 0.15), 40.0, pulse=1.0, rate=0.5),
           "Band": glow("MI_Kit_RelayBand", (0.55, 0.9, 0.15), 12.0),
           "Pool": decal_mi("MI_Kit_LightPool", "T_Kit_LightPool", notes),
           "Insignia": decal_mi("MI_Kit_InsigniaScorched", "T_Kit_InsigniaScorched", notes)}
    mel.set_material_instance_scalar_parameter_value(mis["Insignia"], "DecalEmissive", 0.6)
    eas.save_loaded_asset(mis["Insignia"])
    mel.set_material_instance_scalar_parameter_value(mis["Pool"], "DecalEmissive", 1.5)
    eas.save_loaded_asset(mis["Pool"])
    derrick = {"Beacon": glow("MI_Kit_DerrickBeacon", (1.0, 0.05, 0.02), 70.0, pulse=0.9, rate=0.3),
               "Glow": eas.load_asset(KT + "Dominion/MI_KitD_Glow")}
    pipe = K.ensure_pipeline(notes)
    sme = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    mgr = unreal.InterchangeManager.get_interchange_manager_scripted()
    rows = []
    for man_path in sorted(glob.glob(os.path.join(KIT, "export_story", "SM_Kit_*.manifest.json"))):
        with open(man_path, encoding="utf-8") as fh:
            asset = json.load(fh)["asset"]
        params = unreal.ImportAssetParameters()
        params.is_automated = True
        params.replace_existing = True
        params.override_pipelines.append(pipe)
        mgr.import_asset(DEST, unreal.InterchangeManager.create_source_data(os.path.join(KIT, "export_story", asset + ".fbx")), params)
        mesh = eas.load_asset("%s/%s" % (DEST, asset))
        if mesh is None:
            log("FAILED %s" % asset)
            continue
        slots = []
        for i, sm in enumerate(mesh.static_materials):
            slot = str(sm.material_slot_name)
            import re
            slot = re.sub(r"_\d+$", "", slot)          # Blender's duplicate-name suffix (M_Kit_Decal.001 -> M_Kit_Decal_001)
            key = next((k for k in sorted(mis, key=len, reverse=True) if slot.endswith("_" + k) or slot == k), None)
            if key:
                mesh.set_material(i, (derrick if asset == "SM_Kit_NullDerrick" and key in derrick else mis)[key])
            slots.append("%d:%s" % (i, key))
        if asset in NO_COL:
            try:
                sme.remove_collisions(mesh)
            except Exception as e:
                notes.append("%s %s" % (asset, e))
        try:
            ns = mesh.get_editor_property("nanite_settings")
            ns.enabled = False
            mesh.set_editor_property("nanite_settings", ns)
        except Exception:
            pass
        eas.save_loaded_asset(mesh)
        bb = mesh.get_bounding_box()
        row = {"asset": asset, "size_cm": [round(bb.max.x - bb.min.x), round(bb.max.y - bb.min.y), round(bb.max.z - bb.min.z)],
               "tris": mesh.get_num_triangles(0), "hulls": sme.get_convex_collision_count(mesh), "slots": slots}
        rows.append(row)
        log(json.dumps(row))
    with open(os.path.join(KIT, "review", "ue_import_story.json"), "w", encoding="utf-8") as fh:
        json.dump(rows, fh, indent=1)
    log("story import done, %d meshes, notes %s" % (len(rows), notes))


main()
