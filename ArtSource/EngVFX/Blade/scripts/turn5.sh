#!/usr/bin/env bash
# #163 short render turn (round 6: walk/run clean, ignite, hit, deflects, cast light) (new slot rules, Nov 14 13:25): ONLY while holding a render slot; the caller releases it.
#   A (skip with "nobuild"): editor, rebuild NS_TD_Blade_Aura (+ SYSTEMS).  B: -game 1280x720, Dominion interior:
#   idle from the chase camera, light 1-2, ignite, a hit on a grunt, then block taps against a trooper for deflects.
cd "$(dirname "$0")/../../../.."
export MSYS_NO_PATHCONV=1
S=ArtSource/EngVFX/Blade/scripts; PORT=21630; OUT="$1"; mkdir -p "$OUT" Saved/EngVFX
guard() { for i in $(seq 1 12); do bash ../../Builds/render-slot/can_launch.sh | tail -1 | grep -q GO && return 0; sleep 10; done; echo "GUARD REFUSED"; return 1; }
gone() { for i in $(seq 1 30); do powershell -Command "if (Get-CimInstance Win32_Process -Filter \"Name like 'UnrealEditor%'\" | ? { \$_.CommandLine -match 'ModelContextProtocolPort=$PORT' }) { exit 1 }" && return 0; sleep 2; done; }
if [ "$2" != "nobuild" ]; then
  guard || exit 1
  powershell -ExecutionPolicy Bypass -File Tools/td-play-mcp.ps1 -Editor -Port $PORT | tail -1
  for i in $(seq 1 90); do python ArtSource/EngVFX/scripts/run_editor_py.py $PORT ArtSource/EngVFX/scripts/ping.py 2>/dev/null | grep -q pong && break; sleep 3; done
  bash $S/blade_build.sh ${SYSTEMS:-NS_TD_Blade_Aura} | tail -6
  powershell -ExecutionPolicy Bypass -File Tools/td-play-mcp.ps1 -Stop -Port $PORT | tail -1; gone
  echo "A done $(date +%H:%M:%S)"
fi
guard || exit 1
powershell -ExecutionPolicy Bypass -File Tools/td-play-mcp.ps1 -Port $PORT -Map /Game/TriadDrift/Maps/L_Galaxy -MaxSeconds 600 -ResX 1280 -ResY 720 | tail -1
P="python ArtSource/EngVFX/scripts/play.py $PORT $OUT"
B="con:TD.Ground.Blade"
TS="con:TD.Power.TimeScale"
$P sleep:8 con:TD.Stage\ DominionInterior sleep:22 con:TD.God con:TD.Ground.ThirdPerson\ 1 sleep:3 cap:idle    "$B walk 2.5" sleep:1.2 cap:walk_a sleep:0.5 cap:walk_b sleep:1.2 "$B run 2.5" sleep:1.2 cap:run_a sleep:0.5 cap:run_b sleep:1.5    "$TS 0.25" "$B retract" sleep:1.2 "$B ignite" sleep:0.05 cap:ignite_a sleep:0.1 cap:ignite_b sleep:0.15 cap:ignite_c "$TS 1" sleep:1    "$TS 0.25" "$B light" sleep:0.05 cap:l1 sleep:0.6 "$B light" sleep:0.05 cap:l2 "$TS 1" sleep:1    con:TD.Ground.Spawn\ Grunt\ 200 sleep:1.2 con:TD.Ground.FaceNearest "$TS 0.25" "$B light" sleep:0.25 cap:hit_a sleep:0.1 cap:hit_b "$TS 1" sleep:0.5    con:TD.Dev.DestroySpawned con:TD.Ground.Spawn\ Trooper\ 650 sleep:2 con:TD.Ground.FaceNearest "$B block"    cap:blk_00 sleep:0.4 cap:blk_01 sleep:0.4 cap:blk_02 sleep:0.4 cap:blk_03 sleep:0.4 cap:blk_04 sleep:0.4 cap:blk_05 sleep:0.4 cap:blk_06 sleep:0.4 cap:blk_07    "$B unblock" sleep:0.3 "$B block" cap:blk_08 sleep:0.3 cap:blk_09 sleep:0.3 cap:blk_10 sleep:0.3 cap:blk_11    "$B unblock" sleep:0.3 "$B block" cap:blk_12 sleep:0.3 cap:blk_13 sleep:0.3 cap:blk_14 sleep:0.3 cap:blk_15 "$B unblock"    con:TD.Dev.DestroySpawned con:TD.Ground.SkipTo\ Core sleep:6 cap:core_idle "$B block" sleep:0.5 cap:core_block "$B unblock" con:TD.Ground.ThirdPerson 0 sleep:1.5 cap:fp_idle "$TS 0.25" "$B light" sleep:0.1 cap:fp_light "$TS 1" 2>&1 | tail -2
echo "B done $(date +%H:%M:%S)"
grep -a "TD.Ground.Blade\|eflect\|Stage " Saved/Logs/td-play-mcp_$PORT.log | tail -12 | cut -c1-200
powershell -ExecutionPolicy Bypass -File Tools/td-play-mcp.ps1 -Stop -Port $PORT | tail -1; gone
