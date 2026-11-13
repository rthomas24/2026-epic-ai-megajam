#!/usr/bin/env bash
# pk_play.sh <tag> [nostart]: T2's real pickups wearing DA_Pickup_Visuals, in Play-In-Editor (editor on 8017), Waves1to2,
# the ship held: the four dropped 200 m ahead 30 m apart, then 70 m ahead 8 m apart (the magnet takes anything inside
# 60 m), then each one dropped 30 m ahead, pulled in and collected (slow motion). Captures: Saved/EngVFX/r8/<tag>_*.png
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1
G=ArtSource/EngVFX/rounds/r8/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py 8017 Saved/EngVFX/r8"; T=${1:-c}
if [ "$2" != "nostart" ]; then bash $G "('pie_start',)" >/dev/null; sleep 25; $P con:"TD.Stage Waves1to2" sleep:12 >/dev/null; fi
$P con:"TD.World.Hold 900 0" con:"god" >/dev/null
for d in "200 30" "70 8"; do set -- $d
  $P con:"TD.Pickups.Spawn Hull $1 -$(( $2 * 3 / 2 ))" con:"TD.Pickups.Spawn Shield $1 -$(( $2 / 2 ))" con:"TD.Pickups.Spawn Power $1 $(( $2 / 2 ))" \
     con:"TD.Pickups.Spawn Overcharge $1 $(( $2 * 3 / 2 ))" sleep:1.5 cap:${T}_play_$1 sleep:0.6 cap:${T}_play_$1b >/dev/null
  $P con:"TD.Take" sleep:14 con:"TD.World.Hold 900 0" con:"god" >/dev/null   # a CUT clears the pickups
done
for k in Hull Shield Power Overcharge; do
  $P con:"TD.Pickups.Spawn $k 30" sleep:0.25 con:"slomo 0.08" sleep:0.5 cap:${T}_col_${k}_1 sleep:1.0 cap:${T}_col_${k}_2 sleep:1.5 cap:${T}_col_${k}_3 con:"slomo 1" sleep:2 >/dev/null
done
