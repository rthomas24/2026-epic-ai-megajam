#!/usr/bin/env bash
# r12m.sh <tag>: one slot turn: (1) tractor beam v3 end-on, ship end dimmed; (2) the plasma mine at 60 / 150 m with the ship
# slowed (S held) and the game paused for the frame; (3) Verdance ambient with offsets on the shape module + pad beacons at
# 30 / 120 / 300 m; (4) Cinder ash and embers.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; R=ArtSource/EngVFX/rounds/r8
T=${1:-tm2}; unset NULLRHI; trap 'bash $R/ed.sh unlock' EXIT
bash $R/ed.sh lock "tractor dim, mine, ambient offsets, pad beacon" || exit 1
echo "== turn $(date +%H:%M:%S)"; bash $R/ed.sh start >/dev/null || exit 1
G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $TD_PORT Saved/EngVFX/r8"
B() { (echo "CMD = $1"; cat $R/tb_show.py) > Saved/EngVFX/_tb_show.py; python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_tb_show.py 2>&1 | tail -1 | cut -c1-200; }
A() { (echo "CMD = $1"; cat $R/amb_show.py) > Saved/EngVFX/_amb_show.py; python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_amb_show.py 2>&1 | tail -4 | tr '\n' ' ' | cut -c1-300; echo; }
TB=/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Dominion_TractorBeam3
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage DominionAssault" sleep:14 con:"god" con:"TD.KillAll" con:"TD.Squad.Passive 1" con:"TD.World.Hold 900 0" sleep:1 >/dev/null
B "('$TB', 330, 0, 180, 300, 1.0)"; $P sleep:2.5 cap:${T}_endon >/dev/null
B "'clear'"
$P con:"TD.Input.Keys S 3.0" sleep:3.2 con:"TD.Dev.SpawnMine 60" sleep:0.3 con:"pause" sleep:0.5 cap:${T}_mine_60 con:"pause" sleep:1.5 >/dev/null
$P con:"TD.Input.Keys S 2.0" sleep:2.2 con:"TD.Dev.SpawnMine 150" sleep:0.3 con:"pause" sleep:0.5 cap:${T}_mine_150 con:"pause" >/dev/null
grep -h "SpawnMine" Saved/Logs/td-play-mcp_${TD_PORT}.log 2>/dev/null | tail -2 | cut -c1-200
$P con:"TD.Stage VerdanceOnFoot" sleep:16 con:"god" con:"TD.KillAll" >/dev/null
A "[('NS_TD_Amb_Verdance_Spores', 10, 0, 0), ('NS_TD_Amb_Verdance_Fireflies', 14, -6, 0), ('NS_TD_Amb_Verdance_GroundMist', 12, 4, 0), ('NS_TD_Amb_Verdance_Insects', 5, 3, 0.5), ('NS_TD_Amb_Verdance_FallingLeaves', 9, -3, 0), ('NS_TD_Amb_Shared_PadBeacon', 30, -6, 0), ('NS_TD_Amb_Shared_PadBeacon', 120, 15, 0), ('NS_TD_Amb_Shared_PadBeacon', 300, -10, 0)]"
$P sleep:6 cap:${T}_verdance_a sleep:0.8 cap:${T}_verdance_b >/dev/null
A "'clear'"
$P con:"TD.Stage CinderOnFoot" sleep:16 con:"god" con:"TD.KillAll" >/dev/null
A "[('NS_TD_Amb_Cinder_EmberDrift', 10, 0, 0), ('NS_TD_Amb_Cinder_FallingAsh', 8, 0, 0), ('NS_TD_Amb_Cinder_Vent_M', 10, 3, 0)]"
$P sleep:7 cap:${T}_cinder >/dev/null
A "'clear'"; bash $G "('pie_stop',)" >/dev/null
bash $R/ed.sh unlock; echo "== closed $(date +%H:%M:%S)"
