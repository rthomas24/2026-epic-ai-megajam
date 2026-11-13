#!/usr/bin/env bash
# PIE sheet of the explosions (editor on 8017 must be running with PIE off)
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; G=ArtSource/EngVFX/scripts/g.sh; C=Saved/EngVFX/cap
bash $G py "CMD = ('pie_start',)" >/dev/null; sleep 12
bash $G con "TD.God 1" >/dev/null; bash $G con "TD.Squad.Passive 1" >/dev/null; bash $G con "TD.World.Hold 120 0" >/dev/null; sleep 3
shot() { # name systems dist gap z frames
  bash $G con "slomo 0.03" >/dev/null; bash $G py "CMD = ('clear',)" >/dev/null
  bash $G py "CMD = ('row', $2, $3, $4, 0, $5)"
  for i in $(seq 1 $6); do bash $G cap "$1_$i" 1600 >/dev/null; sleep ${7:-0}; done
  bash $G con "slomo 1" >/dev/null; sleep 2
}
shot x "['NS_TD_Explosion_Small','NS_TD_Explosion_Medium','NS_TD_Explosion_Large']" 6000 3200 600 8 1.5
shot hs "['NS_TD_Explosion_HullSection']" 30000 0 3000 5 2
shot fb "['NS_TD_Explosion_FinalBlast']" 300000 0 30000 6 3
bash $G py "CMD = ('pie_stop',)" >/dev/null
cd $C; bash ../tile.sh x_ 8 2 "1600:620:0:60" x_all.png; bash ../tile.sh hs_ 5 5 "800:620:400:60" hs_all.png; bash ../tile.sh fb_ 6 6 "800:620:400:60" fb_all.png
