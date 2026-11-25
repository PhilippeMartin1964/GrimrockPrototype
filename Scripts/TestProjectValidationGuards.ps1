[CmdletBinding()]
param([switch]$SelfTest)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$RepoRoot = Split-Path -Parent $PSScriptRoot

function Assert-NoRetiredItemAuthority([string]$Source, [string]$File, [string]$Forbidden) {
    if ($Source -match $Forbidden) {
        throw "Retired item authority or Automation bypass in $File"
    }
}

function Assert-OilCookContract([string]$Config, [string]$Materials) {
    if (-not ($Config -cmatch '(?m)^\+DirectoriesToAlwaysCook=\(Path="/Game/GrimrockPrototype/Art/Materials/Surfaces"\)\s*$')) {
        throw 'Oil decal materials are no longer explicitly included in Shipping cook.'
    }
    foreach ($Name in @('M_Surface_Oil.uasset', 'MI_Surface_Oil.uasset')) {
        if (-not (Test-Path -LiteralPath (Join-Path $Materials $Name) -PathType Leaf)) {
            throw "Shipping oil decal asset missing: $Name"
        }
    }
}

$HistoricalSources = '\b(?:LegacyDefinition|OldDefinition|GridLegacyItemLookup|FGridLegacyItemLookup|RuntimeItemDefinitionsById|RegisterItemDefinition|FindItemDefinition|FGridLegacyInventoryRegistrationGate|UGridCanonicalItemDefinitionAsset|UGridCanonicalWeaponCatalogueAsset|UGridCanonicalWeaponVisualCatalogueAsset)\b'
$ReaderBypass = $HistoricalSources + '|\b(?:UGridItemDefinitionAsset|GIsAutomationTesting|WITH_DEV_AUTOMATION_TESTS)\b'
$Readers = @(
    'Private/Runtime/Combat/GridEquippedGuardItemReadModel.h',
    'Private/Runtime/GridItemInteractionReadModel.h',
    'Private/RPG/GridStartingKitEquipmentPolicyReadModel.h',
    'Private/Runtime/GridReceptacleItemMetadataReadModel.h'
)
$OilConfig = Get-Content -LiteralPath (Join-Path $RepoRoot 'Config/DefaultGame.ini') -Raw -Encoding UTF8
$OilMaterials = Join-Path $RepoRoot 'Content/GrimrockPrototype/Art/Materials/Surfaces'

if ($SelfTest) {
    function Assert-Rejected([scriptblock]$Action, [string]$Expected) {
        try { & $Action }
        catch {
            if ($_.Exception.Message -like "$Expected*") { return }
            throw
        }
        throw "Validation guard accepted a negative fixture: $Expected"
    }
    Assert-NoRetiredItemAuthority 'Resolve(Registry, Item)' 'reader fixture' $ReaderBypass
    Assert-NoRetiredItemAuthority 'class UGridItemDefinitionAsset;' 'registry fixture' $HistoricalSources
    foreach ($Bad in @('LegacyDefinition', 'OldDefinition', 'UGridItemDefinitionAsset* Definition',
        '#if WITH_DEV_AUTOMATION_TESTS', 'GIsAutomationTesting', 'RuntimeItemDefinitionsById',
        'RegisterItemDefinition', 'FindItemDefinition', 'FGridLegacyItemLookup', 'UGridCanonicalItemDefinitionAsset')) {
        Assert-Rejected { Assert-NoRetiredItemAuthority $Bad 'negative reader fixture' $ReaderBypass } 'Retired item authority'
    }
    Assert-Rejected { Assert-OilCookContract 'unrelated cook configuration' $OilMaterials } 'Oil decal materials'
    Assert-Rejected { Assert-OilCookContract $OilConfig (Join-Path $RepoRoot 'Config') } 'Shipping oil decal asset missing'
    Write-Host '[OK] Validation guards: valid fixtures and 12 refusals.'
}

# These four native readers project resolved data. No raw definition or test path.
$SourceRoot = Join-Path $RepoRoot 'Source/GrimrockPrototype'
foreach ($Reader in $Readers) {
    $Source = Get-Content -LiteralPath (Join-Path $SourceRoot $Reader) -Raw -Encoding UTF8
    Assert-NoRetiredItemAuthority $Source $Reader $ReaderBypass
}
foreach ($Inventory in @('Public/Runtime/GridPartyInventoryComponent.h', 'Private/Runtime/GridPartyInventoryComponent.cpp')) {
    $Source = Get-Content -LiteralPath (Join-Path $SourceRoot $Inventory) -Raw -Encoding UTF8
    Assert-NoRetiredItemAuthority $Source $Inventory $HistoricalSources
}
Assert-OilCookContract $OilConfig $OilMaterials
Write-Host '[OK] Item reader authority and Shipping oil decal cook contract.'
