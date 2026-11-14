#!/usr/bin/env bash
# range.sh <tag> [nostart]: enemy readability at range (round 9). PIE on $TD_PORT, Waves1to2 with the wave cleared, the
# ship held; a held ShardFighter, HookInterceptor and WallGunship at 300 m, then the same at 1 km. Captures: Saved/EngVFX/r8/
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1
G=ArtSource/EngVFX/rounds/r8/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py ${TD_PORT:-21031} Saved/EngVFX/r8"; T=${1:-a}
if [ "$2" != "nostart" ]; then bash $G "('pie_start',)" >/dev/null; sleep 25; $P con:"TD.Stage Waves1to2" sleep:12 >/dev/null; fi
$P con:"god" con:"TD.KillAll" sleep:2 con:"TD.World.Hold 900 0" >/dev/null
for d in 30000 100000; do
  $P con:"TD.World.SpawnEnemy ShardFighter $d 0 -$((d/5))" con:"TD.World.SpawnEnemy HookInterceptor $d 0 0" con:"TD.World.SpawnEnemy WallGunship $d 0 $((d/5))" sleep:3 cap:${T}_range_$d >/dev/null
  $P con:"TD.KillAll" sleep:4 >/dev/null
done
