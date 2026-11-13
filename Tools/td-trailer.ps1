<#
.SYNOPSIS
  Record the jam's gameplay video from a real game run and cut it (AGENTS.md 5.2). Owner: Teammate 3.
.DESCRIPTION
  Starts the game offscreen at 1920x1080 on a private MCP port (Tools\td-play-mcp.ps1 -Offscreen), records every beat
  in Tools\td_trailer.py (each staged with the game's own dev commands, recorded by the engine's viewport recorder at
  30 fps), stops the game, then cuts the edit list Tools\td_trailer_edl.json into Builds\trailer\TriadDrift_<date>.mp4
  with a contact sheet beside it. Builds\ is git-ignored.
.EXAMPLE
  .\Tools\td-trailer.ps1                      # record every beat, then cut
  .\Tools\td-trailer.ps1 -Only ram,onfoot     # re-record two beats, then cut
  .\Tools\td-trailer.ps1 -CutOnly             # re-cut the existing clips (after editing the edit list)
  .\Tools\td-trailer.ps1 -Music Builds\music.wav
#>
param(
    [string]$Only = "",
    [switch]$CutOnly,
    [string]$Music = "",
    [int]$Port = 8013
)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$py = Join-Path $PSScriptRoot "td_trailer.py"

if (-not $CutOnly) {
    & (Join-Path $PSScriptRoot "td-play-mcp.ps1") -Offscreen -ResX 1920 -ResY 1080 -MaxSeconds 3600 -Port $Port | Out-Host
    try {
        $recordArgs = @($py, "record", "--port", $Port)
        if ($Only) { $recordArgs += @("--only", $Only) }
        & python @recordArgs | Out-Host
    }
    finally {
        & (Join-Path $PSScriptRoot "td-play-mcp.ps1") -Stop -Port $Port | Out-Host
    }
}
$cutArgs = @($py, "cut")
if ($Music) { $cutArgs += @("--music", $Music) }
& python @cutArgs
