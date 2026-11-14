#!/usr/bin/env bash
# r9k.sh <tag>: one batched PIE session: (1) breaks at 80 m (TestShot) and at 200 m (spawned row), (2) the reactor with T4's
# sequence (hold right after the stage, AimAt, fire 6 s; then release and fly in from 600 m firing), (3) the real Exposed path,
# (4) a hyperdrive jump with HyperdriveStreaks set.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1
G=ArtSource/EngVFX/rounds/r8/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py ${TD_PORT:-21031} Saved/EngVFX/r8"; T=${1:-w}; L=Saved/Logs/td-play-mcp_${TD_PORT:-21031}.log
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage Waves3to4" sleep:16 con:"TD.God" con:"TD.KillAll" sleep:2 >/dev/null
for c in 0 1; do $P con:"TD.KillAll" con:"TD.SpaceField.TestShot $c 14" sleep:0.1 cap:${T}_b80_${c}_a sleep:0.1 cap:${T}_b80_${c}_b sleep:2 >/dev/null; done
$P con:"TD.World.Hold 60 0" >/dev/null
bash $G "('row', ['NS_TD_Asteroid_Break_S', 'NS_TD_Asteroid_Break_M', 'NS_TD_Asteroid_Break_L'], 20000, 5000, 0, 0)" >/dev/null; $P sleep:0.08 cap:${T}_b200 >/dev/null; bash $G "('clear',)" >/dev/null
$P con:"TD.World.Hold 0" con:"td.Finale.Boarding 0" con:"TD.Stage ReactorExposed" con:"TD.World.Hold 60 0" sleep:16 con:"TD.World.Hold 60 0" con:"TD.World.AimAt Reactor 1" sleep:1.5 cap:${T}_r315_aim \
   con:"TD.Input.Key LeftMouseButton 6" sleep:1.0 cap:${T}_r315_1 sleep:1.0 cap:${T}_r315_2 sleep:4.5 con:"TD.World.DominionStatus" >/dev/null
grep -a "AimAt TDDominion\|reactor hull" $L | tail -3 | cut -c1-150
cat > Saved/EngVFX/_back.py <<'PY'
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
pawn.set_actor_location(pawn.get_actor_location() - pawn.get_actor_forward_vector() * 28500.0, False, True)
"moved back"
PY
python ArtSource/EngVFX/scripts/run_editor_py.py ${TD_PORT:-21031} Saved/EngVFX/_back.py >/dev/null
$P con:"TD.World.AimAt Reactor 1" sleep:1.2 con:"TD.World.Hold 0" con:"TD.Input.Key LeftMouseButton 6" sleep:2 cap:${T}_r600_1 sleep:2 cap:${T}_r600_2 sleep:2.5 con:"TD.World.DominionStatus" >/dev/null
grep -a "AimAt TDDominion\|reactor hull" $L | tail -2 | cut -c1-150
$P con:"td.Finale.Boarding 0" con:"TD.Stage DominionAssault" con:"TD.World.Hold 60 0" sleep:16 con:"TD.World.Hold 60 0" con:"td.Finale.Boarding 0" con:"TD.World.AimAt Reactor 1" sleep:1.5 \
   con:"TD.World.Dominion DropShields" sleep:0.15 cap:${T}_exp_1 sleep:0.5 cap:${T}_exp_2 sleep:1.2 cap:${T}_exp_3 >/dev/null
$P con:"TD.World.Hold 0" con:"TD.Stage Waves1to2" sleep:14 con:"TD.SkipWave" sleep:3 con:"TD.KillAll" sleep:3 con:"TD.SkipWave" sleep:3 con:"TD.KillAll" sleep:6 con:"TD.KillAll" \
   con:"TD.Input.Key H 1.6" sleep:0.4 con:"TD.KillAll" cap:${T}_hyp_1 sleep:1.2 con:"TD.KillAll" cap:${T}_hyp_2 sleep:2 cap:${T}_hyp_3 sleep:3 cap:${T}_hyp_4 >/dev/null
bash $G "('pie_stop',)" >/dev/null
