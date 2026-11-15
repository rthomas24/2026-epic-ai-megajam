#!/usr/bin/env bash
# r12t.sh <tag>: one slot turn: (1) death pops side by side, NS_TD_Explosion_Small (left) vs NS_TD_Explosion_Pop (right),
# at 100 m and 250 m, frames at about 0.1 / 0.3 / 0.7 s; (2) the turret lock telegraph spawned the code's way at 300 m and
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; R=ArtSource/EngVFX/rounds/r8
T=${1:-tt}; unset NULLRHI; trap 'bash $R/ed.sh unlock' EXIT
bash $R/ed.sh lock "death pop side-by-side, telegraph 300-600 m, escape breakup" || exit 1
echo "== turn $(date +%H:%M:%S)"; bash $R/ed.sh start >/dev/null || { echo "start refused"; exit 1; }
G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $TD_PORT Saved/EngVFX/r8"
B() { (echo "CMD = $1"; cat $R/tb_show.py) > Saved/EngVFX/_tb_show.py; python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_tb_show.py 2>&1 | tail -1 | cut -c1-120; }
S=/Game/TriadDrift/Gameplay/VFX/Systems
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage Wave5" sleep:12 con:"god" con:"TD.KillAll" con:"TD.Squad.Passive 1" con:"TD.World.Hold 900 0" sleep:1 con:"slomo 0.1" >/dev/null
for d in 100 250; do
  o=$((d / 4))
  (echo "CMD = ('$S/NS_TD_Explosion_Small', $d, -$o, 0, 1, 1.0)"; cat $R/tb_show.py | sed 's/EngVFXTB/EngVFXTB1/g') > Saved/EngVFX/_tb1.py
  (echo "CMD = ('$S/NS_TD_Explosion_Pop', $d, $o, 0, 1, 1.0)"; cat $R/tb_show.py | sed 's/EngVFXTB/EngVFXTB2/g') > Saved/EngVFX/_tb2.py
  python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_tb1.py >/dev/null 2>&1; python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_tb2.py >/dev/null 2>&1
  $P cap:${T}_pop${d}_1 sleep:1.5 cap:${T}_pop${d}_2 sleep:3 cap:${T}_pop${d}_3 >/dev/null; sleep 8
done
$P con:"slomo 1" con:"TD.Stage DominionAssault" sleep:14 con:"god" con:"TD.Squad.Passive 1" con:"TD.World.Hold 900 0" sleep:1 con:"slomo 0.1" >/dev/null
for d in 300 600; do
  B "('$S/NS_TD_Turret_LockTelegraph', $d, 0, 180, 1, 1.0)"; $P sleep:2.5 cap:${T}_tele${d}_mid sleep:3 cap:${T}_tele${d}_late sleep:2.5 cap:${T}_tele${d}_snap >/dev/null; B "'clear'"
done
$P con:"slomo 1" >/dev/null; bash $G "('pie_stop',)" >/dev/null
bash $R/ed.sh unlock; echo "== closed $(date +%H:%M:%S)"
