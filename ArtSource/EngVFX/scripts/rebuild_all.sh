#!/usr/bin/env bash
# Triad Drift ENG-37: full tier-1 rebuild in batches, restarting the editor between batches. A Niagara rebuild through
# the toolset grows the editor by roughly 2 GB per system, so one editor must never build all 21 systems.
# usage: rebuild_all.sh [batch ...]     batches: mats A B C  (default: A B C)
#   mats = materials (+ Grok textures)   A = tracers + muzzles   B = engines   C = impacts + explosions
#   T2:<prefixes> = tier 2 systems (build_tier2.py) whose names start with one of the comma-separated prefixes
#   anything else = a comma-separated list of system name prefixes, e.g. NS_TD_Engine_,NS_TD_Muzzle_Enemy
# Run ONE copy at a time: a second copy restarts the editor under the first.
HERE="$(cd "$(dirname "$0")" && pwd)"; ROOT="$(cd "$HERE/../../.." && pwd)"; cd "$ROOT"
export MSYS_NO_PATHCONV=1
PORT="${TD_PORT:-8017}"; TMP=Saved/EngVFX; mkdir -p $TMP
restart() {
  powershell -File Tools/td-play-mcp.ps1 -Port $PORT -Stop | tail -1; sleep 5
  powershell -File Tools/td-play-mcp.ps1 -Port $PORT -Editor -Offscreen | grep -c "SESSION READY"
  # the editor reports ready while it is still loading the map: wait until editor Python answers
  for i in $(seq 1 40); do
    python "ArtSource/EngVFX/scripts/run_editor_py.py" $PORT "ArtSource/EngVFX/scripts/ping.py" 2>/dev/null | grep -q pong && break; sleep 5
  done
}
for b in ${*:-A B C}; do
  restart
  case $b in
    mats) python "ArtSource/EngVFX/scripts/run_editor_py.py" $PORT "ArtSource/EngVFX/scripts/build_materials.py" ;;
    A) (echo "ONLY = ['NS_TD_Tracer', 'NS_TD_Muzzle']"; cat "ArtSource/EngVFX/scripts/build_tier1.py") > $TMP/_batch.py; python "ArtSource/EngVFX/scripts/run_editor_py.py" $PORT $TMP/_batch.py ;;
    B) (echo "ONLY = ['NS_TD_Engine']"; cat "ArtSource/EngVFX/scripts/build_tier1.py") > $TMP/_batch.py; python "ArtSource/EngVFX/scripts/run_editor_py.py" $PORT $TMP/_batch.py ;;
    C) (echo "ONLY = ['NS_TD_Impact', 'NS_TD_Explosion']"; cat "ArtSource/EngVFX/scripts/build_tier1.py") > $TMP/_batch.py; python "ArtSource/EngVFX/scripts/run_editor_py.py" $PORT $TMP/_batch.py ;;
    T2:*) (echo "ONLY = ['__none__']"; cat "ArtSource/EngVFX/scripts/build_tier1.py"; echo; echo "ONLY2 = '${b#T2:}'.split(',')"; cat "ArtSource/EngVFX/scripts/build_tier2.py") > $TMP/_batch.py; python "ArtSource/EngVFX/scripts/run_editor_py.py" $PORT $TMP/_batch.py ;;
    *) (echo "ONLY = '$b'.split(',')"; cat "ArtSource/EngVFX/scripts/build_tier1.py") > $TMP/_batch.py; python "ArtSource/EngVFX/scripts/run_editor_py.py" $PORT $TMP/_batch.py ;;
  esac
  echo "batch $b done; editor GB: $(powershell -Command "(Get-Process UnrealEditor* | ? {\$_.WorkingSet64 -gt 5GB} | % {[math]::Round(\$_.WorkingSet64/1GB,1)}) -join ' '")"
done
restart
echo ALLDONE
