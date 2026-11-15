# 10k: the bomber's plasma mine look through data: DA_Enemy_PlasmaMines_FX.Tracer = NS_TD_Mine_Plasma (ProjectileMesh stays
# empty, so TDProjectile hides the placeholder disc). Sound fields untouched (checked by sound_check.py).
da = unreal.load_asset('/Game/TriadDrift/Gameplay/Characters/FX/DA_Enemy_PlasmaMines_FX')
out = []
for p in ('tracer', 'projectile_mesh'):
    try: out.append('%s before=%s' % (p, da.get_editor_property(p)))
    except Exception as ex: out.append('%s ? %s' % (p, ex))
da.set_editor_property('tracer', unreal.load_asset('/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Mine_Plasma2'))
out.append('tracer after=%s' % da.get_editor_property('tracer'))
out.append('saved' if unreal.EditorAssetLibrary.save_loaded_asset(da, False) else 'SAVE FAILED')
"\n".join(out)
