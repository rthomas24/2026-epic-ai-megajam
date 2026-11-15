#!/usr/bin/env bash
# r12o.sh <tag>: short slot turn: (1) bare systems 40 m ahead in space, no pause: the hut beacon (known good) and the
# plasma mine, then the mine paused; (2) pad beacons on Cinder at 60 / 150 / 300 m (open lava plain).
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; R=ArtSource/EngVFX/rounds/r8
T=${1:-mo}; unset NULLRHI; trap 'bash $R/ed.sh unlock' EXIT
bash $R/ed.sh lock "mine bare + pad 300 m" || exit 1
echo "== turn $(date +%H:%M:%S)"; bash $R/ed.sh start >/dev/null || exit 1
G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $TD_PORT Saved/EngVFX/r8"
B() { (echo "CMD = $1"; cat $R/tb_show.py) > Saved/EngVFX/_tb_show.py; python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_tb_show.py 2>&1 | tail -1 | cut -c1-200; }
A() { (echo "CMD = $1"; cat $R/amb_show.py) > Saved/EngVFX/_amb_show.py; python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_amb_show.py 2>&1 | tail -4 | tr '\n' ' ' | cut -c1-300; echo; }
S=/Game/TriadDrift/Gameplay/VFX/Systems
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage DominionAssault" sleep:14 con:"god" con:"TD.KillAll" con:"TD.Squad.Passive 1" con:"TD.World.Hold 900 0" sleep:1 >/dev/null
B "('$S/Ambient/NS_TD_Amb_Shared_HutBeacon', 60, -8, 0, 1, 1.0)"; $P sleep:0.4 cap:${T}_hut_60 >/dev/null
B "('$S/NS_TD_Mine_Plasma', 60, 8, 0, 1, 1.0)"; $P sleep:0.4 cap:${T}_mine_60 sleep:0.3 cap:${T}_mine_60b >/dev/null
B "'clear'"
$P con:"TD.Stage CinderOnFoot" sleep:16 con:"god" con:"TD.KillAll" >/dev/null
A "[('NS_TD_Amb_Shared_PadBeacon', 60, -10, 0), ('NS_TD_Amb_Shared_PadBeacon', 150, 20, 0), ('NS_TD_Amb_Shared_PadBeacon', 300, -30, 0)]"
$P sleep:3 cap:${T}_pad_1 sleep:0.8 cap:${T}_pad_2 >/dev/null
A "'clear'"; bash $G "('pie_stop',)" >/dev/null
bash $R/ed.sh unlock; echo "== closed $(date +%H:%M:%S)"
