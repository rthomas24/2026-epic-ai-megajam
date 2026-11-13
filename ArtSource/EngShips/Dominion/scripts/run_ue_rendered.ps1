<#
  Run an Unreal editor Python script WITH a renderer (offscreen), so materials can compile (Tools/td-python.ps1
  uses -nullrhi, where MaterialEditingLibrary.get_statistics refuses to compile). Same TDPY/OK convention.
  powershell -File ArtSource\EngShips\Dominion\scripts\run_ue_rendered.ps1 -Script <script.py>
#>
param([Parameter(Mandatory = $true)][string]$Script, [int]$TimeoutSeconds = 1500)
$ErrorActionPreference = "Stop"
$repo = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..\..")).Path
. (Join-Path $repo "Tools\td-common.ps1")
$project = Get-TDProjectFile $repo
$engine = Get-TDEngineRoot $project
$editorCmd = Join-Path $engine "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$scriptPath = (Resolve-Path $Script).Path
$logFile = Join-Path $repo ("Saved\Logs\dominion-rendered_" + (Get-Date -Format "yyyyMMdd_HHmmss") + ".log")
$args_ = @("`"$project`"", "-ExecutePythonScript=`"$scriptPath`"", "-unattended", "-nopause", "-nosplash",
           "-RenderOffscreen", "-nosound", "-NoLiveCoding", "-ModelContextProtocolPort=$(Get-Random -Minimum 21000 -Maximum 29000)",
           "-abslog=`"$logFile`"")
$p = Start-Process -FilePath $editorCmd -ArgumentList $args_ -PassThru -WindowStyle Hidden
if (-not $p.WaitForExit($TimeoutSeconds * 1000)) { try { $p.Kill() } catch {}; Write-Output "TIMED OUT ($logFile)"; exit 3 }
Select-String -Path $logFile -Pattern "TDPY:|LogPython: Error|Failed to compile|\[SM6\]|error X|: error" | Select-Object -First 60 | ForEach-Object { $_.Line }
Write-Output "log: $logFile"
