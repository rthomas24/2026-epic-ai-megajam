#!/usr/bin/env bash
# Tier 2 review captures in the editor's Play-In-Editor session (port 8017). Frames -> Saved/EngVFX/cap/<name>_<i>.png
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; G=ArtSource/EngVFX/scripts/g.sh
bash $G py "CMD = ('pie_start',)" >/dev/null; sleep 14
bash $G con "TD.God 1" >/dev/null; bash $G con "TD.Stage Waves1to2" >/dev/null; sleep 6
bash $G con "TD.KillAll" >/dev/null; bash $G con "TD.Squad.Passive 1" >/dev/null; bash $G con "TD.World.Hold 900 0" >/dev/null; sleep 5; bash $G con "TD.KillAll" >/dev/null
python ArtSource/EngVFX/scripts/run_editor_py.py 8017 ArtSource/EngVFX/scripts/check_active.py
shot() { # name systems dist gap yaw z frames slomo
  bash $G con "slomo $8" >/dev/null; bash $G py "CMD = ('clear',)" >/dev/null
  bash $G py "CMD = ('row', $2, $3, $4, $5, $6)" | tr '\n' ' '
  for i in $(seq 1 $7); do bash $G cap "$1_$i" 1600 >/dev/null; done
  bash $G con "slomo 1" >/dev/null; sleep 1
}
shot field "['NS_TD_Dominion_Field']" 320000 0 0 0 2 1
shot collapse "['NS_TD_Dominion_FieldCollapse']" 320000 0 0 0 6 0.25
shot glitch "['NS_TD_Glitch_Tear']" 2200 0 0 250 3 0.03
shot pw1 "['NS_TD_Portal_Open','NS_TD_Portal_Close','NS_TD_Ram_Streak','NS_TD_Missile_Trail','NS_TD_Missile_Trail_Enemy']" 4200 1700 55 300 3 0.06
shot pw2 "['NS_TD_EMP_Pulse','NS_TD_ShieldDome','NS_TD_GravityWell']" 24000 11000 0 1500 3 0.08
shot pw3 "['NS_TD_Cloak_Shimmer','NS_TD_Hyperdrive_Streaks']" 3500 3000 55 300 2 1
shot foot "['NS_TD_Blade_Swing','NS_TD_Blade_Deflect','NS_TD_Blade_PerfectDeflect','NS_TD_Blade_AirSlam','NS_TD_Crew_Bolt','NS_TD_Crew_BoltImpact','NS_TD_Drone_Thruster']" -900 160 55 420 3 0.04
shot core1 "['NS_TD_Core_Orb','NS_TD_Core_ConduitBreak','NS_TD_Core_ShieldDrop','NS_TD_Core_HeartHit']" 3000 1300 55 300 3 0.05
shot core2 "['NS_TD_Core_LaserSweep','NS_TD_Core_Destroyed']" 9000 6000 55 600 4 0.08
bash $G py "CMD = ('pie_stop',)" >/dev/null
