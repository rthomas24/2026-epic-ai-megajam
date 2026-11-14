#!/usr/bin/env bash
# r12b.sh: headless (-nullrhi): wire the Grunt wind-up (DA_Crew_Grunt_Visuals.Telegraph, VFX only) and rebuild the Verdance set (12b).
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1 NULLRHI=1; export TD_PORT=${TD_PORT:-21033}; R=ArtSource/EngVFX/rounds/r8
bash $R/ed.sh start >/dev/null || exit 1
(echo "da = unreal.load_asset('/Game/TriadDrift/Gameplay/Characters/NullGrunt/DA_Crew_Grunt_Visuals')"
 echo "_t = da.get_editor_property('telegraph')"
 echo "PRE = 'telegraph before: vfx=%s scale=%s' % (_t.get_editor_property('vfx'), _t.get_editor_property('vfx_scale'))"
 echo "PLAN = {'NullGrunt/DA_Crew_Grunt_Visuals': {'telegraph': 'NS_TD_Crew_MeleeWindup'}}"
 sed 's/^PLAN = globals().get.*$/pass/' ArtSource/EngVFX/scripts/wire_fx.py | sed 's/^"\n".join(log)$/PRE + " | " + " ; ".join(log)/') > Saved/EngVFX/_wire.py
python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_wire.py 2>&1 | tail -2 | cut -c1-400
only="'=NS_TD_Amb_Verdance_Spores', '=NS_TD_Amb_Verdance_Fireflies', '=NS_TD_Amb_Verdance_GroundMist', '=NS_TD_Amb_Verdance_FallingLeaves', '=NS_TD_Amb_Verdance_GlowPollen'"
(echo "ONLY = ['__none__']"; cat ArtSource/EngVFX/scripts/build_tier1.py; echo; echo "ONLY2 = [$only]"; cat ArtSource/EngVFX/scripts/build_tier2.py) > Saved/EngVFX/_amb.py
python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_amb.py 2>&1 | grep "NS_TD_Amb\|FAIL\|rror" | cut -c1-260
bash $R/ed.sh stop >/dev/null
python ArtSource/EngVFX/scripts/sound_check.py HEAD | head -3
echo "== r12b done $(date +%H:%M)"
