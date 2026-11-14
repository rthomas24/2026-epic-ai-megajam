#!/bin/bash
# Animation builder round 2 session (needs the render lock held): launch, set up, capture, close.
set -u
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/anim
OUT=Saved/EngAnim/pilot_frames/r2
mkdir -p $OUT
powershell -NoProfile -Command "Set-Location C:\Users\ryant\Documents\myRepos\game-jam\.wt\anim; .\Tools\td-play-mcp.ps1 -Port 21085 -Map /Game/TriadDrift/Maps/L_Planet_Verdance -MaxSeconds 840" | tail -1
python - <<'PY'
import sys,time
sys.path.insert(0,"Tools"); import td_mcp
c=td_mcp.McpClient("http://127.0.0.1:21085/mcp",180); c.connect()
def cmd(s): c.tool("call_tool",{"toolset_name":"ConsoleToolset.ConsoleToolset","tool_name":"RunConsoleCommand","arguments":{"command":s}})
cmd("TD.Stage VerdanceOnFoot"); time.sleep(20); cmd("TD.God"); cmd("TD.Ground.ThirdPerson 1"); time.sleep(1)
PY
cd ArtSource/EngAnim/Pilot/scripts
export TD_PORT=21085 TD_PASS=1
TD_VIEWS=chase,side,front timeout 200 python pilot_capture.py ../../../../$OUT walk_slow,start,stop2,guard_strafe,guard_back,run
TD_VIEWS=chase,side timeout 120 python pilot_capture.py ../../../../$OUT heavy_follow,dash,slide
TD_VIEWS=side,front timeout 120 python pilot_capture.py ../../../../$OUT slope_a,slope_a_off,slope_b,slope_b_off,home
TD_VIEWS=chase,side,front timeout 160 python pilot_capture.py ../../../../$OUT combat_idle,deflect,flinch
cd ../../../..
grep -a "TD.Pilot.Drive\|God mode\|Deflect\|deflect" Saved/Logs/td-play-mcp_21085.log | tail -8
powershell -NoProfile -Command "Set-Location C:\Users\ryant\Documents\myRepos\game-jam\.wt\anim; .\Tools\td-play-mcp.ps1 -Stop -Port 21085" | tail -1
O=/c/Users/ryant/Documents/myRepos/game-jam/Builds/render-slot/LOCK/owner.txt
if grep -q "^anim" $O 2>/dev/null; then rm -r /c/Users/ryant/Documents/myRepos/game-jam/Builds/render-slot/LOCK && echo "lock released $(date +%H:%M:%S)"; fi
