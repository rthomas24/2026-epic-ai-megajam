#!/usr/bin/env bash
# Triad Drift ENG-37: drive the preview Play-In-Editor session of the editor on port 8017. usage:
#   g.sh py "CMD = ('row', [...], 2600, 900)"     run game_spawn.py with that CMD
#   g.sh con "slomo 0.1"                           console command
#   g.sh cap <name> [maxres]                       capture the game viewport to Saved/EngVFX/cap/<name>.png
HERE="$(cd "$(dirname "$0")" && pwd)"; ROOT="$(cd "$HERE/../../.." && pwd)"; PORT="${TD_PORT:-8017}"
mkdir -p "$ROOT/Saved/EngVFX/cap"
case "$1" in
  py) (echo "$2"; cat "$HERE/game_spawn.py") > "$ROOT/Saved/EngVFX/_g.py"; python "$HERE/run_editor_py.py" "$PORT" "$ROOT/Saved/EngVFX/_g.py" ;;
  con) python - "$PORT" "$2" <<'PY'
import sys, json
sys.path.insert(0, 'Tools')  # run from the repo root
import td_mcp
c = td_mcp.McpClient('http://127.0.0.1:%s/mcp' % sys.argv[1], 120); c.connect()
r = c.tool('call_tool', {'toolset_name': 'ConsoleToolset.ConsoleToolset', 'tool_name': 'RunConsoleCommand', 'arguments': {'command': sys.argv[2]}})
print(json.dumps(r)[:200])
PY
  ;;
  cap) python "$HERE/cap.py" "$PORT" "$ROOT/Saved/EngVFX/cap/$2.png" "${3:-1600}" ;;
esac
