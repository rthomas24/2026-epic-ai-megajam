# SPDX-License-Identifier: MIT
# #194 camp buildings -> /Game/TriadDrift/Gameplay/Props/Huts/<Planet>/SM_Hut_<Planet>_<Name> (under Props: cooked).
# One palette per planet on the kit trim (M_KitD_Trim with the planet trim sheet T_Kit_Trim): Verdance teal / white with
# warm windows, Cinder black / amber hazard. Slots by name: Trim, Light (warm windows), Glow, Beacon. Box / hull
# collision from the export (UCX), no Nanite.
#   powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\Kit\scripts\ue_import_huts.py   [TD_HUTS_ONLY=Verdance|Cinder]
import glob
import json
import os
import re
import sys

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import ue_import_kit as K  # noqa: E402

KIT = os.path.dirname(HERE)
ROOT = "/Game/TriadDrift/Gameplay/Props/Huts"
KT = "/Game/TriadDrift/Gameplay/Props/Kit/"
eas, mel = unreal.EditorAssetLibrary, unreal.MaterialEditingLibrary
PALETTE = {   # planet: (BaseTint, BandTint, GrimeAmount, SelfIllum, GrimeRough)
    # r3 (the Lead on the in-game look: flat black silhouettes at dusk): Verdance a clearly lighter off-white with a
    # teal band that survives dusk, Cinder a mid warm grey with amber hazard; both a little self-lit so the panels read
    "Verdance": ((1.35, 1.45, 1.42), (0.2, 0.95, 0.85), 0.2, 0.22, 0.35),   # r4: 0.04 still read black at dusk
    "Cinder": ((0.95, 0.86, 0.76), (1.0, 0.55, 0.08), 0.3, 0.3, 0.35),     # r5: the rig and hopper still read black against the lava sky
}


def log(m):
    unreal.log("TDPY: " + m)


def trim(planet):
    base, band, grime, illum, grough = PALETTE[planet]
    mi = K.instance("MI_Hut_%s_Trim" % planet, eas.load_asset(KT + "Materials/M_KitD_Trim"))
    for p_, t in (("BaseColorTex", "T_Kit_Trim_BC"), ("ORMTex", "T_Kit_Trim_ORM"), ("NormalTex", "T_Kit_Trim_N")):
        mel.set_material_instance_texture_parameter_value(mi, p_, eas.load_asset(KT + "Textures/" + t))
    mel.set_material_instance_vector_parameter_value(mi, "BaseTint", unreal.LinearColor(base[0], base[1], base[2], 1.0))
    mel.set_material_instance_vector_parameter_value(mi, "BandTint", unreal.LinearColor(band[0], band[1], band[2], 1.0))
    mel.set_material_instance_scalar_parameter_value(mi, "GrimeAmount", grime)
    mel.set_material_instance_scalar_parameter_value(mi, "SelfIllum", illum)
    mel.set_material_instance_scalar_parameter_value(mi, "GrimeRough", grough)
    eas.save_loaded_asset(mi)
    return mi


def window_light(planet):
    mi = K.instance("MI_Hut_%s_Window" % planet, eas.load_asset(K.MASTER_GLOW))
    # r2 (the Lead: lit rooms at night, not flat white): warm amber-white, moderate
    mel.set_material_instance_scalar_parameter_value(mi, "GlowStrength", 3.0)
    mel.set_material_instance_scalar_parameter_value(mi, "PulseAmount", 0.0)
    mel.set_material_instance_vector_parameter_value(mi, "GlowColor", unreal.LinearColor(1.0, 0.62, 0.3, 1.0))
    eas.save_loaded_asset(mi)
    return mi


def stone(planet):
    """The planet's stone: the ancient-stone trim, cool grey-green on Verdance, near-black basalt on Cinder."""
    mi = K.instance("MI_Site_%s_Stone" % planet, eas.load_asset(KT + "Materials/M_KitD_Trim"))
    for p_, t in (("BaseColorTex", "T_KitS_Trim_BC"), ("ORMTex", "T_KitS_Trim_ORM"), ("NormalTex", "T_KitS_Trim_N")):
        mel.set_material_instance_texture_parameter_value(mi, p_, eas.load_asset(KT + "Textures/" + t))
    tint, illum = ((0.75, 0.9, 0.8), 0.12) if planet == "Verdance" else ((0.42, 0.38, 0.36), 0.12)
    mel.set_material_instance_vector_parameter_value(mi, "BaseTint", unreal.LinearColor(tint[0], tint[1], tint[2], 1.0))
    mel.set_material_instance_scalar_parameter_value(mi, "GrimeAmount", 0.5)
    mel.set_material_instance_scalar_parameter_value(mi, "SelfIllum", illum)
    eas.save_loaded_asset(mi)
    return mi


