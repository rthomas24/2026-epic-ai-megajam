# 10h: a non-Nanite copy of the Rift hull for the cloak shell (Niagara mesh particles do not draw a Nanite mesh).
EAL = unreal.EditorAssetLibrary
dst = '/Game/TriadDrift/Gameplay/VFX/Meshes/SM_TD_VFX_RiftCloakShell'
out = []
if not EAL.does_asset_exist(dst):
    EAL.duplicate_asset('/Game/TriadDrift/Gameplay/Ships/Rift/SM_Rift', dst)
m = unreal.load_asset(dst)
ns = m.get_editor_property('nanite_settings'); ns.set_editor_property('enabled', False); m.set_editor_property('nanite_settings', ns)
m.set_editor_property('nanite_settings', ns)
b = m.get_bounding_box()
out.append('nanite=%s bounds min=%s max=%s' % (m.get_editor_property('nanite_settings').get_editor_property('enabled'), b.min, b.max))
out.append('saved' if EAL.save_loaded_asset(m, False) else 'SAVE FAILED')
"\n".join(out)
