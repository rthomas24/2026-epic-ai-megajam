#!/usr/bin/env bash
# ice2.sh <tag>: T6's real rocks, TD.SpaceField.TestShot per class, in slow motion 0.25 (captures every ~0.1 s of game time
# around the break) and at normal speed; the field's break log lines are printed after each.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1
G=ArtSource/EngVFX/rounds/r8/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py ${TD_PORT:-21031} Saved/EngVFX/r8"; T=${1:-m}; L=Saved/Logs/td-play-mcp_${TD_PORT:-21031}.log
bash $G "('pie_start',)" >/dev/null; sleep 25; $P con:"TD.Stage Waves1to2" sleep:14 con:"TD.God" con:"TD.KillAll" >/dev/null
for c in 0 1 2 3; do
  $P con:"TD.KillAll" con:"slomo 0.25" con:"TD.SpaceField.TestShot $c 14" sleep:0.4 cap:${T}_s${c}_a sleep:0.4 cap:${T}_s${c}_b sleep:0.6 cap:${T}_s${c}_c sleep:1.0 cap:${T}_s${c}_d con:"slomo 1" sleep:2 >/dev/null
  grep -a "TDSpaceField: break" $L | tail -2 | cut -c1-220
  $P con:"TD.KillAll" con:"TD.SpaceField.TestShot $c 14" sleep:0.15 cap:${T}_n${c}_a sleep:0.15 cap:${T}_n${c}_b sleep:0.3 cap:${T}_n${c}_c sleep:2 >/dev/null
  grep -a "TDSpaceField: break" $L | tail -1 | cut -c1-220
done
bash $G "('pie_stop',)" >/dev/null
