# DA_ReactorCore_FX values (data only; run when T5 names the slots). Edit FIELDS to T5's names if they differ, then:
#   python ArtSource/EngVFX/scripts/run_editor_py.py <port> ArtSource/EngVFX/rounds/r8/data_core.py
PATH = '/Game/TriadDrift/Gameplay/Characters/FX'
NAME = 'DA_ReactorCore_FX'
SYS = '/Game/TriadDrift/Gameplay/VFX/Systems/'
CLASS = getattr(unreal, 'TDReactorCoreFX', None)
FIELDS = {'core_idle': 'NS_TD_Core_Orb', 'conduit_break': 'NS_TD_Core_ConduitBreak', 'shield_drop': 'NS_TD_Core_ShieldDrop',
          'laser_sweep': 'NS_TD_Core_LaserSweep', 'heart_hit': 'NS_TD_Core_HeartHit', 'core_destroyed': 'NS_TD_Core_Destroyed',
          'escape_run_loop': 'NS_TD_Escape_RunLoop', 'escape_flight_loop': 'NS_TD_Escape_FlightLoop'}
EAL = unreal.EditorAssetLibrary
out = []
if CLASS is None:
    out.append('NO CLASS TDReactorCoreFX in this build: merge T5 first')
else:
    if EAL.does_asset_exist(PATH + '/' + NAME):
        da = unreal.load_asset(PATH + '/' + NAME)
    else:
        f = unreal.DataAssetFactory(); f.set_editor_property('data_asset_class', CLASS)
        da = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, PATH, CLASS, f)
    for field, sysname in FIELDS.items():
        try:
            c = da.get_editor_property(field)
        except Exception:
            out.append('NO FIELD ' + field); continue
        c.set_editor_property('vfx', unreal.load_asset(SYS + sysname))   # VFX only; Sound untouched
        da.set_editor_property(field, c)
        out.append('%s = %s' % (field, sysname))
    out.append('saved' if EAL.save_loaded_asset(da, False) else 'SAVE FAILED')
"\n".join(out)
