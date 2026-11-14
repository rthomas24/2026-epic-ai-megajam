#!/usr/bin/env bash
# amb_build.sh: the round-12 ambient systems, headless (-nullrhi), in 4 batches of about 7 emitters per editor session.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1 NULLRHI=1; export TD_PORT=${TD_PORT:-21033}; R=ArtSource/EngVFX/rounds/r8
for batch in "NS_TD_Amb_Verdance_Spores NS_TD_Amb_Verdance_Fireflies NS_TD_Amb_Verdance_GroundMist NS_TD_Amb_Verdance_MushroomPulse NS_TD_Amb_Verdance_Insects" \
             "NS_TD_Amb_Cinder_Vent_S NS_TD_Amb_Cinder_Vent_M" \
             "NS_TD_Amb_Cinder_Vent_L NS_TD_Amb_Cinder_EmberDrift NS_TD_Amb_Cinder_HeatShimmer NS_TD_Amb_Cinder_LavaPop" \
             "NS_TD_Amb_Cinder_FallingAsh NS_TD_Amb_Cinder_SulfurFumes NS_TD_Amb_Shared_PadDust NS_TD_Amb_Shared_HutChimney NS_TD_Amb_Shared_HutBeacon"; do
  bash $R/ed.sh start >/dev/null || exit 1
  only=$(for n in $batch; do printf "'=%s', " $n; done)
  (echo "ONLY = ['__none__']"; cat ArtSource/EngVFX/scripts/build_tier1.py; echo; echo "ONLY2 = [$only]"; cat ArtSource/EngVFX/scripts/build_tier2.py) > Saved/EngVFX/_amb.py
  python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_amb.py 2>&1 | grep "NS_TD_Amb\|FAIL\|rror" | cut -c1-260
  bash $R/ed.sh stop >/dev/null
done
echo "== amb built $(date +%H:%M)"
