# One render-slot turn for the structures builder (Builds/render-slot/README.txt, CURRENT RULES):
# queue in NEXT.txt, wait until in the top three and a slot is free, take it, run can_launch.sh, launch ONE game
# (-TDQuit 600), run the capture script, close only that PID, release the slot.
#   powershell -File struct_slot_run.ps1 -Line "structures builder: ..." -Script struct_canopy_shots.py -ScriptArgs "before" [-Map /Game/...]
param([string]$Line, [string]$Script, [string]$ScriptArgs = "", [string]$Map = "")
$ErrorActionPreference = 'Continue'
$wt = 'C:\Users\ryant\Documents\myRepos\game-jam\.wt\struct'
$slotDir = 'C:\Users\ryant\Documents\myRepos\game-jam\Builds\render-slot'
$bash = 'C:\Program Files\Git\bin\bash.exe'
$env:UE_SKIP_UBT_SDK_SETUP = '1'
$utf8 = New-Object System.Text.UTF8Encoding($false)
function NextLines { if (Test-Path "$slotDir\NEXT.txt") { @([IO.File]::ReadAllLines("$slotDir\NEXT.txt") | Where-Object { $_.Trim() }) } else { @() } }
if (-not ((NextLines) | Where-Object { $_.Trim() -eq $Line })) { [IO.File]::AppendAllText("$slotDir\NEXT.txt", $Line + "`n", $utf8) }
$slot = $null
while (-not $slot) {
    $top = @(NextLines | Select-Object -First 3 | ForEach-Object { $_.Trim() })
    if ($top -contains $Line) {
        foreach ($n in 'LOCK', 'LOCK2', 'LOCK3') {
            try { New-Item -ItemType Directory -Path "$slotDir\$n" -ErrorAction Stop | Out-Null; $slot = "$slotDir\$n"; break } catch {}
        }
    }
    if (-not $slot) { Start-Sleep 10 }
}
[IO.File]::WriteAllText("$slot\owner.txt", ("structures builder {0} {1} (6)" -f (Get-Date -Format HH:mm), $Line), $utf8)
$rest = New-Object System.Collections.Generic.List[string]; $done = $false
foreach ($l in [IO.File]::ReadAllLines("$slotDir\NEXT.txt")) { if (-not $done -and $l.Trim() -eq $Line) { $done = $true; continue }; $rest.Add($l) }
[IO.File]::WriteAllLines("$slotDir\NEXT.txt", $rest, $utf8)
Write-Output ("{0} took {1}" -f (Get-Date -Format HH:mm:ss), (Split-Path $slot -Leaf))
$gamePid = $null
try {
    $go = $false
    for ($i = 0; $i -lt 12; $i++) { & $bash "$slotDir/can_launch.sh"; if ($LASTEXITCODE -eq 0) { $go = $true; break }; Start-Sleep 10 }
    if (-not $go) { Write-Output "guard refused for 2 minutes; slot given back"; return }
    $t0 = Get-Date
    $a = @('-File', "$wt\Tools\td-play-mcp.ps1", '-Offscreen', '-Port', '21014', '-MaxSeconds', '600')
    if ($Map) { $a += @('-Map', $Map) }
    Start-Process powershell -ArgumentList $a -WorkingDirectory $wt -WindowStyle Hidden | Out-Null
    for ($i = 0; $i -lt 30 -and -not $gamePid; $i++) {
        Start-Sleep 2
        $p = Get-CimInstance Win32_Process | Where-Object { $_.Name -match '^UnrealEditor' -and $_.CommandLine -like '*wt\struct\unrealGameJam*' -and $_.CreationDate -ge $t0 } | Select-Object -First 1
        if ($p) { $gamePid = $p.ProcessId }
    }
    Write-Output "game pid $gamePid"
    for ($i = 0; $i -lt 60; $i++) { try { $c = New-Object Net.Sockets.TcpClient; $c.Connect('127.0.0.1', 21014); $c.Close(); break } catch { Start-Sleep 5 } }
    Start-Sleep 15
    $argList = @($Script, '21014') + ($ScriptArgs -split ' ' | Where-Object { $_ })
    $py = Start-Process python -ArgumentList $argList -WorkingDirectory "$wt\ArtSource\EngProps\Kit\scripts" -NoNewWindow -PassThru -RedirectStandardOutput "$env:TEMP\struct_out.txt" -RedirectStandardError "$env:TEMP\struct_err.txt"
    if (-not $py.WaitForExit(420000)) { Write-Output "capture timed out"; Stop-Process -Id $py.Id -Force }
    Get-Content "$env:TEMP\struct_out.txt"
    Get-Content "$env:TEMP\struct_err.txt" -Tail 4
}
finally {
    if ($gamePid) { Stop-Process -Id $gamePid -Force -ErrorAction SilentlyContinue }
    Start-Sleep 3
    Remove-Item -Recurse -Force $slot
    Write-Output ("{0} closed pid {1}, released {2}" -f (Get-Date -Format HH:mm:ss), $gamePid, (Split-Path $slot -Leaf))
}
