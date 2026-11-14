#!/usr/bin/env bash
# r12l.sh: headless: the tractor beam v3 ship end dimmed (Bead08-11, ShipGlow) and every ambient system with an offset
# rebuilt with the offset on the shape module (spores, fireflies, mist, insects, petals, embers, ash, pad beacon).
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1 NULLRHI=1; export TD_PORT=${TD_PORT:-21033}; R=ArtSource/EngVFX/rounds/r8
bash $R/chunk_list.sh NS_TD_Dominion_TractorBeam3 "8:12 15:16" 2>&1 | grep "chunk\|UpToDate\|rror\|FAIL" | cut -c1-300
for batch in "NS_TD_Amb_Verdance_Spores NS_TD_Amb_Verdance_Fireflies NS_TD_Amb_Verdance_GroundMist NS_TD_Amb_Verdance_Insects" \
             "NS_TD_Amb_Verdance_FallingLeaves NS_TD_Amb_Cinder_EmberDrift NS_TD_Amb_Cinder_FallingAsh NS_TD_Amb_Shared_PadBeacon"; do
  bash $R/ed.sh start >/dev/null || exit 1
  only=$(for n in $batch; do printf "'=%s', " $n; done)
  (echo "ONLY = ['__none__']"; cat ArtSource/EngVFX/scripts/build_tier1.py; echo; echo "ONLY2 = [$only]"; cat ArtSource/EngVFX/scripts/build_tier2.py) > Saved/EngVFX/_amb.py
  python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_amb.py 2>&1 | grep "NS_TD_Amb\|FAIL\|rror" | cut -c1-300
  bash $R/ed.sh stop >/dev/null
done
echo "== r12l done $(date +%H:%M)"
