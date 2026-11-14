#!/usr/bin/env bash
# ed.sh start|stop|game|py <file>
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1
export UE_SKIP_UBT_SDK_SETUP=1   # Lead, Nov 13: launches otherwise queue behind the machine-wide UBT mutex (ValidatePlatforms); not for builds
case $1 in
 stop) powershell -File Tools/td-play-mcp.ps1 -Port ${TD_PORT:-21031} -Stop | tail -1; powershell -File Tools/td-play-mcp.ps1 -Port 21032 -Stop | tail -1; sleep 4;;
 start) bash $0 stop
   # Lead, Nov 14: a stop always reaches a running script: touch Saved/EngVFX/STOP and no further launch happens
   [ -f Saved/EngVFX/STOP ] && { echo "STOP file present: not launching"; exit 1; }
   if [ "$NULLRHI" = "1" ]; then   # Lead, Nov 14: script-only builds run without rendering (no slot needed)
     powershell -File Tools/td-play-mcp.ps1 -Port ${TD_PORT:-21031} -Editor -ExtraArgs "-nullrhi" | grep -c "SESSION READY"
     for i in $(seq 1 40); do python ArtSource/EngVFX/scripts/run_editor_py.py ${TD_PORT:-21031} ArtSource/EngVFX/scripts/ping.py 2>/dev/null | grep -q pong && break; sleep 5; done
     exit 0
   fi
   # round 10 (Lead): wait for a slot, launch, recount 10 s later; if we pushed the machine over two, back off and retry
   for t in 1 2 3 4 5 6; do
     bash ArtSource/EngVFX/rounds/r8/waitslot.sh || exit 1
     powershell -File Tools/td-play-mcp.ps1 -Port ${TD_PORT:-21031} -Editor -Offscreen | grep -c "SESSION READY"
     sleep 10
     n=$(powershell -NoProfile -Command "@(Get-CimInstance Win32_Process | ? { (\$_.Name -like 'UnrealEditor*' -or \$_.Name -like 'unrealGameJam*') -and \$_.CommandLine -notmatch 'nullrhi' -and \$_.CommandLine -notmatch 'CrashReport' }).Count" | tr -d "\r")
     [ "${n:-9}" -le 2 ] && break
     echo "over the cap ($n rendering): backing off"; bash $0 stop; sleep 60
   done
   for i in $(seq 1 40); do python ArtSource/EngVFX/scripts/run_editor_py.py ${TD_PORT:-21031} ArtSource/EngVFX/scripts/ping.py 2>/dev/null | grep -q pong && break; sleep 5; done;;
 game) bash $0 stop; powershell -File Tools/td-play-mcp.ps1 -Port 21032 -Offscreen -ResX 1600 -ResY 900 | grep -c "SESSION READY";;
 py) python ArtSource/EngVFX/scripts/run_editor_py.py ${TD_PORT:-21031} "$2";;
esac
