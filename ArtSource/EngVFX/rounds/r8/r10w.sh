#!/usr/bin/env bash
# r10w.sh <tag>: one short slot turn: the unseen tractor v2 and plasma mine set in memory (not saved), then (1) the
# docking sequence frames (UpdateBeam drives Length / Intensity), (2) a wave with Maul bombers inside their 70 m mine range
# of the held ship (TickBomber: AttackRun when within Heavy.Range of its route point), frames every 2 s for 16 s.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; R=ArtSource/EngVFX/rounds/r8
T=${1:-w}; unset NULLRHI; trap 'bash $R/ed.sh unlock' EXIT
bash $R/ed.sh lock "tractor v2 + mines capture" || exit 1
echo "== turn $(date +%H:%M:%S)"; bash $R/ed.sh start >/dev/null || exit 1
python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT $R/preview_data.py 2>&1 | tail -1 | cut -c1-200
G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $TD_PORT Saved/EngVFX/r8"
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage DominionDocking" sleep:1 cap:${T}_dock_1 sleep:1 cap:${T}_dock_2 sleep:1 cap:${T}_dock_3 sleep:1 cap:${T}_dock_4 sleep:1 cap:${T}_dock_5 sleep:1 cap:${T}_dock_6 >/dev/null
$P con:"TD.Stage Wave5" sleep:12 con:"god" con:"TD.KillAll" con:"TD.Squad.Passive 1" sleep:1 con:"TD.World.Hold 900 0" con:"TD.World.SpawnEnemy MaulBomber 12000 1500 0" con:"TD.World.SpawnEnemy MaulBomber 16000 -2000 800" sleep:4 cap:${T}_mines_1 sleep:2 cap:${T}_mines_2 sleep:2 cap:${T}_mines_3 sleep:2 cap:${T}_mines_4 sleep:2 cap:${T}_mines_5 sleep:2 cap:${T}_mines_6 >/dev/null
grep -h "Mine\|MineTrail\|PlasmaMine" Saved/Logs/td-play-mcp_${TD_PORT}.log 2>/dev/null | tail -3 | cut -c1-200
bash $G "('pie_stop',)" >/dev/null
bash $R/ed.sh unlock; echo "== closed $(date +%H:%M:%S)"
