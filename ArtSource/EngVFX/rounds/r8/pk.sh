#!/usr/bin/env bash
# pk.sh <tag>: the pickup review in Play-In-Editor (editor on 8017): Waves1to2, the ship held, the four idles attached in a
# row in front of the ship at 200 m, 45 m and 12 m, then the four collect bursts in slow motion. Captures: Saved/EngVFX/r8/
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1
G=ArtSource/EngVFX/rounds/r8/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py 8017 Saved/EngVFX/r8"; T=${1:-b}
N="['NS_TD_Pickup_Hull', 'NS_TD_Pickup_Shield', 'NS_TD_Pickup_Power', 'NS_TD_Pickup_Overcharge']"
C="['NS_TD_Pickup_Hull_Collect', 'NS_TD_Pickup_Shield_Collect', 'NS_TD_Pickup_Power_Collect', 'NS_TD_Pickup_Overcharge_Collect']"
if [ "$2" != "nostart" ]; then bash $G "('pie_start',)"; sleep 25; $P con:"TD.Stage Waves1to2" sleep:12; fi
$P con:"TD.World.Hold 900 0" con:"god"
for d in "20000 3000 200" "4500 800 45" "1200 450 12"; do set -- $d
  bash $G "('attach_row', $N, $1, $2)" >/dev/null; $P sleep:2 cap:${T}_idle_$3; bash $G "('clear',)" >/dev/null; done
$P con:"slomo 0.05"; bash $G "('attach_row', $C, 2500, 900)" >/dev/null; $P cap:${T}_col_1 sleep:1.5 cap:${T}_col_2 sleep:3 cap:${T}_col_3 con:"slomo 1"
bash $G "('clear',)" >/dev/null
