#!/usr/bin/env bash
# waitslot.sh: wait (30 s steps, up to 20 min) until fewer than 2 rendering Unreal processes run on the machine (Lead's cap).
for i in $(seq 1 40); do
  n=$(powershell -NoProfile -Command "@(Get-CimInstance Win32_Process | ? { (\$_.Name -like 'UnrealEditor*' -or \$_.Name -like 'unrealGameJam*') -and \$_.CommandLine -notmatch 'nullrhi' }).Count" | tr -d "")
  [ "${n:-9}" -lt 2 ] && { echo "slot free ($n running)"; exit 0; }
  sleep 30
done
echo "NO SLOT after 20 min"; exit 1
