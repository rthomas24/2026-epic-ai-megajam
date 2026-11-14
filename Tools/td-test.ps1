<#
.SYNOPSIS
  Run Triad Drift automation tests headless for THIS checkout or worktree.
  Exit code 0 = every matched test passed (and at least one test ran).
.EXAMPLE
  .\Tools\td-test.ps1                       # everything under "TriadDrift"
  .\Tools\td-test.ps1 -Filter TriadDrift.Combat
  .\Tools\td-test.ps1 -Filter TriadDrift.Flight -ShowLog
#>
param(
    [string]$Filter = "TriadDrift",
    [string]$ProjectRoot,
    [int]$TimeoutSeconds = 900,
    [switch]$ShowLog
)
$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "td-common.ps1")
# Skip the editor's startup SDK check (Build.bat -Mode=ValidatePlatforms): it queues behind every other
# worktree's UnrealBuildTool run and stalled agent launches for 5-20 minutes. Win64 only; builds and packaging unaffected.
$env:UE_SKIP_UBT_SDK_SETUP = '1'

$project = Get-TDProjectFile $ProjectRoot
$root = Split-Path -Parent $project
$engine = Get-TDEngineRoot $project
$editorCmd = Join-Path $engine "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"

$stamp = Get-Date -Format "yyyyMMdd_HHmmss_fff"
$logDir = Join-Path $root "Saved\Logs"
New-Item -ItemType Directory -Force -Path $logDir | Out-Null
$logFile = Join-Path $logDir "td-test_$stamp.log"

$arguments = @(
    "`"$project`"",
    "-ExecCmds=`"Automation RunTests $Filter; Quit`"",
    "-TestExit=`"Automation Test Queue Empty`"",
    "-unattended", "-nopause", "-nosplash", "-nullrhi", "-nosound", "-NoLiveCoding",
    "-ModelContextProtocolPort=$(Get-Random -Minimum 21000 -Maximum 29000)",
    "-abslog=`"$logFile`""
)

$sw = [Diagnostics.Stopwatch]::StartNew()
$process = Start-Process -FilePath $editorCmd -ArgumentList $arguments -PassThru -WindowStyle Hidden
if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
    try { $process.Kill() } catch {}
    Write-Output "TESTS TIMED OUT after $TimeoutSeconds s (log: $logFile)"
    exit 3
}

if (-not (Test-Path $logFile)) { Write-Output "TESTS FAILED: no log produced ($logFile)"; exit 2 }
$log = Get-Content $logFile

$results = @()
foreach ($line in $log) {
    if ($line -match 'Test Completed\. Result=\{(?<result>[^}]+)\} Name=\{(?<name>[^}]*)\} Path=\{(?<path>[^}]*)\}') {
        $results += [pscustomobject]@{ Result = $Matches.result; Path = $Matches.path }
    }
}
$passed = @($results | Where-Object { $_.Result -match '^(Passed|Success)$' })
$failed = @($results | Where-Object { $_.Result -notmatch '^(Passed|Success)$' })

foreach ($r in $results) { Write-Output ("[{0}] {1}" -f $r.Result.ToUpper(), $r.Path) }
if ($failed.Count -gt 0) {
    Write-Output "---- FAILURE DETAILS ----"
    $log | Where-Object { $_ -match 'LogAutomationController: (Error|Warning)|Error: .*(Expected|Test)' } |
        Select-Object -First 60 | ForEach-Object { Write-Output $_ }
}
$crash = $log | Where-Object { $_ -match 'Fatal error|Unhandled Exception|Assertion failed|Ensure condition failed' } | Select-Object -First 10
if ($crash) { Write-Output "---- CRASH / ASSERT ----"; $crash | ForEach-Object { Write-Output $_ } }
if ($ShowLog) { $log | Select-Object -Last 80 | ForEach-Object { Write-Output $_ } }

$ok = ($results.Count -gt 0) -and ($failed.Count -eq 0) -and (-not $crash)
$status = if ($ok) { "PASSED" } else { "FAILED" }
if ($results.Count -eq 0) { Write-Output "No tests matched '$Filter' (or the editor failed to start). Log: $logFile" }
Write-Output ("TESTS {0}: {1} passed, {2} failed, filter '{3}', {4:N0}s  (log: {5})" -f $status, $passed.Count, $failed.Count, $Filter, $sw.Elapsed.TotalSeconds, $logFile)
if ($ok) { exit 0 } else { exit 1 }
