<#
.SYNOPSIS
  Run an Unreal editor Python script headless for THIS checkout or worktree (no window, no GPU).
  The script's own log lines are echoed (anything it prints with the "TDPY:" prefix).
  Exit code 0 = the editor ran the script and the script printed "TDPY: OK".
.EXAMPLE
  .\Tools\td-python.ps1 -Script Tools\td-make-maps.py
#>
param(
    [Parameter(Mandatory = $true)][string]$Script,
    [string]$ProjectRoot,
    [int]$TimeoutSeconds = 900
)
$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "td-common.ps1")

$project = Get-TDProjectFile $ProjectRoot
$root = Split-Path -Parent $project
$engine = Get-TDEngineRoot $project
$editorCmd = Join-Path $engine "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$scriptPath = (Resolve-Path $Script).Path

$stamp = Get-Date -Format "yyyyMMdd_HHmmss_fff"
$logDir = Join-Path $root "Saved\Logs"
New-Item -ItemType Directory -Force -Path $logDir | Out-Null
$logFile = Join-Path $logDir "td-python_$stamp.log"

$arguments = @(
    "`"$project`"",
    "-ExecutePythonScript=`"$scriptPath`"",
    "-unattended", "-nopause", "-nosplash", "-nullrhi", "-nosound", "-NoLiveCoding",
    "-abslog=`"$logFile`""
)

$sw = [Diagnostics.Stopwatch]::StartNew()
$process = Start-Process -FilePath $editorCmd -ArgumentList $arguments -PassThru -WindowStyle Hidden
if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
    try { $process.Kill() } catch {}
    Write-Output "PYTHON TIMED OUT after $TimeoutSeconds s (log: $logFile)"
    exit 3
}
if (-not (Test-Path $logFile)) { Write-Output "PYTHON FAILED: no log produced ($logFile)"; exit 2 }

$lines = Select-String -Path $logFile -Pattern "TDPY:" | ForEach-Object { ($_.Line -replace "^.*?TDPY:", "TDPY:") }
$lines | ForEach-Object { Write-Output $_ }
$errors = Select-String -Path $logFile -Pattern "LogPython: Error" | ForEach-Object { $_.Line }
$errors | Select-Object -First 30 | ForEach-Object { Write-Output $_ }

$ok = ($lines | Where-Object { $_ -match "TDPY: OK" }).Count -gt 0
$seconds = [int]$sw.Elapsed.TotalSeconds
if ($ok -and -not $errors) {
    Write-Output "PYTHON OK in ${seconds}s  (log: $logFile)"
    exit 0
}
Write-Output "PYTHON FAILED in ${seconds}s  (log: $logFile)"
exit 1
