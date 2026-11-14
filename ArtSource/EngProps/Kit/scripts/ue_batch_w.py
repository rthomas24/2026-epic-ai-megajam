# SPDX-License-Identifier: MIT
# Batch W, headless: rebuild M_KitD_Trim with the GrimeScale parameter (the usage flags included) from the Dominion
# trim textures already imported, and give the moon shards' strata instances their large-scale value variation.
#   powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\Kit\scripts\ue_batch_w.py
import os
import sys

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ue_import_kit as K  # noqa: E402

eas, mel = unreal.EditorAssetLibrary, unreal.MaterialEditingLibrary
notes = []
D = "/Game/TriadDrift/Gameplay/Props/Kit/Dominion/Textures/T_KitD_Trim_"
tex = {k: eas.load_asset(D + k) for k in ("BC", "N", "ORM")}
mat = K.ensure_trim_grime(tex, notes)
unreal.log("TDPY: master %s ism=%s nanite=%s notes %s" % (mat and mat.get_name(), mat and mat.get_editor_property("used_with_instanced_static_meshes"),
                                                        mat and mat.get_editor_property("used_with_nanite"), notes))
for name, amt in (("MI_KS_Strata", 0.75), ("MI_KS_Strata2", 0.7)):
    path = "/Game/TriadDrift/Gameplay/Props/Kit/Space/" + name
    mi = eas.load_asset(path)
    mel.set_material_instance_scalar_parameter_value(mi, "GrimeScale", 0.08)
    mel.set_material_instance_scalar_parameter_value(mi, "GrimeAmount", amt)
    eas.save_asset(path, only_if_is_dirty=False)
    unreal.log("TDPY: %s GrimeScale=%s GrimeAmount=%s" % (name, mel.get_material_instance_scalar_parameter_value(mi, "GrimeScale"),
                                                         mel.get_material_instance_scalar_parameter_value(mi, "GrimeAmount")))
