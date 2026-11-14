#!/usr/bin/env bash
# ed.sh start|stop|game|py <file>|lock <what>|unlock
# Shared rendering slot (Lead, Nov 14): a rendering launch needs the lock directory below with our owner.txt; take it
# with 'lock' (mkdir, retried every 60 s), release it with 'unlock' (only our own). Headless (NULLRHI=1) needs no lock.
RS=/c/Users/ryant/Documents/myRepos/game-jam/Builds/render-slot
mine_slot() { d=$(cat Saved/EngVFX/myslot 2>/dev/null); [ -n "$d" ] && grep -q '^vfx ' "$RS/$d/owner.txt" 2>/dev/null; }
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
   mine_slot || { echo "no render slot of ours: not launching"; exit 1; }
   # round 10 (Lead): wait for a slot, launch, recount 10 s later; if we pushed the machine over two, back off and retry
   for t in 1 2 3 4 5 6; do
     # 10h: re-check the lock before EVERY launch (a retry once launched after our lock had been released)
     mine_slot || { echo "no render slot of ours: not launching"; exit 1; }
     [ -f Saved/EngVFX/STOP ] && { echo "STOP file present: not launching"; exit 1; }
     # Lead, Nov 14 13:25: the shared guard right before the launch (video memory, renderer count, free RAM), up to 2 min
     ok=0; for w in $(seq 1 12); do bash $RS/can_launch.sh >/dev/null && { ok=1; break; }; sleep 10; done
     [ $ok = 1 ] || { echo "can_launch refused for 2 minutes"; exit 1; }
     powershell -File Tools/td-play-mcp.ps1 -Port ${TD_PORT:-21031} -Editor -Offscreen -ResX 1280 -ResY 720 | grep -c "SESSION READY"
     break
   done
   for i in $(seq 1 40); do python ArtSource/EngVFX/scripts/run_editor_py.py ${TD_PORT:-21031} ArtSource/EngVFX/scripts/ping.py 2>/dev/null | grep -q pong && break; sleep 5; done;;
 lock) # Lead, Nov 14 13:25: three slots (LOCK, LOCK2, LOCK3); the top THREE lines of NEXT.txt may go. Poll 10 s.
   for i in $(seq 1 1080); do
     [ -f Saved/EngVFX/STOP ] && { echo "STOP file present: not locking"; exit 1; }
     for d in LOCK LOCK2 LOCK3; do grep -q '^vfx ' "$RS/$d/owner.txt" 2>/dev/null && { echo "vfx $(date +%H:%M) ${2:-effects capture}" > "$RS/$d/owner.txt"; echo "$d" > Saved/EngVFX/myslot; echo "SLOT $d already ours"; exit 0; }; done
     pos=$(grep -v '^[[:space:]]*$' "$RS/NEXT.txt" 2>/dev/null | grep -n '^vfx' | head -1 | cut -d: -f1)
     if [ -z "$pos" ] && [ -f "$RS/NEXT.txt" ]; then printf 'vfx %s
' "${2:-effects capture}" >> "$RS/NEXT.txt"; echo "queued at the bottom of NEXT.txt"; sleep 10; continue; fi
     if [ -z "$pos" ] || [ "$pos" -le 3 ]; then
       for d in LOCK LOCK2 LOCK3; do
         if mkdir "$RS/$d" 2>/dev/null; then
           echo "vfx $(date +%H:%M) ${2:-effects capture} (10)" > "$RS/$d/owner.txt"; echo "$d" > Saved/EngVFX/myslot
           [ -f "$RS/NEXT.txt" ] && { grep -v '^vfx' "$RS/NEXT.txt" > "$RS/NEXT.tmp.vfx"; mv "$RS/NEXT.tmp.vfx" "$RS/NEXT.txt"; }
           echo "SLOT $d taken $(date +%H:%M)"; exit 0
         fi
       done
     fi
     sleep 10
   done; echo "no slot after 3 h"; exit 1;;
 unlock) bash $0 stop >/dev/null; d=$(cat Saved/EngVFX/myslot 2>/dev/null); [ -n "$d" ] && grep -q '^vfx ' "$RS/$d/owner.txt" 2>/dev/null && rm -r "$RS/$d" && echo "SLOT $d released $(date +%H:%M)"; rm -f Saved/EngVFX/myslot;;
 game) mine_slot || { echo "no render slot of ours: not launching"; exit 1; }
   bash $0 stop; powershell -File Tools/td-play-mcp.ps1 -Port 21032 -Offscreen -ResX 1600 -ResY 900 | grep -c "SESSION READY";;
 py) python ArtSource/EngVFX/scripts/run_editor_py.py ${TD_PORT:-21031} "$2";;
esac
