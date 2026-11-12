param([ValidateSet('Build','Verify','Diagnose')][string]$Stage = 'Build')
$ErrorActionPreference = 'Stop'
$nativeRoot = $PSScriptRoot
$repoRoot = (Resolve-Path (Join-Path $nativeRoot '../../../..')).Path
$engineRoot = 'C:\Program Files\Epic Games\UE_5.9'
$freeGiB = (Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory / 1MB
$competing = Get-Process UnrealEditor*,blender*,ShaderCompileWorker*,cl -ErrorAction SilentlyContinue
if ($freeGiB -lt 4.0 -or $competing) { throw "Heavy slot unavailable: $freeGiB GiB free or competing process" }
# Run only after explicit Lead slot clearance. All host configuration is disposable,
# local, ignored, and inside the owned lane. No shared project files are modified.
$hostRoot = Join-Path $nativeRoot 'Saved/Host'
$contentRoot = Join-Path $repoRoot 'Content/TriadDrift/Environment/SpaceFoundation'
New-Item -ItemType Directory -Path $hostRoot,$contentRoot -Force | Out-Null
$mountParent = Join-Path $hostRoot 'Content/TriadDrift/Environment'
New-Item -ItemType Directory -Path $mountParent -Force | Out-Null
$mountPath = Join-Path $mountParent 'SpaceFoundation'
if (!(Test-Path -LiteralPath $mountPath)) {
    New-Item -ItemType Junction -Path $mountPath -Target $contentRoot | Out-Null
}
$hostProject = Join-Path $hostRoot 'EnvironmentVFXHost.uproject'
@{ FileVersion=3; EngineAssociation='{D15CB821-4A61-18EC-84FD-0EAD60EA46EA}';
   Plugins=@(@{Name='PythonScriptPlugin';Enabled=$true},@{Name='EditorScriptingUtilities';Enabled=$true})
} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $hostProject -Encoding utf8
$scriptName = switch ($Stage) { 'Build' { 'build_native.py' } 'Verify' { 'verify_native.py' } 'Diagnose' { 'diagnose_capture.py' } }
$scriptPath = Join-Path $nativeRoot $scriptName
$logPath = Join-Path $nativeRoot "Saved/$Stage.log"
& (Join-Path $engineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') $hostProject `
  '-run=pythonscript' "-script=$scriptPath" '-AllowCommandletRendering' '-unattended' `
  '-nosound' '-nosplash' '-NoSourceControl' '-NoLiveCoding' '-noraytracing' `
  '-NoShaderWorker' '-UTF8Output' "-abslog=$logPath"
exit $LASTEXITCODE
