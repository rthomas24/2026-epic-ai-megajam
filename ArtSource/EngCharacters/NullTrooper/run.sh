#!/bin/bash
# usage: run.sh <stage script> <in blend> [args...]   (headless Blender)
cd "$(dirname "$0")"
B="/c/Program Files/Blender Foundation/Blender 5.2/blender.exe"
s=$1; b=$2; shift 2
"$B" --background --factory-startup $b --python scripts/$s -- "$@" < /dev/null 2>&1
