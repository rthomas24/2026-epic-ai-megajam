#!/usr/bin/env bash
# rocks.sh <tag>: the asteroid breaks in Play-In-Editor (editor on $TD_PORT), Waves1to2, the ship held, chase camera:
# S, M, L and Crystal in a row 60 m ahead, then the impact scrape at Strength 0.2 and 1 beside the ship; slow motion 0.1.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1
G=ArtSource/EngVFX/rounds/r8/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py ${TD_PORT:-21031} Saved/EngVFX/r8"; T=${1:-a}
bash $G "('pie_start',)" >/dev/null; sleep 25; $P con:"TD.Stage Waves1to2" sleep:12 con:"god" con:"TD.World.Hold 900 0" >/dev/null
$P con:"slomo 0.1" >/dev/null
bash $G "('row', ['NS_TD_Asteroid_Break_S', 'NS_TD_Asteroid_Break_M', 'NS_TD_Asteroid_Break_L', 'NS_TD_Asteroid_Break_Crystal'], 6000, 2200, 0, 0)" >/dev/null
$P sleep:0.6 cap:${T}_rocks_1 sleep:3 cap:${T}_rocks_2 sleep:8 cap:${T}_rocks_3 >/dev/null
bash $G "('clear',)" >/dev/null
for s in 0.2 1.0; do
  bash $G "('attach_set', 'NS_TD_Ship_ImpactScrape', (150, 260, 0), {'Strength': $s})" >/dev/null
  $P sleep:0.6 cap:${T}_scrape_$s sleep:3 cap:${T}_scrape_${s}_b >/dev/null
  bash $G "('clear',)" >/dev/null
done
$P con:"slomo 1" >/dev/null
