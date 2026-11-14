# 10m: point the two data slots at the unseen effects IN MEMORY ONLY (not saved) so a PIE capture shows them through the real
# game code: DA_Dominion_Visuals.TractorBeam = NS_TD_Dominion_TractorBeam2, DA_Enemy_PlasmaMines_FX.Tracer = NS_TD_Mine_Plasma.
S = '/Game/TriadDrift/Gameplay/VFX/Systems/'
d = unreal.load_asset('/Game/TriadDrift/Gameplay/Characters/Dominion/DA_Dominion_Visuals')
c = d.get_editor_property('tractor_beam'); c.set_editor_property('vfx', unreal.load_asset(S + 'NS_TD_Dominion_TractorBeam2')); d.set_editor_property('tractor_beam', c)
m = unreal.load_asset('/Game/TriadDrift/Gameplay/Characters/FX/DA_Enemy_PlasmaMines_FX')
m.set_editor_property('tracer', unreal.load_asset(S + 'NS_TD_Mine_Plasma'))
'preview set (not saved): %s / %s' % (d.get_editor_property('tractor_beam').get_editor_property('vfx').get_name(), m.get_editor_property('tracer').get_name())
