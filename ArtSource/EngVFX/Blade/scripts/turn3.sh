#!/usr/bin/env bash
# #163 round 3 render turn (ONLY while holding the render-slot lock; the caller releases it).
#   A: editor: rebuild the two beam materials + NS_TD_Blade_Aura.   B: -game (TD.Stage only travels in -game): the dark
#   interior, swings, retract / ignite, a hit on a grunt, air slam, block against a trooper. Chase camera frames.
cd "$(dirname "$0")/../../../.."
export MSYS_NO_PATHCONV=1
S=ArtSource/EngVFX/Blade/scripts; PORT=21630; OUT="$1"; mkdir -p "$OUT" Saved/EngVFX
if [ "$2" != "nobuild" ]; then
  powershell -ExecutionPolicy Bypass -File Tools/td-play-mcp.ps1 -Editor -Port $PORT | tail -1
  for i in $(seq 1 90); do python ArtSource/EngVFX/scripts/run_editor_py.py $PORT ArtSource/EngVFX/scripts/ping.py 2>/dev/null | grep -q pong && break; sleep 3; done
  (echo "DO_MESH = False"; echo "ONLY_MATS = ['M_TD_BladeBeamCore_V2', 'M_TD_BladeBeamGlow_V2']"; cat $S/build_blade_mats.py) > Saved/EngVFX/_m.py
  python ArtSource/EngVFX/scripts/run_editor_py.py $PORT Saved/EngVFX/_m.py | tail -3
  bash $S/blade_build.sh ${SYSTEMS:-NS_TD_Blade_Aura} | tail -4
  powershell -ExecutionPolicy Bypass -File Tools/td-play-mcp.ps1 -Stop -Port $PORT | tail -1
  # the stop is asynchronous: wait until no process of ours holds the port any more
  for i in $(seq 1 30); do powershell -Command "if (Get-CimInstance Win32_Process -Filter \"Name like 'UnrealEditor%'\" | ? { \$_.CommandLine -match 'ModelContextProtocolPort=$PORT' }) { exit 1 }" && break; sleep 2; done
  echo "A done $(date +%H:%M:%S)"; sleep 3
fi
powershell -ExecutionPolicy Bypass -File Tools/td-play-mcp.ps1 -Port $PORT -Map /Game/TriadDrift/Maps/L_Galaxy -MaxSeconds 600 -ResX 1280 -ResY 720 -Offscreen | tail -1
P="python ArtSource/EngVFX/scripts/play.py $PORT $OUT"
B="con:TD.Ground.Blade"
$P sleep:8 con:TD.Stage\ DominionInterior sleep:22 con:TD.God con:TD.Ground.ThirdPerson\ 1 sleep:3 cap:dark_idle con:TD.Ground.HandBlade \
   con:TD.Power.TimeScale\ 0.25 "$B light" sleep:0.05 cap:dark_l1a cap:dark_l1b "$B light" sleep:0.05 cap:dark_l2a cap:dark_l2b sleep:1 "$B heavy" sleep:0.2 cap:dark_hva cap:dark_hvb \
   sleep:1.5 "$B retract" sleep:0.05 cap:dark_retract sleep:1.5 "$B ignite" cap:dark_ignite_a cap:dark_ignite_b sleep:1 \
   con:TD.Ground.Spawn\ Grunt\ 260 sleep:1.5 con:TD.Ground.FaceNearest "$B light" sleep:0.3 cap:dark_hit_a cap:dark_hit_b sleep:1 \
   "$B jump" sleep:1.2 "$B slam" sleep:0.6 cap:dark_slam_a cap:dark_slam_b con:TD.Power.TimeScale\ 1 sleep:1 con:TD.Dev.DestroySpawned \
   con:TD.Ground.Spawn\ Trooper\ 900 sleep:1 con:TD.Ground.FaceNearest "$B block" con:TD.Power.TimeScale\ 0.35 \
   cap:dark_blk_00 sleep:0.3 cap:dark_blk_01 sleep:0.3 cap:dark_blk_02 sleep:0.3 cap:dark_blk_03 sleep:0.3 cap:dark_blk_04 sleep:0.3 cap:dark_blk_05 \
   "$B unblock" sleep:0.4 "$B block" cap:dark_blk_06 sleep:0.25 cap:dark_blk_07 sleep:0.25 cap:dark_blk_08 sleep:0.25 cap:dark_blk_09 \
   "$B unblock" sleep:0.4 "$B block" cap:dark_blk_10 sleep:0.25 cap:dark_blk_11 sleep:0.25 cap:dark_blk_12 "$B unblock" con:TD.Power.TimeScale\ 1 \
   con:TD.Ground.Where 2>&1 | tail -3
echo "B done $(date +%H:%M:%S)"
grep -a "TD.Ground.Blade\|HandBlade\|Deflect\|Stage " Saved/Logs/td-play-mcp_$PORT.log | tail -25 | cut -c1-220
powershell -ExecutionPolicy Bypass -File Tools/td-play-mcp.ps1 -Stop -Port $PORT | tail -1
