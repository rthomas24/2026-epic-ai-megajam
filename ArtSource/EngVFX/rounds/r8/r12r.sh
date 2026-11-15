#!/usr/bin/env bash
# r12r.sh <tag>: one slot turn (materials need a renderer): build M_TD_VFX_Dot and the mine v2 crisp core, then in PIE
# (1) mines at 60 / 150 m under slomo 0.05; (2) the Dominion assault held still, a burst of frames for the turret lock telegraph.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; R=ArtSource/EngVFX/rounds/r8
T=${1:-mr}; unset NULLRHI; trap 'bash $R/ed.sh unlock' EXIT
bash $R/ed.sh lock "mine crisp core + turret telegraph" || exit 1
echo "== turn $(date +%H:%M:%S)"; bash $R/ed.sh start >/dev/null || { echo "start refused"; exit 1; }
(echo "ONLY_MATS = ['M_TD_VFX_Dot']"; cat ArtSource/EngVFX/scripts/build_materials.py) > Saved/EngVFX/_mats.py
python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_mats.py 2>&1 | tail -1 | cut -c1-200
(echo "ONLY = ['__none__']"; cat ArtSource/EngVFX/scripts/build_tier1.py; echo; echo "ONLY2 = ['=NS_TD_Mine_Plasma2']"; cat ArtSource/EngVFX/scripts/build_tier2.py) > Saved/EngVFX/_amb.py
python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_amb.py 2>&1 | grep "NS_TD_\|FAIL\|rror" | cut -c1-300
G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $TD_PORT Saved/EngVFX/r8"
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage Wave5" sleep:12 con:"god" con:"TD.KillAll" con:"TD.Squad.Passive 1" con:"TD.World.Hold 900 0" sleep:1 >/dev/null
$P con:"slomo 0.05" con:"TD.Dev.SpawnMine 60" sleep:1.5 cap:${T}_mine60 con:"TD.Dev.SpawnMine 150" sleep:1.5 cap:${T}_mine150 con:"slomo 1" >/dev/null
$P con:"TD.Stage DominionAssault" sleep:14 con:"god" con:"TD.Squad.Passive 1" con:"TD.World.Hold 10" sleep:2 cap:${T}_t01 sleep:0.5 cap:${T}_t02 sleep:0.5 cap:${T}_t03 sleep:0.5 cap:${T}_t04 sleep:0.5 cap:${T}_t05 sleep:0.5 cap:${T}_t06 sleep:0.5 cap:${T}_t07 sleep:0.5 cap:${T}_t08 >/dev/null
grep -h "Turret\|Telegraph" Saved/Logs/td-play-mcp_${TD_PORT}.log 2>/dev/null | grep -v "Cmd:" | tail -4 | cut -c1-200
bash $G "('pie_stop',)" >/dev/null
bash $R/ed.sh unlock; echo "== closed $(date +%H:%M:%S)"
