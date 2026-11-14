#!/usr/bin/env bash
# r12c.sh <tag>: one slot turn: the 12b Verdance set re-shot from the r12a view, then the Grunt melee wind-up at 5 m in slow motion.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; R=ArtSource/EngVFX/rounds/r8
T=${1:-am2}; unset NULLRHI; trap 'bash $R/ed.sh unlock' EXIT
bash $R/ed.sh lock "Verdance ambient re-shoot + Grunt wind-up (#194)" || exit 1
echo "== turn $(date +%H:%M:%S)"; bash $R/ed.sh start >/dev/null || exit 1
G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $TD_PORT Saved/EngVFX/r8"
A() { (echo "CMD = $1"; cat $R/amb_show.py) > Saved/EngVFX/_amb_show.py; python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_amb_show.py 2>&1 | tail -16 | tr '\n' ' ' | cut -c1-400; echo; }
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage VerdanceOnFoot" sleep:16 con:"god" con:"TD.KillAll" >/dev/null
A "[('NS_TD_Amb_Verdance_Spores', 10, 0, 0), ('NS_TD_Amb_Verdance_Fireflies', 14, -6, 0), ('NS_TD_Amb_Verdance_GroundMist', 12, 4, 0), ('NS_TD_Amb_Verdance_MushroomPulse', 6, -3, 1.5), ('NS_TD_Amb_Verdance_Insects', 5, 3, 0.5), ('NS_TD_Amb_Shared_HutChimney', 16, 6, 3), ('NS_TD_Amb_Shared_HutBeacon', 16, 6, 4), ('NS_TD_Amb_Shared_PadDust', 8, 8, 0), ('NS_TD_Amb_Verdance_FallingLeaves', 9, -3, 0), ('NS_TD_Amb_Verdance_GlowPollen', 5, 2, 0), ('NS_TD_Amb_Verdance_GlowPollen', 7, -2, 0)]"
$P sleep:8 cap:${T}_verdance_a sleep:2 cap:${T}_verdance_b sleep:2 cap:${T}_verdance_c >/dev/null
A "'clear'"
$P con:"slomo 0.1" >/dev/null
A "[('/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Crew_MeleeWindup', 5, 0, 0.9, 180)]"
$P cap:${T}_windup_1 sleep:0.6 cap:${T}_windup_2 sleep:0.6 cap:${T}_windup_3 sleep:0.6 cap:${T}_windup_4 sleep:0.6 cap:${T}_windup_5 >/dev/null
$P con:"slomo 1" >/dev/null; A "'clear'"; bash $G "('pie_stop',)" >/dev/null
bash $R/ed.sh unlock; echo "== closed $(date +%H:%M:%S)"
