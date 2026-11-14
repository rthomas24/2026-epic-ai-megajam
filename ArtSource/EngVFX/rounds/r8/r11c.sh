#!/usr/bin/env bash
# r11b.sh <tag>: one short slot turn on main 8d62d14a: (1) the real cloak (unlock, E) with T2's hull swap and my shell
# material, chase and side; (2) TractorBeam2 (in memory) through the real docking with T4's fade; (3) the plasma mine body
# (tracer in memory) with T4's TD.Dev.SpawnMine at 30 / 60 / 300 m.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; R=ArtSource/EngVFX/rounds/r8
T=${1:-r}; unset NULLRHI; trap 'bash $R/ed.sh unlock' EXIT
bash $R/ed.sh lock "cloak + tractor v2 + mine capture" || exit 1
echo "== turn $(date +%H:%M:%S)"; bash $R/ed.sh start >/dev/null || exit 1
: saved data, no preview
G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $TD_PORT Saved/EngVFX/r8"
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage Waves1to2" sleep:12 con:"god" con:"TD.KillAll" con:"TD.Squad.Passive 1" sleep:1 con:"TD.World.Hold 900 0" >/dev/null
for d in 30 60 300; do $P con:"TD.Dev.SpawnMine $d" sleep:1 cap:${T}_mine_${d}m_a sleep:0.4 cap:${T}_mine_${d}m_b >/dev/null; done
$P con:"TD.Stage DominionDocking" sleep:1 cap:${T}_dock_1 sleep:1 cap:${T}_dock_2 sleep:1 cap:${T}_dock_3 sleep:1 cap:${T}_dock_4 sleep:1 cap:${T}_dock_5 sleep:1 cap:${T}_dock_6 >/dev/null
bash $G "('pie_stop',)" >/dev/null
bash $R/ed.sh unlock; echo "== closed $(date +%H:%M:%S)"
