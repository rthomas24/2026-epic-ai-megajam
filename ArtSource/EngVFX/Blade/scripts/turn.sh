#!/usr/bin/env bash
# #163: one render turn, end to end (ONLY while holding the render-slot lock; the caller releases it).
#   turn.sh <outdir> [build]     build = materials + mesh + every blade system + wiring before the captures
HERE=ArtSource/EngVFX/Blade/scripts; cd "$(dirname "$0")/../../../.."
export MSYS_NO_PATHCONV=1
PORT=21630; OUT="$1"; mkdir -p "$OUT" Saved/EngVFX
CAP="python $HERE/blade_cap.py $PORT $OUT"
powershell -ExecutionPolicy Bypass -File Tools/td-play-mcp.ps1 -Editor -Port $PORT -Map /Game/TriadDrift/Maps/L_Dominion_Interior | tail -1
for i in $(seq 1 90); do python ArtSource/EngVFX/scripts/run_editor_py.py $PORT ArtSource/EngVFX/scripts/ping.py 2>/dev/null | grep -q pong && break; sleep 4; done
echo "editor python up $(date +%H:%M:%S)"
if [ "$2" = "build" ]; then
  python ArtSource/EngVFX/scripts/run_editor_py.py $PORT $HERE/build_blade_mats.py | tail -8
  bash $HERE/blade_build.sh ${SYSTEMS:-NS_TD_Blade_Aura,NS_TD_Blade_Ignite,NS_TD_Blade_Retract,NS_TD_Blade_Hit,NS_TD_Blade_Deflect_V2,NS_TD_Blade_PerfectDeflect_V2,NS_TD_Blade_AirSlam_V2} | tail -12
  python ArtSource/EngVFX/scripts/run_editor_py.py $PORT $HERE/blade_wire.py | tail -4
  echo "built $(date +%H:%M:%S)"
fi
# daylight course (the map's test course): chase camera
$CAP act:pie_start sleep:10 con:TD.God con:TD.Ground.ThirdPerson\ 1 sleep:2 cap:day_idle con:TD.Ground.HandBlade close:day_c_idle:230:70 close:day_c_idle_side:260:100 \
     con:slomo\ 0.25 act:light burst:day_light1:4:0.15 sleep:1.5 act:light burst:day_light2:3:0.15 sleep:1.5 act:light burst:day_light3:3:0.2 \
     sleep:2 act:heavy burst:day_heavy:3:0.2 sleep:2 act:light close:day_c_swing:300:80 sleep:2 act:jump sleep:0.6 act:slam burst:day_slam:3:0.25 \
     con:slomo\ 1 sleep:1 act:block sleep:0.4 cap:day_block act:unblock 2>&1 | grep -v "^act:\|^con:" | tail -4
echo "day done $(date +%H:%M:%S)"
# dark: the Dominion interior (built at runtime by the stage)
$CAP con:TD.Stage\ DominionInterior sleep:14 con:TD.God con:TD.Ground.ThirdPerson\ 1 sleep:2 cap:dark_idle close:dark_c_idle:200:70 \
     con:slomo\ 0.25 act:light burst:dark_light1:4:0.15 sleep:1.5 act:light burst:dark_light2:3:0.15 sleep:2 act:heavy burst:dark_heavy:3:0.2 \
     con:slomo\ 1 act:info act:pie_stop sleep:2 2>&1 | grep -v "^con:" | tail -4
echo "dark done $(date +%H:%M:%S)"
powershell -ExecutionPolicy Bypass -File Tools/td-play-mcp.ps1 -Stop -Port $PORT
