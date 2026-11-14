#!/usr/bin/env bash
# r9p.sh <tag>: reactor from above (150 / 350 m), hyperdrive (jump confirmed in the log), the ice-burst breaks at 80 m.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1
G=ArtSource/EngVFX/rounds/r8/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py ${TD_PORT:-21031} Saved/EngVFX/r8"; T=${1:-p}; L=Saved/Logs/td-play-mcp_${TD_PORT:-21031}.log
B="python ArtSource/EngVFX/rounds/r8/burst.py $T"
H=""; for i in $(seq 1 24); do H="$H con:TD.World.Hold\ 60\ 0 sleep:0.5"; done
bash $G "('pie_start',)" >/dev/null; sleep 25
eval $P con:"TD.God" con:"td.Finale.Boarding\ 0" con:"TD.Stage\ ReactorExposed" $H >/dev/null
$B reactor_up 150; $B reactor_up 350
$P con:"TD.World.Hold 0" con:"TD.Stage Waves3to4" sleep:16 con:"TD.KillAll" con:"TD.World.Hold 60 0" sleep:1 >/dev/null
$B rocks 80
$P con:"TD.World.Hold 0" con:"TD.Stage Waves1to2" sleep:14 con:"TD.SkipWave" sleep:3 con:"TD.KillAll" sleep:3 con:"TD.SkipWave" sleep:3 con:"TD.KillAll" sleep:6 con:"TD.KillAll" >/dev/null
$B hyper
grep -a -i "hyperdrive" $L | tail -4 | cut -c1-160
bash $G "('pie_stop',)" >/dev/null
