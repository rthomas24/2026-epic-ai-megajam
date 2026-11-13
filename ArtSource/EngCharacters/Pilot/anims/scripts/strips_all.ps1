# Render a contact strip for every exported FBX in Saved/PilotAnims (run ue_a20_export_check.py first).
param([string[]]$Only)
$repo = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..\..\..")).Path
$blender = "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe"
$outDir = Join-Path $repo "ArtSource\EngCharacters\Pilot\review\anims"
New-Item -ItemType Directory -Force $outDir | Out-Null
if ($Only) { $Only = @($Only | ForEach-Object { $_ -split "," }) }
Get-ChildItem (Join-Path $repo "Saved\PilotAnims\*.fbx") | ForEach-Object {
    if ($Only -and ($Only -notcontains $_.BaseName)) { return }
    $png = Join-Path $outDir ($_.BaseName + "_strip.png")
    & $blender --background --factory-startup --python (Join-Path $PSScriptRoot "b_strip.py") -- $_.FullName $png 2>&1 |
        Select-String "METRIC|STRIP_OK|Traceback|Error" | ForEach-Object { $_.Line }
}
