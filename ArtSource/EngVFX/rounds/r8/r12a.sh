#!/usr/bin/env bash
# r12a.sh <tag>: one slot turn: the round-12 ambient systems on foot in the real Verdance and Cinder levels, pilot in view.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; R=ArtSource/EngVFX/rounds/r8
T=${1:-am}; unset NULLRHI; trap 'bash $R/ed.sh unlock' EXIT
bash $R/ed.sh lock "ambient planet effects capture (#194)" || exit 1
echo "== turn $(date +%H:%M:%S)"; bash $R/ed.sh start >/dev/null || exit 1
G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $TD_PORT Saved/EngVFX/r8"
A() { (echo "CMD = $1"; cat $R/amb_show.py) > Saved/EngVFX/_amb_show.py; python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_amb_show.py 2>&1 | tail -16 | tr '\n' ' ' | cut -c1-400; echo; }
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage VerdanceOnFoot" sleep:16 con:"god" con:"TD.KillAll" >/dev/null
A "[('NS_TD_Amb_Verdance_Spores', 10, 0, 0), ('NS_TD_Amb_Verdance_Fireflies', 14, -6, 0), ('NS_TD_Amb_Verdance_GroundMist', 12, 4, 0), ('NS_TD_Amb_Verdance_MushroomPulse', 6, -3, 1.5), ('NS_TD_Amb_Verdance_Insects', 5, 3, 0.5), ('NS_TD_Amb_Shared_HutChimney', 16, 6, 3), ('NS_TD_Amb_Shared_HutBeacon', 16, 6, 4), ('NS_TD_Amb_Shared_PadDust', 8, 8, 0)]"
$P sleep:6 cap:${T}_verdance_a sleep:2 cap:${T}_verdance_b >/dev/null
A "'clear'"
$P con:"TD.Stage CinderOnFoot" sleep:16 con:"god" con:"TD.KillAll" >/dev/null
A "[('NS_TD_Amb_Cinder_Vent_S', 6, -4, 0), ('NS_TD_Amb_Cinder_Vent_M', 10, 3, 0), ('NS_TD_Amb_Cinder_Vent_L', 16, -6, 0), ('NS_TD_Amb_Cinder_EmberDrift', 10, 0, 0), ('NS_TD_Amb_Cinder_HeatShimmer', 7, 5, 0), ('NS_TD_Amb_Cinder_LavaPop', 8, 1, 0), ('NS_TD_Amb_Cinder_FallingAsh', 8, 0, 0), ('NS_TD_Amb_Cinder_SulfurFumes', 12, 8, 0), ('NS_TD_Amb_Shared_HutChimney', 18, 2, 3), ('NS_TD_Amb_Shared_HutBeacon', 18, 2, 4)]"
$P sleep:6 cap:${T}_cinder_a sleep:2 cap:${T}_cinder_b >/dev/null
A "'clear'"; bash $G "('pie_stop',)" >/dev/null
bash $R/ed.sh unlock; echo "== closed $(date +%H:%M:%S)"
