<#
.SYNOPSIS
  Start the game (or the editor) for THIS checkout with Unreal's built-in MCP server on a private
  port, so an agent can play-test it: capture the viewport, read frame timings and logs, run
  console commands and cheats, inject input. Returns as soon as the server answers.

  Drive it with:   python Tools\td_mcp.py --port <port> toolsets
  Stop it with:    .\Tools\td-play-mcp.ps1 -Stop -Port <port>

  Each worktree should use its own port (the default is derived from the worktree folder name:
  int = 8010, t1 = 8011 ... t5 = 8015, anything else = 8019). Port 8000 is left for Ryan's editor.
.EXAMPLE
  .\Tools\td-play-mcp.ps1                                   # game on L_Galaxy, windowed
  .\Tools\td-play-mcp.ps1 -Map /Game/TriadDrift/Maps/L_Planet_Verdance
  .\Tools\td-play-mcp.ps1 -Editor                           # full editor (all editor toolsets), no window
  .\Tools\td-play-mcp.ps1 -Editor -Audio                    # editor with the audio engine on (offline, silent): audio work
  .\Tools\td-play-mcp.ps1 -Stop
#>
param(
    [string]$ProjectRoot,
    [string]$Map = "/Game/TriadDrift/Maps/L_Galaxy",
    [int]$Port = 0,
    [int]$ResX = 1280,
    [int]$ResY = 720,
    [int]$MaxSeconds = 900,
    [switch]$Editor,
    [switch]$Audio,
    [switch]$Stop
)
$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "td-common.ps1")

$project = Get-TDProjectFile $ProjectRoot
$root = Split-Path -Parent $project
if ($Port -le 0) {
    $leaf = Split-Path -Leaf $root
    $Port = switch -Regex ($leaf) { '^int$' { 8010 } '^t([1-5])$' { 8010 + [int]$Matches[1] } default { 8019 } }
}

function Get-TDMcpProcess {
    Get-CimInstance Win32_Process -Filter "Name='UnrealEditor-Cmd.exe' OR Name='UnrealEditor.exe'" |
        Where-Object { $_.CommandLine -match "ModelContextProtocolPort=$Port(\s|$)" }
}

if ($Stop) {
    $found = @(Get-TDMcpProcess)
    foreach ($p in $found) { Stop-Process -Id $p.ProcessId -Force }
    Write-Output ("MCP SESSION STOPPED: {0} process(es) on port {1}" -f $found.Count, $Port)
    exit 0
}

if (@(Get-TDMcpProcess).Count -gt 0) {
    Write-Output "MCP SESSION ALREADY RUNNING on port $Port (stop it with -Stop)"
    exit 0
}

$engine = Get-TDEngineRoot $project
$exe = Join-Path $engine "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$logFile = Join-Path $root "Saved\Logs\td-play-mcp_$Port.log"
New-Item -ItemType Directory -Force -Path (Split-Path $logFile) | Out-Null
if (Test-Path $logFile) { Remove-Item $logFile -Force }

# -nosound skips MetaSound pre-save and cannot preview or render audio. -Audio swaps it for
# -NonRealtimeAudio: the audio engine runs and can be rendered offline, nothing reaches the speakers.
$soundFlag = if ($Audio) { "-NonRealtimeAudio" } else { "-nosound" }
$common = @("-nosplash", $soundFlag, "-unattended", "-NoLiveCoding",
            "-ModelContextProtocolStartServer", "-ModelContextProtocolPort=$Port", "-abslog=`"$logFile`"")
if ($Editor) {
    # Full editor, hidden: every enabled editor toolset (Blueprint, Niagara, automation tests, cheats in PIE...).
    $arguments = @("`"$project`"", $Map, "-RenderOffscreen") + $common
} else {
    # The game itself. -TDQuit is a safety net so a forgotten session does not run forever.
    $arguments = @("`"$project`"", $Map, "-game", "-windowed", "-ResX=$ResX", "-ResY=$ResY", "-TDQuit=$MaxSeconds") + $common
}
$started = Start-Process -FilePath $exe -ArgumentList $arguments -WindowStyle Minimized -PassThru

$deadline = (Get-Date).AddSeconds(180)
$ready = $false
while ((Get-Date) -lt $deadline) {
    Start-Sleep -Seconds 2
    try {
        $client = New-Object System.Net.Sockets.TcpClient
        $client.Connect("127.0.0.1", $Port)
        $client.Close()
        $ready = $true
        break
    } catch { }
    if (@(Get-TDMcpProcess).Count -eq 0) { break }
}
if ($ready -and -not $Editor) {
    # CaptureGameViewport reports "no drawable area" for a minimised window: show it without stealing focus.
    try {
        Add-Type -Namespace TD -Name Win -MemberDefinition '[DllImport("user32.dll")] public static extern bool ShowWindowAsync(System.IntPtr hWnd, int nCmdShow);' -ErrorAction SilentlyContinue
        foreach ($p in @(Get-TDMcpProcess)) {
            $proc = Get-Process -Id $p.ProcessId -ErrorAction SilentlyContinue
            if ($proc -and $proc.MainWindowHandle -ne [IntPtr]::Zero) { [TD.Win]::ShowWindowAsync($proc.MainWindowHandle, 4) | Out-Null }
        }
    } catch { }
}
if ($ready) {
    Write-Output "MCP SESSION READY on port $Port  ($(if ($Editor) { 'editor' } else { 'game' }), map $Map, log: $logFile)"
    Write-Output "  python Tools\td_mcp.py --port $Port toolsets"
    exit 0
}
Write-Output "MCP SESSION FAILED to start on port $Port (log: $logFile)"
exit 1
