#!/usr/bin/env bash
# r12k.sh <tag>: short slot turn: tractor beam v3 (beads) spawned the bay's way: end-on from the chase camera (mouth 330 m
# ahead, +X back at the ship, 300 m, intensity 0.6 and 1.0) and from the side (300 m and 600 m).
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; R=ArtSource/EngVFX/rounds/r8
T=${1:-tk}; unset NULLRHI; trap 'bash $R/ed.sh unlock' EXIT
bash $R/ed.sh lock "tractor beam v3 capture" || exit 1
echo "== turn $(date +%H:%M:%S)"; bash $R/ed.sh start >/dev/null || exit 1
G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $TD_PORT Saved/EngVFX/r8"
B() { (echo "CMD = $1"; cat $R/tb_show.py) > Saved/EngVFX/_tb_show.py; python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_tb_show.py 2>&1 | tail -1 | cut -c1-200; }
TB=/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Dominion_TractorBeam3
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage DominionAssault" sleep:14 con:"god" con:"TD.KillAll" con:"TD.Squad.Passive 1" con:"TD.World.Hold 900 0" sleep:1 >/dev/null
B "('$TB', 330, 0, 180, 300, 0.6)"; $P sleep:2.5 cap:${T}_endon_06 >/dev/null
B "('$TB', 330, 0, 180, 300, 1.0)"; $P sleep:2.5 cap:${T}_endon_10 >/dev/null
B "('$TB', 420, -170, 90, 300, 0.6)"; $P sleep:2.5 cap:${T}_side_300 >/dev/null
B "('$TB', 600, -300, 90, 600, 1.0)"; $P sleep:2.5 cap:${T}_side_600 >/dev/null
B "'clear'"; bash $G "('pie_stop',)" >/dev/null
bash $R/ed.sh unlock; echo "== closed $(date +%H:%M:%S)"
