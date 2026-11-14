#!/usr/bin/env bash
# r10cap.sh <tag>: round 10 captures in one PIE session (editor on $TD_PORT must be up): the five Null plumes at 300 m and
# 800 m (fixed side camera burst + the chase camera from behind), then the cloud v2 on Verdance and Cinder (r9s.sh).
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}
R=ArtSource/EngVFX/rounds/r8; G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py ${TD_PORT:-21031} Saved/EngVFX/r8"; T=${1:-t}
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage Waves1to2" sleep:12 con:"god" con:"TD.KillAll" sleep:2 con:"TD.World.Hold 900 0" >/dev/null
for d in 300 800; do
  python $R/burst.py $T plumes $d
  $P cap:${T}_plumes_chase_${d}m >/dev/null
  bash $G "('clear',)" >/dev/null
done
bash $G "('pie_stop',)" >/dev/null; sleep 5
bash $R/r9s.sh $T
