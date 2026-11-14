# Next batch (T2 0d767e62 drone playback, T3 c9c9c67d story FX): run in one short editor session once merged and built.
#   DA_Crew_Drone_Visuals.EngineTrail.VFX = NS_TD_Drone_Thruster (Sound left empty; the code writes User.Throttle 0.2..1)
#   DA_StoryFX (UTDStoryFX, new, Gameplay/Characters/FX): TransmissionStatic.VFX = NS_TD_Glitch_Tear
SYS = '/Game/TriadDrift/Gameplay/VFX/Systems/'
EAL = unreal.EditorAssetLibrary
out = []
d = unreal.load_asset('/Game/TriadDrift/Gameplay/Characters/NullDrone/DA_Crew_Drone_Visuals')
c = d.get_editor_property('engine_trail'); c.set_editor_property('vfx', unreal.load_asset(SYS + 'NS_TD_Drone_Thruster')); d.set_editor_property('engine_trail', c)
out.append('drone engine_trail = NS_TD_Drone_Thruster, sound=%s, %s' % (d.get_editor_property('engine_trail').get_editor_property('sound'), 'saved' if EAL.save_loaded_asset(d, False) else 'SAVE FAILED'))
CLS = getattr(unreal, 'TDStoryFX', None)
if CLS is None:
    out.append('NO CLASS TDStoryFX: merge T3 first')
else:
    P = '/Game/TriadDrift/Gameplay/Characters/FX'
    if EAL.does_asset_exist(P + '/DA_StoryFX'):
        s = unreal.load_asset(P + '/DA_StoryFX')
    else:
        f = unreal.DataAssetFactory(); f.set_editor_property('data_asset_class', CLS)
        s = unreal.AssetToolsHelpers.get_asset_tools().create_asset('DA_StoryFX', P, CLS, f)
    c = s.get_editor_property('transmission_static'); c.set_editor_property('vfx', unreal.load_asset(SYS + 'NS_TD_Glitch_Tear')); s.set_editor_property('transmission_static', c)
    out.append('DA_StoryFX transmission_static = NS_TD_Glitch_Tear, %s' % ('saved' if EAL.save_loaded_asset(s, False) else 'SAVE FAILED'))
"\n".join(out)
