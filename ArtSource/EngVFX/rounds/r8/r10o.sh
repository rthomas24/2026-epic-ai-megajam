#!/usr/bin/env bash
# r10o.sh <tag>: 10g. Headless: cloak hull shell, region beacon and plume beacons back on M_TD_VFX_Glow (bigger). One lock
# turn (about 8 min): plumes from the chase camera at 300 / 800 m, region beacon at 300 m / 1.5 km, cloak off / on.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; R=ArtSource/EngVFX/rounds/r8
export NULLRHI=1
for s in "NS_TD_Cloak_Shimmer 0:3" "NS_TD_Region_Beacon 0:1"; do
  bash $R/chunk_list.sh ${s%% *} "${s#* }" 2>&1 | grep "chunk\|UpToDate\|STOP\|rror" | cut -c1-160 || true; done
for s in "NS_TD_Plume_Enemy_Fighter 3:4" "NS_TD_Plume_Enemy_Interceptor 4:6" "NS_TD_Plume_Enemy_Gunship 3:4" "NS_TD_Plume_Enemy_Bomber 2:3" "NS_TD_Plume_Enemy_Carrier 4:8"; do
  bash $R/chunk_list1.sh ${s%% *} "${s#* }" 2>&1 | grep "chunk\|UpToDate\|STOP\|rror" | cut -c1-160 || true; done
unset NULLRHI; trap 'bash $R/ed.sh unlock' EXIT
bash $R/ed.sh lock "plumes, region beacon, cloak captures (about 8 min)" || exit 1
echo "== locked turn $(date +%H:%M)"; bash $R/ed.sh start >/dev/null || exit 1
G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $TD_PORT Saved/EngVFX/r8"; T=${1:-o}
C() { (echo "CMD = '$1'"; cat $R/cloak_prev.py) > Saved/EngVFX/_cl.py; python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_cl.py 2>&1 | tail -1 | cut -c1-120; }
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage Waves1to2" sleep:12 con:"god" con:"TD.KillAll" sleep:2 con:"TD.World.Hold 900 0" >/dev/null
for d in 300 800; do python $R/burst.py $T plumes $d >/dev/null 2>&1; $P cap:${T}_plumes_chase_${d}m >/dev/null; bash $G "('clear',)" >/dev/null; done
bash $G "('row', ['NS_TD_Region_Beacon', 'NS_TD_Region_Beacon'], 30000, 8000)" >/dev/null; $P sleep:1 cap:${T}_beacon_300m >/dev/null; bash $G "('clear',)" >/dev/null
bash $G "('row', ['NS_TD_Region_Beacon', 'NS_TD_Region_Beacon'], 150000, 40000)" >/dev/null; $P sleep:1 cap:${T}_beacon_1500m >/dev/null; bash $G "('clear',)" >/dev/null
C off; $P sleep:1 cap:${T}_cloak_off >/dev/null
C on;  $P sleep:1.5 cap:${T}_cloak_on_a sleep:0.4 cap:${T}_cloak_on_b >/dev/null
C off; bash $G "('pie_stop',)" >/dev/null
bash $R/ed.sh unlock; echo "== closed $(date +%H:%M)"
