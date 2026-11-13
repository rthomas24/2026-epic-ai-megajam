# SPDX-License-Identifier: MIT
# Triad Drift - Dominion finale objectives (generator, generator core, reactor): import + data asset. Headless:
#   powershell -File Tools\td-python.ps1 -Script ArtSource\EngShips\Dominion\scripts\ue_import_dominion_parts.py
#
# Needs the kit's materials (ue_import_dominion.py) to exist. Writes ONLY:
#   Props/Dominion/MI_TD_DominionGlowCyan, MI_TD_DominionGlowReactor   (children of M_TD_DominionGlow)
#   Props/Dominion/SM_Dominion_Generator, SM_Dominion_GeneratorCore, SM_Dominion_Reactor   (Nanite, UCX, slots by name)
#   Characters/Dominion/DA_Dominion_Visuals: GeneratorMesh, GeneratorCoreMesh, ReactorMesh (other fields untouched)
# Does not re-save the hull kit's meshes, masters, textures or the import pipeline.

import json
import os

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
REPO = os.path.dirname(os.path.dirname(os.path.dirname(KIT)))
DEST = "/Game/TriadDrift/Gameplay/Props/Dominion"
DA_PATH = "/Game/TriadDrift/Gameplay/Characters/Dominion/DA_Dominion_Visuals"
GLOW = DEST + "/M_TD_DominionGlow"
PIPELINE = DEST + "/PL_Dominion_StaticMesh"
ASSETS = ["SM_Dominion_Generator", "SM_Dominion_GeneratorCore", "SM_Dominion_Reactor"]

eas = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary


def log(msg):
    unreal.log("TDPY: " + msg)


NEW_MIS = {
    "MI_TD_DominionGlowCyan": ({"GlowStrength": 26.0, "PulseAmount": 0.15, "PulseRate": 2.6}, (0.3, 0.8, 1.0)),
    "MI_TD_DominionGlowReactor": ({"GlowStrength": 30.0, "PulseAmount": 0.12, "PulseRate": 1.6}, (1.0, 0.7, 0.15)),
}
SLOT_TO_MI = {"Panel": "MI_TD_DominionPanel", "Trim": "MI_TD_DominionTrim", "GlowRed": "MI_TD_DominionGlowRed",
              "GlowWhite": "MI_TD_DominionGlowWhite", "GlowCyan": "MI_TD_DominionGlowCyan",
              "GlowReactor": "MI_TD_DominionGlowReactor"}


