# Rebuild only M_TD_Ground (anti-tiling graph, ue_ground_material.py) in place and refresh MI_Ground_Verdance /
# MI_Ground_Cinder, importing T_Ground_MacroNoise. The master keeps its path and its instances keep theirs, so
# ATDPlanetCourse's soft paths are unchanged. Editing a master with instances crashed the -nullrhi editor, so this
# runs with a renderer: back up the three .uassets first (Saved/GroundTiling/backup), then
#   powershell -File ArtSource\EngShips\Dominion\scripts\run_ue_rendered.ps1 -Script ArtSource\EngProps\Planets\scripts\ue_ground_rebuild.py
import os
import sys

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import ue_mat_builder  # noqa: E402
import ue_ground_material as gm  # noqa: E402

TEX = os.path.join(os.path.dirname(HERE), "textures")
DEST = "/Game/TriadDrift/Gameplay/Props/Planets"
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
problems = []


def log(m):
    unreal.log("TDPY: " + str(m))


def import_noise():
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", os.path.join(TEX, "T_Ground_MacroNoise.png").replace("\\", "/"))
    t.set_editor_property("destination_path", DEST)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", False)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
    tex = eal.load_asset(DEST + "/T_Ground_MacroNoise")
    tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
    tex.set_editor_property("srgb", False)
    eal.save_loaded_asset(tex)
    return tex


def main():
    t = {"T_Ground_MacroNoise": import_noise()}
    for s in ("BC", "N", "ORM", "E"):
        t["T_Ground_Verdance_" + s] = eal.load_asset("%s/T_Ground_Verdance_%s" % (DEST, s))
    mat = eal.load_asset(DEST + "/M_TD_Ground")
    mel.delete_all_material_expressions(mat)
    gm.build_ground_graph(ue_mat_builder.MB(mat, problems), t)
    mel.recompile_material(mat)
    mel.layout_material_expressions(mat)
    eal.save_asset(DEST + "/M_TD_Ground", only_if_is_dirty=False)
    st = mel.get_statistics(mat)
    log("M_TD_Ground: pixel %s vertex %s samplers %s" % (st.get_editor_property("num_pixel_shader_instructions"),
                                                        st.get_editor_property("num_vertex_shader_instructions"),
                                                        st.get_editor_property("num_samplers")))
    for g in gm.GROUND:
        path = "%s/MI_Ground_%s" % (DEST, g)
        inst = eal.load_asset(path)
        scalars, vectors = gm.instance_values(g)
        for k, v in scalars.items():
            if not mel.set_material_instance_scalar_parameter_value(inst, k, float(v)):
                problems.append("%s: scalar %s not set" % (g, k))
        for k, v in vectors.items():
            mel.set_material_instance_vector_parameter_value(inst, k, unreal.LinearColor(v[0], v[1], v[2], 1.0))
        mel.update_material_instance(inst)
        eal.save_asset(path, only_if_is_dirty=False)
        st = mel.get_statistics(inst)
        log("%s: parent %s pixel %s samplers %s" % (path, inst.get_editor_property("parent").get_name(),
                                                    st.get_editor_property("num_pixel_shader_instructions"),
                                                    st.get_editor_property("num_samplers")))
    for p in problems:
        log("PROBLEM: " + p)
    log("FAILED (%d problems)" % len(problems) if problems else "OK")


main()
