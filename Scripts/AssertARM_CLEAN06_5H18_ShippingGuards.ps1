[CmdletBinding()]
param([switch]$SelfTest)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'

$Root=Split-Path -Parent $PSScriptRoot
$Models=@(
    @{ path='Source/GrimrockPrototype/Private/Runtime/Combat/GridEquippedGuardItemReadModel.h'; returns='View'; legacy='LegacyDefinition' },
    @{ path='Source/GrimrockPrototype/Private/Runtime/GridCanonicalItemInteractionReadModel.h'; returns='Out'; legacy='OldDefinition' },
    @{ path='Source/GrimrockPrototype/Private/RPG/GridStartingKitEquipmentPolicyReadModel.h'; returns='View'; legacy='LegacyDefinition' },
    @{ path='Source/GrimrockPrototype/Private/Runtime/GridReceptacleItemMetadataReadModel.h'; returns='View'; legacy='LegacyDefinition' }
)
function Assert-ArmH18ShippingGuard([string]$Content,[string]$Name,[string]$ReturnName,[string]$LegacyName) {
    $If='#if WITH_DEV_AUTOMATION_TESTS'
    $End='#endif // WITH_DEV_AUTOMATION_TESTS'
    $Start=$Content.IndexOf($If,[StringComparison]::Ordinal)
    if ($Start -lt 0 -or
        $Content.IndexOf($If,$Start+$If.Length,[StringComparison]::Ordinal) -ge 0) {
        throw ('Exactly one Automation-only legacy block required: '+$Name)
    }
    $Finish=$Content.IndexOf($End,$Start+$If.Length,[StringComparison]::Ordinal)
    if ($Finish -lt 0 -or
        $Content.IndexOf($End,$Finish+$End.Length,[StringComparison]::Ordinal) -ge 0) {
        throw ('Missing unique closing Automation guard: '+$Name)
    }
    $Legacy=$Content.Substring($Start+$If.Length,$Finish-$Start-$If.Length)
    $After=$Content.Substring($Finish+$End.Length)
    if ($Legacy.Contains('#else') -or
        -not $Legacy.Contains('GIsAutomationTesting') -or
        -not $Legacy.Contains($LegacyName) -or
        $After -notmatch ('(?m)^\s*return\s+'+[regex]::Escape($ReturnName)+'\s*;') -or
        $After.Contains($LegacyName+'->')) {
        throw ('Shipping may compile an unreachable or legacy-only path: '+$Name)
    }
}
if ($SelfTest) {
    $Safe=@'
struct Fake {
#if WITH_DEV_AUTOMATION_TESTS
if (!GIsAutomationTesting) return View;
if (LegacyDefinition) LegacyDefinition->Method();
#endif // WITH_DEV_AUTOMATION_TESTS
return View;
};
'@
    Assert-ArmH18ShippingGuard $Safe 'fixture' 'View' 'LegacyDefinition'
    $BadBlock=[string]::Join([Environment]::NewLine,
        @('#else','return View;','#endif // WITH_DEV_AUTOMATION_TESTS'))
    $Broken=$Safe.Replace('#endif // WITH_DEV_AUTOMATION_TESTS',$BadBlock)
    $Rejected=$false
    try {Assert-ArmH18ShippingGuard $Broken 'broken fixture' 'View' 'LegacyDefinition'}
    catch {$Rejected=$true}
    if (-not $Rejected) {throw 'Shipping-unreachable fixture was accepted.'}
}
foreach ($Model in $Models) {
    $File=Join-Path $Root $Model.path
    if (-not (Test-Path -LiteralPath $File -PathType Leaf)) {
        throw ('Runtime read model missing: '+$Model.path)
    }
    $Source=Get-Content -LiteralPath $File -Raw -Encoding UTF8
    Assert-ArmH18ShippingGuard $Source $Model.path $Model.returns $Model.legacy
}
Write-Host '[OK] ARM-CLEAN06.5H.18: four test-only compatibility bodies excluded from Shipping.'
