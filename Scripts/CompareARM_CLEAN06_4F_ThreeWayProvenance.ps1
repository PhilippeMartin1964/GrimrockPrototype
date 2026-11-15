<#
 ARM-CLEAN06.4F: read-only private provenance of the four Sword
 presentations. No binary writes and no private names in GitHub issues.
#>
[CmdletBinding()]
param(
 [Parameter(Mandatory=$true)][string]$WorktreePath,
 [Parameter(Mandatory=$true)][string]$StateRoot,
 [Parameter(Mandatory=$true)][string]$RunDir,
 [Parameter(Mandatory=$true)][string]$ExpectedHead,
 [string]$GitHubRepository='PhilippeMartin1964/GrimrockPrototype'
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$Sha00='c66bf68aa1a42eb8d9c47eb9b9336214d0bccf95'
$Sha2A='2c0a4768cb815baebc431a055f5679b15517cecd'
$Sha4A='27563be7bee162e86b5f5f9ac29704c1b50aa35c'
$Sha4E='e011d62ff9d2f446c427fba9798ac4af80ebcfeb'
$Hash00='03c3b09c1430fc8938e4c4bf1cde6fb229334aeda2348ed902c359c4edca6940'
$Hash4A='c00c395cc19645e9f798df5c4a7c38d94aa9d1ecb1e8487ca27d07f864c2b1f9'
$StageFile=Join-Path $RunDir 'ARMClean064F.stage'
function Stage([string]$Code) {Set-Content -LiteralPath $StageFile -Value $Code -Encoding ASCII}
function Clean {
 $Check=@(& git -C $WorktreePath status --porcelain --untracked-files=all)
 if($LASTEXITCODE -ne 0 -or $Check.Count -ne 0) {
  Stage 'WORKTREE_CHANGED';throw 'Dirty validation worktree.'
 }
}
function Find-Proof([string]$Label,[string]$Filename,[string]$Key,[string]$Sha) {
 $Result=New-Object System.Collections.ArrayList
 $Root=Join-Path $StateRoot 'Runs'
 if(-not (Test-Path -LiteralPath $Root -PathType Container)) {throw 'No private runs.'}
 foreach($D in @(Get-ChildItem -LiteralPath $Root -Directory -ErrorAction Stop)) {
  $P=Join-Path (Join-Path $D.FullName $Label) $Filename
  if(-not (Test-Path -LiteralPath $P -PathType Leaf)) {continue}
  $Data=Get-Content -LiteralPath $P -Raw -Encoding UTF8 | ConvertFrom-Json
  $Id=$Data.PSObject.Properties[$Key]
  if($null -eq $Id -or [string]$Id.Value -cne $Sha) {continue}
  [void]$Result.Add([pscustomobject]@{Path=$P;Data=$Data})
 }
 if($Result.Count -ne 1) {throw 'Private source ambiguous or missing.'}
 return $Result[0]
}
function Manifest([string]$Path,[string]$Hash) {
 if(-not (Test-Path -LiteralPath $Path -PathType Leaf)) {throw 'UE snapshot missing.'}
 $Actual=(Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
 if($Actual -cne $Hash) {throw 'Manifest hash differs from signed report.'}
 $M=Get-Content -LiteralPath $Path -Raw -Encoding UTF8 | ConvertFrom-Json
 if($M.schema -cne 'ARM-CLEAN06.0/read-only/v1' -or
 @($M.assets).Count -ne 56 -or @($M.expanded_profiles).Count -ne 1640 -or
 [int]$M.counts.passive_profiles -ne 260) {throw 'Invalid baseline cardinalities.'}
 return $M
}
function AssetIndex([object]$M) {
 $R=@{}
 foreach($A in @($M.assets)) {$R.Add([string]$A.package,$A)}
 if($R.Count -ne 56) {throw 'Duplicate UE packages.'}
 return $R
}
function Props([object]$Record) {
 $R=@{}
 foreach($P in @($Record.properties.PSObject.Properties)) {
  $R.Add([string]$P.Name,[string]$P.Value)
 }
 return $R
}
function Deltas([object]$A,[object]$B) {
 $Old=Props $A;$New=Props $B
 $Differences=New-Object System.Collections.ArrayList
 $All=@(@($Old.Keys)+@($New.Keys) | Sort-Object -Unique)
 foreach($Key in $All) {
  if(-not $Old.ContainsKey($Key) -or -not $New.ContainsKey($Key) -or
     $Old[$Key] -cne $New[$Key]) {[void]$Differences.Add([string]$Key)}
 }
 return @($Differences.ToArray())
}
function Edges([object[]]$Values) {
 $Sorted=@($Values | ForEach-Object {[string]$_} | Sort-Object -CaseSensitive)
 return [string]::Join([string][char]31,[string[]]$Sorted)
}
Stage 'PREFLIGHT'
foreach($Sha in @($ExpectedHead,$Sha00,$Sha2A,$Sha4A,$Sha4E)) {
 if($Sha -cnotmatch '^[a-f0-9]{40}$') {throw 'Invalid SHA.'}
}
$Head=@(& git -C $WorktreePath rev-parse --verify HEAD)
if($LASTEXITCODE -ne 0 -or $Head.Count -ne 1 -or
 ([string]$Head[0]).Trim() -cne $ExpectedHead) {
 Stage 'HEAD_MISMATCH';throw 'Wrong validation SHA.'
}
Clean
Stage 'READ_PROOFS'
$P0=Find-Proof 'ARM-CLEAN06.0' 'asset-hashes.json' 'git_sha' $Sha00
$PA=Find-Proof 'ARM-CLEAN06.2A' 'source-parity.json' 'audited_head' $Sha2A
$PC=Find-Proof 'ARM-CLEAN06.4A' 'asset-hashes.json' 'git_sha' $Sha4A
$PE=Find-Proof 'ARM-CLEAN06.4E' 'presentation-gate-private.json' 'validation_sha' $Sha4E
if($P0.Data.manifest_sha256 -cne $Hash00 -or
 $PA.Data.schema -cne 'ARM-CLEAN06.2A/source-parity/v1' -or
 $PA.Data.source_baseline_sha -cne $Sha00 -or
 $PA.Data.user_audio_sha -cne 'a60031db0d05aa444a7453f061e7463bd4e0b4d5' -or
 [int]$PA.Data.changed_weapon_definitions -ne 4 -or
 [int]$PA.Data.profile_mismatches -ne 0 -or
 $PC.Data.manifest_sha256 -cne $Hash4A -or
 $PE.Data.schema -cne 'ARM-CLEAN06.4E/presentation-gate/v1' -or
 [bool]$PE.Data.migration_authorized -or
 @($PE.Data.allowed_binary_writes).Count -ne 0) {
 throw 'Source evidence provenance invalid.'
}
$ParityPath=Join-Path (Split-Path -Parent $PE.Path) 'presentation-parity-private.json'
if(-not (Test-Path -LiteralPath $ParityPath -PathType Leaf) -or
 (Get-FileHash -LiteralPath $ParityPath -Algorithm SHA256).Hash.ToLowerInvariant() -cne
 [string]$PE.Data.parity_report_sha256) {
 throw 'Real UE canonical parity report hash invalid.'
}
$Parity=Get-Content -LiteralPath $ParityPath -Raw -Encoding UTF8 | ConvertFrom-Json
if([int]$Parity.fully_matching_assets -ne 4 -or
 [int]$Parity.equal_attack_bindings -ne 4 -or
 [int]$Parity.different_attack_bindings -ne 0 -or
 [int]$Parity.unresolved_attack_bindings -ne 0) {
 throw 'Four authored presentation profiles not strictly paritary.'
}
$M0=Manifest (Join-Path (Split-Path -Parent $P0.Path) 'asset-baseline.json') $Hash00
$MA=Manifest (Join-Path (Split-Path -Parent $PA.Path) 'post-audio-baseline.json') ([string]$PA.Data.current_manifest_sha256)
$MC=Manifest (Join-Path (Split-Path -Parent $PC.Path) 'asset-baseline.json') $Hash4A
Stage 'COMPARE'
$Old=AssetIndex $M0;$Audio=AssetIndex $MA;$Current=AssetIndex $MC
$AudioPackages=0;$CurrentPackages=0
$AudioFields=0;$CurrentFields=0;$PreservedFields=0
$PostAudioDrift=0;$DependencyDrift=0;$ProfileDrift=0
$ChangedNow=@{}
$Details=New-Object System.Collections.ArrayList
foreach($Name in @($Old.Keys)) {
 if(-not $Audio.ContainsKey($Name) -or -not $Current.ContainsKey($Name)) {
  throw '56 package set is not stable.'
 }
 $B=$Old[$Name];$A=$Audio[$Name];$C=$Current[$Name]
 if($B.class -cne $A.class -or $A.class -cne $C.class -or
 $B.kind -cne $A.kind -or $A.kind -cne $C.kind) {
  throw 'Serialized package class drift.'
 }
 $D0A=@(Deltas $B $A);$D0C=@(Deltas $B $C);$DAC=@(Deltas $A $C)
 if($D0A.Count -gt 0) {$AudioPackages++}
 if($D0C.Count -gt 0) {$CurrentPackages++;$ChangedNow[$Name]=$true}
 $AudioFields+=$D0A.Count;$CurrentFields+=$D0C.Count
 $PostAudioDrift+=$DAC.Count
 $Before=Props $B;$AtAudio=Props $A;$AtCurrent=Props $C
 foreach($Field in $D0C) {
  if($D0A -ccontains $Field -and $AtAudio.ContainsKey($Field) -and
     $AtCurrent.ContainsKey($Field) -and
     $AtAudio[$Field] -ceq $AtCurrent[$Field]) {$PreservedFields++}
 }
 if((Edges @($A.dependencies)) -cne (Edges @($C.dependencies))) {
  $DependencyDrift++
 }
 if($D0A.Count -gt 0 -or $D0C.Count -gt 0 -or $DAC.Count -gt 0) {
  [void]$Details.Add([ordered]@{
   package=$Name
   changed_from_baseline_after_audio=@($D0A)
   changed_from_baseline_now=@($D0C)
   changed_since_post_audio=@($DAC)
  })
 }
}
for($I=0;$I -lt 1640;$I++) {
 $X=Props ([pscustomobject]@{properties=$MA.expanded_profiles[$I]})
 $Y=Props ([pscustomobject]@{properties=$MC.expanded_profiles[$I]})
 foreach($Key in @($X.Keys)) {
  if(-not $Y.ContainsKey($Key) -or $X[$Key] -cne $Y[$Key]) {
   $ProfileDrift++;break
  }
 }
}
foreach($Row in @($Parity.sources)) {
 if(-not $ChangedNow.ContainsKey([string]$Row.package) -or
    [string]$Row.status -cne 'MATCHED_PRESENTATIONS') {
  throw 'Current four-package set does not match actual UE parity.'
 }
}
Stage 'WRITE_PRIVATE'
$Folder=Join-Path $RunDir 'ARM-CLEAN06.4F'
New-Item -ItemType Directory -Path $Folder -Force | Out-Null
$IsProven=($AudioPackages -eq 4 -and $CurrentPackages -eq 4 -and
 $AudioFields -eq 8 -and $CurrentFields -eq 8 -and
 $PreservedFields -eq 8 -and $PostAudioDrift -eq 0 -and
 $DependencyDrift -eq 0 -and $ProfileDrift -eq 0)
$Result=[ordered]@{
 schema='ARM-CLEAN06.4F/three-way/v1'
 validation_sha=$ExpectedHead
 baseline_sha=$Sha00
 user_audio_audit_sha=$Sha2A
 recapture_sha=$Sha4A
 canonical_parity_sha=$Sha4E
 changed_packages_06_2A=$AudioPackages
 changed_packages_06_4A=$CurrentPackages
 changed_fields_06_2A=$AudioFields
 changed_fields_06_4A=$CurrentFields
 preserved_post_audio_fields=$PreservedFields
 later_property_drifts=$PostAudioDrift
 later_dependency_drifts=$DependencyDrift
 later_profile_drifts=$ProfileDrift
 source_parity_verified=$IsProven
 migration_authorized=$false
 allowed_binary_writes=@()
 allowed_package_deletions=@()
 allowed_savegame_writes=@()
 private_deltas=@($Details.ToArray())
}
$Result | ConvertTo-Json -Depth 10 |
 Set-Content -LiteralPath (Join-Path $Folder 'three-way-private.json') -Encoding UTF8
Stage 'CHECK_CLEAN'
Clean
Stage 'ISSUE_BUILD'
$Lines=@(
 '# ARM-CLEAN06.4F - three-way historique en lecture seule'
 ''
 ('Commit : '+$ExpectedHead)
 ''
 '| Controle source | Nombre |'
 '|---|---:|'
 ('| Packages deja modifies en 06.2A | '+$AudioPackages+' |')
 ('| Packages modifies en 06.4A | '+$CurrentPackages+' |')
 ('| Champs nouveaux en 06.2A | '+$AudioFields+' |')
 ('| Champs differents en 06.4A | '+$CurrentFields+' |')
 ('| Champs conserves depuis 06.2A | '+$PreservedFields+' |')
 ('| Derives UPROPERTY apres 06.2A | '+$PostAudioDrift+' |')
 ('| Derives de dependances depuis 06.2A | '+$DependencyDrift+' |')
 ('| Derives de profils ARM depuis 06.2A | '+$ProfileDrift+' |')
 ('| Provenance des huit champs prouvee | '+([int]$IsProven)+' |')
 '| Ecritures de binaires autorisees | 0 |'
 ''
 'Les noms et valeurs de source historiques restent uniquement dans le rapport prive.'
 'Une preuve positive ne constitue pas une autorisation de conversion binaire.'
)
$Body=Join-Path $Folder 'github-issue.md'
Set-Content -LiteralPath $Body -Value $Lines -Encoding UTF8
Stage 'ISSUE_WRITE'
$Title='[ARM-CLEAN06.4F] Historical source provenance '+$ExpectedHead.Substring(0,12)
$Link=@(& gh issue create --repo $GitHubRepository --title $Title --body-file $Body)
if($LASTEXITCODE -ne 0 -or $Link.Count -ne 1 -or
 [string]$Link[0] -notmatch '^https://github\.com/[^/]+/[^/]+/issues/[0-9]+$') {
 throw 'Public sanitized provenance receipt failed.'
}
Stage 'SUCCESS'
Write-Host ('[ARM-CLEAN06.4F] THREE_WAY_DONE '+$Link[0])
