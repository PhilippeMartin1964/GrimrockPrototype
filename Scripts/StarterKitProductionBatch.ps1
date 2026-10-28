[CmdletBinding()]
param(
    [ValidateSet('Plan','Validate','Produce')]
    [string]$Action = 'Plan',
    [string]$EngineRoot = 'D:\UE_5.5',
    [string[]]$Archetypes = @(),
    [switch]$Apply,
    [switch]$Push
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$Repo = Split-Path -Parent $PSScriptRoot
$ApprovedPath = Join-Path $Repo 'docs\Design\ARMEMENT01_9_6_STARTER_KIT_RECIPES.csv'
$ManifestPath = Join-Path $Repo 'Config\Armement\WeaponProductionManifest.csv'
$OutputDir = Join-Path $Repo 'Saved\ARMEMENT-LOT02'
$WeaponPipeline = Join-Path $PSScriptRoot 'WeaponPipeline.ps1'
$ValidateUE = Join-Path $PSScriptRoot 'ValidateUE.ps1'

if ($Apply -and $Action -ne 'Produce') { throw '-Apply requires -Action Produce.' }
if ($Push -and ($Action -ne 'Produce' -or -not $Apply)) { throw '-Push requires -Action Produce -Apply.' }
if ($Action -eq 'Produce' -and (-not $Apply -or $Archetypes.Count -eq 0)) {
    throw 'Production requires -Archetypes ARMxxx,... -Apply. Plan and Validate never mutate assets.'
}
if (-not (Test-Path -LiteralPath $ApprovedPath -PathType Leaf) -or
    -not (Test-Path -LiteralPath $ManifestPath -PathType Leaf)) {
    throw 'Missing canonical approved recipes or weapon production manifest.'
}
$Branch = (& git -C $Repo branch --show-current)
if ($LASTEXITCODE -ne 0 -or ([string]$Branch).Trim() -ne 'master') {
    throw 'The batch pipeline supports master only.'
}

$Recipes = @(Import-Csv -LiteralPath $ApprovedPath)
$Manifest = @(Import-Csv -LiteralPath $ManifestPath)
$Headers = @('ClassId','BranchId','KitId','Weapon1ArchetypeId','Weapon1Level','Weapon1Slot',
    'Weapon2ArchetypeId','Weapon2Level','Weapon2Slot','KnownSupportItemId',
    'KnownSupportQuantity','PlannedSupportNeed','ProductionReadiness','ApprovalState','DesignNoteCode')
$ManifestHeaders = @('ArchetypeId','ProductionState','BuildKind','ItemDefinitionId','ItemPath','AttackId',
    'VisualId','VisualFamily','IconPath','MeshPath','MaterialPath','BaseColorPath','NormalPath',
    'OrmPath','ClassId','EquipmentSlots')
function Assert-Headers($Rows, [string[]]$Names, [string]$Label) {
    if ($Rows.Count -eq 0) { throw "Empty $Label" }
    $Actual = @($Rows[0].PSObject.Properties.Name)
    if ($Actual.Count -ne $Names.Count -or ($Actual -join '|') -cne ($Names -join '|')) {
        throw "Unexpected $Label schema."
    }
}
Assert-Headers $Recipes $Headers 'starter-kit CSV'
Assert-Headers $Manifest $ManifestHeaders 'weapon manifest'
if ($Recipes.Count -ne 18) { throw "Expected 18 approved recipes; got $($Recipes.Count)." }
$ManifestById = @{}
foreach ($Row in $Manifest) {
    if (-not ($Row.ArchetypeId -match '^ARM[0-9]{3}$') -or $ManifestById.ContainsKey($Row.ArchetypeId)) {
        throw "Duplicate or invalid manifest archetype $($Row.ArchetypeId)."
    }
    $ManifestById[$Row.ArchetypeId] = $Row
}
$ApprovedIds = @{}
$WeaponUses = @{}
$NonWeapon = [System.Collections.Generic.List[object]]::new()
$KitMatrix = [System.Collections.Generic.List[object]]::new()

function Test-None([string]$Value) {
    return [string]::IsNullOrWhiteSpace($Value) -or $Value -eq 'None'
}
function Convert-AssetPath([string]$UnrealPath) {
    if (-not $UnrealPath.StartsWith('/Game/') -or $UnrealPath -notmatch '^(/Game/[^.]+)\.[^.]+$') {
        throw "Unsupported Unreal asset path: $UnrealPath"
    }
    $Relative = 'Content/' + ($Matches[1].Substring('/Game/'.Length)) + '.uasset'
    return $Relative
}
function Test-Content([string]$Rel) {
    return (Test-Path -LiteralPath (Join-Path $Repo ($Rel.Replace('/', '\'))) -PathType Leaf)
}
function Get-WeaponIssue([string]$Id) {
    if (-not $ManifestById.ContainsKey($Id)) { return "UNDECLARED:$Id" }
    $Spec = $ManifestById[$Id]
    if ($Spec.ProductionState -ne 'Active') { return "NOT_ACTIVE:$Id" }
    $Relative = Convert-AssetPath $Spec.ItemPath
    if (-not (Test-Content $Relative)) { return "ITEM_PACKAGE_MISSING:$Id" }
    return '' # Technical completeness is separately certified by Unreal tests.
}

foreach ($Recipe in $Recipes) {
    if ($ApprovedIds.ContainsKey($Recipe.KitId) -or $Recipe.ApprovalState -ne 'ApprovedDesign' -or
        $Recipe.KitId -ne ('Kit_' + $Recipe.ClassId + '_' + $Recipe.BranchId) -or
        $Recipe.Weapon1Level -ne '1' -or $Recipe.Weapon1Slot -ne 'MainHand') {
        throw "Invalid or duplicate approved recipe $($Recipe.KitId)."
    }
    $ApprovedIds[$Recipe.KitId] = $true
    $HasSecond = -not (Test-None $Recipe.Weapon2ArchetypeId)
    if (($HasSecond -and ($Recipe.Weapon2Level -ne '1' -or @('Bag','OffHand') -notcontains $Recipe.Weapon2Slot)) -or
        (-not $HasSecond -and ($Recipe.Weapon2Level -ne '0' -or $Recipe.Weapon2Slot -ne 'None')) -or
        ((Test-None $Recipe.KnownSupportItemId) -ne ([int]$Recipe.KnownSupportQuantity -eq 0))) {
        throw "Invalid secondary/support contract for $($Recipe.KitId)."
    }
    $WeaponRefs = @($Recipe.Weapon1ArchetypeId)
    if ($HasSecond) { $WeaponRefs += $Recipe.Weapon2ArchetypeId }
    $Reasons = [System.Collections.Generic.List[string]]::new()
    foreach ($Id in $WeaponRefs) {
        if ($Id -notmatch '^ARM[0-9]{3}$') { throw "Invalid weapon ID $Id in $($Recipe.KitId)." }
        if (-not $WeaponUses.ContainsKey($Id)) {
            $WeaponUses[$Id] = [System.Collections.Generic.List[object]]::new()
        }
        $WeaponUses[$Id].Add([pscustomobject]@{
            KitId = $Recipe.KitId; ClassId = $Recipe.ClassId
            Placement = $(if ($Id -eq $Recipe.Weapon1ArchetypeId) { 'MainHand' } else { $Recipe.Weapon2Slot })
        })
        $Issue = Get-WeaponIssue $Id
        if ($Issue) { $Reasons.Add($Issue) }
    }
    if (-not (Test-None $Recipe.PlannedSupportNeed)) {
        $Reasons.Add('PENDING_SUPPORT:' + $Recipe.PlannedSupportNeed)
        $NonWeapon.Add([pscustomobject]@{KitId=$Recipe.KitId;Dependency=$Recipe.PlannedSupportNeed;Kind='TO_AUTHOR';Status='BLOCKED'})
    }
    if (-not (Test-None $Recipe.KnownSupportItemId)) {
        $SupportRel = 'Content/GrimrockPrototype/Core/DataAssets/Items/DA_' + $Recipe.KnownSupportItemId + '.uasset'
        if (-not (Test-Content $SupportRel)) { $Reasons.Add('SUPPORT_PACKAGE_MISSING:' + $Recipe.KnownSupportItemId) }
        $NonWeapon.Add([pscustomobject]@{KitId=$Recipe.KitId;Dependency=$Recipe.KnownSupportItemId;Kind='KNOWN_ITEM';Status=$(if(Test-Content $SupportRel){'PACKAGE_PRESENT'}else{'PACKAGE_MISSING'})})
    }
    # LOT02.5: presence only permits an Unreal verification, not a
    # silent assertion that the bow correctly consumes the authored arrow.
    if ($Recipe.DesignNoteCode -eq 'AmmoSupplyMustBeVerified') {
        $ArrowRel = 'Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Arrows_01.uasset'
        if (-not (Test-Content $ArrowRel)) { $Reasons.Add('AMMO_PACKAGE_MISSING') }
        $NonWeapon.Add([pscustomobject]@{
            KitId=$Recipe.KitId;Dependency='BowAmmoAndConsumption';Kind='GAMEPLAY_CONTRACT'
            Status=$(if (Test-Content $ArrowRel) { 'PACKAGE_PRESENT_REQUIRES_UE_TEST' } else { 'NOT_VERIFIED' })
        })
    }
    $KitMatrix.Add([pscustomobject]@{
        ClassId=$Recipe.ClassId; BranchId=$Recipe.BranchId; KitId=$Recipe.KitId
        Primary=$Recipe.Weapon1ArchetypeId
        Secondary=$(if($HasSecond){$Recipe.Weapon2ArchetypeId}else{'None'})
        KnownSupport=$Recipe.KnownSupportItemId
        Status=$(if($Reasons.Count -eq 0){'CANDIDATE_FOR_PIE'}else{'BLOCKED'})
        Reasons=$(if($Reasons.Count -eq 0){'PreflightOnly;RuntimeNotEnabled'}else{($Reasons -join ';')})
    })
}
if ($WeaponUses.Count -ne 22) { throw "Approved recipes must reference 22 distinct weapons; got $($WeaponUses.Count)." }

$WeaponBacklog = [System.Collections.Generic.List[object]]::new()
foreach ($Id in @($WeaponUses.Keys | Sort-Object)) {
    $Spec = if ($ManifestById.ContainsKey($Id)) { $ManifestById[$Id] } else { $null }
    $Issue = Get-WeaponIssue $Id
    $References = @($WeaponUses[$Id])
    $WeaponBacklog.Add([pscustomobject]@{
        ArchetypeId=$Id
        ManifestState=$(if($null -eq $Spec){'UNDECLARED'}else{$Spec.ProductionState})
        Issue=$(if($Issue){$Issue}else{'NONE_REQUIRES_UE_VALIDATION'})
        KitsAffected=$References.Count
        KitIds=(($References | ForEach-Object {$_.KitId}) -join ';')
        Positions=(($References | ForEach-Object {$_.Placement}) -join ';')
    })
}

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
$KitsReport = Join-Path $OutputDir 'LOT02_3_All18KitMatrix.csv'
$WeaponsReport = Join-Path $OutputDir 'LOT02_3_All22WeaponBacklog.csv'
$SupportReport = Join-Path $OutputDir 'LOT02_3_NonWeaponDependencies.csv'
$KitMatrix.ToArray() | Export-Csv -LiteralPath $KitsReport -NoTypeInformation -Encoding UTF8
$WeaponBacklog.ToArray() | Export-Csv -LiteralPath $WeaponsReport -NoTypeInformation -Encoding UTF8
$NonWeapon.ToArray() | Export-Csv -LiteralPath $SupportReport -NoTypeInformation -Encoding UTF8

$Candidates = @($KitMatrix | Where-Object {$_.Status -eq 'CANDIDATE_FOR_PIE'})
$Missing = @($WeaponBacklog | Where-Object {$_.Issue -ne 'NONE_REQUIRES_UE_VALIDATION'})
Write-Host "[LOT02.3] ApprovedKits=$($KitMatrix.Count) Candidates=$($Candidates.Count) Blocked=$($KitMatrix.Count - $Candidates.Count) UniqueWeapons=$($WeaponBacklog.Count) WeaponIssues=$($Missing.Count)"
Write-Host "[LOT02.3] Reports: $KitsReport | $WeaponsReport | $SupportReport"
Write-Host '[LOT02.3] Missing weapons by impact:'
$Missing | Sort-Object -Property @{Expression='KitsAffected';Descending=$true},ArchetypeId | Format-Table ArchetypeId,ManifestState,KitsAffected,KitIds -AutoSize

if ($Action -eq 'Validate') {
    foreach ($Filter in @('Grimrock.RPG.ARMEMENT.LOT02','Grimrock.RPG.ARMEMENT01.9','Grimrock.RPG.ARMEMENT01.11.1','Grimrock.RPG.ARMEMENT.Industrial')) {
        & $ValidateUE -EngineRoot $EngineRoot -AutomationFilter $Filter
        if (-not $?) { throw "UE validation failed: $Filter" }
    }
    Write-Host '[LOT02.3] All four UE test filters finished successfully.'
    return
}
if ($Action -ne 'Produce') { return }

$Selected = @($Archetypes | ForEach-Object { $_ -split '[,;]' } | ForEach-Object { $_.Trim().ToUpperInvariant() } | Where-Object { $_ })
if ($Selected.Count -ne @($Selected | Select-Object -Unique).Count) { throw 'Duplicate archetype in selection.' }
$StagedBefore = @(& git -C $Repo diff --cached --name-only)
if ($LASTEXITCODE -ne 0 -or $StagedBefore.Count -ne 0) { throw 'Staging area must be empty before batch production.' }
if (@(& git -C $Repo status --porcelain -- Config/Armement/WeaponProductionManifest.csv).Count -ne 0) {
    throw 'Production manifest must be committed before bulk production.'
}
$ReservedPaths = @('Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.uasset')
foreach ($Path in $ReservedPaths) {
    if (@(& git -C $Repo status --porcelain -- $Path).Count -ne 0) { throw "Shared catalogue already dirty: $Path" }
}
$Preflight = [System.Collections.Generic.List[object]]::new()
foreach ($Id in $Selected) {
    if (-not $WeaponUses.ContainsKey($Id) -or -not $ManifestById.ContainsKey($Id)) {
        throw "Undeclared/unapproved weapon $Id. Author genuine art paths in the manifest first."
    }
    $Spec = $ManifestById[$Id]
    if ($Spec.ProductionState -ne 'Planned') { throw "Batch production requires Planned: $Id" }
    $AssetPaths = @($Spec.ItemPath,$Spec.IconPath,$Spec.MeshPath,$Spec.MaterialPath,
        $Spec.BaseColorPath,$Spec.NormalPath,$Spec.OrmPath) | ForEach-Object { Convert-AssetPath $_ }
    $ArtPaths = @($AssetPaths | Select-Object -Skip 1)
    foreach ($Path in $ArtPaths) {
        if (-not (Test-Content $Path)) { throw "Real imported art missing for $Id : $Path" }
    }
    $Preflight.Add([pscustomobject]@{Id=$Id;Assets=$AssetPaths})
}

# The commandlet remains the C++ authority for meshes, texture settings, N1,
# visual/item/palette identity and all UE industrial tests. No local C++ bypass.
foreach ($Candidate in $Preflight) {
    $Id = $Candidate.Id
    Write-Host "=== [LOT02.3] Production preflight: $Id ==="
    & $WeaponPipeline -EngineRoot $EngineRoot -Action Plan -Archetype $Id
    if (-not $?) { throw "Weapon Plan failed: $Id" }
    $PlanCsv = Join-Path $Repo ("Saved\ARMEMENT-INDUS\INDUS02_Plan_$Id.csv")
    if (-not (Test-Path -LiteralPath $PlanCsv)) { throw "Missing Unreal plan report for $Id" }
    $Results = @(Import-Csv -LiteralPath $PlanCsv)
    if ($Results.Count -ne 1 -or $Results[0].Art -ne 'READY' -or $Results[0].Status -match 'CONFLICT|BLOCKED') {
        throw "UE art preflight not ready for $Id. Inspect $PlanCsv"
    }
}

foreach ($Candidate in $Preflight) {
    $Id = $Candidate.Id
    Write-Host "=== [LOT02.3] Producing $Id ==="
    foreach ($Stage in @('Visual','Item','Palette')) {
        & $WeaponPipeline -EngineRoot $EngineRoot -Action Build -Archetype $Id -Stage $Stage -Apply
        if (-not $?) { throw "$Id $Stage failed. Inspect local assets; no automatic rollback." }
    }
    & $WeaponPipeline -EngineRoot $EngineRoot -Action Audit -Archetype $Id
    if (-not $?) { throw "Audit failed for $Id" }
    $AuditFile = Join-Path $Repo ("Saved\ARMEMENT-INDUS\INDUS02_Audit_$Id.csv")
    $Audit = @(Import-Csv -LiteralPath $AuditFile)
    if ($Audit.Count -ne 1 -or $Audit[0].Status -ne 'COMPLETE') { throw "Produced $Id is not COMPLETE" }
    $Paths = @($Candidate.Assets) + $ReservedPaths
    & git -C $Repo add -- @Paths
    if ($LASTEXITCODE -ne 0) { throw "Git add failed for $Id" }
    $Actual = @(& git -C $Repo diff --cached --name-only)
    if ($LASTEXITCODE -ne 0 -or @($Actual | Where-Object {$Paths -notcontains $_}).Count -ne 0) {
        throw "Unexpected staged paths for $Id. Inspect git diff --cached; no commit performed."
    }
    & git -C $Repo diff --cached --check
    if ($LASTEXITCODE -ne 0 -or $Actual.Count -eq 0) { throw "Empty or whitespace-invalid staging for $Id" }
    & git -C $Repo commit -m "ARMEMENT-LOT02.3: produce $Id for approved starter kits"
    if ($LASTEXITCODE -ne 0) { throw "Commit failed for $Id" }
    if ($Push) {
        & git -C $Repo push origin master
        if ($LASTEXITCODE -ne 0) { throw "Push failed for $Id; the local commit remains available." }
    }
}
Write-Host "[LOT02.3] Produced and committed: $($Selected -join ', '). Run Plan again for updated status."