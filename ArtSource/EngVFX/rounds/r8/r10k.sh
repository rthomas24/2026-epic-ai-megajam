#!/usr/bin/env bash
# r10k.sh: 10e. (1) a short RENDERING session for the range-glow material (materials cannot compile under -nullrhi);
# (2) the systems headless; (3) the capture session. Stop at any point: touch Saved/EngVFX/STOP
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; R=ArtSource/EngVFX/rounds/r8
unset NULLRHI; echo "== material $(date +%H:%M)"; bash $R/ed.sh start >/dev/null || exit 1
python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT ArtSource/EngVFX/scripts/build_rangeglow.py 2>&1 | tail -3 | cut -c1-200
bash $R/ed.sh stop >/dev/null
export NULLRHI=1; echo "== systems $(date +%H:%M)"
for s in "NS_TD_Region_Beacon 0:1" "NS_TD_Magma_Heat 0:3" "NS_TD_Dominion_TractorBeam 0:4"; do
  bash $R/chunk_list.sh ${s%% *} "${s#* }" 2>&1 | grep "chunk\|UpToDate\|STOP\|rror" | cut -c1-160 || true; done
for s in "NS_TD_Plume_Enemy_Fighter 3:4" "NS_TD_Plume_Enemy_Interceptor 4:6" "NS_TD_Plume_Enemy_Gunship 3:4" "NS_TD_Plume_Enemy_Bomber 2:3" "NS_TD_Plume_Enemy_Carrier 4:8"; do
  bash $R/chunk_list1.sh ${s%% *} "${s#* }" 2>&1 | grep "chunk\|UpToDate\|STOP\|rror" | cut -c1-160 || true; done
unset NULLRHI; echo "== capture $(date +%H:%M)"
G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $TD_PORT Saved/EngVFX/r8"; T=${1:-k}; SC="python $R/spawncap.py"
bash $R/ed.sh start >/dev/null || exit 1
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage Waves1to2" sleep:12 con:"god" con:"TD.KillAll" sleep:2 con:"TD.World.Hold 900 0" >/dev/null
bash $G "('row', ['NS_TD_Debug_BeaconLOD', 'NS_TD_Debug_BeaconFixed', 'NS_TD_Region_Beacon'], 30000, 8000)" >/dev/null
$P sleep:1 cap:${T}_dbg_lod_fixed_beacon_300m >/dev/null; bash $G "('clear',)" >/dev/null
bash $G "('row', ['NS_TD_Region_Beacon', 'NS_TD_Region_Beacon'], 150000, 40000)" >/dev/null
$P sleep:1 cap:${T}_beacon_1500m >/dev/null; bash $G "('clear',)" >/dev/null
for d in 300 800; do python $R/burst.py $T plumes $d >/dev/null 2>&1; $P cap:${T}_plumes_chase_${d}m >/dev/null; bash $G "('clear',)" >/dev/null; done
$SC ${T}_tractor_600 NS_TD_Dominion_TractorBeam 600 side 1.0 0 '{"Length": 20000, "Intensity": 1.0}' 1 >/dev/null
$SC ${T}_haze_out NS_TD_Debug_HazeBase 800 chase >/dev/null
$SC ${T}_haze_in  NS_TD_Debug_HazeBase 0 chase >/dev/null
bash $G "('clear',)" >/dev/null; bash $G "('pie_stop',)" >/dev/null; sleep 5
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage CinderOnFoot" sleep:14 con:"god" con:"TD.KillAll" >/dev/null
M() { (echo "CMD = '$1'"; cat $R/magma2.py) > Saved/EngVFX/_mag.py; python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_mag.py 2>&1 | tail -1 | cut -c1-120; }
for spot in "5m 4300 4300 0 -12" "20m 2800 4300 0 -5"; do set -- $spot
  $P con:"TD.Ground.Teleport $2 $3 250" sleep:1 con:"TD.Look $4 $5" sleep:3 con:"TD.Look $4 $5" sleep:1 >/dev/null
  M off; $P sleep:1 cap:${T}_magma_$1_off >/dev/null
  M on;  $P sleep:3 cap:${T}_magma_$1_on_a sleep:0.5 cap:${T}_magma_$1_on_b >/dev/null
done
M off; bash $G "('pie_stop',)" >/dev/null
bash $R/ed.sh stop >/dev/null; echo "== closed $(date +%H:%M)"
