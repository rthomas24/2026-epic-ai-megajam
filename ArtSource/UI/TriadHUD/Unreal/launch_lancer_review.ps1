param(
    [switch]$Capture,
    [ValidateRange(8020,8029)][int]$Port=8023,
    [ValidateRange(60,300)][int]$MaxSeconds=180
)
$ErrorActionPreference='Stop'
# Does not build, edit assets/settings, acquire a heavy slot, or attach to another editor.
# The caller must obtain the Lead's current resource window before running this.
$repoTask=(Resolve-Path (Join-Path $PSScriptRoot '../../../..')).Path
$projectTask=Join-Path $repoTask 'unrealGameJam.uproject'
$engineTask='C:/Program Files/Epic Games/UE_5.9/Engine'
$exeTask=Join-Path $engineTask 'Binaries/Win64/UnrealEditor.exe'
if(Get-NetTCPConnection -LocalPort $Port -State Listen -ErrorAction SilentlyContinue) { throw "Private port $Port already has a listener" }
$logsTask=Join-Path $repoTask 'Saved/Logs'
New-Item -ItemType Directory -Force -Path $logsTask | Out-Null
$stampTask=Get-Date -Format yyyyMMdd_HHmmss
$logTask=Join-Path $logsTask "HUDLancerM03_$stampTask.log"
$resultTask=Join-Path $logsTask "HUDLancerM03_$stampTask.result.json"
$argsTask=@("`"$projectTask`"",'/Engine/Maps/Entry?game=/Script/TriadDriftHUD.TDHUDPreviewGameMode',
    '-game','-windowed','-ResX=1920','-ResY=1080','-ForceRes','-NoLiveCoding','-nosound','-nosplash',
    '-TDHUDLancerBenchmark','-ExecCmds="TD.DebugHUD 0"',"-ModelContextProtocolPort=$Port",'-ModelContextProtocolStartServer',
    "-abslog=`"$logTask`"","-ResultFile=`"$resultTask`"",'-HangTimeout=45','-NoProgressTimeout=90',
    '-ini:Engine:[DevOptions.Shaders]:NumUnusedShaderCompilingThreads=15')
if($Capture) { $argsTask+=@('-TDHUDLancerCapture','-AgentMode','-Seconds=40','-ExitTimeout=150') }
else { $argsTask+=@('-unattended',"-Seconds=$MaxSeconds","-ExitTimeout=$($MaxSeconds+60)") }
$processTask=Start-Process -FilePath $exeTask -ArgumentList $argsTask -WindowStyle Hidden -PassThru
Write-Output "HUD REVIEW PID=$($processTask.Id) port=$Port log=$logTask result=$resultTask"
if(!$Capture) { return }
if(!$processTask.WaitForExit(210000)) { $processTask.Kill(); throw 'Own capture process exceeded its outer timeout' }
if($processTask.ExitCode -ne 0) { throw "Capture exit=$($processTask.ExitCode); inspect $logTask" }
if(!(Test-Path -LiteralPath $resultTask)) { throw 'Engine did not write its result file' }
$outcomeTask=Get-Content -LiteralPath $resultTask -Raw | ConvertFrom-Json
Write-Output "CAPTURE exit=$($processTask.ExitCode) reason=$($outcomeTask.reason)"
if($outcomeTask.reason -ne 'ok') { throw 'Engine result did not report reason:ok; inspect before retrying' }
& (Join-Path $engineTask 'Binaries/ThirdParty/Python3/Win64/python.exe') (Join-Path $engineTask 'Extras/AgentTools/ue_log_digest.py') $logTask --json
if($LASTEXITCODE -ne 0) { throw 'Engine log digest found errors' }
Write-Output "Review PNGs: $(Join-Path $repoTask 'Saved/HUDLancerM03')"
