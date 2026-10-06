param(
    [string]$EngineRoot = $env:UE_ROOT,
    [switch]$ShowAutomationOutput
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$RepoRoot = Split-Path -Parent $PSScriptRoot

Write-Host '=== RPG03.10 Global balance / automation / PIE validation ==='
Write-Host "Repository : $RepoRoot"

$Branch = (& git -C $RepoRoot branch --show-current).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Impossible de lire la branche Git courante.' }
if ($Branch -ne 'master') { throw "RPG03.10 doit etre valide sur master. Branche courante : $Branch" }

$Args = @{
    EngineRoot = $EngineRoot
    AutomationFilter = 'Grimrock.RPG.RPG03'
}
if ($ShowAutomationOutput)
{
    $Args.ShowAutomationOutput = $true
}

& (Join-Path $PSScriptRoot 'ValidateUE.ps1') @Args

Write-Host ''
Write-Host '=== Git status after RPG03 global validation ==='
& git -C $RepoRoot status --short --untracked-files=all
if ($LASTEXITCODE -ne 0) { throw 'Impossible de lire le git status final.' }
