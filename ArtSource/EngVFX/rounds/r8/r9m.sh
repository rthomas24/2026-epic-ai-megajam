#!/usr/bin/env bash
# r9m.sh <tag> <part>: one part of the batched review (PIE already running on $TD_PORT).
#   rocks: the 200 m break row   hyp: a hyperdrive jump   exposed: DominionAssault + DropShields, hold re-sent   hit: reactor hits
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1
G=ArtSource/EngVFX/rounds/r8/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py ${TD_PORT:-21031} Saved/EngVFX/r8"; T=$1; L=Saved/Logs/td-play-mcp_${TD_PORT:-21031}.log
H=""; for i in $(seq 1 24); do H="$H con:TD.World.Hold\ 60\ 0 sleep:0.5"; done
case $2 in
 rocks) $P con:"TD.Stage Waves3to4" sleep:16 con:"TD.God" con:"TD.KillAll" con:"TD.World.Hold 60 0" sleep:2 >/dev/null
   bash $G "('row', ['NS_TD_Asteroid_Break_S', 'NS_TD_Asteroid_Break_M', 'NS_TD_Asteroid_Break_L'], 20000, 5000, 0, 0)" >/dev/null; $P sleep:0.06 cap:${T}_b200_a sleep:0.08 cap:${T}_b200_b >/dev/null; bash $G "('clear',)" >/dev/null ;;
 hyp) $P con:"TD.World.Hold 0" con:"TD.Stage Waves1to2" sleep:14 con:"TD.God" con:"TD.SkipWave" sleep:3 con:"TD.KillAll" sleep:3 con:"TD.SkipWave" sleep:3 con:"TD.KillAll" sleep:6 con:"TD.KillAll" \
   con:"TD.Input.Key H 1.6" sleep:0.4 con:"TD.KillAll" cap:${T}_hyp_1 sleep:1.2 con:"TD.KillAll" cap:${T}_hyp_2 sleep:2 cap:${T}_hyp_3 sleep:3 cap:${T}_hyp_4 >/dev/null ;;
 exposed) eval $P con:"TD.God" con:"td.Finale.Boarding\ 0" con:"TD.Stage\ DominionAssault" $H >/dev/null
   $P con:"td.Finale.Boarding 0" con:"TD.World.AimAt Reactor 1" sleep:1.5 con:"TD.World.Dominion DropShields" sleep:0.1 cap:${T}_exp_1 sleep:0.3 cap:${T}_exp_2 sleep:0.6 cap:${T}_exp_3 sleep:1 cap:${T}_exp_4 >/dev/null
   grep -a "AimAt TDDominion\|DominionStatus:" $L | tail -2 | cut -c1-150 ;;
 hit) eval $P con:"TD.God" con:"td.Finale.Boarding\ 0" con:"TD.Stage\ ReactorExposed" $H >/dev/null
   $P con:"TD.World.AimAt Reactor 1" sleep:1.5 con:"TD.Input.Key LeftMouseButton 6" sleep:0.8 cap:${T}_hit_1 sleep:0.4 cap:${T}_hit_2 sleep:0.4 cap:${T}_hit_3 sleep:4.5 con:"TD.World.DominionStatus" >/dev/null
   grep -a "AimAt TDDominion\|reactor hull" $L | tail -2 | cut -c1-150 ;;
esac
