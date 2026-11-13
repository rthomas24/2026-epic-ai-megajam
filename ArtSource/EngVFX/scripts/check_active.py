# Triad Drift ENG-37: spawns every NS_TD_* system once in the running Play-In-Editor world and reports any that does
# not activate (a system can compile clean and still refuse to run). Run with PIE on:
#   python ArtSource/EngVFX/scripts/run_editor_py.py 8017 ArtSource/EngVFX/scripts/check_active.py
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
bad, n = [], 0
for p in sorted(unreal.EditorAssetLibrary.list_assets('/Game/TriadDrift/Gameplay/VFX/Systems')):
    s = unreal.load_asset(p)
    if not isinstance(s, unreal.NiagaraSystem):
        continue
    c = unreal.NiagaraFunctionLibrary.spawn_system_at_location(world, s, pawn.get_actor_location() + unreal.Vector(0, 0, 60000), unreal.Rotator(0, 0, 0),
                                                              unreal.Vector(1, 1, 1), True, True, unreal.NCPoolMethod.NONE, True)
    n += 1
    if not (c and c.is_active()):
        bad.append(s.get_name())
    if c:
        c.destroy_component(c)
'%d systems spawned, not active: %s' % (n, bad or 'none')
