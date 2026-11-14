#!/usr/bin/env bash
# r9b.sh <tag>: round 9b review in PIE on $TD_PORT: the enemy bolt (resized), the escape flight loop, a live hyperdrive
# jump (r8), then the planet ambients in low flight (arrival) and on foot, and the escape run loop on foot.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1
G=ArtSource/EngVFX/rounds/r8/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py ${TD_PORT:-21031} Saved/EngVFX/r8"; T=${1:-b}
bash $G "('pie_start',)" >/dev/null; sleep 25; $P con:"TD.Stage Waves1to2" sleep:14 con:"god" >/dev/null
timeout 300 bash ArtSource/EngVFX/rounds/r8/tracers_cvd.sh ${T}
$P con:"TD.KillAll" con:"TD.World.Hold 900 60" sleep:1 >/dev/null
bash $G "('attach_set', 'NS_TD_Escape_FlightLoop', (0, 0, 0), {'Intensity': 1.0})" >/dev/null; $P sleep:2.5 cap:${T}_escflight_1 sleep:1.5 cap:${T}_escflight_2 >/dev/null; bash $G "('clear',)" >/dev/null
$P con:"TD.World.Hold 0" con:"TD.SkipWave" sleep:3 con:"TD.KillAll" sleep:3 con:"TD.SkipWave" sleep:3 con:"TD.KillAll" sleep:5 \
   con:"TD.Input.Key H 1.6" sleep:0.5 cap:${T}_hyp_1 sleep:1.2 cap:${T}_hyp_2 sleep:2 cap:${T}_hyp_3 sleep:3 cap:${T}_hyp_4 sleep:5 cap:${T}_hyp_5 >/dev/null
for planet in Verdance Cinder; do
  $P con:"TD.Stage ${planet}Arrival" sleep:25 >/dev/null
  bash $G "('attach_set', 'NS_TD_Ambient_${planet}', (0, 0, 0), {'Fade': 1.0})" >/dev/null; $P sleep:4 cap:${T}_amb_${planet}_air >/dev/null; bash $G "('clear',)" >/dev/null
  $P con:"TD.Stage ${planet}OnFoot" sleep:25 >/dev/null
  bash $G "('attach_set', 'NS_TD_Ambient_${planet}', (0, 0, 0), {'Fade': 1.0})" >/dev/null; $P sleep:4 cap:${T}_amb_${planet}_foot >/dev/null; bash $G "('clear',)" >/dev/null
done
bash $G "('attach_set', 'NS_TD_Escape_RunLoop', (0, 0, 0), {'Intensity': 1.0})" >/dev/null; $P sleep:3 cap:${T}_escrun >/dev/null; bash $G "('clear',)" >/dev/null
bash $G "('pie_stop',)" >/dev/null
