#!/usr/bin/env bash
powershell -Command "'free GB: ' + [math]::Round((Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory/1MB,1); Get-CimInstance Win32_Process -Filter \"Name like 'UnrealEditor%'\" | ? { \$_.CommandLine -match 'wt.vfx' } | % { 'MINE pid {0} {1} GB' -f \$_.ProcessId, [math]::Round(\$_.WorkingSetSize/1GB,1) }"
