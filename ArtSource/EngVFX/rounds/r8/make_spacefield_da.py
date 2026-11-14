# Round 9: DA_SpaceField_FX (T6's UTDSpaceFieldFX) in the engineering fallback folder (LoadPresentation looks in
# Presentation first, then Gameplay/Characters/FX; Presentation is Nick's). VFX only, VFXScale 1: each system carries its
# size class in its own User.Scale default (S 1, M 2.2, L 4.5, Crystal 2.2).
PATH = '/Game/TriadDrift/Gameplay/Characters/FX'
NAME = 'DA_SpaceField_FX'
SYS = '/Game/TriadDrift/Gameplay/VFX/Systems/'
EAL = unreal.EditorAssetLibrary
out = []
if EAL.does_asset_exist(PATH + '/' + NAME):
    da = unreal.load_asset(PATH + '/' + NAME); out.append('exists')
else:
    f = unreal.DataAssetFactory(); f.set_editor_property('data_asset_class', unreal.TDSpaceFieldFX)
    da = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, PATH, unreal.TDSpaceFieldFX, f); out.append('created')
for field, sysname in (('break_small', 'NS_TD_Asteroid_Break_S'), ('break_medium', 'NS_TD_Asteroid_Break_M'),
                       ('break_large', 'NS_TD_Asteroid_Break_L'), ('break_special', 'NS_TD_Asteroid_Break_Crystal')):
    cue = da.get_editor_property(field)
    cue.set_editor_property('vfx', unreal.load_asset(SYS + sysname))
    da.set_editor_property(field, cue)
    c = da.get_editor_property(field)
    out.append('%s vfx=%s sound=%s' % (field, str(c.get_editor_property('vfx')).split("'")[1].split('.')[-1], c.get_editor_property('sound')))
out.append('saved' if EAL.save_loaded_asset(da, False) else 'SAVE FAILED')
"\n".join(out)
