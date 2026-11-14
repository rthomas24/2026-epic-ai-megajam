# SPDX-License-Identifier: MIT
# Triad Drift - read-only probe of the staged Fab "Modular Scifi Mechanic Base" (Content/Modular_Scifi_Mechanic_Base,
# staging only): for each picked mesh, its triangles, bounds and slots; for each material, parent and parameters.
import json
import os
import sys

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ue_mechbase_pick import PICK, ROOT  # noqa: E402

eas = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
out = {"meshes": {}, "materials": {}}
mats = set()
for rel in PICK:
    m = eas.load_asset(ROOT + rel)
    if m is None:
        out["meshes"][rel] = None
        continue
    b = m.get_bounding_box()
    slots = []
    for sm in m.static_materials:
        mi = sm.get_editor_property("material_interface")
        slots.append(mi.get_path_name() if mi else None)
        if mi:
            mats.add(mi.get_path_name().split(".")[0])
    out["meshes"][rel] = {"tris": int(m.get_num_triangles(0)), "size": [round(b.max.x - b.min.x), round(b.max.y - b.min.y), round(b.max.z - b.min.z)],
                          "slots": slots, "nanite": m.get_editor_property("nanite_settings").enabled}
for p in sorted(mats):
    a = eas.load_asset(p)
    row = {"class": type(a).__name__}
    if isinstance(a, unreal.MaterialInstanceConstant):
        row["parent"] = a.get_editor_property("parent").get_path_name() if a.get_editor_property("parent") else None
        row["vectors"] = {str(v.parameter_info.name): [round(v.parameter_value.r, 3), round(v.parameter_value.g, 3), round(v.parameter_value.b, 3)]
                          for v in a.get_editor_property("vector_parameter_values")}
        row["scalars"] = {str(v.parameter_info.name): round(v.parameter_value, 3) for v in a.get_editor_property("scalar_parameter_values")}
        row["textures"] = {str(v.parameter_info.name): v.parameter_value.get_name() if v.parameter_value else None
                           for v in a.get_editor_property("texture_parameter_values")}
        parent = a.get_editor_property("parent")
        while parent and isinstance(parent, unreal.MaterialInstance):
            parent = parent.get_editor_property("parent")
        if parent:
            row["master_vectors"] = [str(n) for n in mel.get_vector_parameter_names(parent)]
            row["master_scalars"] = [str(n) for n in mel.get_scalar_parameter_names(parent)]
    out["materials"][p] = row
unreal.log("TDPY: PROBE " + json.dumps(out))
unreal.log("TDPY: OK")