def main():
    glow = eas.load_asset(GLOW)
    if glow is None or not eas.does_asset_exist(PIPELINE):
        log("FAILED: kit materials / pipeline missing (run ue_import_dominion.py first)")
        return
    mis = {}
    for name, (scalars, rgb) in NEW_MIS.items():
        path = "%s/%s" % (DEST, name)
        mi = eas.load_asset(path) if eas.does_asset_exist(path) else tools.create_asset(
            name, DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi, glow)
        for k, v in scalars.items():
            mel.set_material_instance_scalar_parameter_value(mi, k, v)
        mel.set_material_instance_vector_parameter_value(mi, "GlowColor", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
        eas.save_loaded_asset(mi)
    for key, name in SLOT_TO_MI.items():
        mis[key] = eas.load_asset("%s/%s" % (DEST, name))
        if mis[key] is None:
            log("FAILED: missing " + name)
            return
    sme = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    mgr = unreal.InterchangeManager.get_interchange_manager_scripted()
    rows, meshes, bad = [], {}, 0
    for asset in ASSETS:
        fbx = os.path.join(KIT, asset, asset + ".fbx")
        params = unreal.ImportAssetParameters()
        params.is_automated = True
        params.replace_existing = True
        params.override_pipelines.append(unreal.SoftObjectPath("%s.%s" % (PIPELINE, PIPELINE.rsplit("/", 1)[-1])))
        mgr.import_asset(DEST, unreal.InterchangeManager.create_source_data(fbx), params)
        mesh = eas.load_asset("%s/%s" % (DEST, asset))
        if mesh is None:
            log("FAILED %s: not found after import" % asset)
            bad += 1
            continue
        notes = []
        try:
            sme.set_generate_lightmap_uv(mesh, False)
        except Exception as error:
            notes.append("lightmap: %s" % error)
        ns = mesh.get_editor_property("nanite_settings")
        ns.enabled = True
        mesh.set_editor_property("nanite_settings", ns)
        slots = []
        for i, sm in enumerate(mesh.static_materials):
            slot = str(sm.material_slot_name)
            key = next((k for k in SLOT_TO_MI if slot.endswith("_" + k) or slot == k), None)
            if key is None:
                notes.append("slot %s unmapped" % slot)
                bad += 1
                continue
            mesh.set_material(i, mis[key])
            slots.append("%s->%s" % (slot, SLOT_TO_MI[key]))
        eas.save_loaded_asset(mesh)
        b = mesh.get_bounds()
        body = mesh.get_editor_property("body_setup")
        convex = sme.get_convex_collision_count(mesh)
        try:
            tris = int(mesh.get_num_triangles(0))
        except Exception:
            tris = -1
        row = {"asset": asset, "tris": tris, "nanite": mesh.get_editor_property("nanite_settings").enabled,
               "bounds_min_cm": [round(b.origin.x - b.box_extent.x, 1), round(b.origin.y - b.box_extent.y, 1),
                                 round(b.origin.z - b.box_extent.z, 1)],
               "bounds_max_cm": [round(b.origin.x + b.box_extent.x, 1), round(b.origin.y + b.box_extent.y, 1),
                                 round(b.origin.z + b.box_extent.z, 1)],
               "convex_hulls": convex, "body_setup": body is not None, "slots": slots, "notes": notes}
        if body is not None:
            try:
                agg = body.get_editor_property("agg_geom")
                hulls = []
                for c in agg.get_editor_property("convex_elems"):
                    bb = c.get_editor_property("elem_box")
                    mn, mx = bb.get_editor_property("min"), bb.get_editor_property("max")
                    hulls.append([[round(mn.x), round(mn.y), round(mn.z)], [round(mx.x), round(mx.y), round(mx.z)]])
                row["hull_boxes_cm"] = hulls
                row["collision_trace"] = str(body.get_editor_property("collision_trace_flag"))
            except Exception as error:
                row["notes"].append("agg_geom: %s" % error)
        rows.append(row)
        meshes[asset] = mesh
        log(json.dumps(row))
    da = eas.load_asset(DA_PATH)
    if da is None:
        log("FAILED: %s missing" % DA_PATH)
        bad += 1
    else:
        before = {p: str(da.get_editor_property(p)) for p in ("hull_mesh", "bridge_mesh", "trench_mesh",
                                                             "shield_emitter_mesh")}
        for prop, asset in (("generator_mesh", "SM_Dominion_Generator"), ("generator_core_mesh", "SM_Dominion_GeneratorCore"),
                            ("reactor_mesh", "SM_Dominion_Reactor")):
            if asset in meshes:
                da.set_editor_property(prop, meshes[asset])
        eas.save_loaded_asset(da)
        after = {p: str(da.get_editor_property(p)) for p in ("generator_mesh", "generator_core_mesh", "reactor_mesh",
                                                            "hull_mesh", "bridge_mesh", "trench_mesh", "shield_emitter_mesh")}
        log("DA: " + json.dumps(after))
        rows.append({"data_asset": DA_PATH, "after": after, "untouched_before": before})
    out = os.path.join(REPO, "Saved", "DominionParts", "import_report.json")
    os.makedirs(os.path.dirname(out), exist_ok=True)
    with open(out, "w", encoding="utf-8") as fh:
        json.dump(rows, fh, indent=2)
    log("imported %d mesh(es), %d problem(s)" % (len(meshes), bad))
    log("OK" if not bad else "FAILED: see above")


main()
