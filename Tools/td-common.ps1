# Shared helpers for the Triad Drift build/test tools. Dot-sourced by td-build.ps1 and td-test.ps1.

function Get-TDProjectFile {
    param([string]$ProjectRoot)
    if (-not $ProjectRoot) { $ProjectRoot = Split-Path -Parent $PSScriptRoot }
    $file = Join-Path $ProjectRoot "unrealGameJam.uproject"
    if (-not (Test-Path $file)) { throw "unrealGameJam.uproject not found in $ProjectRoot" }
    return (Resolve-Path $file).Path
}

function Get-TDEngineRoot {
    param([string]$ProjectFile)
    if ($env:TD_ENGINE_ROOT -and (Test-Path $env:TD_ENGINE_ROOT)) { return $env:TD_ENGINE_ROOT }
    $assoc = (Get-Content $ProjectFile -Raw | ConvertFrom-Json).EngineAssociation
    $builds = "HKCU:\Software\Epic Games\Unreal Engine\Builds"
    if ($assoc -and (Test-Path $builds)) {
        $path = (Get-ItemProperty $builds -ErrorAction SilentlyContinue).$assoc
        if ($path -and (Test-Path $path)) { return $path }
    }
    foreach ($candidate in @("C:\Program Files\Epic Games\UE_5.9", "D:\Epic Games\UE_5.9")) {
        if (Test-Path $candidate) { return $candidate }
    }
    throw "Unreal Engine 5.9 not found. Set TD_ENGINE_ROOT or register the engine (AGENTS.md section 3)."
}
