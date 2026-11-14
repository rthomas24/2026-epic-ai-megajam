#!/usr/bin/env bash
# r10s.sh <tag>: ONE queued lock turn (<= 15 min), in the Lead's order: breakup (T4's camera), tractor beam in a real dock,
# plasma mines in a real wave, then the last cloak pass (its material needs the renderer, so it is built here).
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; R=ArtSource/EngVFX/rounds/r8
unset NULLRHI; trap 'bash $R/ed.sh unlock' EXIT
bash $R/ed.sh lock "breakup, tractor, mines, cloak (15 min)" || exit 1
echo "== locked turn $(date +%H:%M)"; bash $R/ed.sh start >/dev/null || exit 1
G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $TD_PORT Saved/EngVFX/r8"; T=${1:-s}
bash $G "('pie_start',)" >/dev/null; sleep 25
echo "-- breakup $(date +%H:%M:%S)"
$P con:"TD.Stage ReactorExposed" sleep:14 con:"god" con:"TD.Director.ReactorKill" sleep:1 cap:${T}_breakup_1 sleep:1.5 cap:${T}_breakup_2 sleep:1.5 cap:${T}_breakup_3 sleep:1.5 cap:${T}_breakup_4 sleep:1.5 cap:${T}_breakup_5 sleep:1.5 cap:${T}_breakup_6 sleep:2 cap:${T}_breakup_7 sleep:3 cap:${T}_breakup_8 >/dev/null
echo "-- dock $(date +%H:%M:%S)"
$P con:"TD.Stage DominionDocking" sleep:14 con:"god" sleep:2 cap:${T}_dock_1 sleep:1.5 cap:${T}_dock_2 sleep:1.5 cap:${T}_dock_3 sleep:1.5 cap:${T}_dock_4 >/dev/null
echo "-- mines $(date +%H:%M:%S)"
$P con:"TD.Stage Wave5" sleep:12 con:"god" con:"TD.KillAll" sleep:1 con:"TD.World.Hold 900 0" con:"TD.World.SpawnEnemy MaulBomber 25000 3000 0" con:"TD.World.SpawnEnemy MaulBomber 30000 -3000 1500" sleep:9 cap:${T}_mines_1 sleep:2 cap:${T}_mines_2 sleep:2 cap:${T}_mines_3 >/dev/null
bash $G "('pie_stop',)" >/dev/null; sleep 4
echo "-- cloak $(date +%H:%M:%S)"
(echo "ONLY_MATS = ['M_TD_VFX_CloakShell2']"; cat ArtSource/EngVFX/scripts/build_materials.py) > Saved/EngVFX/_mats.py
python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_mats.py 2>&1 | tail -1 | cut -c1-200
(echo "ONLY = ['__none__']"; cat ArtSource/EngVFX/scripts/build_tier1.py; echo; echo "ONLY2 = ['=NS_TD_Cloak_Shimmer']"; echo "CHUNK = (0, 1)"; cat ArtSource/EngVFX/scripts/build_tier2.py) > Saved/EngVFX/_chunk.py
python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_chunk.py 2>&1 | grep "NS_TD_Cloak_Shimmer" | cut -c1-200
C() { (echo "CMD = '$1'"; cat $R/cloak_prev.py) > Saved/EngVFX/_cl.py; python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_cl.py 2>&1 | tail -1 | cut -c1-120; }
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage Waves1to2" sleep:12 con:"god" con:"TD.KillAll" sleep:2 con:"TD.World.Hold 900 0" >/dev/null
C off; $P sleep:1 cap:${T}_cloak_off >/dev/null; python Saved/EngVFX/_side.py ${T}_cloak_side_off
C on;  $P sleep:1.5 cap:${T}_cloak_on_a >/dev/null; python Saved/EngVFX/_side.py ${T}_cloak_side_on
C off; bash $G "('pie_stop',)" >/dev/null
bash $R/ed.sh unlock; echo "== closed $(date +%H:%M)"
