#!/usr/bin/env bash
# r10e.sh: round 10c headless rebuilds (-nullrhi, no slot) and the wiring job. Stop it with: touch Saved/EngVFX/STOP
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031} NULLRHI=1; R=ArtSource/EngVFX/rounds/r8
bash $R/ed.sh start >/dev/null || exit 1
(echo "ONLY_MATS = ['M_TD_VFX_FieldHex2']"; cat ArtSource/EngVFX/scripts/build_materials.py) > Saved/EngVFX/_mats.py
python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_mats.py 2>&1 | tail -1 | cut -c1-200
bash $R/ed.sh stop >/dev/null
for s in "NS_TD_Dominion_ApproachField 0:4" "NS_TD_Region_Beacon 0:1" "NS_TD_Dominion_TractorBeam 0:4" "NS_TD_Dominion_LightRow 0:6 6:12"; do
  bash $R/chunk_list.sh ${s%% *} "${s#* }" 2>&1 | grep "chunk\|UpToDate\|STOP\|Error" | cut -c1-160 || true; done
for s in "NS_TD_Plume_Enemy_Fighter 0:4" "NS_TD_Plume_Enemy_Interceptor 0:3 3:6" "NS_TD_Plume_Enemy_Gunship 0:4" "NS_TD_Plume_Enemy_Bomber 0:3" "NS_TD_Plume_Enemy_Carrier 0:4 4:8"; do
  bash $R/chunk_list1.sh ${s%% *} "${s#* }" 2>&1 | grep "chunk\|UpToDate\|STOP\|Error" | cut -c1-160 || true; done
bash $R/ed.sh start >/dev/null || exit 1
python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT ArtSource/EngVFX/scripts/wire_fx.py 2>&1 | tail -3 | cut -c1-250
bash $R/ed.sh stop >/dev/null
python ArtSource/EngVFX/scripts/sound_check.py HEAD | head -3
echo "== r10e done $(date +%H:%M)"
