#!/usr/bin/env bash
# r9d.sh <tag>: ambients in low flight and on foot (Cinder), a live hyperdrive jump, and the reactor-exposed stage (PIE on $TD_PORT).
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1
G=ArtSource/EngVFX/rounds/r8/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py ${TD_PORT:-21031} Saved/EngVFX/r8"; T=${1:-d}
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage Waves1to2" sleep:14 con:"god" con:"TD.SkipWave" sleep:3 con:"TD.KillAll" sleep:3 con:"TD.SkipWave" sleep:3 con:"TD.KillAll" sleep:6 con:"TD.KillAll" sleep:0.5 \
   con:"TD.Input.Key H 1.6" sleep:0.4 con:"TD.KillAll" sleep:0.3 cap:${T}_hyp_1 sleep:1.0 con:"TD.KillAll" cap:${T}_hyp_2 sleep:2 con:"TD.KillAll" cap:${T}_hyp_3 sleep:3 cap:${T}_hyp_4 sleep:5 cap:${T}_hyp_5 >/dev/null
for planet in Verdance Cinder; do
  $P con:"TD.Stage ${planet}Arrival" sleep:25 >/dev/null
  bash $G "('attach_set', 'NS_TD_Ambient_${planet}', (0, 0, 0), {'Fade': 1.0})" >/dev/null; $P sleep:4 cap:${T}_amb_${planet}_air >/dev/null; bash $G "('clear',)" >/dev/null
done
$P con:"TD.Stage CinderOnFoot" sleep:25 >/dev/null
bash $G "('attach_set', 'NS_TD_Ambient_Cinder', (0, 0, 0), {'Fade': 1.0})" >/dev/null; $P sleep:4 cap:${T}_amb_Cinder_foot >/dev/null; bash $G "('clear',)" >/dev/null
$P con:"TD.Stage ReactorExposed" sleep:20 con:"god" con:"TD.World.AimAt Reactor 2" sleep:2.5 cap:${T}_reactor_0 con:"TD.Input.Key LeftMouseButton 3" sleep:1.2 cap:${T}_reactor_hit_1 sleep:1.0 cap:${T}_reactor_hit_2 >/dev/null
bash $G "('pie_stop',)" >/dev/null
