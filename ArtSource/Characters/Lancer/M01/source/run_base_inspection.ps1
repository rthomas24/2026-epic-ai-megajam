param([switch]$LeadCleared)
$ErrorActionPreference='Stop'
if(-not $LeadCleared){throw 'Fresh explicit Lead grant required, including installed Blender5.1 approval'}
$taskOwned=Split-Path -Parent $PSScriptRoot
$taskRoot=$taskOwned
while(-not (Test-Path -LiteralPath (Join-Path $taskRoot 'unrealGameJam.uproject'))){$taskRoot=Split-Path -Parent $taskRoot;if(-not $taskRoot){throw 'No project root'}}
if((git -C $taskRoot branch --show-current) -ne 'nick/characters-lancer'){throw 'Wrong branch'}
$taskExpected=Join-Path $taskRoot 'ArtSource/Characters/Lancer/M01'
if([IO.Path]::GetFullPath($taskExpected) -ne [IO.Path]::GetFullPath($taskOwned)){throw 'Wrong ownership root'}
$taskHeavy=@(Get-CimInstance Win32_Process -Filter "Name='UnrealEditor.exe' OR Name='UnrealEditor-Cmd.exe' OR Name='ShaderCompileWorker.exe' OR Name='blender.exe' OR Name='cl.exe'")
$taskFree=(Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory/1MB
if($taskHeavy.Count -or $taskFree -lt 3){throw 'Resource guard failed; no launch'}
$taskSaved=Join-Path $taskRoot 'Saved/Characters/Lancer/M01/BaseInspection02'
if(Test-Path -LiteralPath $taskSaved){throw 'Attempt exists; no overwrite/retry'}
$taskBase=Join-Path $taskOwned 'source/base'
$taskBlend=Join-Path $taskBase 'Lancer_Manny_Reference_v02.blend'
$taskInventory=$taskBlend -replace '\.blend$','.inventory.json'
$taskNative=$taskBlend -replace '\.blend$','.verification.json'
foreach($taskOutput in @($taskBlend,$taskInventory,$taskNative)){if(Test-Path -LiteralPath $taskOutput){throw 'Output exists'}}
$taskInputs=@{}
Get-ChildItem -LiteralPath $taskBase -File | ForEach-Object {$taskInputs[$_.FullName]=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash}
New-Item -ItemType Directory -Path $taskSaved | Out-Null
$taskContext=Join-Path $taskSaved 'context.json'
@{owned_root=$taskOwned} | ConvertTo-Json | Set-Content -LiteralPath $taskContext -Encoding utf8
$taskOld=$env:TD_LANCER_BASE_CONTEXT;$taskProcess=$null;$taskCode=$null;$taskFailure=$null
$taskWatch=[Diagnostics.Stopwatch]::StartNew()
try {
    $env:TD_LANCER_BASE_CONTEXT=$taskContext
    try {$taskProcess=Start-Process -FilePath 'C:/Program Files/Blender Foundation/Blender 5.1/blender.exe' -ArgumentList @('--background','--factory-startup','--threads','2','--python-exit-code','1','--python',('"{0}"' -f (Join-Path $PSScriptRoot 'inspect_manny_base.py'))) -WindowStyle Hidden -PassThru -WorkingDirectory $taskSaved -RedirectStandardOutput (Join-Path $taskSaved 'stdout.log') -RedirectStandardError (Join-Path $taskSaved 'stderr.log')}
    finally {$env:TD_LANCER_BASE_CONTEXT=$taskOld}
    Write-Output "START BaseInspection02 PID=$($taskProcess.Id) guardGiB=$taskFree"
    @{pid=$taskProcess.Id;started_utc=[DateTime]::UtcNow.ToString('o')} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $taskSaved 'process.json')
    while(-not $taskProcess.WaitForExit(2000)){
        $taskAvailable=(Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory/1MB
        if($taskWatch.Elapsed.TotalSeconds -gt 120 -or $taskAvailable -lt 1.5){throw 'Own timeout/pressure stop; no retry'}
    }
    $taskProcess.WaitForExit();$taskCode=$taskProcess.ExitCode
    if($taskCode -ne 0){throw "Blender failed: $taskCode"}
    if(-not (Select-String -LiteralPath (Join-Path $taskSaved 'stdout.log') -SimpleMatch 'LANCER_BASE_INVENTORY_OK' -Quiet)){throw 'Success marker absent'}
    $taskReport=Get-Content -Raw -LiteralPath $taskInventory | ConvertFrom-Json
    if((Get-FileHash -LiteralPath $taskBlend -Algorithm SHA256).Hash.ToLowerInvariant() -ne $taskReport.blend_sha256){throw 'Blend hash mismatch'}
}catch{
    $taskFailure=$_.Exception.Message
    if($null -ne $taskProcess){if(-not $taskProcess.HasExited){$taskProcess.Kill($true);$taskProcess.WaitForExit()};$taskCode=$taskProcess.ExitCode}
}finally{
    $taskChanged=@($taskInputs.Keys | Where-Object {(Get-FileHash -LiteralPath $_ -Algorithm SHA256).Hash -ne $taskInputs[$_]})
    if($taskChanged.Count){$taskFailure='Frozen base input changed'}
    $taskResult=@{pass=($null -eq $taskFailure);failure=$taskFailure;os_exit_code=$taskCode;duration_s=[math]::Round($taskWatch.Elapsed.TotalSeconds,2);protected_inputs=$taskInputs;changed=$taskChanged;guard_free_gib=$taskFree;threads=2;timeout_s=120;head=(git -C $taskRoot rev-parse HEAD)}
    $taskResult | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $taskSaved 'verification.json') -Encoding utf8
}
if($taskFailure){throw $taskFailure}
$taskResult | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $taskNative -Encoding utf8
'PASS '+($taskResult | ConvertTo-Json -Depth 6 -Compress)
