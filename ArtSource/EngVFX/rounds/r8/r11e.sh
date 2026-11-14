#!/usr/bin/env bash
# r11e.sh <tag>: short slot turn: the turret lock telegraph in a real Dominion assault, ship 150 m off a hull turret, ~6
# frames per second over 6 s from the chase camera.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; R=ArtSource/EngVFX/rounds/r8
T=${1:-tt}; unset NULLRHI; trap 'bash $R/ed.sh unlock' EXIT
bash $R/ed.sh lock "turret telegraph capture" || exit 1
echo "== turn $(date +%H:%M:%S)"; bash $R/ed.sh start >/dev/null || exit 1
G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $TD_PORT Saved/EngVFX/r8"
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"td.Finale.Boarding 0" con:"TD.Stage DominionAssault" sleep:14 con:"god" con:"TD.Squad.Passive 1" sleep:1 >/dev/null
python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT $R/turret_place.py 2>&1 | tail -1 | cut -c1-200
$P con:"TD.World.Hold 900 0" sleep:1 >/dev/null
caps=""; for i in $(seq 1 24); do caps="$caps cap:${T}_t$(printf %02d $i)"; done
$P $caps >/dev/null
bash $G "('pie_stop',)" >/dev/null
bash $R/ed.sh unlock; echo "== closed $(date +%H:%M:%S)"
