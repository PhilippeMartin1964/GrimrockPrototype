param(
    [string]$EngineRoot = $env:UE_ROOT
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$Validate = Join-Path $PSScriptRoot 'ValidateUE.ps1'

if (-not (Test-Path -LiteralPath $Validate -PathType Leaf))
{
    throw "ValidateUE.ps1 introuvable : $Validate"
}

Write-Host '=== UI-RPG-VISUAL01.4 final validation ==='

& $Validate `
    -EngineRoot $EngineRoot `
    -AutomationFilter 'Grimrock.UI.RPG.VISUAL01.FinalAudit'

& $Validate `
    -EngineRoot $EngineRoot `
    -SkipBuild `
    -AutomationFilter 'Grimrock.UI.RPG01.ProductionAssets'

& $Validate `
    -EngineRoot $EngineRoot `
    -SkipBuild `
    -AutomationFilter 'Grimrock.UI.RPG02.ProductionPresentation'

& $Validate `
    -EngineRoot $EngineRoot `
    -SkipBuild `
    -AutomationFilter 'Grimrock.UI.RPG03.NodeBinding'

& $Validate `
    -EngineRoot $EngineRoot `
    -SkipBuild `
    -AutomationFilter 'Grimrock.UI.RPG.CODEAUDIT01.BlueprintCompatibility'

& $Validate `
    -EngineRoot $EngineRoot `
    -SkipBuild `
    -AutomationFilter 'Grimrock.UI.RPG.DESC01'

Write-Host ''
Write-Host '[OK] UI-RPG-VISUAL01.4 validation suite completed successfully.'
