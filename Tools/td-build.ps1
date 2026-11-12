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
$output = & $buildBat $Target Win64 $Configuration "-Project=$project" -WaitMutex -NoHotReload 2>&1 | ForEach-Object { "$_" }
$code = $LASTEXITCODE

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
