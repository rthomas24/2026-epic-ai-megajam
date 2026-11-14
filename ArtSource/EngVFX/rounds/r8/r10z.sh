#!/usr/bin/env bash
# r10y.sh <tag>: one short slot turn: the turret telegraph head-on (aimed at the player) at 150 m and 400 m, mid-charge and
# at the snap; the switch grace on the flown ship at 0.3 / 0.8 / 1.3 s from the chase camera.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; R=ArtSource/EngVFX/rounds/r8
T=${1:-y}; unset NULLRHI; trap 'bash $R/ed.sh unlock' EXIT
bash $R/ed.sh lock "telegraph head-on + grace capture" || exit 1
echo "== turn $(date +%H:%M:%S)"; bash $R/ed.sh start >/dev/null || exit 1
G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $TD_PORT Saved/EngVFX/r8"
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage Waves1to2" sleep:12 con:"god" con:"TD.KillAll" con:"TD.Squad.Passive 1" sleep:1 con:"TD.World.Hold 900 0" >/dev/null
for d in; do
  bash $G "('row', ['NS_TD_Turret_LockTelegraph'], $((d*100)), 0, 180, 300)" >/dev/null; $P sleep:0.55 cap:${T}_tele_${d}m_mid sleep:0.4 cap:${T}_tele_${d}m_snap >/dev/null; bash $G "('clear',)" >/dev/null; sleep 1
done
bash $G "('attach_set', 'NS_TD_Ship_SwitchGrace', (0, 0, 0), {})" >/dev/null; $P sleep:0.3 cap:${T}_grace_03 sleep:0.5 cap:${T}_grace_08 sleep:0.5 cap:${T}_grace_13 >/dev/null
bash $G "('clear',)" >/dev/null; bash $G "('pie_stop',)" >/dev/null
bash $R/ed.sh unlock; echo "== closed $(date +%H:%M:%S)"
