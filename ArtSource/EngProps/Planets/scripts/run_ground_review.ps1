<#
  Render the ground-material review (ue_render_ground_review.py) in an offscreen editor with a renderer and
  -NoTextureStreaming (every mip resident from load, so captures are deterministic), then delete the throwaway
  Review level. Captures: Saved\GroundTiling\<Tag>\<Planet>_<view>_<Tag>.png
    powershell -File ArtSource\EngProps\Planets\scripts\run_ground_review.ps1 -Tag after
  -Set "Verdance.EmissiveStrength=0;Cinder.FarScale=4" applies in-memory instance overrides (never saved).
#>
param([string]$Tag = "review", [string]$Source = "SCS_FINAL_COLOR_LDR", [string]$Set = "", [int]$TimeoutSeconds = 900)
$ErrorActionPreference = "Stop"
$repo = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..\..")).Path
. (Join-Path $repo "Tools\td-common.ps1")
$project = Get-TDProjectFile $repo
$engine = Get-TDEngineRoot $project
$editorCmd = Join-Path $engine "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$script = Join-Path $PSScriptRoot "ue_render_ground_review.py"
$logFile = Join-Path $repo ("Saved\Logs\ground-review_" + (Get-Date -Format "yyyyMMdd_HHmmss") + ".log")
$env:GT_TAG = $Tag; $env:GT_SOURCE = $Source; $env:GT_SET = $Set
if (-not $env:GT_BIAS_C) { $env:GT_BIAS_C = "-3.5" }
if (-not $env:GT_BIAS_V) { $env:GT_BIAS_V = "0" }
$args_ = @("`"$project`"", "-ExecutePythonScript=`"$script`"", "-unattended", "-nopause", "-nosplash",
           "-RenderOffscreen", "-NoTextureStreaming", "-nosound", "-NoLiveCoding",
           "-ModelContextProtocolPort=$(Get-Random -Minimum 21000 -Maximum 29000)", "-abslog=`"$logFile`"")
$p = Start-Process -FilePath $editorCmd -ArgumentList $args_ -PassThru -WindowStyle Hidden
if (-not $p.WaitForExit($TimeoutSeconds * 1000)) { try { $p.Kill() } catch {}; Write-Output "TIMED OUT ($logFile)" }
$review = Join-Path $repo "Content\TriadDrift\Gameplay\Props\Planets\Review"
if (Test-Path $review) { Remove-Item -Recurse -Force $review }
Select-String -Path $logFile -Pattern "TDPY:|LogPython: Error|Failed to compile|: error" | Select-Object -First 40 | ForEach-Object { $_.Line }
Write-Output "log: $logFile"
