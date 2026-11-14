# SPDX-License-Identifier: MIT
# Landing gear -> /Game/TriadDrift/Gameplay/Ships/<Ship>/SM_<Ship>_Gear (+ MI_<Ship>_GearAccent in the squad colour).
#   powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\Kit\scripts\ue_import_gear.py
import json
import os
import sys

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import ue_import_kit as K  # noqa: E402

KIT = os.path.dirname(HERE)
eas, mel = unreal.EditorAssetLibrary, unreal.MaterialEditingLibrary
KT = "/Game/TriadDrift/Gameplay/Props/Kit/"
SQUAD = {"Rift": (1.0, 0.1, 0.75), "Lancer": (0.0, 0.85, 1.0), "Bastion": (1.0, 0.6, 0.05)}


def log(m):
    unreal.log("TDPY: " + m)


def main():
    notes = []
    sme = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    mgr = unreal.InterchangeManager.get_interchange_manager_scripted()
    trim = eas.load_asset(KT + "MI_Kit_Trim")
    pad = eas.load_asset(KT + "Dominion/MI_KitD_Trim")
    for ship, rgb in SQUAD.items():
        dest = "/Game/TriadDrift/Gameplay/Ships/%s" % ship
        K.DEST = dest
        K.PIPELINE = KT + "Pipelines/PL_Kit_StaticMesh"
        pipe = K.ensure_pipeline(notes)
        acc = K.instance("MI_%s_GearAccent" % ship, eas.load_asset(K.MASTER_GLOW))
        mel.set_material_instance_scalar_parameter_value(acc, "GlowStrength", 2.0)
        mel.set_material_instance_scalar_parameter_value(acc, "PulseAmount", 0.0)
        mel.set_material_instance_vector_parameter_value(acc, "GlowColor", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
        eas.save_loaded_asset(acc)
        asset = "SM_%s_Gear" % ship
        params = unreal.ImportAssetParameters()
        params.is_automated = True
        params.replace_existing = True
        params.override_pipelines.append(pipe)
        mgr.import_asset(dest, unreal.InterchangeManager.create_source_data(os.path.join(KIT, "export_gear", asset + ".fbx")), params)
        mesh = eas.load_asset("%s/%s" % (dest, asset))
        if mesh is None:
            log("FAILED %s" % asset)
            continue
        mis = {"Trim": trim, "Accent": acc, "Pad": pad}
        slots = []
        for i, sm in enumerate(mesh.static_materials):
            slot = str(sm.material_slot_name)
            key = next((k for k in mis if slot.endswith("_" + k) or slot == k), None)
            if key:
                mesh.set_material(i, mis[key])
            slots.append("%d:%s" % (i, key))
        try:
            sme.remove_collisions(mesh)
        except Exception as e:
            notes.append(str(e))
        ns = mesh.get_editor_property("nanite_settings")
        ns.enabled = False
        mesh.set_editor_property("nanite_settings", ns)
        bb = mesh.get_bounding_box()
        if mesh.find_socket("Skid") is None:          # Interchange drops the FBX socket: add it here at the pad bottom
            sock = unreal.StaticMeshSocket(mesh)
            sock.set_editor_property("socket_name", "Skid")
            sock.set_editor_property("relative_location", unreal.Vector(0.0, 0.0, bb.min.z))
            mesh.add_socket(sock)
        eas.save_loaded_asset(mesh)
        has_skid = False
        try:
            has_skid = mesh.find_socket("Skid") is not None
        except Exception as e:
            notes.append("socket %s" % e)
        log(json.dumps({"asset": "%s/%s" % (dest, asset), "tris": mesh.get_num_triangles(0), "slots": slots, "skid": has_skid,
                        "min_z": round(bb.min.z, 1), "max_z": round(bb.max.z, 1)}))
    log("gear import done, notes %s" % notes)


main()
