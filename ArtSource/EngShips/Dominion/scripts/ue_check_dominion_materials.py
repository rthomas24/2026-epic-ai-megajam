# SPDX-License-Identifier: MIT
# Triad Drift - compile check for the Dominion masters (instruction counts via MaterialEditingLibrary.get_statistics,
# which compiles the material for the editor's shader platform). Read-only: saves nothing.
#   powershell -File Tools\td-python.ps1 -Script ArtSource\EngShips\Dominion\scripts\ue_check_dominion_materials.py
import unreal

ok = True
for path in ("/Game/TriadDrift/Gameplay/Props/Dominion/M_TD_DominionPanel", "/Game/TriadDrift/Gameplay/Props/Dominion/M_TD_DominionGlow"):
    m = unreal.EditorAssetLibrary.load_asset(path)
    if m is None:
        unreal.log("TDPY: FAILED missing " + path)
        ok = False
        continue
    try:
        st = unreal.MaterialEditingLibrary.get_statistics(m)
        vals = {}
        for k in ("num_vertex_shader_instructions", "num_pixel_shader_instructions", "num_samplers",
                  "num_pixel_texture_samples", "num_uv_scalars"):
            try:
                vals[k] = st.get_editor_property(k)
            except Exception as e:
                vals[k] = "?"
        unreal.log("TDPY: %s stats %s" % (path, vals))
        if not vals.get("num_pixel_shader_instructions"):
            ok = False
    except Exception as e:
        unreal.log("TDPY: stats failed for %s (%s)" % (path, e))
        ok = False
    try:
        errs = unreal.MaterialEditingLibrary.get_material_compile_errors(m) if hasattr(unreal.MaterialEditingLibrary, "get_material_compile_errors") else "n/a"
        unreal.log("TDPY: %s compile errors: %s" % (path, errs))
    except Exception as e:
        unreal.log("TDPY: compile errors query failed (%s)" % e)
unreal.log("TDPY: OK" if ok else "TDPY: FAILED")
