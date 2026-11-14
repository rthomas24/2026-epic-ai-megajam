#!/usr/bin/env bash
# waitslot.sh: wait (30 s steps, up to 15 min) until fewer than 3 rendering Unreal processes run on the machine (Lead's cap).
for i in $(seq 1 30); do
  n=$(powershell -NoProfile -Command "(Get-CimInstance Win32_Process | ? { (\$_.Name -like 'UnrealEditor*' -or \$_.Name -like 'unrealGameJam*') -and \$_.CommandLine -notmatch 'nullrhi' }).Count")
  [ "$n" -lt 3 ] && { echo "slot free ($n running)"; exit 0; }
  sleep 30
done
echo "NO SLOT after 15 min"; exit 1
