m = unreal.load_asset('/Game/TriadDrift/Gameplay/VFX/Materials/M_TD_VFX_CloakOverlay')
for f in ('used_with_skeletal_mesh', 'used_with_static_lighting'):
    try: m.set_editor_property(f, f == 'used_with_skeletal_mesh')
    except Exception: pass
unreal.MaterialEditingLibrary.recompile_material(m)
'saved' if unreal.EditorAssetLibrary.save_loaded_asset(m, False) else 'SAVE FAILED'
