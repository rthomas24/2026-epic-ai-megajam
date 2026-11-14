#!/usr/bin/env bash
# reactor.sh <tag>: the space reactor in PIE on $TD_PORT: DominionAssault, shields dropped, the player sent to the reactor,
# aimed and firing; captures of the exposed flash and the hits.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1
G=ArtSource/EngVFX/rounds/r8/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py ${TD_PORT:-21031} Saved/EngVFX/r8"; T=${1:-e}
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"td.Finale.Boarding 0" con:"TD.Stage DominionAssault" sleep:18 con:"god" con:"td.Finale.Boarding 0" con:"TD.World.DominionStatus" con:"TD.World.Dominion DropShields" sleep:0.4 cap:${T}_dom_0 sleep:1.5 cap:${T}_dom_1 \
   con:"TD.World.Goto Reactor" sleep:3 con:"TD.World.AimAt Reactor 2" sleep:2.5 cap:${T}_reactor_0 con:"TD.Input.Key LeftMouseButton 4" sleep:1.0 cap:${T}_reactor_hit_1 sleep:1.0 cap:${T}_reactor_hit_2 con:"TD.World.DominionStatus" >/dev/null
bash $G "('pie_stop',)" >/dev/null
