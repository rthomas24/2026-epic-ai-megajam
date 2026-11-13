<#
  Render the magma surface review (ue_render_magma_review.py) in an offscreen editor with a renderer and
  -NoTextureStreaming, then delete the throwaway review level. Captures: Saved\MagmaSurface\<Tag>\<mode>_<view>_<Tag>.png
    powershell -File ArtSource\EngProps\PlanetKit\scripts\run_magma_review.ps1 -Tag r1
  -Set "EmissiveStrength=5;ChannelMax=0.4" applies in-memory MI overrides (never saved); -Modes after skips "before".
#>
param([string]$Tag = "review", [string]$Set = "", [string]$Modes = "before,after", [string]$Bias = "-3.5", [int]$TimeoutSeconds = 900)
$ErrorActionPreference = "Stop"
$repo = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..\..")).Path
. (Join-Path $repo "Tools\td-common.ps1")
$project = Get-TDProjectFile $repo
$engine = Get-TDEngineRoot $project
$editorCmd = Join-Path $engine "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$script = Join-Path $PSScriptRoot "ue_render_magma_review.py"
$logFile = Join-Path $repo ("Saved\Logs\magma-review_" + (Get-Date -Format "yyyyMMdd_HHmmss") + ".log")
$env:MG_TAG = $Tag; $env:MG_SET = $Set; $env:MG_MODES = $Modes; $env:MG_BIAS = $Bias
$args_ = @("`"$project`"", "-ExecutePythonScript=`"$script`"", "-unattended", "-nopause", "-nosplash",
           "-RenderOffscreen", "-NoTextureStreaming", "-nosound", "-NoLiveCoding",
           "-ModelContextProtocolPort=$(Get-Random -Minimum 21000 -Maximum 29000)", "-abslog=`"$logFile`"")
$p = Start-Process -FilePath $editorCmd -ArgumentList $args_ -PassThru -WindowStyle Hidden
if (-not $p.WaitForExit($TimeoutSeconds * 1000)) { try { $p.Kill() } catch {}; Write-Output "TIMED OUT ($logFile)" }
$review = Join-Path $repo "Content\TriadDrift\Gameplay\Props\Planet\Review_MagmaTmp"
if (Test-Path $review) { Remove-Item -Recurse -Force $review }
Select-String -Path $logFile -Pattern "TDPY:|LogPython: Error|Failed to compile|: error" | Select-Object -First 40 | ForEach-Object { $_.Line }
Write-Output "log: $logFile"
