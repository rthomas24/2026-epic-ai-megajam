param([ValidateSet('import','reload','neon-recovery','neon-recovery02','remaining','four-recovery','final','all-reload','project-clone','project-clone02')][string]$Phase='import')
$ErrorActionPreference='Stop'
$taskRoot=Split-Path $PSScriptRoot -Parent
$repoRoot=Split-Path (Split-Path (Split-Path $taskRoot -Parent) -Parent) -Parent
$hostRoot=Join-Path $repoRoot 'Saved/WeaponIntake01/Host'
if($Phase -in @('project-clone','project-clone02')){$hostRoot=Join-Path $repoRoot 'Saved/WeaponIntake01/GameValidation'}
$evidenceRoot=Join-Path $repoRoot 'ArtSource/Weapons/MeshyIntake01/evidence'
$scriptName=switch($Phase){'import'{'import_weapons.py'} 'reload'{'verify_saved_weapons.py'} 'neon-recovery'{'verify_neon_recovery.py'} 'neon-recovery02'{'verify_neon_recovery02.py'} 'remaining'{'import_remaining_weapons.py'} 'four-recovery'{'verify_preserved_four.py'} 'final'{'import_final_weapon.py'} 'all-reload'{'verify_all_saved.py'} 'project-clone'{'verify_project_copy.py'} 'project-clone02'{'verify_project_copy.py'}}
$scriptPath=Join-Path $PSScriptRoot $scriptName
$jobOut=Join-Path $repoRoot "Saved/WeaponIntake01/$Phase-01"
if(Test-Path -LiteralPath $jobOut){throw 'Existing job evidence; do not automatically retry or overwrite'}
$free=(Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory/1MB
$busy=@(Get-CimInstance Win32_Process | Where-Object {$_.Name -match '(?i)^(blender|UnrealEditor.*|unrealGameJam.*|ShaderCompileWorker|cl|dotnet|UnrealBuildTool)\.exe$'})
if($free -lt 4.25 -or $busy.Count){throw "Guard failed: free=$free GiB; heavy=$($busy.Count)"}
New-Item -ItemType Directory -Path $jobOut | Out-Null
$watch=[Diagnostics.Stopwatch]::StartNew();$started=[DateTime]::UtcNow
$arguments=@(('"'+(Join-Path $hostRoot 'WeaponIntake.uproject')+'"'),'-run=pythonscript',('-script="'+$scriptPath+'"'),'-unattended','-nop4','-nosplash','-nosound','-NullRHI','-NoShaderCompile','-DDC=WeaponIntake',('-abslog="'+(Join-Path $jobOut 'engine.log')+'"'))
$proc=Start-Process -FilePath 'C:/Program Files/Epic Games/UE_5.9/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $jobOut 'stdout.log') -RedirectStandardError (Join-Path $jobOut 'stderr.log')
[long]$peak=0;$minimum=$free;$abort='none';$lowSince=$null;$monitorError=$null
try{
  $retainedProcessHandle=$proc.Handle
  $proc.PriorityClass='BelowNormal';$proc.ProcessorAffinity=3
  Write-Output "START $Phase PID=$($proc.Id); free_GiB=$free"
  while(-not $proc.HasExited){
    $proc.Refresh();$peak=[math]::Max([long]$peak,[long]$proc.PeakWorkingSet64)
    $nowFree=(Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory/1MB;$minimum=[math]::Min($minimum,$nowFree)
    if($nowFree -lt 1.75){if($null -eq $lowSince){$lowSince=$watch.Elapsed.TotalSeconds}}else{$lowSince=$null}
    if($watch.Elapsed.TotalSeconds -ge 180 -or ($null -ne $lowSince -and $watch.Elapsed.TotalSeconds-$lowSince -ge 10)){$abort='resource_or_time_limit';$proc.Kill();break}
    Start-Sleep -Milliseconds 300
  }
}catch{
  $abort='monitor_exception';$monitorError=$_.Exception.Message
}finally{
  if(-not $proc.HasExited){
    $owned=Get-CimInstance Win32_Process -Filter "ProcessId=$($proc.Id)"
    if($owned -and $owned.CommandLine.Contains((Join-Path $hostRoot 'WeaponIntake.uproject'))){$proc.Kill()}
    else{throw 'Refusing cleanup: owned private-host command could not be confirmed'}
  }
  $proc.WaitForExit();$proc.Refresh();$watch.Stop()
}
$metrics=[ordered]@{phase=$Phase;pid=$proc.Id;exit_code=$proc.ExitCode;started_utc=$started.ToString('o');elapsed_seconds=$watch.Elapsed.TotalSeconds;peak_working_set_mib=$peak/1MB;minimum_free_gib=$minimum;abort=$abort;monitor_error=$monitorError;cpu_affinity=3;fresh_slot_grant_required=$true}
$metrics | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $jobOut 'process_metrics.json') -Encoding UTF8
$metrics | ConvertTo-Json -Compress
if($abort -ne 'none'){exit 124}
if($null -eq $proc.ExitCode){throw 'Native exit code unknown; never treat null as success'}
if($proc.ExitCode -ne 0){exit $proc.ExitCode}
$reportName=switch($Phase){'import'{'import_report.json'} 'reload'{'reload_report.json'} 'neon-recovery'{'neon_recovery_report.json'} 'neon-recovery02'{'neon_recovery02_report.json'} 'remaining'{'remaining_import_report.json'} 'four-recovery'{'four_recovery_report.json'} 'final'{'final_import_report.json'} 'all-reload'{'all_saved_reload_report.json'} 'project-clone'{'project_clone_report.json'} 'project-clone02'{'project_clone_report.json'}}
$result=Get-Content -LiteralPath (Join-Path $evidenceRoot $reportName) -Raw | ConvertFrom-Json
if($result.passed -ne $true){throw 'Native script report did not pass'}
exit 0