def bark():
    mi = K.instance("MI_Site_Verdance_Bark", eas.load_asset(KT + "Materials/M_KitD_Trim"))
    for p_, t in (("BaseColorTex", "T_KitS_Trim_BC"), ("ORMTex", "T_KitS_Trim_ORM"), ("NormalTex", "T_KitS_Trim_N")):
        mel.set_material_instance_texture_parameter_value(mi, p_, eas.load_asset(KT + "Textures/" + t))
    mel.set_material_instance_vector_parameter_value(mi, "BaseTint", unreal.LinearColor(0.55, 0.36, 0.24, 1.0))
    mel.set_material_instance_scalar_parameter_value(mi, "GrimeAmount", 0.6)
    mel.set_material_instance_scalar_parameter_value(mi, "SelfIllum", 0.12)
    eas.save_loaded_asset(mi)
    return mi


def ember():
    mi = K.instance("MI_Site_Cinder_Ember", eas.load_asset(K.MASTER_GLOW))
    mel.set_material_instance_scalar_parameter_value(mi, "GlowStrength", 4.0)
    mel.set_material_instance_scalar_parameter_value(mi, "PulseAmount", 0.3)
    mel.set_material_instance_scalar_parameter_value(mi, "PulseRate", 0.4)
    mel.set_material_instance_vector_parameter_value(mi, "GlowColor", unreal.LinearColor(1.0, 0.28, 0.04, 1.0))
    eas.save_loaded_asset(mi)
    return mi


def main():
    only = os.environ.get("TD_HUTS_ONLY", "")
    notes, rows = [], []
    sme = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    mgr = unreal.InterchangeManager.get_interchange_manager_scripted()
    for planet in ("Verdance", "Cinder"):
        if only and planet != only:
            continue
        dest = "%s/%s" % (ROOT, planet)
        K.DEST = dest
        K.ensure_dir(dest)
        K.PIPELINE = dest + "/Pipelines/PL_Hut_%s" % planet
        pipe = K.ensure_pipeline(notes)
        mis = {"Trim": trim(planet), "Light": window_light(planet), "Glow": eas.load_asset(KT + "MI_Kit_Glow_%s" % planet),
               "Beacon": eas.load_asset(KT + "MI_Kit_Beacon_%s" % planet), "Stone": stone(planet), "Bark": bark(),
               "Ember": ember()}
        # #194 r2: the huts, the camp perimeter (SM_Hut_*) and the ground-break pieces (SM_Site_*) all land here
        mans = []
        for exp, prefix in (("export_huts", "SM_Hut_"), ("export_perimeter", "SM_Hut_"), ("export_ground", "SM_Site_")):
            mans += [(exp, m) for m in sorted(glob.glob(os.path.join(KIT, exp, "%s%s_*.manifest.json" % (prefix, planet))))]
        for exp, man in mans:
            with open(man, encoding="utf-8") as fh:
                asset = json.load(fh)["asset"]
            params = unreal.ImportAssetParameters()
            params.is_automated = True
            params.replace_existing = True
            params.override_pipelines.append(pipe)
            mgr.import_asset(dest, unreal.InterchangeManager.create_source_data(os.path.join(KIT, exp, asset + ".fbx")), params)
            mesh = eas.load_asset("%s/%s" % (dest, asset))
            if mesh is None:
                log("FAILED %s" % asset)
                continue
            slots = []
            for i, sm in enumerate(mesh.static_materials):
                slot = re.sub(r"_\d+$", "", str(sm.material_slot_name))
                key = next((k for k in sorted(mis, key=len, reverse=True) if slot.endswith("_" + k) or slot == k), None)
                if key and mis[key]:
                    mesh.set_material(i, mis[key])
                slots.append("%d:%s" % (i, key))
            if asset.endswith("_LightStrip"):
                unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem).remove_collisions(mesh)
            ns = mesh.get_editor_property("nanite_settings")
            ns.enabled = False
            mesh.set_editor_property("nanite_settings", ns)
            eas.save_loaded_asset(mesh)
            bb = mesh.get_bounding_box()
            row = {"asset": asset, "size_cm": [round(bb.max.x - bb.min.x), round(bb.max.y - bb.min.y), round(bb.max.z - bb.min.z)],
                   "tris": mesh.get_num_triangles(0), "hulls": sme.get_convex_collision_count(mesh), "slots": slots}
            rows.append(row)
            log(json.dumps(row))
    with open(os.path.join(KIT, "review", "ue_import_huts.json"), "w", encoding="utf-8") as fh:
        json.dump(rows, fh, indent=1)
    log("huts import done, %d meshes, notes %s" % (len(rows), notes))


main()
