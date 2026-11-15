#!/usr/bin/env bash
# r12s.sh <tag>: one slot turn: (1) the Dominion turret lock telegraph from 400 m and 600 m (ship placed on the turret's
# line and held, frames every 0.4 s); (2) a multi-kill: 6 fighters and 2 interceptors killed at once with the new death pop.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; R=ArtSource/EngVFX/rounds/r8
T=${1:-ts}; unset NULLRHI; trap 'bash $R/ed.sh unlock' EXIT
bash $R/ed.sh lock "turret telegraph + multi-kill pop" || exit 1
echo "== turn $(date +%H:%M:%S)"; bash $R/ed.sh start >/dev/null || { echo "start refused"; exit 1; }
G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $TD_PORT Saved/EngVFX/r8"
F() { (echo "DIST = $1"; cat $R/face_turret.py) > Saved/EngVFX/_ft.py; python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_ft.py 2>&1 | tail -1 | cut -c1-200; }
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage DominionAssault" sleep:14 con:"god" con:"TD.Squad.Passive 1" >/dev/null
F 400; $P con:"TD.World.Hold 12" sleep:0.5 >/dev/null
for i in 01 02 03 04 05 06 07 08 09 10; do $P cap:${T}_t400_$i sleep:0.25 >/dev/null; done
F 600; $P con:"TD.World.Hold 12" sleep:0.5 >/dev/null
for i in 01 02 03 04 05 06 07 08 09 10; do $P cap:${T}_t600_$i sleep:0.25 >/dev/null; done
$P con:"TD.Stage Wave5" sleep:12 con:"god" con:"TD.KillAll" con:"TD.Squad.Passive 1" con:"TD.World.Hold 20" sleep:0.5 con:"TD.Spawn ShardFighter 6 9000" con:"TD.Spawn HookInterceptor 2 11000" sleep:2.5 cap:${T}_pre con:"TD.KillAll" sleep:0.12 cap:${T}_kill_1 sleep:0.25 cap:${T}_kill_2 sleep:0.4 cap:${T}_kill_3 >/dev/null
bash $G "('pie_stop',)" >/dev/null
bash $R/ed.sh unlock; echo "== closed $(date +%H:%M:%S)"
