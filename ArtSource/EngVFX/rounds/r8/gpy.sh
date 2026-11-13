#!/usr/bin/env bash
# gpy.sh "<CMD tuple>"   run game_spawn.py in the PIE world of the editor on 8017 with that CMD
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1
(echo "CMD = $1"; cat ArtSource/EngVFX/scripts/game_spawn.py) > Saved/EngVFX/_g8.py
python ArtSource/EngVFX/scripts/run_editor_py.py 8017 Saved/EngVFX/_g8.py
