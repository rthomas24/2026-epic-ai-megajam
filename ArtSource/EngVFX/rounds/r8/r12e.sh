#!/usr/bin/env bash
# r12e.sh: headless (-nullrhi): wire TractorBeam2 (DA_Dominion_Visuals.TractorBeam) and the mine tracer (DA_Enemy_PlasmaMines_FX.Tracer)
# for the post-51f45b35 check, and build NS_TD_Amb_Shared_PadBeacon.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1 NULLRHI=1; export TD_PORT=${TD_PORT:-21033}; R=ArtSource/EngVFX/rounds/r8
bash $R/ed.sh start >/dev/null || exit 1
(echo "PLAN = {'Dominion/DA_Dominion_Visuals': {'tractor_beam': 'NS_TD_Dominion_TractorBeam3'}}"
 sed 's/^PLAN = globals().get.*$/pass/' ArtSource/EngVFX/scripts/wire_fx.py) > Saved/EngVFX/_wire.py
python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_wire.py 2>&1 | tail -2 | cut -c1-300
python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT $R/mine_wire.py 2>&1 | tail -2 | cut -c1-300
(echo "ONLY = ['__none__']"; cat ArtSource/EngVFX/scripts/build_tier1.py; echo; echo "ONLY2 = ['=NS_TD_Amb_Shared_PadBeacon']"; cat ArtSource/EngVFX/scripts/build_tier2.py) > Saved/EngVFX/_amb.py
python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_amb.py 2>&1 | grep "NS_TD_Amb\|FAIL\|rror" | cut -c1-260
bash $R/ed.sh stop >/dev/null
python ArtSource/EngVFX/scripts/sound_check.py HEAD | head -3
echo "== r12e done $(date +%H:%M)"
