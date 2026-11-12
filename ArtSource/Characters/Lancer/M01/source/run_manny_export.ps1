param([switch]$LeadCleared,[ValidateRange(1,99)][int]$Attempt=1)
$ErrorActionPreference='Stop'
if(-not $LeadCleared){throw 'Requires a fresh explicit Lead execution grant; this switch is not permission'}
$taskOwned=Split-Path -Parent $PSScriptRoot
$taskRoot=$taskOwned
while(-not (Test-Path -LiteralPath (Join-Path $taskRoot 'unrealGameJam.uproject'))){
    $taskParent=Split-Path -Parent $taskRoot
    if(-not $taskParent -or $taskParent -eq $taskRoot){throw 'Character root missing'}
    $taskRoot=$taskParent
}
$taskExpected=Join-Path $taskRoot 'ArtSource/Characters/Lancer/M01'
if([IO.Path]::GetFullPath($taskOwned) -ne [IO.Path]::GetFullPath($taskExpected)){throw 'Outside owned M01 root'}
if((git -C $taskRoot branch --show-current) -ne 'nick/characters-lancer'){throw 'Wrong branch'}
$taskHost=Join-Path (Split-Path -Parent $taskRoot) 'hud-lancer-polish'
if((git -C $taskHost rev-parse HEAD) -ne 'ca579ab7380f830647aa3d49e4f442f5cf3714a2'){throw 'Frozen HUD head changed'}
if(git -C $taskHost status --porcelain){throw 'HUD must be clean before read-only reuse'}
function Get-ResourceGuard {
    $taskFree=(Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory/1MB
    $taskOther=@(Get-CimInstance Win32_Process -Filter "Name='UnrealEditor.exe' OR Name='UnrealEditor-Cmd.exe' OR Name='blender.exe' OR Name='ShaderCompileWorker.exe' OR Name='cl.exe' OR Name='UnrealBuildTool.exe'")
    if($taskFree -lt 4.0 -or $taskOther.Count){throw "Resource guard: $taskFree GiB; $($taskOther.Count) heavy processes"}
    return [math]::Round($taskFree,3)
}
function Get-ProtectedSnapshot {
    $taskPaths=@(git -C $taskHost ls-files -- Content/Characters Content/FirstPerson Content/FirstPersonArms Content/Variant_Shooter Content/Variant_Horror Content/Weapons Content/LevelPrototyping Plugins/TriadDriftHUD Content/TriadDrift/UI/TriadHUD ArtSource/UI/TriadHUD/Unreal Config unrealGameJam.uproject)
    $taskSnapshot=@{}
    foreach($taskPath in $taskPaths){
        $taskInput=Join-Path $taskHost $taskPath
        $taskSnapshot[$taskPath]=if(Test-Path -LiteralPath $taskInput){(Get-FileHash -LiteralPath $taskInput -Algorithm SHA256).Hash.ToLowerInvariant()}else{'__MISSING__'}
    }
    if($taskSnapshot.Count -lt 10){throw 'Protected input enumeration failed'}
    return $taskSnapshot
}
$taskGuard=Get-ResourceGuard
$taskLabel='MannequinExport{0:D2}' -f $Attempt
$taskSaved=Join-Path $taskRoot "Saved/Characters/Lancer/M01/$taskLabel"
if(Test-Path -LiteralPath $taskSaved){throw 'Attempt already exists; preserve it and request a new grant'}
New-Item -ItemType Directory -Path $taskSaved | Out-Null
$taskFbx=Join-Path $taskOwned ('source/base/UE5_Manny_Simple_readonly_v{0:D2}.fbx' -f $Attempt)
$taskNative=$taskFbx -replace '\.fbx$','.native.json'
if((Test-Path -LiteralPath $taskFbx) -or (Test-Path -LiteralPath $taskNative) -or (Test-Path -LiteralPath ($taskFbx -replace '\.fbx$','.export.json'))){throw 'Never overwrite output'}
$taskCheckpoint=Get-Content -Raw -LiteralPath (Join-Path $taskOwned 'export-inputs.json') | ConvertFrom-Json
$taskBefore=Get-ProtectedSnapshot
if($taskBefore.Values -contains '__MISSING__'){throw 'Protected input missing; no launch'}
$taskBefore | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $taskSaved 'protected-before.json') -Encoding utf8
$taskScripts=@(Get-FileHash -LiteralPath $PSCommandPath,(Join-Path $PSScriptRoot 'export_manny_readonly.py') -Algorithm SHA256 | ForEach-Object {@{path=$_.Path;sha256=$_.Hash.ToLowerInvariant()}})
$taskOwnedBefore=@(Get-ChildItem -LiteralPath $taskOwned -File -Recurse | ForEach-Object {$_.FullName})
$taskContext=@{owned_root=$taskOwned;host_root=$taskHost;fbx=$taskFbx;packages=$taskCheckpoint.read_only_packages;scripts=$taskScripts}
$taskContextPath=Join-Path $taskSaved 'context.json'
$taskContext | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $taskContextPath -Encoding utf8
$taskExe='C:/Program Files/Epic Games/UE_5.9/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$taskLog=Join-Path $taskSaved 'native.log'
$taskResult=Join-Path $taskSaved 'native.result.json'
$taskScript=Join-Path $PSScriptRoot 'export_manny_readonly.py'
$taskTemp=Join-Path $taskSaved 'Temp'
New-Item -ItemType Directory -Path $taskTemp | Out-Null
$taskArgs=@(('"{0}"' -f (Join-Path $taskHost 'unrealGameJam.uproject')),'-AgentMode','-run=pythonscript',('-script="{0}"' -f $taskScript),'-nullrhi','-nosound','-nop4','-NoLiveCoding','-ModelContextProtocolPort=8023',('-UserDir="{0}/"' -f (Join-Path $taskSaved 'HostUser')),('-ZenDataPath="{0}"' -f (Join-Path $taskSaved 'Zen')),('-abslog="{0}"' -f $taskLog),('-ResultFile="{0}"' -f $taskResult),'-ExitTimeout=180','-NoProgressTimeout=60','-HangTimeout=60')
$taskOldContext=$env:TD_LANCER_EXPORT_CONTEXT;$taskOldTemp=$env:TEMP;$taskOldTmp=$env:TMP
$taskWatch=[Diagnostics.Stopwatch]::StartNew();$taskCode=$null;$taskFailure=$null;$taskPid=$null;$taskProcess=$null;$taskEngine=$null
try {
    $taskGuard=Get-ResourceGuard
    $env:TD_LANCER_EXPORT_CONTEXT=$taskContextPath;$env:TEMP=$taskTemp;$env:TMP=$taskTemp
    try {
        $taskProcess=Start-Process -FilePath $taskExe -ArgumentList $taskArgs -WorkingDirectory $taskSaved -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $taskSaved 'stdout.jsonl') -RedirectStandardError (Join-Path $taskSaved 'stderr.txt')
    } finally {$env:TD_LANCER_EXPORT_CONTEXT=$taskOldContext;$env:TEMP=$taskOldTemp;$env:TMP=$taskOldTmp}
    $taskPid=$taskProcess.Id
    @{pid=$taskPid;started_utc=[DateTime]::UtcNow.ToString('o');guard_free_gib=$taskGuard;arguments=$taskArgs} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $taskSaved 'process.json') -Encoding utf8
    Write-Output "START $taskLabel PID=$taskPid; guard=$taskGuard GiB"
    $taskPressure=0
    while(-not $taskProcess.WaitForExit(5000)){
        $taskFree=(Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory/1MB
        if($taskFree -lt 1.5){$taskPressure++}else{$taskPressure=0}
        $taskError=(Test-Path -LiteralPath $taskLog) -and (Select-String -LiteralPath $taskLog -Pattern ': Error:|Fatal error|Ensure condition failed' -Quiet)
        if($taskPressure -ge 2 -or $taskWatch.Elapsed.TotalSeconds -gt 240 -or $taskError){
            $taskProcess.Kill($true);$taskProcess.WaitForExit();throw 'Own export stopped on pressure/error/outer bound; inspect evidence, no retry'
        }
    }
    $taskProcess.WaitForExit();$taskCode=$taskProcess.ExitCode
    if($taskCode -ne 0){throw "Native process failed: $taskCode"}
    $taskEngine=Get-Content -Raw -LiteralPath $taskResult | ConvertFrom-Json
    if($taskEngine.exit_code -ne 0 -or $taskEngine.reason -ne 'ok' -or $taskEngine.log.errors -ne 0 -or $taskEngine.suppressed_dialogs.Count){throw 'Engine result not clean; no retry'}
    if(-not (Select-String -LiteralPath $taskLog -SimpleMatch 'TD_LANCER_EXPORT SCRIPT_OK' -Quiet)){throw 'Script success marker absent'}
    $taskExport=Get-Content -Raw -LiteralPath ($taskFbx -replace '\.fbx$','.export.json') | ConvertFrom-Json
    if((Get-FileHash -LiteralPath $taskFbx -Algorithm SHA256).Hash.ToLowerInvariant() -ne $taskExport.fbx.sha256){throw 'Export hash mismatch'}
    $taskUnexpected=@(Get-ChildItem -LiteralPath $taskOwned -File -Recurse | Where-Object {$_.FullName -notin $taskOwnedBefore -and $_.FullName -notin @($taskFbx,($taskFbx -replace '\.fbx$','.export.json'))})
    if($taskUnexpected.Count){throw 'Unexpected owned output/sidecar; preserve and inspect'}
} catch {
    $taskFailure=$_.Exception.Message
    if($null -ne $taskProcess){
        if(-not $taskProcess.HasExited){$taskProcess.Kill($true);$taskProcess.WaitForExit()}
        $taskCode=$taskProcess.ExitCode
    }
} finally {
    $taskAfter=Get-ProtectedSnapshot
    $taskAfter | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $taskSaved 'protected-after.json') -Encoding utf8
    $taskChanged=@($taskBefore.Keys | Where-Object {-not $taskAfter.ContainsKey($_) -or $taskBefore[$_] -ne $taskAfter[$_]}) + @($taskAfter.Keys | Where-Object {-not $taskBefore.ContainsKey($_)})
    if($taskChanged.Count -or (git -C $taskHost status --porcelain) -or (git -C $taskHost rev-parse HEAD) -ne 'ca579ab7380f830647aa3d49e4f442f5cf3714a2'){$taskFailure='Protected HUD/template/config/project input changed; stop and report'}
    $taskFinal=@{pass=($null -eq $taskFailure);pid=$taskPid;os_exit_code=$taskCode;failure=$taskFailure;duration_s=[math]::Round($taskWatch.Elapsed.TotalSeconds,2);protected_file_count=$taskBefore.Count;protected_changed=$taskChanged;guard_free_gib=$taskGuard;host_head=(git -C $taskHost rev-parse HEAD);character_head=(git -C $taskRoot rev-parse HEAD);fbx=$taskFbx;result_file=$taskResult;engine_exit_code=$taskEngine.exit_code;engine_reason=$taskEngine.reason;engine_log_errors=$taskEngine.log.errors;engine_log_warnings=$taskEngine.log.warnings}
    $taskFinal | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $taskSaved 'verification.json') -Encoding utf8
}
if($taskFailure){throw $taskFailure}
if(Test-Path -LiteralPath $taskNative){throw 'Native evidence already exists'}
$taskFinal | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $taskNative -Encoding utf8
Write-Output ('PASS '+($taskFinal | ConvertTo-Json -Depth 5 -Compress))
