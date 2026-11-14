#!/usr/bin/env bash
# r12d.sh <tag>: short slot turn: the Grunt wind-up v2 on foot (Verdance), 5 m and 9 m, slow motion 0.1.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; R=ArtSource/EngVFX/rounds/r8
T=${1:-wu2}; unset NULLRHI; trap 'bash $R/ed.sh unlock' EXIT
bash $R/ed.sh lock "Grunt wind-up v2 capture" || exit 1
echo "== turn $(date +%H:%M:%S)"; bash $R/ed.sh start >/dev/null || exit 1
G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $TD_PORT Saved/EngVFX/r8"
A() { (echo "CMD = $1"; cat $R/amb_show.py) > Saved/EngVFX/_amb_show.py; python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_amb_show.py 2>&1 | tail -4 | tr '\n' ' ' | cut -c1-300; echo; }
W=/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Crew_MeleeWindup
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage VerdanceOnFoot" sleep:16 con:"god" con:"TD.KillAll" con:"slomo 0.1" >/dev/null
A "[('$W', 5, 0, 0.9, 180), ('$W', 9, 3, 0.9, 180)]"
$P cap:${T}_1 sleep:0.6 cap:${T}_2 sleep:0.6 cap:${T}_3 sleep:0.6 cap:${T}_4 sleep:0.6 cap:${T}_5 >/dev/null
$P con:"slomo 1" >/dev/null; A "'clear'"; bash $G "('pie_stop',)" >/dev/null
bash $R/ed.sh unlock; echo "== closed $(date +%H:%M:%S)"
