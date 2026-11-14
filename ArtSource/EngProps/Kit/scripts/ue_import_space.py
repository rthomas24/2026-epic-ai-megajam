# SPDX-License-Identifier: MIT
# Space set pieces (T6's regions) -> /Game/TriadDrift/Gameplay/Props/Space/SetPieces: imports export_space/*.fbx with
# the kit's Interchange pipeline (UCX hulls, no Nanite), assigns instances by slot, builds 3 LODs and reports bounds.
#   powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\Kit\scripts\ue_import_space.py
import glob
import json
import os
import sys

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import ue_import_kit as K  # noqa: E402   (guarded main: only its helpers)

KIT = os.path.dirname(HERE)
REGION = os.environ.get("TD_SPACE_SET") == "region"       # the region set pieces (Props/Kit/Space, export_region, SM_KS_*)
DEST = "/Game/TriadDrift/Gameplay/Props/Kit/Space" if REGION else "/Game/TriadDrift/Gameplay/Props/Space/SetPieces"
EXPORT = "export_region" if REGION else "export_space"
PREFIX = "SM_KS_" if REGION else "SM_SP_"
K.DEST = DEST
K.PIPELINE = DEST + "/Pipelines/PL_Space_StaticMesh"
eas, mel = unreal.EditorAssetLibrary, unreal.MaterialEditingLibrary
KT = "/Game/TriadDrift/Gameplay/Props/Kit/"
SP = "/Game/TriadDrift/Gameplay/Props/Space/"


def log(m):
    unreal.log("TDPY: " + m)


def trim_mi(name, grime, grime_rough, tint=(1.0, 1.0, 1.0), self_illum=0.0, stem="T_Kit_Trim"):
    """r2 (Lead: near-black blocks in space light): the kit trim with weathering, a BaseTint for two-tone panels and a
    faint SelfIllum so the shadow side keeps its form against the nebula without a fill light."""
    parent = eas.load_asset(KT + "Materials/M_KitD_Trim")
    mi = K.instance(name, parent)
    for p_, t in (("BaseColorTex", stem + "_BC"), ("ORMTex", stem + "_ORM"), ("NormalTex", stem + "_N")):
        mel.set_material_instance_texture_parameter_value(mi, p_, eas.load_asset(KT + "Textures/" + t))
    mel.set_material_instance_scalar_parameter_value(mi, "GrimeAmount", grime)
    mel.set_material_instance_scalar_parameter_value(mi, "GrimeRough", grime_rough)
    mel.set_material_instance_vector_parameter_value(mi, "BaseTint", unreal.LinearColor(tint[0], tint[1], tint[2], 1.0))
    mel.set_material_instance_scalar_parameter_value(mi, "SelfIllum", self_illum)
    eas.save_loaded_asset(mi)
    return mi


