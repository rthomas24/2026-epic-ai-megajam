<#
.SYNOPSIS
  Package a Windows build of THIS checkout into Builds\<name> (git-ignored). Exit code 0 = packaged.
.EXAMPLE
  .\Tools\td-package.ps1                       # Shipping
  .\Tools\td-package.ps1 -Configuration Development -Name smoke
#>
param(
    [string]$ProjectRoot,
    [ValidateSet("Shipping", "Development")] [string]$Configuration = "Shipping",
    [string]$Name
)
$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "td-common.ps1")

$project = Get-TDProjectFile $ProjectRoot
$root = Split-Path -Parent $project
$engine = Get-TDEngineRoot $project
$uat = Join-Path $engine "Engine\Build\BatchFiles\RunUAT.bat"
if (-not $Name) { $Name = "{0}_{1}" -f $Configuration, (Get-Date -Format "MMdd_HHmm") }
$archive = Join-Path $root "Builds\$Name"
$logFile = Join-Path $root "Saved\Logs\td-package_$Name.log"
New-Item -ItemType Directory -Force -Path (Split-Path $logFile) | Out-Null

$sw = [Diagnostics.Stopwatch]::StartNew()
& $uat BuildCookRun "-project=$project" -noP4 -platform=Win64 "-clientconfig=$Configuration" `
    -build -cook -stage -pak -archive "-archivedirectory=$archive" -prereqs -utf8output -unattended -nocompileeditor *> $logFile
$code = $LASTEXITCODE

$log = Get-Content $logFile
$errors = $log | Where-Object { $_ -match '(Error: |ERROR: |: error )' } | Select-Object -First 40
if ($errors) { Write-Output "---- ERRORS ----"; $errors | ForEach-Object { Write-Output $_ } }
$exe = Get-ChildItem $archive -Recurse -Filter *.exe -ErrorAction SilentlyContinue | Where-Object { $_.Name -match 'unrealGameJam' } | Select-Object -First 1
$sizeMB = if (Test-Path $archive) { (Get-ChildItem $archive -Recurse -File | Measure-Object Length -Sum).Sum / 1MB } else { 0 }
$result = if ($code -eq 0 -and $exe) { "SUCCEEDED" } else { "FAILED" }
Write-Output ("PACKAGE {0} in {1:N1} min  size={2:N0} MB  exe={3}  (log: {4})" -f $result, $sw.Elapsed.TotalMinutes, $sizeMB, $(if ($exe) { $exe.FullName } else { "none" }), $logFile)
if ($result -eq "SUCCEEDED") { exit 0 } else { exit 1 }
