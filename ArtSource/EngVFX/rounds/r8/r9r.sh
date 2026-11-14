#!/usr/bin/env bash
# r9r.sh <tag>: the shatter breaks at 80 m and 200 m, burst method.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1
G=ArtSource/EngVFX/rounds/r8/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py ${TD_PORT:-21031} Saved/EngVFX/r8"; T=${1:-r}
B="python ArtSource/EngVFX/rounds/r8/burst.py $T"
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage Waves3to4" sleep:16 con:"TD.God" con:"TD.KillAll" con:"TD.World.Hold 60 0" sleep:1 >/dev/null
$B rocks 80; $B rocks 200
bash $G "('pie_stop',)" >/dev/null
