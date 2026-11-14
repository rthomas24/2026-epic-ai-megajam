#!/usr/bin/env bash
# The Lancer's engine and boost plume only (T1, Nov 14): rebuilds NS_TD_Engine_Lancer and NS_TD_EngineBoost_Lancer in the
# running EDITOR session (TD_PORT) from build_tier1.py. Only while holding a render slot.
HERE="$(cd "$(dirname "$0")" && pwd)"; ROOT="$(cd "$HERE/../../.." && pwd)"; cd "$ROOT"
PORT="${TD_PORT:-21011}"; TMP=Saved/EngVFX; mkdir -p $TMP
(echo "ONLY = ['NS_TD_Engine_Lancer', 'NS_TD_EngineBoost_Lancer']"; cat ArtSource/EngVFX/scripts/build_tier1.py) > $TMP/_lancer_plume.py
python ArtSource/EngVFX/scripts/run_editor_py.py $PORT $TMP/_lancer_plume.py
