#!/usr/bin/env bash
# r10x.sh <tag>: one short slot turn: the turret lock telegraph (side at 60 m, and aimed at the player from 150 m) and the
# switch-grace shimmer on the flown ship (chase and side), both spawned by script.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; R=ArtSource/EngVFX/rounds/r8
T=${1:-x}; unset NULLRHI; trap 'bash $R/ed.sh unlock' EXIT
bash $R/ed.sh lock "turret telegraph + switch grace capture" || exit 1
echo "== turn $(date +%H:%M:%S)"; bash $R/ed.sh start >/dev/null || exit 1
G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $TD_PORT Saved/EngVFX/r8"; SC="python $R/spawncap.py"
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage Waves1to2" sleep:12 con:"god" con:"TD.KillAll" con:"TD.Squad.Passive 1" sleep:1 con:"TD.World.Hold 900 0" >/dev/null
$SC ${T}_tele_side NS_TD_Turret_LockTelegraph 60 side 1.0 90 '{}' 6 1 40 >/dev/null
$SC ${T}_tele_aim NS_TD_Turret_LockTelegraph 150 chase 1.0 180 '{}' >/dev/null
bash $G "('attach_set', 'NS_TD_Ship_SwitchGrace', (0, 0, 0), {})" >/dev/null; $P sleep:0.4 cap:${T}_grace_a >/dev/null; python Saved/EngVFX/_side.py ${T}_grace_side
bash $G "('clear',)" >/dev/null; bash $G "('pie_stop',)" >/dev/null
bash $R/ed.sh unlock; echo "== closed $(date +%H:%M:%S)"
