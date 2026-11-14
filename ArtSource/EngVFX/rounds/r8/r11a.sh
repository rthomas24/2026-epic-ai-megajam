#!/usr/bin/env bash
# r11a.sh: a 2-minute slot turn: rebuild M_TD_VFX_CloakShell2 in place (T2's code points at this path), more see-through.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; R=ArtSource/EngVFX/rounds/r8
unset NULLRHI; trap 'bash $R/ed.sh unlock' EXIT
bash $R/ed.sh lock "cloak shell material (2 min)" || exit 1
bash $R/ed.sh start >/dev/null || exit 1
(echo "ONLY_MATS = ['M_TD_VFX_CloakShell2']"; cat ArtSource/EngVFX/scripts/build_materials.py) > Saved/EngVFX/_mats.py
python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_mats.py 2>&1 | tail -2 | cut -c1-300
bash $R/ed.sh unlock; echo "== closed $(date +%H:%M:%S)"
