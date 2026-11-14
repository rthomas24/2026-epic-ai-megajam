#!/usr/bin/env bash
# r10m.sh <tag>: 10f cloak. Headless: NS_TD_Cloak_Shimmer v2 (no saucer shell). One lock turn (about 8 min):
# M_TD_VFX_CloakOverlay, then the chase camera with the cloak off and on (overlay + shimmer applied by cloak_prev.py).
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; R=ArtSource/EngVFX/rounds/r8
export NULLRHI=1; bash $R/chunk_list.sh NS_TD_Cloak_Shimmer "0:2" 2>&1 | grep "chunk\|UpToDate\|STOP\|rror" | cut -c1-160
unset NULLRHI; trap 'bash $R/ed.sh unlock' EXIT
bash $R/ed.sh lock "cloak overlay material + cloak capture (about 8 min)" || exit 1
echo "== locked turn $(date +%H:%M)"; bash $R/ed.sh start >/dev/null || exit 1
(echo "ONLY_MATS = ['M_TD_VFX_CloakOverlay']"; cat ArtSource/EngVFX/scripts/build_materials.py) > Saved/EngVFX/_mats.py
python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_mats.py 2>&1 | tail -1 | cut -c1-200
python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT $R/cloak_flag.py 2>&1 | tail -1 | cut -c1-200
G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $TD_PORT Saved/EngVFX/r8"; T=${1:-m}
C() { (echo "CMD = '$1'"; cat $R/cloak_prev.py) > Saved/EngVFX/_cl.py; python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_cl.py 2>&1 | tail -1 | cut -c1-120; }
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage Waves1to2" sleep:12 con:"god" con:"TD.KillAll" sleep:2 con:"TD.World.Hold 900 0" >/dev/null
C off; $P sleep:1 cap:${T}_cloak_off >/dev/null
C on;  $P sleep:1.5 cap:${T}_cloak_on_a sleep:0.4 cap:${T}_cloak_on_b >/dev/null
C off; bash $G "('pie_stop',)" >/dev/null
bash $R/ed.sh unlock; echo "== closed $(date +%H:%M)"
