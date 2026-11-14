#!/usr/bin/env bash
# r10h.sh: 3-minute debug: spawn the debug row 300 m ahead and print where each lands on screen (no images needed).
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; unset NULLRHI
R=ArtSource/EngVFX/rounds/r8; G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $TD_PORT Saved/EngVFX/r8"
bash $R/ed.sh start || exit 1
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage Waves1to2" sleep:12 con:"god" con:"TD.KillAll" sleep:2 con:"TD.World.Hold 900 0" >/dev/null
bash $G "('row', ['NS_TD_Debug_BeaconFixed', 'NS_TD_Debug_HazeField', 'NS_TD_Region_Beacon'], 30000, 6000)" >/dev/null
sleep 1; python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT $R/dbg_view.py 2>&1 | tail -6 | cut -c1-300
$P cap:z_dbg >/dev/null
bash $G "('clear',)" >/dev/null; bash $G "('pie_stop',)" >/dev/null; bash $R/ed.sh stop >/dev/null; echo "== closed $(date +%H:%M)"
