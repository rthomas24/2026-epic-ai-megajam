#!/usr/bin/env bash
# r9cap.sh <tag>: round 9 review in PIE on $TD_PORT: the escape flight loop on the ship, the hull-section chain, the reactor
# exposed / hit and the dock opening in a row ahead (galaxy), then the escape run loop on foot (Verdance). Captures in Saved/EngVFX/r8.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1
G=ArtSource/EngVFX/rounds/r8/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py ${TD_PORT:-21031} Saved/EngVFX/r8"; T=${1:-a}
bash $G "('pie_start',)" >/dev/null; sleep 25; $P con:"TD.Stage Waves1to2" sleep:12 con:"god" con:"TD.KillAll" con:"TD.World.Hold 900 60" >/dev/null
bash $G "('attach_set', 'NS_TD_Escape_FlightLoop', (0, 0, 0), {'Intensity': 1.0})" >/dev/null; $P sleep:3 cap:${T}_escflight_1 sleep:2 cap:${T}_escflight_2 >/dev/null
bash $G "('clear',)" >/dev/null; $P con:"TD.World.Hold 900 0" con:"slomo 0.25" >/dev/null
# the chain: five sections in a row 400 m ahead, staggered by a few frames
for i in 1 2 3; do bash $G "('row', ['NS_TD_Explosion_HullSection', 'NS_TD_Explosion_HullSection'], 40000, 9000, 0, $((i*3000-6000)))" >/dev/null; $P sleep:0.3 >/dev/null; done
$P sleep:0.8 cap:${T}_hull_1 sleep:2 cap:${T}_hull_2 sleep:4 cap:${T}_hull_3 >/dev/null
bash $G "('clear',)" >/dev/null
bash $G "('row', ['NS_TD_Reactor_Exposed'], 50000, 0, 0, 0)" >/dev/null; $P sleep:0.5 cap:${T}_exposed_1 sleep:2 cap:${T}_exposed_2 >/dev/null; bash $G "('clear',)" >/dev/null
bash $G "('row', ['NS_TD_Reactor_Hit', 'NS_TD_Reactor_Hit', 'NS_TD_Reactor_Hit'], 8000, 900, 0, 0)" >/dev/null; $P sleep:0.25 cap:${T}_hit >/dev/null; bash $G "('clear',)" >/dev/null
bash $G "('row', ['NS_TD_Dominion_DockOpen'], 15000, 0, 180, 0)" >/dev/null; $P sleep:1.5 cap:${T}_dock_1 sleep:5 cap:${T}_dock_2 >/dev/null; bash $G "('clear',)" >/dev/null
$P con:"slomo 1" con:"TD.Stage VerdanceOnFoot" sleep:25 >/dev/null
bash $G "('attach_set', 'NS_TD_Escape_RunLoop', (0, 0, 0), {'Intensity': 1.0})" >/dev/null; $P sleep:3 cap:${T}_escrun_1 sleep:2 cap:${T}_escrun_2 >/dev/null
bash $G "('clear',)" >/dev/null
