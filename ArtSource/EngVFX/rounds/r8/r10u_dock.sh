#!/usr/bin/env bash
# r10u_dock.sh <tag>: short slot turn: the tractor beam in a real dock (TD.Stage DominionDocking, then TD.Travel.ToDock
# 350 m out on the bay axis so the near-field capture starts the beam), chase-camera frames over about 5 s.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; R=ArtSource/EngVFX/rounds/r8
T=${1:-u}; unset NULLRHI; trap 'bash $R/ed.sh unlock' EXIT
bash $R/ed.sh lock "dock capture" || exit 1
echo "== dock turn $(date +%H:%M:%S)"; bash $R/ed.sh start >/dev/null || exit 1
G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $TD_PORT Saved/EngVFX/r8"
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage DominionDocking" sleep:20 con:"god" con:"TD.Travel.ToDock 350 0 1" sleep:1.5 cap:${T}_dock_1 sleep:1.2 cap:${T}_dock_2 sleep:1.2 cap:${T}_dock_3 sleep:1.2 cap:${T}_dock_4 >/dev/null
grep -h "TD.Travel.ToDock\|TractorBeam\|Docking" Saved/Logs/td-play-mcp_${TD_PORT}.log 2>/dev/null | tail -4 | cut -c1-200
bash $G "('pie_stop',)" >/dev/null
bash $R/ed.sh unlock; echo "== dock closed $(date +%H:%M:%S)"
