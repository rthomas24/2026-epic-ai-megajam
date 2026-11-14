#!/usr/bin/env bash
# ed.sh start|stop|game|py <file>
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1
export UE_SKIP_UBT_SDK_SETUP=1   # Lead, Nov 13: launches otherwise queue behind the machine-wide UBT mutex (ValidatePlatforms); not for builds
case $1 in
 stop) powershell -File Tools/td-play-mcp.ps1 -Port ${TD_PORT:-21031} -Stop | tail -1; powershell -File Tools/td-play-mcp.ps1 -Port 21032 -Stop | tail -1; sleep 4;;
 start) bash $0 stop; powershell -File Tools/td-play-mcp.ps1 -Port ${TD_PORT:-21031} -Editor -Offscreen | grep -c "SESSION READY"
   for i in $(seq 1 40); do python ArtSource/EngVFX/scripts/run_editor_py.py ${TD_PORT:-21031} ArtSource/EngVFX/scripts/ping.py 2>/dev/null | grep -q pong && break; sleep 5; done;;
 game) bash $0 stop; powershell -File Tools/td-play-mcp.ps1 -Port 21032 -Offscreen -ResX 1600 -ResY 900 | grep -c "SESSION READY";;
 py) python ArtSource/EngVFX/scripts/run_editor_py.py ${TD_PORT:-21031} "$2";;
esac
