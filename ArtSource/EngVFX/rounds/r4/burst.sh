#!/usr/bin/env bash
# burst.sh <outdir> <prefix> <n> <gap seconds> [pre steps...]  : n slow-motion captures, gap seconds of real play between
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx
O=$1; P=$2; N=$3; GAP=$4; shift 4; a=("$@")
for i in $(seq 1 $N); do a+=("con:slomo 0.05" "cap:${P}$i" "con:slomo 1" "sleep:$GAP"); done
python ArtSource/EngVFX/scripts/play.py ${TD_PORT:-8018} $O "${a[@]}"
