#!/usr/bin/env bash
# tile.sh <prefix> <n> <cols> <crop> <out>
in=""; fc=""; lab=""; for i in $(seq 1 $2); do in="$in -i $1$i.png"; fc="$fc[$((i-1))]crop=$4[c$i];"; lab="$lab[c$i]"; done
rows=$(( ($2 + $3 - 1) / $3 ))
ffmpeg -y -loglevel error $in -filter_complex "${fc}${lab}xstack=inputs=$2:grid=$3x${rows},scale=2400:-1" $5
