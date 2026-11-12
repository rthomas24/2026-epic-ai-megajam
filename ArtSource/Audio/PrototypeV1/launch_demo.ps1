param([switch]$SelfTest)
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
$hostProject=Join-Path $repo 'Saved\AudioMusic\PrototypeV1\Host\TDAudioV1.uproject'
$engineExe='C:\Program Files\Epic Games\UE_5.9\Engine\Binaries\Win64\UnrealEditor.exe'
if (!(Test-Path -LiteralPath $hostProject)) { throw 'Prepare and build the private host first.' }
$freeGiB=(Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory/1MB
if ($freeGiB -lt 5) { throw ('Only {0:N1} GiB RAM free; coordinate the heavy slot with Lead.' -f $freeGiB) }
$existing=@(Get-CimInstance Win32_Process -Filter "Name='UnrealEditor.exe' OR Name='UnrealEditor-Cmd.exe' OR Name='blender.exe'")
if ($existing.Count) { throw 'Another heavy application is running. Coordinate with Lead before launching.' }
$arguments=@(('"'+$hostProject+'"'),'/Game/TriadDrift/Audio/PrototypeV1/L_AudioPrototypeV1','-game','-windowed','-ResX=1280','-ResY=720','-NoLiveCoding','-ModelContextProtocolPort=8025','-ExitTimeout=900','-NoProgressTimeout=0','-HangTimeout=120')
if ($SelfTest) {
 $arguments+=@('-AudioV1SelfTest','-RenderOffscreen','-unattended','-Seconds=25')
 $job=Start-Process -FilePath $engineExe -ArgumentList $arguments -WindowStyle Hidden -PassThru
} else {
 # This is the explicitly requested interactive listening environment.
 $job=Start-Process -FilePath $engineExe -ArgumentList $arguments -WindowStyle Normal -PassThru
}
Write-Output ('Audio prototype PID {0}; private host {1}' -f $job.Id,$hostProject)
