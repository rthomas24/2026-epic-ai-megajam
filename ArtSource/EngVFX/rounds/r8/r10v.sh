#!/usr/bin/env bash
# r10v.sh <tag>: one short slot turn: (1) the docking sequence itself (TD.Stage DominionDocking plays the 8 s docking story
# sequence: frames every second from its start, when the tractor beam runs); (2) mines: wingmen passive, two Maul bombers
# 150-200 m ahead, frames after they drop.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; R=ArtSource/EngVFX/rounds/r8
T=${1:-v}; unset NULLRHI; trap 'bash $R/ed.sh unlock' EXIT
bash $R/ed.sh lock "dock sequence + mines capture" || exit 1
echo "== turn $(date +%H:%M:%S)"; bash $R/ed.sh start >/dev/null || exit 1
G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $TD_PORT Saved/EngVFX/r8"
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage Wave5" sleep:12 con:"god" con:"TD.KillAll" con:"TD.Squad.Passive 1" sleep:1 con:"TD.World.Hold 900 0" con:"TD.World.SpawnEnemy MaulBomber 15000 2000 0" con:"TD.World.SpawnEnemy MaulBomber 20000 -2500 1000" sleep:6 cap:${T}_mines_1 sleep:2 cap:${T}_mines_2 sleep:2 cap:${T}_mines_3 >/dev/null
$P con:"TD.Stage DominionDocking" sleep:1 cap:${T}_dock_1 sleep:1 cap:${T}_dock_2 sleep:1 cap:${T}_dock_3 sleep:1 cap:${T}_dock_4 sleep:1 cap:${T}_dock_5 sleep:1 cap:${T}_dock_6 sleep:1.5 cap:${T}_dock_7 >/dev/null
bash $G "('pie_stop',)" >/dev/null
bash $R/ed.sh unlock; echo "== closed $(date +%H:%M:%S)"
