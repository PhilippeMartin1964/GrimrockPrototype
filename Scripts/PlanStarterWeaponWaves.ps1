[CmdletBinding()]
param([ValidateSet('Plan','Validate')][string]$Action='Plan',
      [string]$EngineRoot='D:\UE_5.5')
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$Repo=Split-Path -Parent $PSScriptRoot
if ((& git -C $Repo branch --show-current).Trim() -ne 'master') { throw 'Master only.' }
$Specs=@(Import-Csv (Join-Path $Repo 'docs/Design/ARMEMENT_LOT02_6_MISSING_WEAPON_WAVES.csv'))
$Recipes=@(Import-Csv (Join-Path $Repo 'docs/Design/ARMEMENT01_9_6_STARTER_KIT_RECIPES.csv'))
$Manifest=@(Import-Csv (Join-Path $Repo 'Config/Armement/WeaponProductionManifest.csv'))
if($Specs.Count -ne 11 -or $Recipes.Count -ne 18){throw 'Expected 11 waves and 18 recipes.'}
$Wanted=@('ARM038','ARM048','ARM054','ARM057','ARM058','ARM061','ARM062','ARM067','ARM069','ARM070','ARM081')
if((@($Specs.ArchetypeId | Sort-Object) -join ',') -cne (($Wanted | Sort-Object) -join ',')) { throw '11 wave identities mismatch.' }
$Rows=[System.Collections.Generic.List[object]]::new()
foreach($S in $Specs){
    $Kits=@($Recipes | Where-Object { $_.Weapon1ArchetypeId -eq $S.ArchetypeId -or $_.Weapon2ArchetypeId -eq $S.ArchetypeId })
    $M=@($Manifest | Where-Object { $_.ArchetypeId -eq $S.ArchetypeId })
    if($M.Count -gt 1) { throw 'Duplicate archetype in manifest.' }
    $State=if($M.Count -eq 0){'UNDECLARED'}else{$M[0].ProductionState}
    $Rows.Add([pscustomobject]@{
        ArchetypeId=$S.ArchetypeId;Wave=$S.Wave;ProposedMode=$S.ProposedMode
        PipelineSupported=$S.PipelineSupported;ManifestState=$State
        KitsAffected=$Kits.Count;KitIds=(@($Kits.KitId) -join ';');Review=$S.Review
        NextAction=$(if($State -eq 'Active'){'UE_VERIFY_ACTIVE'}elseif($S.PipelineSupported -eq 'Yes'){'IMPORT_AUTHENTIC_ART'}else{'REVIEW_GAMEPLAY_HANDLER'})
    })
}
$Folder=Join-Path $Repo 'Saved/ARMEMENT-LOT02'
New-Item -ItemType Directory -Force -Path $Folder | Out-Null
$Out=Join-Path $Folder 'LOT02_6_WeaponWaves.csv'
$Rows | Sort-Object Wave,ArchetypeId | Export-Csv -LiteralPath $Out -NoTypeInformation -Encoding UTF8
Write-Host "[LOT02.6] Grouped eleven-weapon production report: $Out"
$Rows | Format-Table ArchetypeId,Wave,ProposedMode,ManifestState,KitsAffected,NextAction -AutoSize
if($Action -eq 'Validate'){
    & (Join-Path $PSScriptRoot 'StarterKitProductionBatch.ps1') -Action Validate -EngineRoot $EngineRoot
    if(-not $?) {throw 'Grouped Unreal validation failed.'}
}
