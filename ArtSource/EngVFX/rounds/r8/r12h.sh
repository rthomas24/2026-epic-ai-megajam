#!/usr/bin/env bash
# r12h.sh: headless: build NS_TD_Dominion_TractorBeam3 and the pad beacon v2.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1 NULLRHI=1; export TD_PORT=${TD_PORT:-21033}; R=ArtSource/EngVFX/rounds/r8
bash $R/ed.sh start >/dev/null || exit 1
(echo "ONLY = ['__none__']"; cat ArtSource/EngVFX/scripts/build_tier1.py; echo; echo "ONLY2 = ['=NS_TD_Mine_Plasma2']"; cat ArtSource/EngVFX/scripts/build_tier2.py) > Saved/EngVFX/_amb.py
python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_amb.py 2>&1 | grep "NS_TD_\|FAIL\|rror" | cut -c1-300
bash $R/ed.sh stop >/dev/null; echo "== r12h done $(date +%H:%M)"
