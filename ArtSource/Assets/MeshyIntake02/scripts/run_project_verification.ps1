param([ValidatePattern('^verify[0-9]{2}$')][string]$Attempt='verify01')
$ErrorActionPreference='Stop'
$sourceRoot=Split-Path $PSScriptRoot -Parent
$repoRoot=Split-Path (Split-Path (Split-Path $sourceRoot -Parent) -Parent) -Parent
$hostRoot=Join-Path $repoRoot 'Saved/I02V/Host'
$project=Join-Path $hostRoot 'Intake02Verify.uproject'
$jobRoot=Join-Path $repoRoot "Saved/I02V/$Attempt"
$reportPath=Join-Path $sourceRoot "evidence/project_$Attempt.json"
if((Test-Path -LiteralPath $jobRoot) -or (Test-Path -LiteralPath $reportPath)){throw 'Existing evidence; no overwrite or automatic retry'}
if(-not (Test-Path -LiteralPath (Join-Path $sourceRoot 'evidence/project_transfer.json'))){throw 'Verified project transfer required'}
$free=(Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory/1MB
$heavy=@(Get-CimInstance Win32_Process | Where-Object {$_.Name -match '(?i)^(blender|UnrealEditor.*|unrealGameJam.*|ShaderCompileWorker|cl|dotnet|UnrealBuildTool)\.exe$'})
if($free -lt 4.25 -or $heavy.Count){throw "Resource guard: free=$free GiB; heavy=$($heavy.Count)"}
New-Item -ItemType Directory -Path $jobRoot | Out-Null
$env:TD_I02_ATTEMPT=$Attempt
$arguments=@(('"'+$project+'"'),'-run=pythonscript',('-script="'+(Join-Path $PSScriptRoot 'verify_project.py')+'"'),'-unattended','-nop4','-nosplash','-nosound','-NullRHI','-NoShaderCompile','-DDC=Intake02',('-abslog="'+(Join-Path $jobRoot 'engine.log')+'"'))
$watch=[Diagnostics.Stopwatch]::StartNew();$started=[DateTime]::UtcNow
[long]$peak=0;$minimum=$free;$abort=$null;$lowSince=$null;$monitorError=$null
$proc=Start-Process -FilePath 'C:/Program Files/Epic Games/UE_5.9/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $jobRoot 'stdout.log') -RedirectStandardError (Join-Path $jobRoot 'stderr.log')
try{
  $retainedHandle=$proc.Handle
  $proc.PriorityClass='BelowNormal';$proc.ProcessorAffinity=3
  Write-Output "START $Attempt PID=$($proc.Id); free_GiB=$free"
  while(-not $proc.HasExited){
    $proc.Refresh();$peak=[math]::Max([long]$peak,[long]$proc.PeakWorkingSet64)
    $now=(Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory/1MB;$minimum=[math]::Min($minimum,$now)
    if($now -lt 1.75){if($null -eq $lowSince){$lowSince=$watch.Elapsed.TotalSeconds}}else{$lowSince=$null}
    if($watch.Elapsed.TotalSeconds -ge 180 -or ($null -ne $lowSince -and $watch.Elapsed.TotalSeconds-$lowSince -ge 10)){$abort='time_or_memory_limit';break}
    Start-Sleep -Milliseconds 300
  }
}catch{$abort='monitor_exception';$monitorError=$_.Exception.Message}
finally{
  # This is the Process object returned by this launch, with a retained handle;
  # cleanup never searches for or stops another editor by process name.
  if(-not $proc.HasExited){$proc.Kill()}
  $proc.WaitForExit();$proc.Refresh();$watch.Stop()
}
$metrics=[ordered]@{attempt=$Attempt;pid=$proc.Id;exit_code=$proc.ExitCode;started_utc=$started.ToString('o');elapsed_seconds=$watch.Elapsed.TotalSeconds;peak_working_set_mib=$peak/1MB;minimum_free_gib=$minimum;abort=$abort;monitor_error=$monitorError;cpu_affinity=3;project=$project}
$metrics|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $jobRoot 'process_metrics.json') -Encoding UTF8
$metrics|ConvertTo-Json -Compress
if($abort){exit 124}
if($null -eq $proc.ExitCode){throw 'Native exit code unknown'}
if($proc.ExitCode -ne 0){exit $proc.ExitCode}
$result=Get-Content -LiteralPath $reportPath -Raw|ConvertFrom-Json
if($result.passed -ne $true -or $result.errors.Count){throw 'Structured QA report failed'}
exit 0
