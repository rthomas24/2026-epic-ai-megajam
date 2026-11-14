# SPDX-License-Identifier: MIT
# Crew readability (Lead, Nov 13 night): report the crew materials (parents, parameters, values, the mesh slots and the
# DA material overrides) before any change.  powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\Kit\scripts\ue_crew_inspect.py
import unreal

eas = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
C = "/Game/TriadDrift/Gameplay/Characters/"


def log(m):
    unreal.log("TDPY: " + m)


for path in sorted(eas.list_assets(C, recursive=True, include_folder=False)):
    if "/Anims/" in path:
        continue
    a = eas.load_asset(path.split(".")[0])
    if isinstance(a, unreal.MaterialInstanceConstant):
        p = a.get_editor_property("parent")
        s = {n: round(mel.get_material_instance_scalar_parameter_value(a, n), 3) for n in mel.get_scalar_parameter_names(a)}
        v = {}
        for n in mel.get_vector_parameter_names(a):
            c = mel.get_material_instance_vector_parameter_value(a, n)
            v[str(n)] = (round(c.r, 3), round(c.g, 3), round(c.b, 3))
        t = {}
        for n in mel.get_texture_parameter_names(a):
            tx = mel.get_material_instance_texture_parameter_value(a, n)
            t[str(n)] = tx.get_name() if tx else None
        log("MI %s parent=%s scalars=%s vectors=%s textures=%s" % (path.split(".")[0], p.get_path_name() if p else None,
                                                                     {str(k): x for k, x in s.items()}, v, t))
    elif isinstance(a, unreal.Material):
        log("M %s blend=%s shading=%s" % (path.split(".")[0], a.get_editor_property("blend_mode"), a.get_editor_property("shading_model")))
    elif isinstance(a, unreal.SkeletalMesh):
        log("SK %s slots=%s" % (path.split(".")[0], [(str(m.material_slot_name), m.material_interface.get_name() if m.material_interface else None)
                                                    for m in a.get_editor_property("materials")]))
    elif isinstance(a, unreal.StaticMesh) and "Drone" in path:
        log("SM %s slots=%s" % (path.split(".")[0], [(str(m.material_slot_name), m.material_interface.get_name() if m.material_interface else None)
                                                    for m in a.get_editor_property("static_materials")]))
    elif a is not None and type(a).__name__.startswith("TD") or (a is not None and "DA_Crew" in path):
        try:
            mo = a.get_editor_property("material_overrides")
            log("DA %s overrides=%s" % (path.split(".")[0], [str(x) for x in mo]))
        except Exception as e:
            log("DA %s (%s)" % (path.split(".")[0], e))
# parents that are not under Characters
for mp in ("/Game/TriadDrift/Gameplay/Materials/M_TD_PropTextured",):
    m = eas.load_asset(mp)
    if m:
        log("PARENT %s scalars=%s vectors=%s" % (mp, [str(n) for n in mel.get_scalar_parameter_names(m)], [str(n) for n in mel.get_vector_parameter_names(m)]))
log("crew inspect done")
