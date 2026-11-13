param([ValidateSet('build','tests','game')][string]$Phase,[switch]$LeadCleared,[ValidateRange(1,9)][int]$Attempt=1,[switch]$FocusedKill)
$ErrorActionPreference='Stop'
if(-not $LeadCleared){throw 'Fresh explicit Lead grant required for this phase'}
$taskRoot=$PSScriptRoot
while(-not (Test-Path -LiteralPath (Join-Path $taskRoot 'unrealGameJam.uproject'))){$taskParent=Split-Path -Parent $taskRoot;if(-not $taskParent -or $taskParent -eq $taskRoot){throw 'Project root missing'};$taskRoot=$taskParent}
if((git -C $taskRoot branch --show-current) -ne 'nick/hud-usability-20261112'){throw 'Wrong branch'}
$taskHeavy=@(Get-CimInstance Win32_Process -Filter "Name='UnrealEditor.exe' OR Name='UnrealEditor-Cmd.exe' OR Name='ShaderCompileWorker.exe' OR Name='blender.exe' OR Name='cl.exe' OR Name='dotnet.exe'")
$taskFree=(Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory/1MB
if($taskHeavy.Count -or $taskFree -lt 4){throw 'Fresh zero-heavy/4GiB guard failed'}
$taskPort=8026
if($Phase -ne 'build' -and @(Get-NetTCPConnection -LocalPort $taskPort -State Listen -ErrorAction SilentlyContinue).Count){throw 'Private MCP port occupied'}
$taskSaved=Join-Path $taskRoot ('Saved/HUDUsabilityM04/{0}-{1:D2}' -f $Phase,$Attempt)
if(Test-Path -LiteralPath $taskSaved){throw 'Attempt exists; no overwrite or automatic retry'}
$taskProject=Join-Path $taskRoot 'unrealGameJam.uproject'
$taskEngine='C:/Program Files/Epic Games/UE_5.9/Engine'
$taskProtected=@{}
Get-ChildItem -LiteralPath (Join-Path $taskRoot 'Source'),(Join-Path $taskRoot 'Config') -Recurse -File | ForEach-Object {$taskProtected[$_.FullName]=(Get-FileHash -LiteralPath $_.FullName).Hash}
$taskProtected[$taskProject]=(Get-FileHash -LiteralPath $taskProject).Hash
New-Item -ItemType Directory -Path $taskSaved | Out-Null
$taskLog=Join-Path $taskSaved 'engine.log';$taskResult=Join-Path $taskSaved 'engine-result.json'
if($Phase -eq 'build'){
    $taskExe=$env:ComSpec
    $taskArgs=@('/d','/s','/c',('""{0}" unrealGameJamEditor Win64 Development "-Project={1}" -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA"' -f (Join-Path $taskEngine 'Build/BatchFiles/Build.bat'),$taskProject))
    $taskWork=Join-Path $taskEngine 'Source';$taskBound=600
}else{
    $taskExe=Join-Path $taskEngine ('Binaries/Win64/'+$(if($Phase -eq 'tests'){'UnrealEditor-Cmd.exe'}else{'UnrealEditor.exe'}))
    $taskArgs=@(('"{0}"' -f $taskProject),'-NoLiveCoding','-nosound','-nosplash','-unattended','-nopause',"-ModelContextProtocolPort=$taskPort",'-ModelContextProtocolStartServer',('-abslog="{0}"' -f $taskLog),('-ResultFile="{0}"' -f $taskResult),'-ini:Engine:[DevOptions.Shaders]:NumUnusedShaderCompilingThreads=15')
    if($Phase -eq 'tests'){
        $taskArgs+=@('-AgentMode','-nullrhi','-ExecCmds="Automation RunTests TriadDrift.UI; Quit"','-TestExit="Automation Test Queue Empty"','-ExitTimeout=180','-NoProgressTimeout=90','-HangTimeout=60');$taskBound=240
    }else{
        $taskArgs=@(('"{0}"' -f $taskProject),'/Game/TriadDrift/Maps/L_Galaxy')+$taskArgs[1..($taskArgs.Count-1)]
        $taskSeconds=if($FocusedKill){180}else{360};$taskExit=$taskSeconds+30;$taskBound=$taskSeconds+60
        $taskArgs+=@('-game','-windowed','-ResX=1920','-ResY=1080','-ForceRes',('-UserDir="{0}/"' -f (Join-Path $taskSaved 'UserDir')),"-Seconds=$taskSeconds","-ExitTimeout=$taskExit",'-NoProgressTimeout=0','-HangTimeout=60')
    }
    $taskWork=$taskRoot
}
$taskProcess=$null;$taskError=$null;$taskCode=$null;$taskTests=@();$taskWatch=[Diagnostics.Stopwatch]::StartNew()
try{
    $taskProcess=Start-Process -FilePath $taskExe -ArgumentList $taskArgs -WorkingDirectory $taskWork -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $taskSaved 'stdout.log') -RedirectStandardError (Join-Path $taskSaved 'stderr.log')
    "START $Phase PID=$($taskProcess.Id) guardGiB=$taskFree"
    @{phase=$Phase;pid=$taskProcess.Id;port=$taskPort;started_utc=[DateTime]::UtcNow.ToString('o')} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $taskSaved 'process.json')
    while(-not $taskProcess.WaitForExit(2000)){
        if($taskWatch.Elapsed.TotalSeconds -gt $taskBound -or (Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory/1MB -lt 1.5){throw 'Own timeout/memory stop; preserve attempt, no automatic retry'}
    }
    $taskProcess.WaitForExit();$taskCode=$taskProcess.ExitCode
    if($taskCode -ne 0){throw "Native $Phase failed with exit $taskCode"}
    if($Phase -eq 'tests'){
        $taskLines=Get-Content -LiteralPath $taskLog
        $taskTests=@($taskLines | Select-String 'Test Completed\. Result=\{Success\}.*Path=\{TriadDrift.UI\.')
        $taskFailures=@($taskLines | Select-String 'Test Completed\. Result=\{(?!Success)|Fatal error|Assertion failed|Ensure condition failed')
        if($taskTests.Count -ne 14 -or $taskFailures.Count){throw "Expected14UItests; passed=$($taskTests.Count), failures=$($taskFailures.Count)"}
        $taskTests | ForEach-Object {$_.Line}
    }
}catch{
    $taskError=$_.Exception.Message
    if($null -ne $taskProcess){if(-not $taskProcess.HasExited){$taskProcess.Kill($true);$taskProcess.WaitForExit()};$taskCode=$taskProcess.ExitCode}
}finally{
    $taskChanged=@($taskProtected.Keys | Where-Object {(Get-FileHash -LiteralPath $_).Hash -ne $taskProtected[$_]})
    if($taskChanged.Count){$taskError='Protected gameplay/config/project input changed'}
    @{pass=($null -eq $taskError);phase=$Phase;failure=$taskError;os_exit_code=$taskCode;duration_s=$taskWatch.Elapsed.TotalSeconds;guard_free_gib=$taskFree;passed_ui_tests=$taskTests.Count;protected_changed=$taskChanged;head=(git -C $taskRoot rev-parse HEAD)} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $taskSaved 'verification.json')
}
if($taskError){throw $taskError}
"PASS $Phase; gameplay/visual/Shipping acceptance requires separate evidence"
