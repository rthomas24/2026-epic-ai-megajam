#!/usr/bin/env bash
# r9l.sh <tag>: the reactor with the hold re-issued every 0.5 s from the stage start (the stage resets the ship's speed),
# then AimAt and 6 s of fire; then the 200 m break row.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1
G=ArtSource/EngVFX/rounds/r8/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py ${TD_PORT:-21031} Saved/EngVFX/r8"; T=${1:-x}; L=Saved/Logs/td-play-mcp_${TD_PORT:-21031}.log
bash $G "('pie_start',)" >/dev/null; sleep 25
H=""; for i in $(seq 1 24); do H="$H con:TD.World.Hold\ 60\ 0 sleep:0.5"; done
eval $P con:"TD.God" con:"td.Finale.Boarding\ 0" con:"TD.Stage\ ReactorExposed" $H >/dev/null
$P con:"TD.World.AimAt Reactor 1" sleep:1.5 cap:${T}_r315_aim con:"TD.Input.Key LeftMouseButton 6" sleep:0.8 cap:${T}_r315_1 sleep:0.8 cap:${T}_r315_2 sleep:0.8 cap:${T}_r315_3 sleep:4 con:"TD.World.DominionStatus" >/dev/null
grep -a "AimAt TDDominion\|reactor hull\|God mode" $L | tail -4 | cut -c1-150
$P con:"TD.World.Hold 0" sleep:4 con:"TD.World.AimAt Reactor 1" con:"TD.Input.Key LeftMouseButton 5" sleep:1.5 cap:${T}_rclose_1 sleep:1.5 cap:${T}_rclose_2 sleep:2.5 con:"TD.World.DominionStatus" >/dev/null
grep -a "AimAt TDDominion\|reactor hull" $L | tail -2 | cut -c1-150
bash $G "('pie_stop',)" >/dev/null
