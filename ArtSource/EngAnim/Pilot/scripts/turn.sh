#!/bin/bash
# One short capture turn (slot already held by the caller): guard, launch 1280x720, Verdance on foot, capture the
# given move groups, log errors, close our own game. Usage: turn.sh <outdir> "<views>|<moves>" ["<views>|<moves>" ...]
set -u
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/anim
OUT=$1; shift
mkdir -p "$OUT"
for i in $(seq 1 12); do bash /c/Users/ryant/Documents/myRepos/game-jam/Builds/render-slot/can_launch.sh && break; sleep 10; done
powershell -NoProfile -Command "Set-Location C:\Users\ryant\Documents\myRepos\game-jam\.wt\anim; .\Tools\td-play-mcp.ps1 -Port 21085 -Map /Game/TriadDrift/Maps/L_Planet_Verdance -MaxSeconds 600 -Offscreen" | tail -1
python - <<'PY'
import sys,time
sys.path.insert(0,"Tools"); import td_mcp
c=td_mcp.McpClient("http://127.0.0.1:21085/mcp",180); c.connect()
def cmd(s): c.tool("call_tool",{"toolset_name":"ConsoleToolset.ConsoleToolset","tool_name":"RunConsoleCommand","arguments":{"command":s}})
cmd("TD.Stage VerdanceOnFoot"); time.sleep(28); cmd("TD.God"); cmd("TD.Ground.ThirdPerson 1"); time.sleep(1)
PY
cd ArtSource/EngAnim/Pilot/scripts
for g in "$@"; do
  V=${g%%|*}; M=${g#*|}
  if [ "$V" = "CREW" ]; then    # held crew frames: ../../PilotFixes/scripts/crew_capture.py <out> <tag> <Type>:<Clip>:<t01>,...
    TD_PORT=21085 timeout 150 python ../../PilotFixes/scripts/crew_capture.py "/c/Users/ryant/Documents/myRepos/game-jam/.wt/anim/$OUT" crew "$M" | tail -4
    continue
  fi
  if [ "$V" = "FIGHT" ]; then   # crew in a real fight: ../../PilotFixes/scripts/fight_capture.py <out> <Type>[:dist],...
    TD_PORT=21085 TD_FIGHT_S=10 TD_CMDS_Heavy="3.0:TD.Ground.BreakGuard;3.1:@" TD_CMDS_Warden="2.5:TD.Ground.FaceNearest;2.6:TD.Key LeftMouseButton 0.5;3.2:@" timeout 200 python ../../PilotFixes/scripts/fight_capture.py "/c/Users/ryant/Documents/myRepos/game-jam/.wt/anim/$OUT" "$M" | tail -12
    continue
  fi
  TD_PORT=21085 TD_PASS=1 TD_VIEWS=$V timeout 200 python pilot_capture.py "/c/Users/ryant/Documents/myRepos/game-jam/.wt/anim/$OUT" "$M" | tail -20
done
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/anim
python ArtSource/EngAnim/Pilot/scripts/foot_slide.py Saved/Logs/td-play-mcp_21085.log > "$OUT/foot_slide.txt" 2>&1
echo "--- log errors / crashes:"; grep -a -E "Error:|Fatal|Assertion|Ensure" Saved/Logs/td-play-mcp_21085.log | grep -v -E "FError that|LogPython" | tail -10
powershell -NoProfile -Command "Set-Location C:\Users\ryant\Documents\myRepos\game-jam\.wt\anim; .\Tools\td-play-mcp.ps1 -Stop -Port 21085" | tail -1
