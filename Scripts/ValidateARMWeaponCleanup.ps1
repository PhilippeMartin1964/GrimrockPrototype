<#
.SYNOPSIS
  ARM-CLEAN01..04 — Validation locale UE5.5.4 avant PIE.
.DESCRIPTION
  Lance l'audit XLSX, le build/suite Grimrock, puis le dry-run de
  compression. SANS -ApplyCompact, aucune modification d'asset.
  -ApplyCompact est opt-in et requiert un working tree propre; la
  suite Grimrock est relancee apres sauvegarde du DataAsset.
.EXAMPLE
  .\Scripts\ValidateARMWeaponCleanup.ps1 -EngineRoot D:\UE_5.5
.EXAMPLE
  .\Scripts\ValidateARMWeaponCleanup.ps1 -EngineRoot D:\UE_5.5 -ApplyCompact
#>
[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\UE_5.5',
    [switch]$ApplyCompact
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$PowerShellExe = (Get-Process -Id $PID).Path
if (-not (Test-Path -LiteralPath $PowerShellExe)) {
    throw 'ARM-CLEAN: impossible de determiner le chemin du processus PowerShell.'
}
function Run-Stage([string]$ScriptName, [string[]]$Arguments) {
    $Path = Join-Path $PSScriptRoot $ScriptName
    Write-Host ""
    Write-Host ("=== ARM-CLEAN: {0} {1} ===" -f $ScriptName, ($Arguments -join ' '))
    & $PowerShellExe -NoProfile -ExecutionPolicy Bypass -File $Path @Arguments
    $Code = $LASTEXITCODE
    if ($Code -ne 0) {
        throw "ARM-CLEAN: $ScriptName a echoue (code $Code). Aucune migration supplementaire ne sera effectuee."
    }
}

Run-Stage 'AnalyzeARMWeaponProgression.ps1' @('-ExportFirst')
Run-Stage 'ValidateUE.ps1' @('-EngineRoot', $EngineRoot, '-AutomationFilter', 'Grimrock')
Run-Stage 'CompactARMWeaponCatalogue.ps1' @('-EngineRoot', $EngineRoot)
if ($ApplyCompact) {
    Run-Stage 'CompactARMWeaponCatalogue.ps1' @('-EngineRoot', $EngineRoot, '-Apply')
    Run-Stage 'ValidateUE.ps1' @('-EngineRoot', $EngineRoot, '-SkipBuild', '-AutomationFilter', 'Grimrock')
    Write-Host '[ARM-CLEAN] Migration sauvegardee et suite automation executee. Validation PIE et controle du diff uasset obligatoires avant commit.'
} else {
    Write-Host '[ARM-CLEAN] Validation et dry-run termines. Le DataAsset original demeure intact.'
}
