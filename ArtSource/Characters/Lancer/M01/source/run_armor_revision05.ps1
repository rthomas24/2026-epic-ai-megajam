param([ValidateSet('BUILD','RENDER')][string]$Phase,[switch]$LeadCleared)
$ErrorActionPreference='Stop'
if(-not $LeadCleared){throw 'Fresh explicit grant required for this phase'}
$taskOwned=Split-Path -Parent $PSScriptRoot;$taskRoot=$taskOwned
while(-not (Test-Path -LiteralPath (Join-Path $taskRoot 'unrealGameJam.uproject'))){$taskRoot=Split-Path -Parent $taskRoot;if(-not $taskRoot){throw 'Root missing'}}
if((git -C $taskRoot branch --show-current) -ne 'nick/characters-lancer'){throw 'Wrong branch'}
if([IO.Path]::GetFullPath($taskOwned) -ne [IO.Path]::GetFullPath((Join-Path $taskRoot 'ArtSource/Characters/Lancer/M01'))){throw 'Outside exact owned root'}
$taskHeavy=@(Get-CimInstance Win32_Process -Filter "Name='UnrealEditor.exe' OR Name='UnrealEditor-Cmd.exe' OR Name='ShaderCompileWorker.exe' OR Name='blender.exe' OR Name='cl.exe' OR Name='dotnet.exe'")
$taskFree=(Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory/1MB
if($taskHeavy.Count -or $taskFree -lt 4){throw 'Fresh zero-heavy/4GiB guard failed'}
$taskSaved=Join-Path $taskRoot "Saved/Characters/Lancer/M01/ArmorRevision05-$Phase"
if(Test-Path -LiteralPath $taskSaved){throw 'Attempt exists; no overwrite or automatic retry'}
$taskInput=if($Phase -eq 'BUILD'){Join-Path $taskOwned 'source/base/Lancer_Manny_Reference_v02.blend'}else{Join-Path $taskOwned 'source/armor/Revision05/Lancer_Armor_Revision05.blend'}
$taskOutput=if($Phase -eq 'BUILD'){Join-Path $taskOwned 'source/armor/Revision05'}else{Join-Path $taskOwned 'review/Revision05'}
if(Test-Path -LiteralPath $taskOutput){throw 'Output exists'}
$taskBefore=@{};Get-ChildItem -LiteralPath (Join-Path $taskOwned 'source/base') -File | ForEach-Object {$taskBefore[$_.FullName]=(Get-FileHash -LiteralPath $_.FullName).Hash}
$taskInputHash=(Get-FileHash -LiteralPath $taskInput).Hash
New-Item -ItemType Directory -Path $taskSaved | Out-Null
$taskScript=if($Phase -eq 'BUILD'){'build_lancer_revision05.py'}else{'render_lancer_revision05.py'}
$taskProcess=$null;$taskError=$null;$taskCode=$null;$taskTechnical=$null;$taskOld=$env:TD_LANCER_BLOCKOUT_CLEARED;$taskWatch=[Diagnostics.Stopwatch]::StartNew()
try{
    $env:TD_LANCER_BLOCKOUT_CLEARED=$Phase
    try{$taskProcess=Start-Process -FilePath 'C:/Program Files/Blender Foundation/Blender 5.1/blender.exe' -ArgumentList @('--background','--factory-startup',('"{0}"' -f $taskInput),'--threads','2','--python-exit-code','1','--python',('"{0}"' -f (Join-Path $PSScriptRoot $taskScript))) -PassThru -WindowStyle Hidden -WorkingDirectory $taskSaved -RedirectStandardOutput (Join-Path $taskSaved 'stdout.log') -RedirectStandardError (Join-Path $taskSaved 'stderr.log')}
    finally{$env:TD_LANCER_BLOCKOUT_CLEARED=$taskOld}
    Write-Output "START ArmorRevision05-$Phase PID=$($taskProcess.Id) guardGiB=$taskFree"
    @{pid=$taskProcess.Id;phase=$Phase;utc=[DateTime]::UtcNow.ToString('o')} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $taskSaved 'process.json')
    while(-not $taskProcess.WaitForExit(2000)){
        if($taskWatch.Elapsed.TotalSeconds -gt 120 -or (Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory/1MB -lt 1.5){throw 'Own timeout/pressure stop; preserve, no retry'}
    }
    $taskProcess.WaitForExit();$taskCode=$taskProcess.ExitCode
    if($taskCode -ne 0){throw "Blender failed $taskCode"}
    $taskMarker=if($Phase -eq 'BUILD'){'LANCER_BLOCKOUT_BUILD_OK'}else{'LANCER_BLOCKOUT_RENDER_OK'}
    if(-not (Select-String -LiteralPath (Join-Path $taskSaved 'stdout.log') -SimpleMatch $taskMarker -Quiet)){throw 'Success marker missing'}
    $taskQA=Get-Content -Raw -LiteralPath (Join-Path $taskOwned 'source/armor/Revision05/blockout-qa.json') | ConvertFrom-Json
    $taskTechnical=$taskQA.technical_checks_passed
    if(-not $taskTechnical){Write-Output 'QA_FAILED: diagnostic output only; process success is not technical acceptance'}
}catch{$taskError=$_.Exception.Message;if($null -ne $taskProcess){if(-not $taskProcess.HasExited){$taskProcess.Kill($true);$taskProcess.WaitForExit()};$taskCode=$taskProcess.ExitCode}}
finally{
    $taskChanged=@($taskBefore.Keys | Where-Object {(Get-FileHash -LiteralPath $_).Hash -ne $taskBefore[$_]})
    if($taskChanged.Count -or (Get-FileHash -LiteralPath $taskInput).Hash -ne $taskInputHash){$taskError='Frozen input changed'}
    @{pass=($null -eq $taskError);pass_scope='Process completion only';technical_checks_passed=$taskTechnical;failure=$taskError;os_exit_code=$taskCode;duration_s=$taskWatch.Elapsed.TotalSeconds;phase=$Phase;guard_free_gib=$taskFree;protected_changed=$taskChanged;input_sha256=$taskInputHash} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $taskSaved 'verification.json')
}
if($taskError){throw $taskError}
'PASS phase '+$Phase+'; geometric and subjective acceptance depend on QA/view review'
