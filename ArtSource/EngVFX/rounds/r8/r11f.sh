#!/usr/bin/env bash
# r11f.sh <tag>: one slot turn: the Lancer boosting against the cyan nebula (DominionAssault) BEFORE, then M_TD_VFX_Flame
# rebuilt in place with CoreWhite 0.4 (was a fixed 0.9 white core), then AFTER.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; R=ArtSource/EngVFX/rounds/r8
T=${1:-fl}; unset NULLRHI; trap 'bash $R/ed.sh unlock' EXIT
bash $R/ed.sh lock "Lancer flame core: material + before/after" || exit 1
echo "== turn $(date +%H:%M:%S)"; bash $R/ed.sh start >/dev/null || exit 1
G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $TD_PORT Saved/EngVFX/r8"
shot() { bash $G "('pie_start',)" >/dev/null; sleep 25
  $P con:"td.Finale.Boarding 0" con:"TD.Stage DominionAssault" sleep:14 con:"god" con:"TD.Squad.Passive 1" con:"TD.Squad.Switch Lancer" sleep:2 con:"TD.Input.Keys W+LeftShift 3.0" sleep:1.2 cap:${T}_$1_a sleep:0.6 cap:${T}_$1_b >/dev/null
  bash $G "('pie_stop',)" >/dev/null; sleep 4; }
shot before
(echo "ONLY_MATS = ['M_TD_VFX_Flame']"; cat ArtSource/EngVFX/scripts/build_materials.py) > Saved/EngVFX/_mats.py
python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_mats.py 2>&1 | tail -1 | cut -c1-300
shot after
bash $R/ed.sh unlock; echo "== closed $(date +%H:%M:%S)"
