#!/usr/bin/env bash
# r10f.sh <tag>: the short 10c capture (Lead: about 8 minutes on the shared slot): plumes from the chase camera, beacon,
# tractor, haze, approach and light row (the Lead's changes), magma on foot at 5 m and 20 m. Stop: touch Saved/EngVFX/STOP
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; unset NULLRHI
R=ArtSource/EngVFX/rounds/r8; G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $TD_PORT Saved/EngVFX/r8"; T=${1:-x}; SC="python $R/spawncap.py"
echo "== start $(date +%H:%M)"; bash $R/ed.sh start || exit 1
# materials need a renderer (-nullrhi cannot compile them): FieldHex2, then the approach field that uses it, in this editor
(echo "ONLY_MATS = ['M_TD_VFX_FieldHex2']"; cat ArtSource/EngVFX/scripts/build_materials.py) > Saved/EngVFX/_mats.py
python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_mats.py 2>&1 | tail -1 | cut -c1-200
(echo "ONLY = ['__none__']"; cat ArtSource/EngVFX/scripts/build_tier1.py; echo; echo "ONLY2 = ['=NS_TD_Dominion_ApproachField']"; echo "CHUNK = (0, 4)"; cat ArtSource/EngVFX/scripts/build_tier2.py) > Saved/EngVFX/_chunk.py
python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_chunk.py 2>&1 | grep "NS_TD_Dominion_ApproachField" | cut -c1-200
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage Waves1to2" sleep:12 con:"god" con:"TD.KillAll" sleep:2 con:"TD.World.Hold 900 0" >/dev/null
for d in 60 300 800; do python $R/burst.py $T plumes $d 2>&1 | tail -1; $P cap:${T}_plumes_chase_${d}m >/dev/null; bash $G "('clear',)" >/dev/null; done
$SC ${T}_beacon_300  NS_TD_Region_Beacon 300 chase 1.0 0 '{"Color": [1.0, 0.62, 0.18]}'
$SC ${T}_beacon_1500 NS_TD_Region_Beacon 1500 chase 1.0 0 '{"Color": [1.0, 0.62, 0.18]}'
$SC ${T}_tractor    NS_TD_Dominion_TractorBeam 300 side 1.0 0 '{"Length": 20000, "Intensity": 1.0}' 1
$SC ${T}_tractor_c  NS_TD_Dominion_TractorBeam 250 chase 1.0 180 '{"Length": 20000, "Intensity": 1.0}'
$SC ${T}_approach_c NS_TD_Dominion_ApproachField 1500 chase 1.0 0 '{}'
$SC ${T}_lightrow   NS_TD_Dominion_LightRow 1500 chase 1.0 180 '{}'
python $R/haze.py $T 2>&1 | tail -1
bash $G "('clear',)" >/dev/null; bash $G "('pie_stop',)" >/dev/null; sleep 5
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage CinderOnFoot" sleep:25 con:"god" >/dev/null; python $R/magmacap.py $T 2>&1 | tail -1
bash $G "('pie_stop',)" >/dev/null
bash $R/ed.sh stop >/dev/null; echo "== closed $(date +%H:%M)"
