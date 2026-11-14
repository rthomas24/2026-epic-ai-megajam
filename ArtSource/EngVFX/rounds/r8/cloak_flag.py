# M_TD_VFX_CloakOverlay usage flags (needs a renderer to recompile): skeletal meshes and, since 10h, Niagara mesh particles
m = unreal.load_asset('/Game/TriadDrift/Gameplay/VFX/Materials/M_TD_VFX_CloakOverlay')
for f in ('used_with_skeletal_mesh', 'used_with_niagara_mesh_particles', 'used_with_niagara_sprites'):
    m.set_editor_property(f, True)
unreal.MaterialEditingLibrary.recompile_material(m)
'saved' if unreal.EditorAssetLibrary.save_loaded_asset(m, False) else 'SAVE FAILED'
