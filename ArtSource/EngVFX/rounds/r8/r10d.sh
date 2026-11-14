#!/usr/bin/env bash
# r10d.sh <tag>: the ONE round-10 capture session (Lead: at most 25 min on the shared slot). ed.sh start counts renderers,
# recounts 10 s after launch and backs off. Plumes 60/300/800 m (chase + side), approach, tractor, light row 1.5 km,
# beacon 300 m / 1.5 km, haze out/in, both clouds, magma on Cinder on foot (5 m, 20 m).
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; unset NULLRHI
R=ArtSource/EngVFX/rounds/r8; G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $TD_PORT Saved/EngVFX/r8"; T=${1:-w}; SC="python $R/spawncap.py"
echo "== start $(date +%H:%M)"; bash $R/ed.sh start || exit 1
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage Waves1to2" sleep:12 con:"god" con:"TD.KillAll" sleep:2 con:"TD.World.Hold 900 0" >/dev/null
for d in 60 300 800; do python $R/burst.py $T plumes $d 2>&1 | tail -1; $P cap:${T}_plumes_chase_${d}m >/dev/null; bash $G "('clear',)" >/dev/null; done
$SC ${T}_approach   NS_TD_Dominion_ApproachField 2500 side 1.0 0 '{}' 1
$SC ${T}_approach_c NS_TD_Dominion_ApproachField 1500 chase 1.0 0 '{}'
$SC ${T}_tractor    NS_TD_Dominion_TractorBeam 300 side 1.0 0 '{"Length": 20000, "Intensity": 1.0}' 1
$SC ${T}_tractor_c  NS_TD_Dominion_TractorBeam 250 chase 1.0 180 '{"Length": 20000, "Intensity": 1.0}'
for i in 1 2; do $SC ${T}_lightrow_$i NS_TD_Dominion_LightRow 1500 chase 1.0 180 '{}'; done
$SC ${T}_beacon_300  NS_TD_Region_Beacon 300 chase 1.0 0 '{"Color": [1.0, 0.62, 0.18]}'
$SC ${T}_beacon_1500 NS_TD_Region_Beacon 1500 chase 1.0 0 '{"Color": [1.0, 0.62, 0.18]}'
python $R/haze.py $T 2>&1 | tail -1
bash $G "('clear',)" >/dev/null; bash $G "('pie_stop',)" >/dev/null; sleep 5
echo "== clouds $(date +%H:%M)"; bash $R/r9s.sh $T
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage CinderOnFoot" sleep:25 con:"god" >/dev/null; python $R/magmacap.py $T 2>&1 | tail -1
bash $G "('pie_stop',)" >/dev/null
bash $R/ed.sh stop >/dev/null; echo "== closed $(date +%H:%M)"
