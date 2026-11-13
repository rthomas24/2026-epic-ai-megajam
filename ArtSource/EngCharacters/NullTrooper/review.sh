#!/bin/bash
# usage: review.sh <blend> <label> <mode> [views...]
cd "$(dirname "$0")"
B="/c/Program Files/Blender Foundation/Blender 5.2/blender.exe"
b=$1; l=$2; m=$3; shift 3
"$B" --background --factory-startup $b --python ../../../.claude/skills/blender-mcp/scripts/review_renders.py -- --collection NullTrooper --out-dir "$(pwd -W)/review" --label $l --mode $m --views "$@" < /dev/null 2>&1 | grep "__RESULT__\|Error\|error" | cut -c1-300
