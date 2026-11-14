<#
.SYNOPSIS
  Launch the zero-asset space sandbox (or any map / game mode) in a window, without the editor UI.
.EXAMPLE
  .\Tools\td-play.ps1                                    # space sandbox on an empty map
  .\Tools\td-play.ps1 -Map /Game/FirstPerson/Lvl_FirstPerson -GameMode ""
#>
param(
    [string]$ProjectRoot,
    [string]$Map = "/Engine/Maps/Entry",
    [string]$GameMode = "/Script/unrealGameJam.TDSpaceGameMode",
    [int]$ResX = 1600,
    [int]$ResY = 900,
    [string]$ExtraArgs = ""
)
$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "td-common.ps1")
# Skip the editor's startup SDK check (Build.bat -Mode=ValidatePlatforms): it queues behind every other
# worktree's UnrealBuildTool run and stalled agent launches for 5-20 minutes. Win64 only; builds and packaging unaffected.
$env:UE_SKIP_UBT_SDK_SETUP = '1'

$project = Get-TDProjectFile $ProjectRoot
$engine = Get-TDEngineRoot $project
$editor = Join-Path $engine "Engine\Binaries\Win64\UnrealEditor.exe"
$url = if ($GameMode) { "$Map`?game=$GameMode" } else { $Map }
$arguments = @("`"$project`"", $url, "-game", "-windowed", "-ResX=$ResX", "-ResY=$ResY", "-log")
if ($ExtraArgs) { $arguments += $ExtraArgs }
Start-Process -FilePath $editor -ArgumentList $arguments | Out-Null
Write-Output "Launched: $url"
