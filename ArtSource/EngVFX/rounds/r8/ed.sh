#!/usr/bin/env bash
# ed.sh start|stop|game|py <file>|lock <what>|unlock
# Shared rendering slot (Lead, Nov 14): a rendering launch needs the lock directory below with our owner.txt; take it
# with 'lock' (mkdir, retried every 60 s), release it with 'unlock' (only our own). Headless (NULLRHI=1) needs no lock.
LOCKD=/c/Users/ryant/Documents/myRepos/game-jam/Builds/render-slot/LOCK
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
   grep -q '^vfx ' "$LOCKD/owner.txt" 2>/dev/null || { echo "no render-slot LOCK of ours: not launching"; exit 1; }
   # round 10 (Lead): wait for a slot, launch, recount 10 s later; if we pushed the machine over two, back off and retry
   for t in 1 2 3 4 5 6; do
     # 10h: re-check the lock before EVERY launch (a retry once launched after our lock had been released)
     grep -q '^vfx ' "$LOCKD/owner.txt" 2>/dev/null || { echo "no render-slot LOCK of ours: not launching"; exit 1; }
     [ -f Saved/EngVFX/STOP ] && { echo "STOP file present: not launching"; exit 1; }
     bash ArtSource/EngVFX/rounds/r8/waitslot.sh || exit 1
     powershell -File Tools/td-play-mcp.ps1 -Port ${TD_PORT:-21031} -Editor -Offscreen | grep -c "SESSION READY"
     sleep 10
     n=$(powershell -NoProfile -Command "@(Get-CimInstance Win32_Process | ? { (\$_.Name -like 'UnrealEditor*' -or \$_.Name -like 'unrealGameJam*') -and \$_.CommandLine -notmatch 'nullrhi' -and \$_.CommandLine -notmatch 'CrashReport' }).Count" | tr -d "\r")
     [ "${n:-9}" -le 2 ] && break
     echo "over the cap ($n rendering): backing off"; bash $0 stop; sleep 60
   done
   for i in $(seq 1 40); do python ArtSource/EngVFX/scripts/run_editor_py.py ${TD_PORT:-21031} ArtSource/EngVFX/scripts/ping.py 2>/dev/null | grep -q pong && break; sleep 5; done;;
 lock) if grep -q '^vfx ' "$LOCKD/owner.txt" 2>/dev/null; then echo "vfx $(date +%H:%M) ${2:-effects capture}" > "$LOCKD/owner.txt"; echo "LOCK already ours $(date +%H:%M)"; exit 0; fi
   # Lead, Nov 14 11:56: NEXT.txt is an ordered queue; take the lock only as its top line (or when it is empty), or as the
   # second line when the lock has been free and the top line unchanged for 2 minutes. Remove only our own line. Poll 10 s.
   NEXTF=$(dirname "$LOCKD")/NEXT.txt; top0=""; since=0
   for i in $(seq 1 1080); do
     [ -f Saved/EngVFX/STOP ] && { echo "STOP file present: not locking"; exit 1; }
     top=$(grep -v '^[[:space:]]*$' "$NEXTF" 2>/dev/null | head -1); second=$(grep -v '^[[:space:]]*$' "$NEXTF" 2>/dev/null | sed -n 2p)
     mine=0; case "$top" in vfx*|"") mine=1;; esac
     if [ $mine = 0 ] && [ ! -d "$LOCKD" ]; then
       [ "$top" = "$top0" ] && since=$((since+10)) || { top0="$top"; since=0; }
       case "$second" in vfx*) [ $since -ge 120 ] && mine=1;; esac
     else top0="$top"; since=0; fi
     if [ $mine = 1 ] && mkdir "$LOCKD" 2>/dev/null; then
       echo "vfx $(date +%H:%M) ${2:-effects capture}" > "$LOCKD/owner.txt"
       [ -f "$NEXTF" ] && { grep -v '^vfx' "$NEXTF" > "$NEXTF.tmp.vfx"; mv "$NEXTF.tmp.vfx" "$NEXTF"; }
       echo "LOCK taken $(date +%H:%M)"; exit 0
     fi
     sleep 10
   done; echo "LOCK not free after 3 h"; exit 1;;
 unlock) bash $0 stop >/dev/null; grep -q '^vfx ' "$LOCKD/owner.txt" 2>/dev/null && rm -r "$LOCKD" && echo "LOCK released $(date +%H:%M)";;
 game) grep -q '^vfx ' "$LOCKD/owner.txt" 2>/dev/null || { echo "no render-slot LOCK of ours: not launching"; exit 1; }
   bash $0 stop; powershell -File Tools/td-play-mcp.ps1 -Port 21032 -Offscreen -ResX 1600 -ResY 900 | grep -c "SESSION READY";;
 py) python ArtSource/EngVFX/scripts/run_editor_py.py ${TD_PORT:-21031} "$2";;
esac
