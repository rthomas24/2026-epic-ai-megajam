# Hyperdrive look (T7): start or stop Play-In-Editor in the editor session. CMD = 'start' | 'stop' (prepended by the caller).
CMD = globals().get('CMD', 'start')
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if CMD == 'start':
    les.editor_request_begin_play()
else:
    les.editor_request_end_play()
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
'PIE %s requested; game world now: %s' % (CMD, world.get_name() if world else None)
