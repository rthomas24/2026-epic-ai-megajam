#!/usr/bin/env bash
# r9o.sh <tag>: the reactor (side camera 150 m, 350 m) and the resized breaks (80 m, 200 m), burst method.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1
G=ArtSource/EngVFX/rounds/r8/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py ${TD_PORT:-21031} Saved/EngVFX/r8"; T=${1:-o}
B="python ArtSource/EngVFX/rounds/r8/burst.py $T"
H=""; for i in $(seq 1 24); do H="$H con:TD.World.Hold\ 60\ 0 sleep:0.5"; done
bash $G "('pie_start',)" >/dev/null; sleep 25
eval $P con:"TD.God" con:"td.Finale.Boarding\ 0" con:"TD.Stage\ ReactorExposed" $H >/dev/null
$B reactor 150; $B reactor 350
$P con:"TD.World.Hold 0" con:"TD.Stage Waves3to4" sleep:16 con:"TD.KillAll" con:"TD.World.Hold 60 0" sleep:1 >/dev/null
$B rocks 80; $B rocks 200
bash $G "('pie_stop',)" >/dev/null
