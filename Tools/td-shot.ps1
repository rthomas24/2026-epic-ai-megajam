<#
.SYNOPSIS
  Launch the game unattended, capture screenshots at given times, run console commands, quit.
  Prints the PNG paths so an agent can look at them. Exit code 0 = at least one screenshot written.
.EXAMPLE
  .\Tools\td-shot.ps1                                         # space sandbox, shots at 3, 6, 10 s
  .\Tools\td-shot.ps1 -Shots "2,5" -Exec "TD.God" -ExecAt "3:TD.SkipWave"
  .\Tools\td-shot.ps1 -GameMode /Script/unrealGameJam.TDGroundGameMode -Name ground
#>
param(
    [string]$ProjectRoot,
    [string]$Map = "/Engine/Maps/Entry",
    [string]$GameMode = "/Script/unrealGameJam.TDSpaceGameMode",
    [string]$Shots = "3,6,10",
    [string]$Exec = "",
    [string[]]$ExecAt = @(),
    [string]$Name = "sandbox",
    [int]$ResX = 1280,
    [int]$ResY = 720,
    [int]$TimeoutSeconds = 240
)
$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "td-common.ps1")

$project = Get-TDProjectFile $ProjectRoot
$root = Split-Path -Parent $project
$engine = Get-TDEngineRoot $project
$editor = Join-Path $engine "Engine\Binaries\Win64\UnrealEditor.exe"

$stamp = Get-Date -Format "MMdd_HHmmss"
$shotDir = Join-Path $root "Saved\TDShots\${Name}_$stamp"
New-Item -ItemType Directory -Force -Path $shotDir | Out-Null
$logFile = Join-Path $root "Saved\Logs\td-shot_${Name}_$stamp.log"
$shotList = @($Shots -split "[,; ]+" | Where-Object { $_ } | ForEach-Object { [double]::Parse($_, [Globalization.CultureInfo]::InvariantCulture) })
$quit = ($shotList | Measure-Object -Maximum).Maximum + 1.5
$url = if ($GameMode) { "$Map`?game=$GameMode" } else { $Map }

$arguments = @(
    "`"$project`"", $url, "-game", "-windowed", "-ResX=$ResX", "-ResY=$ResY", "-nosplash", "-nosound", "-unattended",
    "-NoLiveCoding", "-TDShots=$(($shotList | ForEach-Object { $_.ToString([Globalization.CultureInfo]::InvariantCulture) }) -join ',')", "-TDShotDir=`"$shotDir`"", "-TDQuit=$quit", "-abslog=`"$logFile`""
)
if ($Exec) { $arguments += "-TDExec=`"$Exec`"" }
foreach ($e in $ExecAt) { $arguments += "-TDExecAt=$e" }

$sw = [Diagnostics.Stopwatch]::StartNew()
$process = Start-Process -FilePath $editor -ArgumentList $arguments -PassThru
if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
    try { $process.Kill() } catch {}
    Write-Output "SHOT TIMED OUT after $TimeoutSeconds s (log: $logFile)"
}

$files = Get-ChildItem $shotDir -Filter *.png -ErrorAction SilentlyContinue | Sort-Object Name
foreach ($f in $files) { Write-Output $f.FullName }
if (Test-Path $logFile) {
    $problems = Get-Content $logFile | Where-Object { $_ -match 'Fatal error|Unhandled Exception|Assertion failed|Ensure condition failed|LogTriadDrift: (Error|Warning)' } | Select-Object -First 15
    if ($problems) { Write-Output "---- LOG PROBLEMS ----"; $problems | ForEach-Object { Write-Output $_ } }
}
Write-Output ("SHOTS: {0} written in {1:N0}s  (log: {2})" -f @($files).Count, $sw.Elapsed.TotalSeconds, $logFile)
if (@($files).Count -gt 0) { exit 0 } else { exit 1 }
