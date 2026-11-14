#!/usr/bin/env bash
# ice3.sh <tag>: S and M breaks on T6's rocks in a quiet later phase (Waves3to4, enemies killed; the Aftermath tactical panel
# only follows the tutorial waves), captures right after TestShot; three shots per class for different backgrounds.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1
G=ArtSource/EngVFX/rounds/r8/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py ${TD_PORT:-21031} Saved/EngVFX/r8"; T=${1:-v}; L=Saved/Logs/td-play-mcp_${TD_PORT:-21031}.log
bash $G "('pie_start',)" >/dev/null; sleep 25; $P con:"TD.Stage Waves3to4" sleep:16 con:"TD.God" con:"TD.KillAll" sleep:2 con:"TD.KillAll" >/dev/null
for c in 0 1; do for k in 1 2 3; do
  $P con:"TD.KillAll" con:"TD.SpaceField.TestShot $c 14" sleep:0.1 cap:${T}_c${c}_${k}_a sleep:0.15 cap:${T}_c${c}_${k}_b sleep:0.3 cap:${T}_c${c}_${k}_c sleep:2 >/dev/null
  grep -a "TDSpaceField: break" $L | tail -1 | cut -c1-160
done; done
bash $G "('pie_stop',)" >/dev/null
