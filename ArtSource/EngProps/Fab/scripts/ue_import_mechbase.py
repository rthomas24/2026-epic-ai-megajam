# SPDX-License-Identifier: MIT
# Triad Drift - take 22 parts of Fab "Modular Scifi Mechanic Base" (Jay Zheng, Fab Standard) into the project, headless
# (Tools\td-python.ps1). Before the run the pack's Content folder is copied unchanged to Content/Modular_Scifi_Mechanic_Base
# (staging, never committed). The script:
#   1. collects the picked static meshes (ue_mechbase_pick.py) and everything they depend on inside the pack
#      (material instances, masters, material function, textures);
#   2. moves them in one batch to /Game/TriadDrift/Gameplay/Props/Fab/MechanicBase/<same sub-path> (references follow);
#   3. keeps a copy of every moved material instance as <name>_orig, then recolours the instances to the Null palette
#      (black armour, crimson accents, red-orange light) by their base / emission colour parameters only;
#   4. caps the pack's textures at 2048 (five trim textures were 8K), saves, and deletes the staging folder.
import json
import os
import sys

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ue_mechbase_pick import PICK, ROOT  # noqa: E402

STAGE = "/Game/Modular_Scifi_Mechanic_Base"
DEST = "/Game/TriadDrift/Gameplay/Props/Fab/MechanicBase"
eas = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
reg = unreal.AssetRegistryHelpers.get_asset_registry()
mel = unreal.MaterialEditingLibrary
fails = []
LC = unreal.LinearColor
# Null palette by instance: BASE_COLOR / SECOND_COLOR (M_Base), BASE_COLOR (decals), Color (subset decals), EMS_COLOR (emission)
NULL = {
    "MI_Metal_01": {"BASE_COLOR": LC(0.018, 0.018, 0.02, 1), "SECOND_COLOR": LC(0.035, 0.034, 0.038, 1)},
    "MI_Metal_02": {"BASE_COLOR": LC(0.03, 0.03, 0.032, 1)},
    "MI_Metal_03": {"BASE_COLOR": LC(0.02, 0.02, 0.022, 1)},
    "MI_Metal_04": {"BASE_COLOR": LC(0.008, 0.008, 0.009, 1), "SECOND_COLOR": LC(0.03, 0.03, 0.032, 1)},
    "MI_Metal_05": {"BASE_COLOR": LC(0.035, 0.035, 0.038, 1), "SECOND_COLOR": LC(0.025, 0.025, 0.027, 1)},
    "MI_Metal_06": {"BASE_COLOR": LC(0.45, 0.02, 0.02, 1), "SECOND_COLOR": LC(0.2, 0.01, 0.01, 1)},       # crimson accent
    "MI_Metal_08": {"BASE_COLOR": LC(0.025, 0.025, 0.027, 1), "SECOND_COLOR": LC(0.03, 0.03, 0.032, 1)},
    "MI_Metal_09": {"BASE_COLOR": LC(0.04, 0.04, 0.043, 1), "SECOND_COLOR": LC(0.03, 0.03, 0.032, 1)},
    "MI_Metal_10": {"BASE_COLOR": LC(0.32, 0.015, 0.015, 1), "SECOND_COLOR": LC(0.15, 0.01, 0.01, 1)},     # crimson accent
    "MI_INFO_DECALS": {"BASE_COLOR": LC(0.12, 0.02, 0.02, 1)},
    "MI_EMISION": {"EMS_COLOR": LC(2.0, 0.3, 0.06, 1)},
    "MI_EMISION_2": {"EMS_COLOR": LC(1.2, 0.18, 0.04, 1)},
    "MI_EMISION_4": {"EMS_COLOR": LC(2.0, 0.25, 0.05, 1)},
    "MI_EMISION_8": {"EMS_COLOR": LC(2.0, 0.3, 0.06, 1)},
}


def log(m):
    unreal.log("TDPY: " + m)


def deps(pkg, seen):
    opts = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True)
    for d in reg.get_dependencies(pkg, opts) or []:
        d = str(d)
        if d.startswith(STAGE) and d not in seen:
            seen.add(d)
            deps(d, seen)


pkgs = set()
for rel in PICK:
    p = ROOT + rel
    if not eas.does_asset_exist(p):
        fails.append("missing " + rel)
        continue
    pkgs.add(p)
    deps(p, pkgs)
log("moving %d packages (%d meshes)" % (len(pkgs), len(PICK)))
data = []
for p in sorted(pkgs):
    a = eas.load_asset(p)
    if a is None:
        fails.append("load " + p)
        continue
    sub = p[len(STAGE):].rsplit("/", 1)[0]
    folder = DEST + sub
    if not eas.does_directory_exist(folder):
        eas.make_directory(folder)
    data.append(unreal.AssetRenameData(a, folder, p.rsplit("/", 1)[1]))
ok = tools.rename_assets(data)
log("rename_assets: %s" % ok)
moved = [DEST + p[len(STAGE):] for p in sorted(pkgs)]
recoloured = {}
for p in moved:
    a = eas.load_asset(p)
    if a is None:
        fails.append("after move " + p)
        continue
    name = p.rsplit("/", 1)[1]
    if isinstance(a, unreal.Texture2D):
        a.set_editor_property("max_texture_size", 2048)
    if isinstance(a, unreal.MaterialInstanceConstant) and name in NULL:
        keep = p + "_orig"
        if not eas.does_asset_exist(keep):
            eas.duplicate_asset(p, keep)
        before = {}
        for pname, col in NULL[name].items():
            before[pname] = [round(c, 3) for c in (lambda v: (v.r, v.g, v.b))(mel.get_material_instance_vector_parameter_value(a, pname))]
            mel.set_material_instance_vector_parameter_value(a, pname, col)
        recoloured[name] = before
    if not eas.save_loaded_asset(a, False):
        fails.append("save " + p)
for p in eas.list_assets(DEST, recursive=True, include_folder=False):
    if p.split(".")[0].endswith("_orig"):
        eas.save_asset(p.split(".")[0], only_if_is_dirty=False)
rows = []
for rel in PICK:
    m = eas.load_asset(DEST + "/Mesh/SM/" + rel)
    if m is None:
        fails.append("mesh " + rel)
        continue
    mats = [sm.get_editor_property("material_interface") for sm in m.static_materials]
    bad = [x.get_path_name() for x in mats if x and not x.get_path_name().startswith(DEST)]
    if bad:
        fails.append("%s still points at %s" % (rel, bad))
    rows.append({"mesh": m.get_name(), "tris": int(m.get_num_triangles(0)), "slots": len(mats)})
eas.delete_directory(STAGE)
log("staging left: %s" % eas.does_directory_exist(STAGE))
log("RECOLOURED " + json.dumps(recoloured))
log("MESHES " + json.dumps(rows))
log("OK" if not fails else "FAILED %s" % fails[:20])
