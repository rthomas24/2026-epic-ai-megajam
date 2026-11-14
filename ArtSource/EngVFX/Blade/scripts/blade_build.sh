#!/usr/bin/env bash
# #163: build blade Niagara systems in the running editor session (TD_PORT). usage: blade_build.sh NS_TD_Blade_Aura[,NS_...]
# Only while holding the render-slot lock. Joins build_tier1.py + build_tier2.py (helpers only) + build_blade_fx.py.
HERE="$(cd "$(dirname "$0")" && pwd)"; ROOT="$(cd "$HERE/../../../.." && pwd)"; cd "$ROOT"
PORT="${TD_PORT:-21630}"; TMP=Saved/EngVFX; mkdir -p $TMP
(echo "ONLY = ['__none__']"; cat ArtSource/EngVFX/scripts/build_tier1.py; echo; echo "ONLY2 = ['=__none__']"; cat ArtSource/EngVFX/scripts/build_tier2.py; echo;
 echo "BLADE_ONLY = '$1'.split(',')"; cat "$HERE/build_blade_fx.py") > $TMP/_blade_batch.py
python ArtSource/EngVFX/scripts/run_editor_py.py $PORT $TMP/_blade_batch.py
