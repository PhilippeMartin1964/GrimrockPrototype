[CmdletBinding()]
param([switch]$SelfTest)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'

$Root=Split-Path -Parent $PSScriptRoot
$Models=@(
    @{ path='Source/GrimrockPrototype/Private/Runtime/Combat/GridEquippedGuardItemReadModel.h'; legacy='LegacyDefinition' },
    @{ path='Source/GrimrockPrototype/Private/Runtime/GridItemInteractionReadModel.h'; legacy='OldDefinition' },
    @{ path='Source/GrimrockPrototype/Private/RPG/GridStartingKitEquipmentPolicyReadModel.h'; legacy='LegacyDefinition' },
    @{ path='Source/GrimrockPrototype/Private/Runtime/GridReceptacleItemMetadataReadModel.h'; legacy='LegacyDefinition' }
)
function Assert-ArmH18RetiredReader([string]$Content,[string]$Name,[string]$LegacyName) {
    if ($Content -match '\b(UGridItemDefinitionAsset|GIsAutomationTesting|WITH_DEV_AUTOMATION_TESTS)\b' -or
        $Content -match ('\b'+[regex]::Escape($LegacyName)+'\b')) {
        throw ('Retired item reader still accepts a compatibility source: '+$Name)
    }
}
if ($SelfTest) {
    Assert-ArmH18RetiredReader 'Resolve(Registry, Item)' 'canonical fixture' 'LegacyDefinition'
    foreach ($Bad in @('Resolve(Registry, LegacyDefinition, Item)', '#if WITH_DEV_AUTOMATION_TESTS', 'UGridItemDefinitionAsset* Definition')) {
        $Rejected=$false
        try {Assert-ArmH18RetiredReader $Bad 'broken fixture' 'LegacyDefinition'}
        catch {$Rejected=$true}
        if (-not $Rejected) {throw 'Retired reader input was accepted.'}
    }
}
foreach ($Model in $Models) {
    $File=Join-Path $Root $Model.path
    if (-not (Test-Path -LiteralPath $File -PathType Leaf)) {
        throw ('Runtime read model missing: '+$Model.path)
    }
    $Source=Get-Content -LiteralPath $File -Raw -Encoding UTF8
    Assert-ArmH18RetiredReader $Source $Model.path $Model.legacy
}
Write-Host '[OK] ARM-CLEAN06.5H.18: retired item-reader inputs and fixture bypasses absent.'
