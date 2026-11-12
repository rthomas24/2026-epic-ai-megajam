"""Run through ue_run_python after native audio build, in the private host."""
path='/Game/TriadDrift/Audio/PrototypeV1/L_AudioPrototypeV1'
world=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).new_level(path)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('default_game_mode',unreal.TDAudioV1GameMode.static_class())
unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(unreal.PlayerStart,unreal.Vector(0,0,0))
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.EditorAssetLibrary.save_directory('/Game/TriadDrift/Audio/PrototypeV1',only_if_is_dirty=True,recursive=True)
{'map':path,'game_mode':str(world.get_world_settings().get_editor_property('default_game_mode')),'errors':[]}
