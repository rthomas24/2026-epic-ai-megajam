param([ValidateSet('Build','Capture')][string]$Stage='Build')
$ErrorActionPreference='Stop'
$heroRoot=$PSScriptRoot
$repoRoot=(Resolve-Path (Join-Path $heroRoot '../../../../..')).Path
$freeGiB=(Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory/1MB
if($freeGiB -lt 4 -or (Get-Process UnrealEditor*,blender*,ShaderCompileWorker*,cl -ErrorAction SilentlyContinue)){
    throw "Heavy slot unavailable; free RAM $freeGiB GiB"
}
# Lead must explicitly schedule the slot. Disposable host stays in this owned lane.
$hostRoot=Join-Path $heroRoot 'Saved/Host'
New-Item -ItemType Directory -Path $hostRoot -Force | Out-Null
$hostConfig=Join-Path $hostRoot 'Config'
New-Item -ItemType Directory -Path $hostConfig -Force | Out-Null
$projectEngineConfig=Join-Path $repoRoot 'Config/DefaultEngine.ini'
$rendererSection=[regex]::Match([System.IO.File]::ReadAllText($projectEngineConfig),'(?ms)^\[/Script/Engine.RendererSettings\]\r?\n.*?(?=^\[|\z)').Value
if(!$rendererSection){throw 'Missing real-project RendererSettings; refusing a mismatched host'}
$privatePlatform=@'
[/Script/WindowsTargetPlatform.WindowsTargetSettings]
DefaultGraphicsRHI=DefaultGraphicsRHI_DX12
-D3D12TargetedShaderFormats=PCD3D_SM5
+D3D12TargetedShaderFormats=PCD3D_SM6

[DevOptions.Shaders]
bAllowCompilingThroughWorkers=True
NumUnusedShaderCompilingThreads=15
NumUnusedShaderCompilingThreadsDuringGame=15
ShaderCompilerCoreCountThreshold=999
'@
($rendererSection+$privatePlatform) | Set-Content -LiteralPath (Join-Path $hostConfig 'DefaultEngine.ini') -Encoding utf8
$copiedRendererSection=[regex]::Match([System.IO.File]::ReadAllText((Join-Path $hostConfig 'DefaultEngine.ini')),'(?ms)^\[/Script/Engine.RendererSettings\]\r?\n.*?(?=^\[|\z)').Value
if($copiedRendererSection -cne $rendererSection){throw 'Private renderer section differs from source'}
@{source_config_sha256=(Get-FileHash -LiteralPath $projectEngineConfig -Algorithm SHA256).Hash;
  source_config=$projectEngineConfig;renderer_section=$rendererSection;copied_section_exact=$true;
  target_shader_workers=1;memory_pressure_settings='inherited engine defaults; no override';shared_file_modified=$false} |
  ConvertTo-Json | Set-Content -LiteralPath (Join-Path $heroRoot 'Saved/renderer_config_snapshot.json') -Encoding utf8
$mountParent=Join-Path $hostRoot 'Content/TriadDrift/Environment'
New-Item -ItemType Directory -Path $mountParent -Force | Out-Null
$mount=Join-Path $mountParent 'SpaceFoundation'
$content=(Resolve-Path (Join-Path $repoRoot 'Content/TriadDrift/Environment/SpaceFoundation')).Path
if(!(Test-Path -LiteralPath $mount)){New-Item -ItemType Junction -Path $mount -Target $content | Out-Null}
if((Get-Item -LiteralPath $mount).Target -ne $content){throw 'Unexpected content mount'}
$project=Join-Path $hostRoot 'HeroM02Host.uproject'
@{FileVersion=3;EngineAssociation='{D15CB821-4A61-18EC-84FD-0EAD60EA46EA}';
  Plugins=@(@{Name='PythonScriptPlugin';Enabled=$true},@{Name='EditorScriptingUtilities';Enabled=$true})
} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $project -Encoding utf8
$script=if($Stage -eq 'Build'){'build_hero.py'}else{'capture_hero.py'}
$ueArgs=@($project,'-AgentMode','-run=pythonscript',"-script=$(Join-Path $heroRoot $script)",
  '-AllowCommandletRendering','-ExitTimeout=900','-NoProgressTimeout=600','-ModelContextProtocolPort=8024',
  '-d3d12','-sm6','-nosound','-NoSourceControl','-NoLiveCoding','-UTF8Output',
  "-abslog=$(Join-Path $heroRoot "Saved/$Stage.log")",
  "-ResultFile=$(Join-Path $heroRoot "Saved/$Stage.result.json")",
  "-UERunMarkerLog=$(Join-Path $heroRoot "Saved/$Stage.markers.log")")
$ueProcess=Start-Process -FilePath 'C:\Program Files\Epic Games\UE_5.9\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' `
  -ArgumentList $ueArgs -WindowStyle Hidden -PassThru `
  -RedirectStandardOutput (Join-Path $heroRoot "Saved/$Stage.stdout.log") `
  -RedirectStandardError (Join-Path $heroRoot "Saved/$Stage.stderr.log")
Write-Output "Unreal $Stage PID=$($ueProcess.Id)"
if(!$ueProcess.WaitForExit(960000)){
    $ownedProcess=Get-CimInstance Win32_Process -Filter "ProcessId=$($ueProcess.Id)"
    if($ownedProcess.CommandLine -like '*HeroM02Host.uproject*'){$ueProcess.Kill($true)}
    throw 'Unreal exceeded its bounded outer timeout'
}
$stageExitCode=$ueProcess.ExitCode
@{stage=$Stage;process_id=$ueProcess.Id;exit_code=$stageExitCode;private_mcp_port=8024;
  completed_utc=[DateTime]::UtcNow.ToString('o')} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $heroRoot "Saved/$Stage.process.json") -Encoding utf8
Write-Output "Unreal $Stage exit=$stageExitCode"
exit $stageExitCode
