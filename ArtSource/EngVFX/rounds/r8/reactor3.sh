#!/usr/bin/env bash
# reactor3.sh <tag>: T4's route with god mode confirmed: the reactor at 149 m (TD.Stage ReactorExposed), real fire for 6 s,
# then the ship moved back to about 600 m and fire again; then the real ReactorExposed path (DominionAssault + DropShields).
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1
G=ArtSource/EngVFX/rounds/r8/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py ${TD_PORT:-21031} Saved/EngVFX/r8"; T=${1:-r}; L=Saved/Logs/td-play-mcp_${TD_PORT:-21031}.log
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"td.Finale.Boarding 0" con:"TD.Stage ReactorExposed" sleep:18 con:"TD.God" sleep:0.5 >/dev/null
grep -a -i "god" $L | tail -2 | cut -c1-160
grep -a -i "god" $L | tail -1 | grep -qi "off\|disabled\|false" && { $P con:"TD.God" >/dev/null; echo "god toggled again"; grep -a -i "god" $L | tail -1 | cut -c1-160; }
$P con:"TD.World.AimAt Reactor 1" sleep:1.5 cap:${T}_149_aim con:"TD.Input.Key LeftMouseButton 6" sleep:0.8 cap:${T}_149_fire_1 sleep:1.2 cap:${T}_149_fire_2 sleep:1.0 cap:${T}_149_fire_3 sleep:3.5 con:"TD.World.DominionStatus" >/dev/null
cat > Saved/EngVFX/_back.py <<'PY'
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
pawn.set_actor_location(pawn.get_actor_location() - pawn.get_actor_forward_vector() * 45000.0, False, True)
"moved back"
PY
python ArtSource/EngVFX/scripts/run_editor_py.py ${TD_PORT:-21031} Saved/EngVFX/_back.py
$P con:"TD.World.Hold 20 0" con:"TD.World.AimAt Reactor 1" sleep:1.5 con:"TD.Input.Key LeftMouseButton 6" sleep:2.0 cap:${T}_600_fire_1 sleep:1.5 cap:${T}_600_fire_2 sleep:3 con:"TD.World.DominionStatus" >/dev/null
grep -a "DominionStatus:\|reactor hull" $L | tail -4 | cut -c1-160
# the real exposed path
$P con:"td.Finale.Boarding 0" con:"TD.Stage DominionAssault" sleep:18 con:"TD.God" con:"td.Finale.Boarding 0" con:"TD.World.AimAt Reactor 1" sleep:2 con:"TD.World.Dominion DropShields" sleep:0.3 cap:${T}_exp_1 sleep:0.8 cap:${T}_exp_2 sleep:1.5 cap:${T}_exp_3 >/dev/null
bash $G "('pie_stop',)" >/dev/null
