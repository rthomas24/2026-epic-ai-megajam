<#
  Render the terrain-material review (ue_render_terrain_review.py) in an offscreen editor with a renderer and
  -NoTextureStreaming, then delete the throwaway Review level. Needs Saved\TerrainMat\terrain_<Planet>.bin/.json
  (build_terrain_review_mesh.py). Captures: Saved\TerrainMat\<Tag>\<Planet>_<view>_<Tag>.png
    powershell -File ArtSource\EngProps\Planets\scripts\run_terrain_review.ps1 -Tag after
  -Tag before renders the same terrain with MI_Ground_<Planet>. -Set "Cinder.ValleyWidth=0.3" applies in-memory
  instance overrides (never saved). -NoFog leaves the height fog out. -Script runs another script the same way
  (ue_terrain_build.py to create / rebuild the materials).
#>
param([string]$Tag = "after", [string]$Set = "", [string]$Only = "Verdance,Cinder", [switch]$NoFog,
      [string]$Script = "ue_render_terrain_review.py", [int]$TimeoutSeconds = 1500)
$ErrorActionPreference = "Stop"
$repo = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..\..")).Path
. (Join-Path $repo "Tools\td-common.ps1")
$project = Get-TDProjectFile $repo
$engine = Get-TDEngineRoot $project
$editorCmd = Join-Path $engine "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$scriptPath = Join-Path $PSScriptRoot $Script
$logFile = Join-Path $repo ("Saved\Logs\terrain-mat_" + (Get-Date -Format "yyyyMMdd_HHmmss") + ".log")
$env:TM_TAG = $Tag; $env:TM_SET = $Set; $env:TM_ONLY = $Only; $env:TM_FOG = $(if ($NoFog) { "0" } else { "1" })
$args_ = @("`"$project`"", "-ExecutePythonScript=`"$scriptPath`"", "-unattended", "-nopause", "-nosplash",
           "-RenderOffscreen", "-NoTextureStreaming", "-nosound", "-NoLiveCoding",
           "-ModelContextProtocolPort=$(Get-Random -Minimum 21000 -Maximum 29000)", "-abslog=`"$logFile`"")
$p = Start-Process -FilePath $editorCmd -ArgumentList $args_ -PassThru -WindowStyle Hidden
if (-not $p.WaitForExit($TimeoutSeconds * 1000)) { try { $p.Kill() } catch {}; Write-Output "TIMED OUT ($logFile)" }
$review = Join-Path $repo "Content\TriadDrift\Gameplay\Props\Planets\Review"
if (Test-Path $review) { Remove-Item -Recurse -Force $review }
Select-String -Path $logFile -Pattern "TDPY:|LogPython: Error|Failed to compile|\[SM6\]|error X|: error|Traceback|Error:" | Select-Object -First 60 | ForEach-Object { $_.Line }
Write-Output "log: $logFile"
