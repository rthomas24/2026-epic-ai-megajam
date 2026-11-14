#!/usr/bin/env bash
# r10t.sh <part> [tag]: one SHORT render-slot turn (Lead 13:25: aim 5 min, cap 10) per part: cloak | breakup | dock | mines.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; R=ArtSource/EngVFX/rounds/r8
PART=$1; T=${2:-t}; unset NULLRHI; trap 'bash $R/ed.sh unlock' EXIT
bash $R/ed.sh lock "$PART capture" || exit 1
echo "== $PART turn $(date +%H:%M:%S)"; bash $R/ed.sh start >/dev/null || exit 1
G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $TD_PORT Saved/EngVFX/r8"
case $PART in
 cloak)
  (echo "ONLY_MATS = ['M_TD_VFX_CloakShell2']"; cat ArtSource/EngVFX/scripts/build_materials.py) > Saved/EngVFX/_mats.py
  python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_mats.py 2>&1 | tail -1 | cut -c1-200
  (echo "ONLY = ['__none__']"; cat ArtSource/EngVFX/scripts/build_tier1.py; echo; echo "ONLY2 = ['=NS_TD_Cloak_Shimmer']"; echo "CHUNK = (0, 1)"; cat ArtSource/EngVFX/scripts/build_tier2.py) > Saved/EngVFX/_chunk.py
  python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_chunk.py 2>&1 | grep "NS_TD_Cloak_Shimmer" | cut -c1-200
  python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_cloakfx.py 2>&1 | tail -1 | cut -c1-200
  bash $G "('pie_start',)" >/dev/null; sleep 25
  $P con:"TD.Stage Waves1to2" sleep:12 con:"god" con:"TD.KillAll" sleep:2 con:"TD.World.Hold 900 0" con:"TD.Unlock Cloak Rift" sleep:1 cap:${T}_cloak_off >/dev/null
  python Saved/EngVFX/_side.py ${T}_cloak_side_off
  $P con:"TD.Input.Key E 0.2" sleep:1.0 cap:${T}_cloak_E_a >/dev/null; python Saved/EngVFX/_side.py ${T}_cloak_E_side; $P sleep:0.5 cap:${T}_cloak_E_b >/dev/null
  python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_cloakq.py 2>&1 | tail -1 | cut -c1-400;;
 cloak2)   # no rebuild (the system and material are saved): the real E, then the preview, each with the component state
  C() { (echo "CMD = '$1'"; cat $R/cloak_prev.py) > Saved/EngVFX/_cl.py; python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_cl.py 2>&1 | tail -1 | cut -c1-120; }
  bash $G "('pie_start',)" >/dev/null; sleep 25
  $P con:"TD.Stage Waves1to2" sleep:12 con:"god" con:"TD.KillAll" sleep:2 con:"TD.World.Hold 900 0" con:"TD.Unlock Cloak Rift" sleep:1 >/dev/null
  $P con:"TD.Input.Key E 0.2" sleep:0.3 >/dev/null; python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_cloakq.py 2>&1 | tail -1 | cut -c1-400
  $P sleep:0.6 cap:${T}_cloak_E_a >/dev/null; python Saved/EngVFX/_side.py ${T}_cloak_E_side
  python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_cloakq.py 2>&1 | tail -1 | cut -c1-400
  $P sleep:7 >/dev/null; C on; python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_cloakq.py 2>&1 | tail -1 | cut -c1-400
  $P sleep:1 cap:${T}_cloak_prev_a >/dev/null; python Saved/EngVFX/_side.py ${T}_cloak_prev_side; C off;;
 breakup)
  bash $G "('pie_start',)" >/dev/null; sleep 25
  $P con:"TD.Stage ReactorExposed" sleep:14 con:"god" con:"TD.Director.ReactorKill" sleep:1 cap:${T}_breakup_1 sleep:1.5 cap:${T}_breakup_2 sleep:1.5 cap:${T}_breakup_3 sleep:1.5 cap:${T}_breakup_4 sleep:1.5 cap:${T}_breakup_5 sleep:1.5 cap:${T}_breakup_6 sleep:2 cap:${T}_breakup_7 sleep:3 cap:${T}_breakup_8 >/dev/null;;
 dock)
  bash $G "('pie_start',)" >/dev/null; sleep 25
  $P con:"TD.Stage DominionDocking" sleep:14 con:"god" sleep:2 cap:${T}_dock_1 sleep:1.5 cap:${T}_dock_2 sleep:1.5 cap:${T}_dock_3 sleep:1.5 cap:${T}_dock_4 >/dev/null;;
 mines)
  bash $G "('pie_start',)" >/dev/null; sleep 25
  $P con:"TD.Stage Wave5" sleep:12 con:"god" con:"TD.KillAll" sleep:1 con:"TD.World.Hold 900 0" con:"TD.World.SpawnEnemy MaulBomber 25000 3000 0" con:"TD.World.SpawnEnemy MaulBomber 30000 -3000 1500" sleep:9 cap:${T}_mines_1 sleep:2 cap:${T}_mines_2 sleep:2 cap:${T}_mines_3 >/dev/null;;
esac
bash $G "('pie_stop',)" >/dev/null
bash $R/ed.sh unlock; echo "== $PART closed $(date +%H:%M:%S)"