def glow_mi(name, rgb, strength, pulse=0.0, rate=1.0):
    mi = K.instance(name, eas.load_asset(K.MASTER_GLOW))
    mel.set_material_instance_scalar_parameter_value(mi, "GlowStrength", strength)
    mel.set_material_instance_scalar_parameter_value(mi, "PulseAmount", pulse)
    mel.set_material_instance_scalar_parameter_value(mi, "PulseRate", rate)
    mel.set_material_instance_vector_parameter_value(mi, "GlowColor", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    eas.save_loaded_asset(mi)
    return mi


def main():
    notes = []
    K.ensure_dir(DEST)
    patrol = trim_mi("MI_SP_PatrolHull", 0.2, 0.25, (1.0, 1.0, 1.0), 0.25)            # the squad's off-white hull (r3: lighter)
    patrol2 = trim_mi("MI_SP_PatrolHull2", 0.3, 0.25, (0.7, 0.72, 0.76), 0.25)        # its mid-grey second tone
    stripe = trim_mi("MI_SP_PatrolStripe", 0.2, 0.2, (0.3, 0.72, 0.78), 0.25)          # one muted squad-cyan band
    research = trim_mi("MI_SP_ResearchHull", 0.35, 0.3, (0.95, 0.93, 0.88), 0.2)        # older civilian: weathered off-white
    research2 = trim_mi("MI_SP_ResearchHull2", 0.45, 0.3, (0.6, 0.58, 0.55), 0.2)      # and a warm mid-grey
    scorch = trim_mi("MI_SP_Scorch", 1.0, 0.4, (0.12, 0.1, 0.09), 0.02)              # burnt plates and frames
    hulk = trim_mi("MI_KS_HulkDark", 0.4, 0.3, (0.62, 0.62, 0.66), 0.14)              # r4: mid-grey hull (was near-black)
    hulk2 = trim_mi("MI_KS_HulkDark2", 0.6, 0.35, (0.3, 0.3, 0.33), 0.08)             # and its dark panels
    strata = trim_mi("MI_KS_Strata", 0.5, 0.3, (0.5, 0.45, 0.4), 0.05, stem="T_KitS_Trim")      # r3: subtle strata (about 25 %)
    strata2 = trim_mi("MI_KS_Strata2", 0.45, 0.3, (0.64, 0.58, 0.51), 0.05, stem="T_KitS_Trim")
    rim = trim_mi("MI_KS_FractureRim", 0.3, 0.3, (0.9, 0.86, 0.78), 0.08, stem="T_KitS_Trim")    # the pale band along the break
    rock = eas.load_asset(KT + "Materials/M_KitD_Trim")
    rock = K.instance("MI_KS_Rock", rock)                       # r2: the asteroid textures on the trim master (world-position
    for p_, t in (("BaseColorTex", "T_Asteroid_03_BC"), ("ORMTex", "T_Asteroid_03_ORM"), ("NormalTex", "T_Asteroid_03_N")):  # macro variation)
        mel.set_material_instance_texture_parameter_value(rock, p_, eas.load_asset(SP + t))
    mel.set_material_instance_scalar_parameter_value(rock, "GrimeAmount", 0.55)
    mel.set_material_instance_scalar_parameter_value(rock, "GrimeRough", 0.15)
    mel.set_material_instance_vector_parameter_value(rock, "BaseTint", unreal.LinearColor(1.2, 1.08, 0.92, 1.0))   # r3: warmer (no blue)
    mel.set_material_instance_scalar_parameter_value(rock, "SelfIllum", 0.04)
    eas.save_loaded_asset(rock)
    amber = glow_mi("MI_KS_AmberBlink", (1.0, 0.55, 0.1), 40.0, pulse=1.0, rate=0.6)
    cut = trim_mi("MI_SP_RockCut", 0.5, 0.2, (0.85, 0.82, 0.78), 0.06, stem="T_KitS_Trim")   # the rigs' machined face
    common = {"Dom": eas.load_asset(KT + "Dominion/MI_KitD_Trim"), "Glow": eas.load_asset(KT + "Dominion/MI_KitD_Glow"),
              "Eye": glow_mi("MI_SP_NullEye", (1.0, 1.0, 1.0), 25.0), "Light": glow_mi("MI_SP_Window", (1.0, 0.72, 0.42), 14.0),
              "Flicker": glow_mi("MI_SP_WindowFlicker", (1.0, 0.72, 0.42), 10.0, pulse=0.85, rate=3.7),
              "Run": glow_mi("MI_SP_RunLight", (1.0, 0.18, 0.12), 12.0, pulse=0.9, rate=0.5),
              "Beacon": glow_mi("MI_SP_RigBeacon", (1.0, 0.05, 0.02), 54.0, pulse=0.95, rate=0.25),       # r4: -40 %, slower
              "Breach": glow_mi("MI_SP_Breach", (0.25, 0.55, 1.0), 8.0, pulse=0.25, rate=0.8),
              "Work": glow_mi("MI_SP_WorkLight", (1.0, 0.9, 0.7), 45.0),
              "Strobe": glow_mi("MI_SP_DistressStrobe", (0.0, 0.85, 1.0), 70.0, pulse=1.0, rate=1.3),      # r4: squad cyan, not enemy-eye white
              "Nav": glow_mi("MI_SP_NavAmber", (0.55, 0.9, 0.15), 25.0, pulse=0.0, rate=0.4),          # r4: green-amber, steady
              "Stripe": stripe, "BoreEnd": scorch,
              "CyanRun": glow_mi("MI_SP_CyanRun", (0.0, 0.85, 1.0), 8.0),
              "Cold": glow_mi("MI_KS_ColdRun", (0.65, 0.82, 1.0), 30.0),           # r4: cold blue-white
              "RimLight": glow_mi("MI_KS_RimLight", (0.7, 0.85, 1.0), 4.0),
              "Strata": strata, "Strata2": strata2, "Rim": rim, "DomRaw": eas.load_asset(KT + "Dominion/MI_KitD_Trim"),
              "CrackHot": glow_mi("MI_KS_CrackHot", (1.0, 0.72, 0.42), 10.0),
              "CrackDim": glow_mi("MI_KS_CrackDim", (0.7, 0.06, 0.02), 1.5),
              "Seam": glow_mi("MI_KS_Seam", (1.0, 0.38, 0.06), 6.0),
              "Work": glow_mi("MI_SP_WorkLight", (1.0, 0.9, 0.7), 45.0),          # r5: steady, under the band
              "Scorch": scorch, "Cut": cut,
              "Rock": eas.load_asset(SP + "MI_Asteroid_03"), "Bore": eas.load_asset(SP + "MI_SpaceDebris_01")}
    pipe = K.ensure_pipeline(notes)
    sme = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    mgr = unreal.InterchangeManager.get_interchange_manager_scripted()
    rows = []
    for man_path in sorted(glob.glob(os.path.join(KIT, EXPORT, PREFIX + "*.manifest.json"))):
        with open(man_path, encoding="utf-8") as fh:
            man = json.load(fh)
        asset = man["asset"]
        params = unreal.ImportAssetParameters()
        params.is_automated = True
        params.replace_existing = True
        params.override_pipelines.append(pipe)
        mgr.import_asset(DEST, unreal.InterchangeManager.create_source_data(os.path.join(KIT, EXPORT, asset + ".fbx")), params)
        mesh = eas.load_asset("%s/%s" % (DEST, asset))
        if mesh is None:
            log("FAILED %s" % asset)
            continue
        mis = dict(common)
        mis["Trim"] = research if "Research" in asset else patrol
        mis["Trim2"] = research2 if "Research" in asset else patrol2
        if "Hulk" in asset:                                   # the cut-up cruiser: dark, cold
            mis["Trim"], mis["Trim2"] = hulk, hulk2
        if "PicketRing" in asset:
            mis["Nav"] = amber
        if REGION:
            mis["Rock"] = rock                                 # region pieces: the hard-faceted, tiled asteroid look
        if "Hulk" in asset:
            mis["Nav"] = glow_mi("MI_KS_AmberDim", (1.0, 0.5, 0.08), 15.0, pulse=0.5, rate=0.3)
        if "ScarRig" in asset:
            mis["Beacon"] = glow_mi("MI_KS_NullBeacon", (1.0, 0.04, 0.02), 50.0, pulse=0.6, rate=0.3)
            mis["Work"] = glow_mi("MI_KS_HazardWork", (1.0, 0.75, 0.1), 30.0)
            mis["Strobe"] = glow_mi("MI_KS_WhiteStrobe", (1.0, 1.0, 1.0), 60.0, pulse=1.0, rate=0.4)
        slots = []
        for i, sm in enumerate(mesh.static_materials):
            slot = str(sm.material_slot_name)
            key = next((k for k in mis if slot.endswith("_" + k) or slot == k), None)
            if key:
                mesh.set_material(i, mis[key])
            slots.append("%d:%s" % (i, key))
        try:
            ns = mesh.get_editor_property("nanite_settings")
            ns.enabled = False
            mesh.set_editor_property("nanite_settings", ns)
        except Exception as e:
            notes.append("nanite %s" % e)
        try:
            opts = unreal.EditorScriptingMeshReductionOptions()
            opts.set_editor_property("auto_compute_lod_screen_size", True)
            opts.set_editor_property("reduction_settings", [unreal.EditorScriptingMeshReductionSettings(1.0, 1.0),
                                                             unreal.EditorScriptingMeshReductionSettings(0.45, 0.3),
                                                             unreal.EditorScriptingMeshReductionSettings(0.18, 0.1)])
            lods = sme.set_lods(mesh, opts)
        except Exception as e:
            lods = -1
            notes.append("lods %s: %s" % (asset, e))
        eas.save_loaded_asset(mesh)
        bb = mesh.get_bounding_box()
        row = {"asset": asset, "size_m": [round((bb.max.x - bb.min.x) / 100, 1), round((bb.max.y - bb.min.y) / 100, 1), round((bb.max.z - bb.min.z) / 100, 1)],
               "tris_lod0": mesh.get_num_triangles(0), "lods": lods, "hulls": sme.get_convex_collision_count(mesh), "slots": slots}
        rows.append(row)
        log(json.dumps(row))
    for n in ("SM_Asteroid_01", "SM_Asteroid_02", "SM_Asteroid_03", "SM_Asteroid_04"):
        m = eas.load_asset(SP + n)
        bb = m.get_bounding_box()
        log("ROCK %s min (%.0f %.0f %.0f) max (%.0f %.0f %.0f)" % (n, bb.min.x, bb.min.y, bb.min.z, bb.max.x, bb.max.y, bb.max.z))
    with open(os.path.join(KIT, "review", "ue_import_space.json"), "w", encoding="utf-8") as fh:
        json.dump(rows, fh, indent=1)
    log("space import done, %d meshes, notes %s" % (len(rows), notes))


main()
