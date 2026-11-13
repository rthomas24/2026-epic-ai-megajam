param([ValidateSet('tests','game')][string]$Phase,[switch]$LeadCleared,[ValidateRange(1,9)][int]$Attempt=1)
$ErrorActionPreference='Stop'
if(-not $LeadCleared){throw 'Requires explicit current Lead clearance'}
$taskRoot=$PSScriptRoot
while(-not (Test-Path -LiteralPath (Join-Path $taskRoot 'unrealGameJam.uproject'))){$taskParent=Split-Path -Parent $taskRoot;if(-not $taskParent -or $taskParent -eq $taskRoot){throw 'Project root not found'};$taskRoot=$taskParent}
$taskFreeGiB=(Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory/1MB
$taskHeavy=@(Get-CimInstance Win32_Process -Filter "Name='UnrealEditor.exe' OR Name='UnrealEditor-Cmd.exe' OR Name='blender.exe' OR Name='cl.exe' OR Name='ShaderCompileWorker.exe'")
if($taskFreeGiB -lt 4.5 -or $taskHeavy.Count){throw 'Fresh resource guard failed'}
$taskProject=Join-Path $taskRoot 'unrealGameJam.uproject'
$taskLabel='{0}-{1:D2}' -f $Phase,$Attempt
$taskLog=Join-Path $taskRoot "Saved/Logs/M03Objective-$taskLabel.log"
$taskResult=Join-Path $taskRoot "Saved/Logs/M03Objective-$taskLabel.result.json"
$taskEngine='C:/Program Files/Epic Games/UE_5.9/Engine/Binaries/Win64'
$taskArgs=@("`"$taskProject`"",'-NoLiveCoding','-nosound','-nosplash','-unattended','-nopause','-ModelContextProtocolPort=8023','-ModelContextProtocolStartServer',"-abslog=`"$taskLog`"","-ResultFile=`"$taskResult`"",'-ini:Engine:[DevOptions.Shaders]:NumUnusedShaderCompilingThreads=15')
if($Phase -eq 'tests'){
    $taskExe=Join-Path $taskEngine 'UnrealEditor-Cmd.exe'
    $taskArgs+=@('-AgentMode','-nullrhi','-ExecCmds="Automation RunTests TriadDrift.UI; Quit"','-TestExit="Automation Test Queue Empty"','-ExitTimeout=180','-NoProgressTimeout=90','-HangTimeout=60')
    $taskOuterSeconds=240
} else {
    $taskExe=Join-Path $taskEngine 'UnrealEditor.exe'
    $taskArgs=@("`"$taskProject`"",'/Game/TriadDrift/Maps/L_Galaxy')+$taskArgs[1..($taskArgs.Count-1)]
    $taskArgs+=@('-game','-windowed','-ResX=1920','-ResY=1080','-ForceRes','-Seconds=600','-ExitTimeout=660','-NoProgressTimeout=0','-HangTimeout=60')
    $taskOuterSeconds=720
}
$taskWatch=[Diagnostics.Stopwatch]::StartNew()
$taskProcess=Start-Process -FilePath $taskExe -ArgumentList $taskArgs -PassThru -WindowStyle Hidden
"START $Phase PID=$($taskProcess.Id) guardGiB=$([math]::Round($taskFreeGiB,2))"
$taskState=@{phase=$Phase;pid=$taskProcess.Id;guard_free_gib=[math]::Round($taskFreeGiB,2);head=(git -C $taskRoot rev-parse HEAD);started_utc=[DateTime]::UtcNow.ToString('o')}
$taskState | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $taskRoot "Saved/M03Objective-$Phase-state.json")
while(-not $taskProcess.WaitForExit(5000)){
    $taskAvailable=(Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory/1MB
    if($taskAvailable -lt 0.75 -or $taskWatch.Elapsed.TotalSeconds -gt $taskOuterSeconds){
        $taskProcess.Kill($true)
        throw "Own process stopped: RAM/outer bound; $taskAvailable GiB"
    }
}
$taskProcess.WaitForExit();$taskCode=$taskProcess.ExitCode
"EXIT $Phase code=$taskCode elapsed=$([math]::Round($taskWatch.Elapsed.TotalSeconds,1))"
if(Test-Path -LiteralPath $taskResult){
    $taskNative=Get-Content -Raw -LiteralPath $taskResult | ConvertFrom-Json
    "RESULT reason=$($taskNative.reason) code=$($taskNative.exit_code) duration=$($taskNative.duration_s)"
}
if($Phase -eq 'tests'){
    $taskLines=Get-Content -LiteralPath $taskLog
    $taskPass=@($taskLines | Select-String 'Test Completed\. Result=\{Success\}.*Path=\{TriadDrift.UI\.')
    $taskFailure=@($taskLines | Select-String 'Test Completed\. Result=\{(?!Success)|Fatal error|Assertion failed|Ensure condition failed')
    $taskPass | ForEach-Object {$_.Line}
    if($taskPass.Count -ne 10 -or $taskFailure.Count){throw "Expected10passed tests; actual=$($taskPass.Count), failures=$($taskFailure.Count)"}
}
exit $taskCode
