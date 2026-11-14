#!/usr/bin/env bash
# r9n.sh <tag>: the Lead's burst method in one PIE session: reactor (150 m and 350 m side camera), rocks (80 m and 200 m),
# hyperdrive (200 m side camera); the real reactor fire for proof (hull and log).
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1
G=ArtSource/EngVFX/rounds/r8/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py ${TD_PORT:-21031} Saved/EngVFX/r8"; T=${1:-n}; L=Saved/Logs/td-play-mcp_${TD_PORT:-21031}.log
B="python ArtSource/EngVFX/rounds/r8/burst.py $T"
H=""; for i in $(seq 1 24); do H="$H con:TD.World.Hold\ 60\ 0 sleep:0.5"; done
bash $G "('pie_start',)" >/dev/null; sleep 25
eval $P con:"TD.God" con:"td.Finale.Boarding\ 0" con:"TD.Stage\ ReactorExposed" $H >/dev/null
$B reactor 150; $B reactor 350
$P con:"TD.World.AimAt Reactor 1" sleep:1.5 con:"TD.Input.Key LeftMouseButton 4" sleep:5 con:"TD.World.DominionStatus" >/dev/null
grep -a "reactor hull\|ReactorHit\|Dominion.Reactor" $L | tail -3 | cut -c1-150
$P con:"TD.World.Hold 0" con:"TD.Stage Waves3to4" sleep:16 con:"TD.KillAll" con:"TD.World.Hold 60 0" sleep:1 >/dev/null
$B rocks 80; $B rocks 200
$P con:"TD.World.Hold 0" con:"TD.Stage Waves1to2" sleep:14 con:"TD.SkipWave" sleep:3 con:"TD.KillAll" sleep:3 con:"TD.SkipWave" sleep:3 con:"TD.KillAll" sleep:5 con:"TD.KillAll" >/dev/null
$B hyper
bash $G "('pie_stop',)" >/dev/null
