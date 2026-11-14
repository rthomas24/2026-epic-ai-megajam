#!/usr/bin/env bash
# chunks.sh <system> <total emitters> [per=5]: build a big tier-2 system in chunks, one fresh editor per chunk (Lead, Nov 13:
# the toolset keeps ~1 GB per emitter until the editor quits). The editor is stopped after the last chunk.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; PORT=${TD_PORT:-21031}
SYSN=$1; N=$2; PER=${3:-5}
for ((s=0; s<N; s+=PER)); do
  e=$((s+PER)); bash ArtSource/EngVFX/rounds/r8/ed.sh start >/dev/null
  (echo "ONLY = ['__none__']"; cat ArtSource/EngVFX/scripts/build_tier1.py; echo; echo "ONLY2 = ['=$SYSN']"; echo "CHUNK = ($s, $e)"; cat ArtSource/EngVFX/scripts/build_tier2.py) > Saved/EngVFX/_chunk.py
  python ArtSource/EngVFX/scripts/run_editor_py.py $PORT Saved/EngVFX/_chunk.py | grep -E "$SYSN|FAIL|rror"
  bash ArtSource/EngVFX/rounds/r8/mem.sh | tr '\n' ' '; echo
  bash ArtSource/EngVFX/rounds/r8/ed.sh stop >/dev/null
done
echo CHUNKS DONE
