#!/usr/bin/env bash
# Hyperdrive look (T7): run the builders in an editor session started with
#   powershell -File Tools/td-play-mcp.ps1 -Editor -Offscreen -Port <port>
# usage: run.sh <port> mats | build [NS_TD_Hyperdrive_Spool,...] | py <file>
HERE="$(cd "$(dirname "$0")" && pwd)"; ROOT="$(cd "$HERE/../../.." && pwd)"; cd "$ROOT"
export MSYS_NO_PATHCONV=1
PORT="$1"; TMP=Saved/EngVFX; mkdir -p $TMP
RUN="python ArtSource/EngVFX/scripts/run_editor_py.py $PORT"
case $2 in
  mats) $RUN ArtSource/EngVFX/hyperdrive/build_hyper_materials.py ;;
  build)
    { echo "ONLY = ['__none__']"; [ -n "$3" ] && echo "ONLY_H = '$3'.split(',')"; cat ArtSource/EngVFX/scripts/build_tier1.py; echo; cat ArtSource/EngVFX/hyperdrive/build_hyperdrive.py; } > $TMP/_hyper.py
    $RUN $TMP/_hyper.py ;;
  py) $RUN "$3" ;;
esac
