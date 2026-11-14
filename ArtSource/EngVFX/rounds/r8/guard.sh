#!/usr/bin/env bash
# guard.sh: kill any Unreal process of this worktree over the Lead's cap (Nov 13): 12 GB while free RAM is above 20 GB, else 8 GB.
while true; do
  powershell -NoProfile -Command "\$free = (Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory/1MB; \$lim = if (\$free -gt 20) { 12GB } else { 8GB }; Get-CimInstance Win32_Process -Filter \"Name like 'UnrealEditor%'\" | ? { \$_.CommandLine -match 'wt.vfx' -and \$_.WorkingSetSize -gt \$lim } | % { 'GUARD KILL ' + \$_.ProcessId + ' ' + [math]::Round(\$_.WorkingSetSize/1GB,1) + ' GB (free ' + [math]::Round(\$free,1) + ')'; Stop-Process -Id \$_.ProcessId -Force }"
  sleep 10
done
