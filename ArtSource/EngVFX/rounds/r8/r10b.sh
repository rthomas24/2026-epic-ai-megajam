#!/usr/bin/env bash
# r10b.sh: round 10 second batch: M_TD_VFX_Beacon + NS_TD_Region_Beacon, M_TD_RegionHaze.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; PORT=${TD_PORT:-21031}; R=ArtSource/EngVFX/rounds/r8
bash $R/ed.sh start >/dev/null
(echo "ONLY_MATS = ['M_TD_VFX_Beacon']"; cat ArtSource/EngVFX/scripts/build_materials.py) > Saved/EngVFX/_mats.py
python ArtSource/EngVFX/scripts/run_editor_py.py $PORT Saved/EngVFX/_mats.py 2>&1 | tail -2 | cut -c1-200
python ArtSource/EngVFX/scripts/run_editor_py.py $PORT ArtSource/EngVFX/scripts/build_regionhaze.py 2>&1 | tail -4 | cut -c1-300
bash $R/ed.sh stop >/dev/null
bash $R/chunk_list.sh NS_TD_Region_Beacon "0:1"
echo "== r10b done $(date +%H:%M)"
