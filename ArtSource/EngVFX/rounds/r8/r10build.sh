#!/usr/bin/env bash
# r10build.sh: round 10, after T7's window. Compile (lead/int-q merged), the CloudVeil material, the tier-2 systems
# (cloud v2, approach field, shield drop, tractor beam, light row), the five Null plumes, the cloud sounds and DA_ReactorCore_FX.
# One fresh editor per chunk (Niagara toolset memory); the guard and the rendering cap as always.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; PORT=${TD_PORT:-21031}
R=ArtSource/EngVFX/rounds/r8; S=.claude/skills/ue-game-audio/scripts
echo "== compile $(date +%H:%M)"; powershell -NoProfile -File Tools/td-build.ps1 2>&1 | tail -3
bash $R/waitslot.sh || exit 1
echo "== material $(date +%H:%M)"
bash $R/ed.sh start >/dev/null
(echo "ONLY_MATS = ['M_TD_VFX_CloudVeil']"; cat ArtSource/EngVFX/scripts/build_materials.py) > Saved/EngVFX/_mats.py
python ArtSource/EngVFX/scripts/run_editor_py.py $PORT Saved/EngVFX/_mats.py 2>&1 | tail -3 | cut -c1-200
echo "== sounds"
python $S/ue_run_python.py --port $PORT $S/ue_audio_build.py --spec ArtSource/EngAudio/build/spec_cloud_build_00.json 2>&1 | tail -3 | cut -c1-300
python $S/ue_run_python.py --port $PORT $S/ue_audio_build.py --spec ArtSource/EngAudio/build/spec_cloud_build_01.json 2>&1 | tail -3 | cut -c1-300
python $S/ue_run_python.py --port $PORT ArtSource/EngAudio/scripts/ue_wire.py --spec ArtSource/EngAudio/build/spec_cloud_wire.json 2>&1 | tail -3 | cut -c1-300
echo "== reactor core data"
python ArtSource/EngVFX/scripts/run_editor_py.py $PORT $R/data_core.py 2>&1 | tail -12 | cut -c1-200
python ArtSource/EngVFX/scripts/sound_check.py 2>&1 | tail -2
bash $R/ed.sh stop >/dev/null
echo "== tier 2 $(date +%H:%M)"
bash $R/chunk_list.sh NS_TD_Entry_CloudWisps "0:4"
bash $R/chunk_list.sh NS_TD_Dominion_ApproachField "0:4"
bash $R/chunk_list.sh NS_TD_Dominion_ShieldDrop "0:4 4:7"
bash $R/chunk_list.sh NS_TD_Dominion_TractorBeam "0:4"
bash $R/chunk_list.sh NS_TD_Dominion_LightRow "0:6 6:12"
echo "== plumes $(date +%H:%M)"
bash $R/chunk_list1.sh NS_TD_Plume_Enemy_Fighter "0:4"
bash $R/chunk_list1.sh NS_TD_Plume_Enemy_Interceptor "0:4 4:8"
bash $R/chunk_list1.sh NS_TD_Plume_Enemy_Gunship "0:4"
bash $R/chunk_list1.sh NS_TD_Plume_Enemy_Bomber "0:3"
bash $R/chunk_list1.sh NS_TD_Plume_Enemy_Carrier "0:4 4:8"
bash $R/chunk_list.sh NS_TD_Asteroid_Break_L "5:6"
echo "== done $(date +%H:%M)"
