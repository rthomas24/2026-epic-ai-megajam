#!/usr/bin/env bash
# r10c.sh: round 10b fixes from the first captures, rebuilt, then every round-10 capture again.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; PORT=$TD_PORT; R=ArtSource/EngVFX/rounds/r8
bash $R/ed.sh start >/dev/null
(echo "ONLY_MATS = ['M_TD_VFX_FieldHex']"; cat ArtSource/EngVFX/scripts/build_materials.py) > Saved/EngVFX/_mats.py
python ArtSource/EngVFX/scripts/run_editor_py.py $PORT Saved/EngVFX/_mats.py 2>&1 | tail -2 | cut -c1-200
python ArtSource/EngVFX/scripts/run_editor_py.py $PORT $R/haze_flag.py 2>&1 | tail -2 | cut -c1-200
bash $R/ed.sh stop >/dev/null
for s in "NS_TD_Region_Beacon 0:1" "NS_TD_Debug_HazeSphere 0:1" "NS_TD_Dominion_TractorBeam 0:4" "NS_TD_Dominion_ApproachField 0:4" "NS_TD_Entry_CloudWisps 0:4" "NS_TD_Dominion_LightRow 0:6 6:12"; do
  bash $R/chunk_list.sh ${s%% *} "${s#* }" 2>&1 | grep -v "LogContentValidation\|LIST DONE"; done
for s in "NS_TD_Plume_Enemy_Fighter 0:4" "NS_TD_Plume_Enemy_Interceptor 0:3 3:6" "NS_TD_Plume_Enemy_Gunship 0:4" "NS_TD_Plume_Enemy_Bomber 0:3" "NS_TD_Plume_Enemy_Carrier 0:4 4:8"; do
  bash $R/chunk_list1.sh ${s%% *} "${s#* }" 2>&1 | grep -v "LogContentValidation\|LIST DONE"; done
echo "== built $(date +%H:%M)"
bash $R/ed.sh start >/dev/null
G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $PORT Saved/EngVFX/r8"
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage Waves1to2" sleep:12 con:"god" con:"TD.KillAll" sleep:2 con:"TD.World.Hold 900 0" >/dev/null
for d in 300 800; do python $R/burst.py v plumes $d 2>&1 | tail -1; $P cap:v_plumes_chase_${d}m >/dev/null; bash $G "('clear',)" >/dev/null; done
python $R/burst.py v plumes 60 2>&1 | tail -1; $P cap:v_plumes_chase_60m >/dev/null; bash $G "('clear',)" >/dev/null
bash $G "('pie_stop',)" >/dev/null; sleep 5
bash $R/r10cap2.sh v 2>&1 | grep -v "^spawned" | tail -6 | cut -c1-200
bash $R/r9s.sh v
bash $R/ed.sh stop >/dev/null
echo "== captured $(date +%H:%M)"
