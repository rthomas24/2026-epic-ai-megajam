# Batch M data edits (Lead): VFX fields, plus the one Sound restore the Lead asked for (DescentRoar, dropped in a merge).
CH = '/Game/TriadDrift/Gameplay/Characters/FX/'
SYS = '/Game/TriadDrift/Gameplay/VFX/Systems/'
HYP = '/Game/TriadDrift/Gameplay/VFX/Hyperdrive/'
EAL = unreal.EditorAssetLibrary
out = []
def setvfx(da, field, path, scale=None):
    c = da.get_editor_property(field)
    s = unreal.load_asset(path)
    if not s:
        out.append('NO ASSET ' + path); return
    c.set_editor_property('vfx', s)
    if scale is not None:
        c.set_editor_property('vfx_scale', unreal.Vector(scale, scale, scale))
    da.set_editor_property(field, c)
    out.append('%s.%s = %s' % (da.get_name(), field, path.split('/')[-1]))
# travel
t = unreal.load_asset(CH + 'DA_Travel_FX')
streaks = HYP + 'NS_TD_Hyperdrive_Streaks' if EAL.does_asset_exist(HYP + 'NS_TD_Hyperdrive_Streaks') else SYS + 'NS_TD_Hyperdrive_Streaks'
setvfx(t, 'hyperdrive_streaks', streaks)
c = t.get_editor_property('descent_roar')
c.set_editor_property('sound', unreal.load_asset('/Game/TriadDrift/Gameplay/Audio/Travel/SW_Travel_DescentRoar_01'))
t.set_editor_property('descent_roar', c)
out.append('DA_Travel_FX.descent_roar.sound = %s' % t.get_editor_property('descent_roar').get_editor_property('sound'))
out.append('saved travel' if EAL.save_loaded_asset(t, False) else 'SAVE FAILED travel')
# crew blaster
if EAL.does_asset_exist(CH + 'DA_CrewBlaster_FX'):
    w = unreal.load_asset(CH + 'DA_CrewBlaster_FX')
else:
    f = unreal.DataAssetFactory(); f.set_editor_property('data_asset_class', unreal.TDWeaponFX)
    w = unreal.AssetToolsHelpers.get_asset_tools().create_asset('DA_CrewBlaster_FX', CH[:-1], unreal.TDWeaponFX, f)
w.set_editor_property('tracer', unreal.load_asset(SYS + 'NS_TD_Crew_Bolt'))
out.append('DA_CrewBlaster_FX.tracer = NS_TD_Crew_Bolt')
for f_ in ('impact_hull', 'impact_world', 'impact_shield'):
    setvfx(w, f_, SYS + 'NS_TD_Crew_BoltImpact')
out.append('saved crew' if EAL.save_loaded_asset(w, False) else 'SAVE FAILED crew')
# missiles
for name, sysname in (('DA_SpikeRockets_FX', 'NS_TD_Missile_Trail'), ('DA_MissileSwarm_FX', 'NS_TD_Missile_Trail'),
                      ('DA_Enemy_HomingMissile_FX', 'NS_TD_Missile_Trail_Enemy'), ('DA_Enemy_SalvoMissile_FX', 'NS_TD_Missile_Trail_Enemy')):
    d = unreal.load_asset(CH + name)
    d.set_editor_property('tracer', unreal.load_asset(SYS + sysname))
    out.append('%s.tracer = %s, %s' % (name, sysname, 'saved' if EAL.save_loaded_asset(d, False) else 'SAVE FAILED'))
"\n".join(out)
