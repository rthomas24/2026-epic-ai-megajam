param(
    [Parameter(Mandatory)][string]$HostProject,
    [Parameter(Mandatory)][string]$LeadGrant,
    [switch]$Execute
)
$ErrorActionPreference='Stop'
$python='C:/Program Files/Epic Games/UE_5.9/Engine/Binaries/ThirdParty/Python3/Win64/python.exe'
$runArgs=@((Join-Path $PSScriptRoot 'run_test.py'),'--host-project',$HostProject,'--lead-grant',$LeadGrant)
if($Execute){$runArgs+='--execute'}
& $python @runArgs
exit $LASTEXITCODE
