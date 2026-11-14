#!/usr/bin/env bash
# r10cap2.sh <tag>: round 10 Dominion and region captures (editor up on $TD_PORT): approach field, shield drop, tractor
# beam, Aftermath light row, region beacon (300 m, 1.5 km), region haze (outside 500 m, inside), L break at 200 m.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1
R=ArtSource/EngVFX/rounds/r8; G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py ${TD_PORT:-21031} Saved/EngVFX/r8/spawn"; T=${1:-u}; SC="python $R/spawncap.py"
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage Waves1to2" sleep:12 con:"god" con:"TD.KillAll" sleep:2 con:"TD.World.Hold 900 0" >/dev/null
$SC ${T}_approach   NS_TD_Dominion_ApproachField 2500 side 1.0 0 '{}' 1
$SC ${T}_approach_c NS_TD_Dominion_ApproachField 1500 chase 1.0 0 '{}'
$SC ${T}_shielddrop NS_TD_Dominion_ShieldDrop 1500 side 0.46 0 '{}' 6 0.5 60
$SC ${T}_tractor    NS_TD_Dominion_TractorBeam 300 side 1.0 0 '{"Length": 20000, "Intensity": 1.0}' 1
$SC ${T}_tractor_c  NS_TD_Dominion_TractorBeam 250 chase 1.0 180 '{"Length": 20000, "Intensity": 1.0}'
for i in 1 2 3; do $SC ${T}_lightrow_$i NS_TD_Dominion_LightRow 1500 chase 1.0 180 '{}'; sleep 0.4; done
$SC ${T}_beacon_300  NS_TD_Region_Beacon 300 chase 1.0 0 '{"Color": [1.0, 0.62, 0.18]}'
$SC ${T}_beacon_1500 NS_TD_Region_Beacon 1500 chase 1.0 0 '{"Color": [1.0, 0.62, 0.18]}'
$SC ${T}_breakL NS_TD_Asteroid_Break_L 200 side 4.5 0 '{}' 6 0.25 50
bash $G "('clear',)" >/dev/null
python $R/haze.py $T
bash $G "('pie_stop',)" >/dev/null
