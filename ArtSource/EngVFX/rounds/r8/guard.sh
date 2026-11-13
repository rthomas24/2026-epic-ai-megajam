#!/usr/bin/env bash
# guard.sh [GB=8]: kill any Unreal process of this worktree whose working set passes the limit (Lead's rule, Nov 13). Runs until killed.
LIM=${1:-8}
while true; do
  powershell -NoProfile -Command "Get-CimInstance Win32_Process -Filter \"Name like 'UnrealEditor%'\" | ? { \$_.CommandLine -match 'wt.vfx' -and \$_.WorkingSetSize -gt ${LIM}GB } | % { 'GUARD KILL ' + \$_.ProcessId + ' ' + [math]::Round(\$_.WorkingSetSize/1GB,1) + ' GB'; Stop-Process -Id \$_.ProcessId -Force }"
  sleep 10
done
