#!/usr/bin/env bash
# r12p.sh <tag>: one slot turn, unpaused: the plasma mine v2 through DA_Enemy_PlasmaMines_FX.Tracer. (a) TD.Dev.SpawnMine 60
# and 150 under slomo 0.05 (the ship nearly still, Niagara still ticking); (b) bombers laying mines in Wave5 at normal speed.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; R=ArtSource/EngVFX/rounds/r8
T=${1:-mq}; unset NULLRHI; trap 'bash $R/ed.sh unlock' EXIT
bash $R/ed.sh lock "plasma mine v2 capture" || exit 1
echo "== turn $(date +%H:%M:%S)"; bash $R/ed.sh start >/dev/null || exit 1
G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $TD_PORT Saved/EngVFX/r8"
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage Wave5" sleep:12 con:"god" con:"TD.KillAll" con:"TD.Squad.Passive 1" con:"TD.World.Hold 900 0" sleep:1 >/dev/null
$P con:"slomo 0.05" con:"TD.Dev.SpawnMine 60" sleep:1.5 cap:${T}_60 sleep:0.8 cap:${T}_60b con:"TD.Dev.SpawnMine 150" sleep:1.5 cap:${T}_150 sleep:0.8 cap:${T}_150b con:"slomo 1" >/dev/null
python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT $R/mine_probe.py 2>&1 | tail -3 | cut -c1-500
$P con:"TD.World.SpawnEnemy MaulBomber 12000 1500 0" con:"TD.World.SpawnEnemy MaulBomber 16000 -2000 800" sleep:4 cap:${T}_lay_1 sleep:2 cap:${T}_lay_2 sleep:2 cap:${T}_lay_3 sleep:2 cap:${T}_lay_4 >/dev/null
bash $G "('pie_stop',)" >/dev/null
bash $R/ed.sh unlock; echo "== closed $(date +%H:%M:%S)"
