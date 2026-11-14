#!/usr/bin/env bash
# r10i.sh: 10d headless rebuilds (-nullrhi). Stop: touch Saved/EngVFX/STOP
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031} NULLRHI=1; R=ArtSource/EngVFX/rounds/r8
for s in "NS_TD_Region_Beacon 0:1" "NS_TD_Debug_HazeSphere 0:2" "NS_TD_Debug_HazeBase 0:2" "NS_TD_Magma_Heat 0:3"; do
  bash $R/chunk_list.sh ${s%% *} "${s#* }" 2>&1 | grep "chunk\|UpToDate\|STOP\|rror" | cut -c1-160 || true; done
for s in "NS_TD_Plume_Enemy_Fighter 0:4" "NS_TD_Plume_Enemy_Interceptor 0:3 3:6" "NS_TD_Plume_Enemy_Gunship 0:4" "NS_TD_Plume_Enemy_Bomber 0:3" "NS_TD_Plume_Enemy_Carrier 4:8"; do
  bash $R/chunk_list1.sh ${s%% *} "${s#* }" 2>&1 | grep "chunk\|UpToDate\|STOP\|rror" | cut -c1-160 || true; done
echo "== r10i done $(date +%H:%M)"
