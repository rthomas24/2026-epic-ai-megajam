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
powershell -ExecutionPolicy Bypass -File Tools/td-play-mcp.ps1 -Port $PORT -Map /Game/TriadDrift/Maps/L_Galaxy -MaxSeconds 600 -ResX 1280 -ResY 720 -Offscreen | tail -1
P="python ArtSource/EngVFX/scripts/play.py $PORT $OUT"
B="con:TD.Ground.Blade"
TS="con:TD.Power.TimeScale"
$P sleep:8 con:TD.Stage\ DominionInterior sleep:22 con:TD.God con:TD.Ground.ThirdPerson\ 1 sleep:3 "$B pilot rift" sleep:0.5 con:TD.Ground.Spawn\ Grunt\ 110 sleep:1.2 con:TD.Ground.FaceNearest sleep:0.4 cap:rift_3p con:TD.Ground.ThirdPerson\ 0 sleep:1.0 con:TD.Ground.FaceNearest sleep:0.3 cap:rift_fp "$TS 0.3" "$B light" sleep:0.25 cap:rift_fp_hit "$TS 1" con:TD.Ground.ThirdPerson\ 1 con:TD.Dev.DestroySpawned sleep:1.0 "$B pilot lancer" sleep:0.5 con:TD.Ground.Spawn\ Grunt\ 110 sleep:1.2 con:TD.Ground.FaceNearest sleep:0.4 cap:lancer_3p con:TD.Ground.ThirdPerson\ 0 sleep:1.0 con:TD.Ground.FaceNearest sleep:0.3 cap:lancer_fp "$TS 0.3" "$B light" sleep:0.25 cap:lancer_fp_hit "$TS 1" con:TD.Ground.ThirdPerson\ 1 con:TD.Dev.DestroySpawned sleep:1.0 "$B pilot bastion" sleep:0.5 con:TD.Ground.Spawn\ Grunt\ 110 sleep:1.2 con:TD.Ground.FaceNearest sleep:0.4 cap:bastion_3p con:TD.Ground.ThirdPerson\ 0 sleep:1.0 con:TD.Ground.FaceNearest sleep:0.3 cap:bastion_fp "$TS 0.3" "$B light" sleep:0.25 cap:bastion_fp_hit "$TS 1" con:TD.Ground.ThirdPerson\ 1 con:TD.Dev.DestroySpawned sleep:1.0 2>&1 | tail -2
echo "B done $(date +%H:%M:%S)"
grep -a "TD.Ground.Blade\|eflect\|Stage " Saved/Logs/td-play-mcp_$PORT.log | tail -12 | cut -c1-200
powershell -ExecutionPolicy Bypass -File Tools/td-play-mcp.ps1 -Stop -Port $PORT | tail -1; gone
