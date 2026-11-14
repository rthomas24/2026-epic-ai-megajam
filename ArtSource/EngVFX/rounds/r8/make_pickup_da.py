# Round 8: the engineering DA_Pickup_Visuals (T2's UTDPickupVisuals) in the fallback folder, VFX fields only (Sound empty:
# Nicholas picks the sounds). Run in the editor:  python ArtSource/EngVFX/scripts/run_editor_py.py 8017 <this file>
PATH = '/Game/TriadDrift/Gameplay/Characters/FX'
NAME = 'DA_Pickup_Visuals'
SYS = '/Game/TriadDrift/Gameplay/VFX/Systems/'
EAL = unreal.EditorAssetLibrary
out = []
if EAL.does_asset_exist(PATH + '/' + NAME):
    da = unreal.load_asset(PATH + '/' + NAME)
    out.append('exists')
else:
    f = unreal.DataAssetFactory()
    f.set_editor_property('data_asset_class', unreal.TDPickupVisuals)
    da = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, PATH, unreal.TDPickupVisuals, f)
    out.append('created')
for field, base in (('hull', 'Hull'), ('shield', 'Shield'), ('power', 'Power'), ('overcharge', 'Overcharge')):
    look = da.get_editor_property(field)
    for part, sysname in (('loop', 'NS_TD_Pickup_' + base), ('collect', 'NS_TD_Pickup_' + base + '_Collect')):
        cue = look.get_editor_property(part)
        cue.set_editor_property('vfx', unreal.load_asset(SYS + sysname))
        look.set_editor_property(part, cue)
    da.set_editor_property(field, look)
for field in ('hull', 'shield', 'power', 'overcharge'):
    look = da.get_editor_property(field)
    for part in ('loop', 'collect'):
        cue = look.get_editor_property(part)
        out.append('%s.%s vfx=%s sound=%s' % (field, part, str(cue.get_editor_property('vfx')).split('.')[-1][:40], cue.get_editor_property('sound')))
out.append('saved' if EAL.save_loaded_asset(da, False) else 'SAVE FAILED')
"\n".join(out)
