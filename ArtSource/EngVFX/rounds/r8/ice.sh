#!/usr/bin/env bash
# ice.sh <tag>: the asteroid breaks on T6's real rocks (PIE on $TD_PORT, Waves1to2): TD.SpaceField.TestShot for classes
# 0, 1, 2 and 3 (special), slow motion 0.25, captures as the bolts land.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1
G=ArtSource/EngVFX/rounds/r8/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py ${TD_PORT:-21031} Saved/EngVFX/r8"; T=${1:-j}
bash $G "('pie_start',)" >/dev/null; sleep 25; $P con:"TD.Stage Waves1to2" sleep:14 con:"TD.God" con:"TD.KillAll" >/dev/null
for c in 0 1 2 3; do
  $P con:"TD.World.Hold 30 0" con:"TD.SpaceField.TestShot $c 14" sleep:0.4 con:"slomo 0.25" sleep:0.6 cap:${T}_break${c}_1 sleep:1.0 cap:${T}_break${c}_2 con:"slomo 1" sleep:2 >/dev/null
done
bash $G "('pie_stop',)" >/dev/null
