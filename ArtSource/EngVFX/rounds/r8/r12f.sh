#!/usr/bin/env bash
# r12f.sh <tag>: one slot turn: (1) the tractor beam v2 in a real docking pull (TD.Stage DominionDocking, TD.Travel.ToDock,
# td.Docking.BeamLog 1), (2) the plasma mine body at flight distance (TD.Dev.SpawnMine 40 / 120), (3) the pad beacon on foot at 30 / 120 / 300 m.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; R=ArtSource/EngVFX/rounds/r8
T=${1:-tm}; unset NULLRHI; trap 'bash $R/ed.sh unlock' EXIT
bash $R/ed.sh lock "tractor v2 docking, mine body, pad beacon" || exit 1
echo "== turn $(date +%H:%M:%S)"; bash $R/ed.sh start >/dev/null || exit 1
G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $TD_PORT Saved/EngVFX/r8"
A() { (echo "CMD = $1"; cat $R/amb_show.py) > Saved/EngVFX/_amb_show.py; python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_amb_show.py 2>&1 | tail -4 | tr '\n' ' ' | cut -c1-300; echo; }
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage DominionDocking" sleep:20 con:"god" con:"td.Docking.BeamLog 1" con:"TD.Travel.ToDock 350 0 1" sleep:1.5 cap:${T}_dock_1 sleep:1.2 cap:${T}_dock_2 sleep:1.2 cap:${T}_dock_3 sleep:1.2 cap:${T}_dock_4 sleep:1.2 cap:${T}_dock_5 >/dev/null
grep -h "BeamLog\|Beam fx\|TractorBeam\|beam" Saved/Logs/td-play-mcp_${TD_PORT}.log 2>/dev/null | tail -3 | cut -c1-220
$P con:"td.Docking.BeamLog 0" con:"TD.Stage Wave5" sleep:12 con:"god" con:"TD.KillAll" con:"TD.Squad.Passive 1" con:"TD.World.Hold 900 0" sleep:1 con:"TD.Dev.SpawnMine 40" con:"TD.Dev.SpawnMine 120" sleep:1.5 cap:${T}_mine_1 sleep:1.5 cap:${T}_mine_2 >/dev/null
grep -h "SpawnMine" Saved/Logs/td-play-mcp_${TD_PORT}.log 2>/dev/null | tail -2 | cut -c1-200
$P con:"TD.Stage VerdanceOnFoot" sleep:16 con:"god" con:"TD.KillAll" >/dev/null
A "[('NS_TD_Amb_Shared_PadBeacon', 30, -6, 0), ('NS_TD_Amb_Shared_PadBeacon', 120, 15, 0), ('NS_TD_Amb_Shared_PadBeacon', 300, -40, 0)]"
$P sleep:3 cap:${T}_pad_1 sleep:0.8 cap:${T}_pad_2 >/dev/null
A "'clear'"; bash $G "('pie_stop',)" >/dev/null
bash $R/ed.sh unlock; echo "== closed $(date +%H:%M:%S)"
