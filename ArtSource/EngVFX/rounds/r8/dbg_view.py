# 10d: where do spawned review systems land on screen? Prints the pawn forward, the camera, and each preview component's
# screen position (project_world_to_screen) and active state.
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc = unreal.GameplayStatics.get_player_controller(world, 0)
pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
cm = unreal.GameplayStatics.get_player_camera_manager(world, 0)
out = ['pawn %s fwd %s' % (pawn.get_actor_location(), pawn.get_actor_forward_vector()),
       'cam %s rot %s fov %.1f' % (cm.get_camera_location(), cm.get_camera_rotation(), cm.get_fov_angle())]
for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
    for k in a.get_components_by_tag(unreal.NiagaraComponent, 'EngVFXPreview'):
        sp = unreal.GameplayStatics.project_world_to_screen(pc, k.get_world_location(), False); ok = sp is not None
        out.append('%s active=%s vis=%s loc=%s screen=%s %s' % (k.get_asset().get_name(), k.is_active(), k.is_visible(), k.get_world_location(), ok, sp))
"\n".join(out)
