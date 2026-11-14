# M_TD_RegionHaze: allow it on Niagara mesh particles (review sphere NS_TD_Debug_HazeSphere); flags only, no expressions touched.
m = unreal.load_asset('/Game/TriadDrift/Gameplay/VFX/Materials/M_TD_RegionHaze')
m.set_editor_property('used_with_niagara_mesh_particles', True)
unreal.MaterialEditingLibrary.recompile_material(m)
'saved' if unreal.EditorAssetLibrary.save_loaded_asset(m, False) else 'SAVE FAILED'
