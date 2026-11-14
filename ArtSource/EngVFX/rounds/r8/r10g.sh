#!/usr/bin/env bash
# r10g.sh <tag>: round 10d capture: (1) debug row - beacon fixed / beacon from LODDistance / region beacon / haze on the field
# material / haze base / haze review instance, 300 m ahead, 60 m apart, chase camera; (2) magma at the environment builder's
# s13 spots (5 m, 20 m), the effect OFF then ON. Stop: touch Saved/EngVFX/STOP
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; unset NULLRHI
R=ArtSource/EngVFX/rounds/r8; G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $TD_PORT Saved/EngVFX/r8"; T=${1:-y}
echo "== start $(date +%H:%M)"; bash $R/ed.sh start || exit 1
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage Waves1to2" sleep:12 con:"god" con:"TD.KillAll" sleep:2 con:"TD.World.Hold 900 0" >/dev/null
bash $G "('row', ['NS_TD_Debug_BeaconFixed', 'NS_TD_Debug_BeaconLOD', 'NS_TD_Region_Beacon', 'NS_TD_Debug_HazeField', 'NS_TD_Debug_HazeBase', 'NS_TD_Debug_HazeSphere'], 30000, 6000)" | tail -6
$P sleep:1 cap:${T}_dbg_300m >/dev/null; bash $G "('clear',)" >/dev/null
bash $G "('row', ['NS_TD_Debug_BeaconFixed', 'NS_TD_Debug_BeaconLOD', 'NS_TD_Region_Beacon'], 150000, 20000)" >/dev/null
$P sleep:1 cap:${T}_dbg_1500m >/dev/null; bash $G "('clear',)" >/dev/null
bash $G "('pie_stop',)" >/dev/null; sleep 5
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage CinderOnFoot" sleep:14 con:"god" con:"TD.KillAll" >/dev/null
M() { (echo "CMD = '$1'"; cat $R/magma2.py) > Saved/EngVFX/_mag.py; python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_mag.py 2>&1 | tail -1 | cut -c1-200; }
for spot in "5m 4300 4300 0 -12" "20m 2800 4300 0 -5"; do set -- $spot
  $P con:"TD.Ground.Teleport $2 $3 250" sleep:1 con:"TD.Look $4 $5" sleep:3 con:"TD.Look $4 $5" sleep:1 >/dev/null
  M off; $P sleep:1 cap:${T}_magma_$1_off >/dev/null
  M on;  $P sleep:3 cap:${T}_magma_$1_on_a sleep:0.5 cap:${T}_magma_$1_on_b >/dev/null
done
M off
bash $G "('pie_stop',)" >/dev/null
bash $R/ed.sh stop >/dev/null; echo "== closed $(date +%H:%M)"
