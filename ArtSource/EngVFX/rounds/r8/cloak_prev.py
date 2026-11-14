# 10f: preview the cloak on the flown ship: CMD = 'on' sets M_TD_VFX_CloakOverlay as the overlay material on every mesh
# component of the pawn and attaches NS_TD_Cloak_Shimmer (tagged EngVFXPreview); 'off' removes both.
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
ov = None   # 10g: the Rift hull is Nanite (no overlay); the cloak is now NS_TD_Cloak_Shimmer's hull shell alone
n = 0
for m in pawn.get_components_by_class(unreal.MeshComponent):
    if isinstance(m, unreal.NiagaraComponent):
        continue
    m.set_overlay_material(ov); n += 1
for k in pawn.get_components_by_tag(unreal.NiagaraComponent, 'EngVFXPreview'):
    k.destroy_component(k)
if CMD == 'on':
    c = unreal.NiagaraFunctionLibrary.spawn_system_attached(unreal.load_asset('/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Cloak_Shimmer'), pawn.root_component, 'None',
        unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0), unreal.AttachLocation.KEEP_RELATIVE_OFFSET, True, True, unreal.NCPoolMethod.NONE, True)
    c.component_tags = ['EngVFXPreview']
'%s: overlay on %d mesh components' % (CMD, n)
