#!/usr/bin/env bash
# r12u.sh <tag>: one slot turn: the Dominion breakup in the escape flight, seen from the ship. TD.Stage EscapeFlight, god;
# the ship turned 180 to look back at the burning hull (hull-section bursts) for ~10 s; then turned forward and boosted to
# the exit radius, turned back again for the final blast.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; R=ArtSource/EngVFX/rounds/r8
T=${1:-bu}; unset NULLRHI; trap 'bash $R/ed.sh unlock' EXIT
bash $R/ed.sh lock "escape-flight breakup look-back" || exit 1
echo "== turn $(date +%H:%M:%S)"; bash $R/ed.sh start >/dev/null || { echo "start refused"; exit 1; }
G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $TD_PORT Saved/EngVFX/r8"
Y() { (echo "YAW = $1"; cat $R/look_yaw.py) > Saved/EngVFX/_ly.py; python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_ly.py 2>&1 | tail -1 | cut -c1-150; }
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage EscapeFlight" sleep:10 con:"god" >/dev/null
Y 180; $P sleep:0.6 cap:${T}_back_1 sleep:1.2 cap:${T}_back_2 sleep:1.2 cap:${T}_back_3 sleep:1.2 cap:${T}_back_4 sleep:1.2 cap:${T}_back_5 sleep:1.2 cap:${T}_back_6 >/dev/null
Y 180; $P con:"TD.Input.Keys W+LeftShift 14.0" sleep:14.5 >/dev/null; Y 0
$P cap:${T}_out_1 sleep:1 cap:${T}_out_2 >/dev/null
Y 180; $P sleep:0.4 cap:${T}_far_1 sleep:1 cap:${T}_far_2 sleep:1 cap:${T}_far_3 >/dev/null
Y 180; $P con:"TD.Input.Keys W+LeftShift 18.0" sleep:18.5 >/dev/null; Y 180
$P cap:${T}_end_1 sleep:0.6 cap:${T}_end_2 sleep:0.6 cap:${T}_end_3 sleep:1 cap:${T}_end_4 sleep:1.5 cap:${T}_end_5 >/dev/null
grep -h "escape flight\|Breakup\|ClearSpace\|FinalBlast" Saved/Logs/td-play-mcp_${TD_PORT}.log 2>/dev/null | grep -v "Cmd:" | tail -4 | cut -c1-200
bash $G "('pie_stop',)" >/dev/null
bash $R/ed.sh unlock; echo "== closed $(date +%H:%M:%S)"
