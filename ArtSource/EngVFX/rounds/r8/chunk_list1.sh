#!/usr/bin/env bash
# chunk_list1.sh <tier-1 system> "<s:e> <s:e> ...": rebuild the given emitter ranges, one fresh editor each (see chunks.sh).
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; PORT=${TD_PORT:-21031}
for r in $2; do s=${r%:*}; e=${r#*:}
  bash ArtSource/EngVFX/rounds/r8/ed.sh start >/dev/null
  (echo "ONLY = ['$1']"; echo "CHUNK1 = ($s, $e)"; cat ArtSource/EngVFX/scripts/build_tier1.py) > Saved/EngVFX/_chunk.py
  python ArtSource/EngVFX/scripts/run_editor_py.py $PORT Saved/EngVFX/_chunk.py 2>&1 | grep -E "$1|FAIL|McpError" | cut -c1-200
  bash ArtSource/EngVFX/rounds/r8/mem.sh | tr '\n' ' '; echo; tail -1 Saved/Logs/td-play-mcp_$PORT.log | cut -c1-200
  bash ArtSource/EngVFX/rounds/r8/ed.sh stop >/dev/null
done
echo LIST DONE
