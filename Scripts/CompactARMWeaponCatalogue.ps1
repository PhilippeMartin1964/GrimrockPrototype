<#
.SYNOPSIS
  ARM-CLEAN03.2: audit / migration lossless du catalogue d'armes.
.DESCRIPTION
  Sans -Apply : lecture seule, comparaison stricte des 1640 profils avec
  82 progressions N1 + regles + exceptions, aucun .uasset sauvegarde.
  Avec -Apply : sauvegarde du .uasset compresse seulement si parite exacte.
  Fermer Unreal Editor avant l'execution. Compiler le module Editor d'abord.
.EXAMPLE
  .\Scripts\CompactARMWeaponCatalogue.ps1 -EngineRoot D:\UE_5.5
.EXAMPLE
  .\Scripts\CompactARMWeaponCatalogue.ps1 -EngineRoot D:\UE_5.5 -Apply
#>
[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\UE_5.5',
    [switch]$Apply
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Project = Join-Path $Root 'GrimrockPrototype.uproject'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if (-not (Test-Path -LiteralPath $Project -PathType Leaf) -or
    -not (Test-Path -LiteralPath $Editor -PathType Leaf)) {
    throw "ARM-CLEAN03: projet ou UnrealEditor-Cmd.exe absent ($Project / $Editor)."
}
if (Get-Process -Name UnrealEditor -ErrorAction SilentlyContinue) {
    throw 'ARM-CLEAN03: fermez Unreal Editor avant de manipuler le catalogue.'
}
if ($Apply) {
    $Branch = (& git -C $Root branch --show-current).Trim()
    if ($LASTEXITCODE -ne 0 -or $Branch -ne 'master') {
        throw "ARM-CLEAN03: -Apply impose master (branche actuelle : $Branch)."
    }
    $Dirty = @(& git -C $Root status --porcelain)
    if ($LASTEXITCODE -ne 0 -or $Dirty.Count -gt 0) {
        throw 'ARM-CLEAN03: -Apply requiert un working tree propre ; aucun fichier local ne doit etre ecrase.'
    }
}
$Arguments = @($Project, '-run=GridWeaponCatalogueCompact', '-unattended', '-NoSplash', '-NoP4', '-NullRHI', '-log')
if ($Apply) { $Arguments += '-Apply' }
Write-Host "[ARM-CLEAN03] Mode : $(if ($Apply) { 'APPLY' } else { 'DRY_RUN' })"
& $Editor @Arguments
$Code = $LASTEXITCODE
if ($Code -ne 0) {
    throw "ARM-CLEAN03: commandlet interrompu ($Code). Ne commitez aucun .uasset."
}
if ($Apply) {
    Write-Host '[ARM-CLEAN03] Catalogue en attente de validation UE5.5.4 ; NE PAS COMMITTER sans tests.'
    & git -C $Root status --short
} else {
    Write-Host '[ARM-CLEAN03] Audit sans ecriture termine.'
}
