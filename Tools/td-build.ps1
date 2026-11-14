<#
.SYNOPSIS
  Compile the editor target for THIS checkout or worktree. Exit code 0 = clean build.
.EXAMPLE
  .\Tools\td-build.ps1
  .\Tools\td-build.ps1 -Configuration DebugGame
#>
param(
    [string]$ProjectRoot,
    [ValidateSet("Development", "DebugGame")] [string]$Configuration = "Development",
    [string]$Target = "unrealGameJamEditor"
)
$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "td-common.ps1")

$project = Get-TDProjectFile $ProjectRoot
$engine = Get-TDEngineRoot $project
$buildBat = Join-Path $engine "Engine\Build\BatchFiles\Build.bat"

$editor = Get-Process UnrealEditor -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowTitle -and $_.Path }
$sw = [Diagnostics.Stopwatch]::StartNew()
# Two compiles at once on Ryan's machine (Lead, Nov 14 14:50): the engine's single machine-wide build lock made every agent
# wait 10-20 minutes. A second compile in another worktree was measured safe (282 s, both succeeded), so where the slot
# folder exists the script takes one of two slots and runs with -NoMutex; anywhere else it keeps -WaitMutex.
$slotRoot = "C:\Users\ryant\Documents\myRepos\game-jam\Builds\compile-slot"
$slot = $null
$mutexArg = "-WaitMutex"
if (Test-Path $slotRoot) {
    while (-not $slot) {
        foreach ($n in 1, 2) {
            $dir = Join-Path $slotRoot "C$n"
            if (Test-Path $dir) {
                $ownerFile = Join-Path $dir "pid.txt"
                $ownerPid = if (Test-Path $ownerFile) { (Get-Content $ownerFile -ErrorAction SilentlyContinue | Select-Object -First 1) } else { $null }
                $age = ((Get-Date) - (Get-Item $dir).CreationTime).TotalSeconds
                if (($ownerPid -and -not (Get-Process -Id $ownerPid -ErrorAction SilentlyContinue)) -or (-not $ownerPid -and $age -gt 30)) {
                    Remove-Item -Recurse -Force $dir -ErrorAction SilentlyContinue   # its owner is gone
                }
            }
            if (-not (Test-Path $dir)) {
                try { New-Item -ItemType Directory -Path $dir -ErrorAction Stop | Out-Null; Set-Content -Path (Join-Path $dir "pid.txt") -Value $PID; $slot = $dir; break } catch { }
            }
        }
        if (-not $slot) { Start-Sleep -Seconds 5 }
    }
    $mutexArg = "-NoMutex"
}
try {
    $output = & $buildBat $Target Win64 $Configuration "-Project=$project" $mutexArg -NoHotReload 2>&1 | ForEach-Object { "$_" }
    $code = $LASTEXITCODE
} finally {
    if ($slot) { Remove-Item -Recurse -Force $slot -ErrorAction SilentlyContinue }
}

$errors = $output | Where-Object { $_ -match '(: error |: fatal error |error C\d+|error LNK\d+|Error: |ERROR: )' }
$warnings = $output | Where-Object { $_ -match '(: warning |warning C\d+)' -and $_ -notmatch 'Engine\\Source' }
$toolchain = $output | Where-Object { $_ -match 'Using Visual Studio' } | Select-Object -First 1

if ($toolchain) { Write-Output $toolchain }
if ($errors) { Write-Output "---- ERRORS ($(@($errors).Count)) ----"; $errors | Select-Object -First 60 | ForEach-Object { Write-Output $_ } }
if ($warnings) { Write-Output "---- WARNINGS ($(@($warnings).Count)) ----"; $warnings | Select-Object -First 30 | ForEach-Object { Write-Output $_ } }
if ($code -ne 0 -and -not $errors) { Write-Output "---- LAST OUTPUT ----"; $output | Select-Object -Last 25 | ForEach-Object { Write-Output $_ } }

$result = if ($code -eq 0) { "SUCCEEDED" } else { "FAILED" }
Write-Output ("BUILD {0} in {1:N0}s  errors={2} warnings={3}  ({4})" -f $result, $sw.Elapsed.TotalSeconds, @($errors).Count, @($warnings).Count, $project)
exit $code
