#!/usr/bin/env bash
# r12n.sh <tag>: short slot turn: the plasma mine. (a) NS_TD_Mine_Plasma spawned bare 60 m ahead (is the effect visible?);
# (b) TD.Dev.SpawnMine 60 with the ship slowed, paused, and the projectile's components listed.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; R=ArtSource/EngVFX/rounds/r8
T=${1:-mn}; unset NULLRHI; trap 'bash $R/ed.sh unlock' EXIT
bash $R/ed.sh lock "plasma mine probe" || exit 1
echo "== turn $(date +%H:%M:%S)"; bash $R/ed.sh start >/dev/null || exit 1
G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $TD_PORT Saved/EngVFX/r8"
B() { (echo "CMD = $1"; cat $R/tb_show.py) > Saved/EngVFX/_tb_show.py; python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_tb_show.py 2>&1 | tail -1 | cut -c1-200; }
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage DominionAssault" sleep:14 con:"god" con:"TD.KillAll" con:"TD.Squad.Passive 1" con:"TD.World.Hold 900 0" con:"TD.Input.Keys S 3.0" sleep:3.2 >/dev/null
B "('/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Mine_Plasma', 40, 0, 0, 1, 1.0)"; $P con:"pause" sleep:0.5 cap:${T}_bare_40 con:"pause" >/dev/null
B "'clear'"
$P con:"TD.Input.Keys S 2.0" sleep:2.2 con:"TD.Dev.SpawnMine 60" sleep:0.3 con:"pause" sleep:0.5 cap:${T}_spawn_60 >/dev/null
python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT $R/mine_probe.py 2>&1 | tail -6 | cut -c1-600
$P con:"pause" >/dev/null; bash $G "('pie_stop',)" >/dev/null
bash $R/ed.sh unlock; echo "== closed $(date +%H:%M:%S)"
