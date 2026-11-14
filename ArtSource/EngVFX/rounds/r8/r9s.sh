#!/usr/bin/env bash
# r9s.sh <tag>: the cloud layer attached to the flown ship at 60 m/s on Verdance (teal) and Cinder (ash + embers), seen from the
# chase camera (PIE viewport capture), 4 frames each.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1
G=ArtSource/EngVFX/rounds/r8/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py ${TD_PORT:-21031} Saved/EngVFX/r8"; T=${1:-s}
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage VerdanceArrival" sleep:25 con:"TD.God" con:"TD.World.Hold 60 60" >/dev/null
bash $G "('attach_set', 'NS_TD_Entry_CloudWisps', (0, 0, 0), {'Intensity': 1.0, 'Embers': 0.0})" >/dev/null
$P sleep:1.5 cap:${T}_cloud_verd_1 sleep:0.5 cap:${T}_cloud_verd_2 >/dev/null; bash $G "('clear',)" >/dev/null
$P con:"TD.Stage CinderArrival" sleep:25 con:"TD.God" con:"TD.World.Hold 60 60" >/dev/null
cat > Saved/EngVFX/_cin.py <<'PY'
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
c = unreal.NiagaraFunctionLibrary.spawn_system_attached(unreal.load_asset('/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Entry_CloudWisps'), pawn.root_component, 'None', unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0), unreal.AttachLocation.KEEP_RELATIVE_OFFSET, True, True, unreal.NCPoolMethod.NONE, True)
c.component_tags = ['EngVFXPreview']
c.set_variable_linear_color('Color', unreal.LinearColor(0.45, 0.36, 0.30, 1.0)); c.set_variable_float('Embers', 1.0)
"cinder cloud"
PY
python ArtSource/EngVFX/scripts/run_editor_py.py ${TD_PORT:-21031} Saved/EngVFX/_cin.py
$P sleep:1.5 cap:${T}_cloud_cin_1 sleep:0.5 cap:${T}_cloud_cin_2 >/dev/null; bash $G "('clear',)" >/dev/null
bash $G "('pie_stop',)" >/dev/null
