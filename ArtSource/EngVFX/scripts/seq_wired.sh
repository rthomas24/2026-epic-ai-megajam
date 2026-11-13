#!/usr/bin/env bash
# Triad Drift ENG-37: the in-play capture run used for the wired before/after sheets (game session, default port 8018).
# usage: seq_wired.sh <tag>   -> Saved/EngVFX/job1/<tag>_*.png
HERE="$(cd "$(dirname "$0")" && pwd)"; ROOT="$(cd "$HERE/../../.." && pwd)"; cd "$ROOT"
T="$1"; P="${TD_PORT:-8018}"; O=Saved/EngVFX/job1
python ArtSource/EngVFX/scripts/play.py $P $O "con:TD.God 1" "con:TD.Stage Wave1" sleep:5 "con:TD.Squad.Passive 1" \
 "con:TD.Squad.Switch Rift" sleep:2 "con:TD.Input.Keys LeftMouseButton 3" sleep:1.3 "con:slomo 0.05" cap:${T}_rift "con:slomo 1" sleep:2 \
 "con:TD.Squad.Switch Lancer" sleep:2 "con:TD.Input.Keys LeftMouseButton 3" sleep:1.3 "con:slomo 0.05" cap:${T}_lancer "con:slomo 1" sleep:2 \
 "con:TD.Squad.Switch Bastion" sleep:2 "con:TD.Input.Keys LeftMouseButton 3" sleep:1.3 "con:slomo 0.05" cap:${T}_bastion "con:slomo 1" sleep:2 \
 "con:TD.Squad.Switch Lancer" sleep:2 "con:TD.World.Hold 60 0" sleep:2.5 "con:TD.World.SpawnEnemy WallGunship 6000 0" sleep:0.5 "con:TD.Input.Keys LeftMouseButton 2" sleep:1.0 "con:slomo 0.05" cap:${T}_hullhit1 sleep:0.6 cap:${T}_hullhit2 "con:slomo 1" sleep:1.5  "con:slomo 0.25" "con:TD.World.KillNearest 70" sleep:0.4 cap:${T}_boom1 sleep:0.5 cap:${T}_boom2 sleep:0.5 cap:${T}_boom3 sleep:0.6 cap:${T}_boom4 sleep:1.0 cap:${T}_boom5 sleep:1.5 cap:${T}_boom6 sleep:2 cap:${T}_boom7 "con:slomo 1"  "con:TD.Squad.Passive 0" "con:TD.Spawn ShardFighter 4 9000" sleep:3 "con:slomo 0.05" cap:${T}_enemy1 "con:slomo 1" sleep:1.5 "con:slomo 0.05" cap:${T}_enemy2 "con:slomo 1" sleep:1.5 "con:slomo 0.05" cap:${T}_enemy3 "con:slomo 1" "con:TD.World.Hold 0"
