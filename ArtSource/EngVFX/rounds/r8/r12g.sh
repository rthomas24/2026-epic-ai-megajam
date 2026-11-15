#!/usr/bin/env bash
# r12g.sh <tag>: one slot turn: tractor beam v3 spawned the bay's way, seen end-on from the chase camera and from the side;
# then the plasma mine (SpawnMine 150 / 300, captured at once), the pad beacon v2 on foot and the Grunt wind-up v3 (slomo 0.1, 5 m).
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; R=ArtSource/EngVFX/rounds/r8
T=${1:-tb3}; unset NULLRHI; trap 'bash $R/ed.sh unlock' EXIT
bash $R/ed.sh lock "tractor beam v3, mine, pad beacon" || exit 1
echo "== turn $(date +%H:%M:%S)"; bash $R/ed.sh start >/dev/null || exit 1
G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $TD_PORT Saved/EngVFX/r8"
B() { (echo "CMD = $1"; cat $R/tb_show.py) > Saved/EngVFX/_tb_show.py; python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_tb_show.py 2>&1 | tail -1 | cut -c1-200; }
A() { (echo "CMD = $1"; cat $R/amb_show.py) > Saved/EngVFX/_amb_show.py; python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_amb_show.py 2>&1 | tail -4 | tr '\n' ' ' | cut -c1-300; echo; }
TB=/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Dominion_TractorBeam3
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage DominionAssault" sleep:14 con:"god" con:"TD.KillAll" con:"TD.Squad.Passive 1" con:"TD.World.Hold 900 0" sleep:1 >/dev/null
B "('$TB', 330, 0, 180, 300, 0.6)"; $P sleep:2.5 cap:${T}_endon_1 sleep:0.6 cap:${T}_endon_2 >/dev/null
B "('$TB', 330, 0, 180, 300, 1.0)"; $P sleep:2.5 cap:${T}_endon_3 >/dev/null
B "('$TB', 420, -170, 90, 340, 0.6)"; $P sleep:2.5 cap:${T}_side_1 sleep:0.6 cap:${T}_side_2 >/dev/null
B "'clear'"
$P con:"TD.Dev.SpawnMine 150" con:"TD.Dev.SpawnMine 300" cap:${T}_mine_1 sleep:0.4 cap:${T}_mine_2 >/dev/null
grep -h "SpawnMine" Saved/Logs/td-play-mcp_${TD_PORT}.log 2>/dev/null | tail -2 | cut -c1-200
$P con:"TD.Stage VerdanceOnFoot" sleep:16 con:"god" con:"TD.KillAll" >/dev/null
A "[('NS_TD_Amb_Shared_PadBeacon', 30, -6, 0), ('NS_TD_Amb_Shared_PadBeacon', 120, 15, 0), ('NS_TD_Amb_Shared_PadBeacon', 300, -10, 0)]"
$P sleep:3 cap:${T}_pad_1 sleep:0.8 cap:${T}_pad_2 >/dev/null
A "'clear'"
$P con:"slomo 0.1" >/dev/null
A "[('/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Crew_MeleeWindup', 5, 0, 0.9, 180)]"
$P cap:${T}_wu_1 sleep:0.6 cap:${T}_wu_2 sleep:0.6 cap:${T}_wu_3 sleep:0.6 cap:${T}_wu_4 >/dev/null
$P con:"slomo 1" >/dev/null; A "'clear'"; bash $G "('pie_stop',)" >/dev/null
bash $R/ed.sh unlock; echo "== closed $(date +%H:%M:%S)"
