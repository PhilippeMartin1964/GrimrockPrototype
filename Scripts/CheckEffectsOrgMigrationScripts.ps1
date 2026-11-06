# EFFECTS-ORG01.2D — read-only PowerShell 5.1 syntax check.
# Does NOT execute either migration script or touch any .uasset.
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$Root = Split-Path -Parent $PSScriptRoot
$Scripts = @(
    'Scripts\MigrateEffectsOrgTorch.ps1',
    'Scripts\RunEffectsOrgTorchStandalone.ps1'
)
foreach ($Relative in $Scripts) {
    $File = Join-Path $Root $Relative
    if (-not (Test-Path -LiteralPath $File -PathType Leaf)) {
        throw ('Missing project script: ' + $Relative)
    }
    $Tokens = $null
    $Errors = $null
    [void][System.Management.Automation.Language.Parser]::ParseFile(
        $File, [ref]$Tokens, [ref]$Errors)
    if ($null -ne $Errors -and $Errors.Count -gt 0) {
        throw ('Invalid Windows PowerShell syntax: ' + $Relative)
    }
}
Write-Host '[EFFECTS-ORG01.2D] Two migration scripts parsed successfully. No migration executed.'
