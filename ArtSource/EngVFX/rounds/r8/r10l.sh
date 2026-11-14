#!/usr/bin/env bash
# r10l.sh <tag>: 10f. Headless: magma (spatter count up), debug sprites (fixed bounds / offset / range-glow). One lock turn:
# M_TD_RegionHaze2 + its review sphere (materials need a renderer), then the capture: debug row at 300 m with screen
# positions, haze v2 outside and inside, magma at 20 m off/on.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; R=ArtSource/EngVFX/rounds/r8
export NULLRHI=1
for s in "NS_TD_Magma_Heat 0:3" "NS_TD_Debug_BeaconFixedB 0:1" "NS_TD_Debug_GlowOffset 0:1" "NS_TD_Debug_RangeA 0:1"; do
  bash $R/chunk_list.sh ${s%% *} "${s#* }" 2>&1 | grep "chunk\|UpToDate\|STOP\|rror" | cut -c1-160 || true; done
unset NULLRHI; trap 'bash $R/ed.sh unlock' EXIT
bash $R/ed.sh lock "haze v2 material + debug/haze/magma capture (about 15 min)" || exit 1
echo "== locked turn $(date +%H:%M)"; bash $R/ed.sh start >/dev/null || exit 1
python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT ArtSource/EngVFX/scripts/build_regionhaze2.py 2>&1 | tail -3 | cut -c1-200
(echo "ONLY = ['__none__']"; cat ArtSource/EngVFX/scripts/build_tier1.py; echo; echo "ONLY2 = ['=NS_TD_Debug_Haze2']"; echo "CHUNK = (0, 2)"; cat ArtSource/EngVFX/scripts/build_tier2.py) > Saved/EngVFX/_chunk.py
python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_chunk.py 2>&1 | grep "NS_TD_Debug_Haze2" | cut -c1-160
G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $TD_PORT Saved/EngVFX/r8"; T=${1:-l}; SC="python $R/spawncap.py"
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage Waves1to2" sleep:12 con:"god" con:"TD.KillAll" sleep:2 con:"TD.World.Hold 900 0" >/dev/null
bash $G "('row', ['NS_TD_Debug_BeaconFixed', 'NS_TD_Debug_BeaconFixedB', 'NS_TD_Debug_GlowOffset', 'NS_TD_Debug_RangeA', 'NS_TD_Region_Beacon'], 30000, 6000)" >/dev/null
sleep 1; python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT $R/dbg_view.py 2>&1 | tail -7 | sed 's/<Struct .Vector. ([^)]*)//g; s/<Struct .Vector2D. ([^)]*)//g' | cut -c1-200
$P cap:${T}_dbg_300m >/dev/null; bash $G "('clear',)" >/dev/null
$SC ${T}_haze2_out NS_TD_Debug_Haze2 800 chase >/dev/null
$SC ${T}_haze2_in  NS_TD_Debug_Haze2 0 chase >/dev/null
bash $G "('clear',)" >/dev/null; bash $G "('pie_stop',)" >/dev/null; sleep 5
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage CinderOnFoot" sleep:14 con:"god" con:"TD.KillAll" >/dev/null
M() { (echo "CMD = '$1'"; cat $R/magma2.py) > Saved/EngVFX/_mag.py; python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_mag.py 2>&1 | tail -1 | cut -c1-60; }
$P con:"TD.Ground.Teleport 2800 4300 250" sleep:1 con:"TD.Look 0 -5" sleep:3 con:"TD.Look 0 -5" sleep:1 >/dev/null
M off; $P sleep:1 cap:${T}_magma_20m_off >/dev/null
M on;  $P sleep:3 cap:${T}_magma_20m_on_a sleep:0.5 cap:${T}_magma_20m_on_b >/dev/null
M off; bash $G "('pie_stop',)" >/dev/null
bash $R/ed.sh unlock; echo "== closed $(date +%H:%M)"
