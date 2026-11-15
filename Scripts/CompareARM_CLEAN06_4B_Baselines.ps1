<#
 ARM-CLEAN06.4B -- compare existing UE baseline manifests and binary receipts.
 Read-only: all property data and changed package names remain private.
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
$OldSha='c66bf68aa1a42eb8d9c47eb9b9336214d0bccf95'
$NewSha='27563be7bee162e86b5f5f9ac29704c1b50aa35c'
$StagePath=Join-Path $RunDir 'ARMClean064B.stage'

function Stage([string]$Value) { Set-Content -LiteralPath $StagePath -Value $Value -Encoding ASCII }
function Equal([object]$A,[object]$B) {
 return [string]::Equals([string]$A,[string]$B,[System.StringComparison]::Ordinal)
}
function Read-Snapshot([string]$Label,[string]$Sha) {
 $Root=Join-Path $StateRoot 'Runs'
 if (-not (Test-Path -LiteralPath $Root -PathType Container)) { throw 'No private runs directory.' }
 $Found=New-Object System.Collections.ArrayList
 foreach ($Run in @(Get-ChildItem -LiteralPath $Root -Directory -ErrorAction Stop)) {
  $Folder=Join-Path $Run.FullName $Label
  $RP=Join-Path $Folder 'asset-hashes.json'
  $MP=Join-Path $Folder 'asset-baseline.json'
  if (-not (Test-Path -LiteralPath $RP -PathType Leaf) -or -not (Test-Path -LiteralPath $MP -PathType Leaf)) { continue }
  $R=Get-Content -LiteralPath $RP -Raw -Encoding UTF8 | ConvertFrom-Json
  if ($R.git_sha -cne $Sha) { continue }
  [void]$Found.Add([pscustomobject]@{ Receipt=$R; ManifestPath=$MP })
 }
 if ($Found.Count -ne 1) { throw ('Private snapshot missing or ambiguous: '+$Label) }
 $Data=$Found[0]
 if ($Data.Receipt.schema -cne ($Label+'/asset-hashes/v1')) { throw 'Receipt schema mismatch.' }
 $Hash=(Get-FileHash -LiteralPath $Data.ManifestPath -Algorithm SHA256).Hash.ToLowerInvariant()
 if (-not (Equal $Hash $Data.Receipt.manifest_sha256)) { throw 'Private manifest hash mismatch.' }
 $M=Get-Content -LiteralPath $Data.ManifestPath -Raw -Encoding UTF8 | ConvertFrom-Json
 if ($M.schema -cne 'ARM-CLEAN06.0/read-only/v1' -or @($M.assets).Count -ne 56 -or
     @($Data.Receipt.assets).Count -ne 56 -or
     @($M.expanded_profiles).Count -ne 1640 -or [int]$M.counts.passive_profiles -ne 260) {
  throw 'Private UE inventory invariants invalid.'
 }
 foreach ($A in @($Data.Receipt.assets)) {
  if ($A.sha256 -cnotmatch '^[a-f0-9]{64}$') { throw 'Asset hash absent or invalid.' }
 }
 return [pscustomobject]@{ Manifest=$M; Receipt=$Data.Receipt; Hash=$Hash }
}
function EdgeText([object[]]$A) {
 $All=@($A | ForEach-Object { [string]$_ } | Sort-Object -CaseSensitive)
 return [string]::Join([string][char]31,[string[]]$All)
}
Stage 'PREFLIGHT'
foreach($Value in @($OldSha,$NewSha,$ExpectedHead)) {
 if ($Value -cnotmatch '^[a-f0-9]{40}$') { throw 'Invalid source SHA.' }
}
$Head=@(& git -C $WorktreePath rev-parse --verify HEAD)
if ($LASTEXITCODE -ne 0 -or $Head.Count -ne 1 -or ([string]$Head[0]).Trim() -cne $ExpectedHead) {
 Stage 'HEAD_MISMATCH'; throw 'Unexpected worktree HEAD.'
}
$Dirty=@(& git -C $WorktreePath status --porcelain --untracked-files=all)
if ($LASTEXITCODE -ne 0 -or $Dirty.Count -ne 0) {
 Stage 'WORKTREE_CHANGED'; throw 'Worktree is dirty.'
}
Stage 'READ_BASELINE'
$Old=Read-Snapshot 'ARM-CLEAN06.0' $OldSha
Stage 'READ_CURRENT'
$Now=Read-Snapshot 'ARM-CLEAN06.4A' $NewSha
if ($null -eq $Now.Receipt.PSObject.Properties['canonical_item_system'] -or
    $Now.Receipt.canonical_item_system.sha256 -cnotmatch '^[a-f0-9]{64}$') {
 throw 'Current canonical root hash is absent.'
}
Stage 'COMPARE'
$Changes=New-Object System.Collections.ArrayList
$Affected=@{}
$Totals=[ordered]@{
 package_set=0
 class_or_kind=0
 property_values=0
 dependencies=0
 referencers=0
 binary_hashes=0
 expanded_profile_fields=0
 changed_packages=0
 changed_profile_rows=0
}
function Record([string]$Kind,[string]$Owner,[string]$Field,[object]$Before,[object]$After) {
 [void]$Changes.Add([ordered]@{
  category=$Kind; owner=$Owner; field=$Field; before=$Before; after=$After
 })
 if ($Kind -ne 'expanded_profile_fields') { $Affected[$Owner]=$true }
}
function Compare-Fields([object]$A,[object]$B,[string]$Owner,[string]$Kind) {
 # PSCustomObject member collection indexing is not portable across
 # PowerShell 5.1 and 7. Materialize explicit name/value maps instead.
 $MapStage=if($Kind -eq 'property_values'){'COMPARE_FIELDS_MAP'}else{'COMPARE_PROFILE_MAP'}
 $ValueStage=if($Kind -eq 'property_values'){'COMPARE_FIELDS_VALUES'}else{'COMPARE_PROFILE_VALUES'}
 Stage $MapStage
 if ($null -eq $A -or $null -eq $B) { throw 'Missing UE serialized property object.' }
 $LM=@{}; $RM=@{}
 foreach($P in @($A.PSObject.Properties)) {
  if ($LM.ContainsKey([string]$P.Name)) { throw 'Duplicate old property name.' }
  $LM.Add([string]$P.Name,[string]$P.Value)
 }
 foreach($P in @($B.PSObject.Properties)) {
  if ($RM.ContainsKey([string]$P.Name)) { throw 'Duplicate current property name.' }
  $RM.Add([string]$P.Name,[string]$P.Value)
 }
 $Names=@(@($LM.Keys)+@($RM.Keys) | Sort-Object -Unique)
 Stage $ValueStage
 foreach ($Name in $Names) {
  $HasOld=$LM.ContainsKey([string]$Name)
  $HasNew=$RM.ContainsKey([string]$Name)
  $LV=if($HasOld){$LM[$Name]}else{'<MISSING>'}
  $RV=if($HasNew){$RM[$Name]}else{'<MISSING>'}
  if ($HasOld -ne $HasNew -or -not (Equal $LV $RV)) {
   $Totals[$Kind]=[int]$Totals[$Kind]+1
   Record $Kind $Owner ([string]$Name) $LV $RV
  }
 }
}
Stage 'COMPARE_MAPS'
$OM=@{}; $NM=@{}; $OH=@{}; $NH=@{}
foreach($A in @($Old.Manifest.assets)) { $OM.Add([string]$A.package,$A) }
foreach($A in @($Now.Manifest.assets)) { $NM.Add([string]$A.package,$A) }
foreach($A in @($Old.Receipt.assets)) { $OH.Add([string]$A.package,$A) }
foreach($A in @($Now.Receipt.assets)) { $NH.Add([string]$A.package,$A) }
if ($OM.Count -ne 56 -or $NM.Count -ne 56 -or $OH.Count -ne 56 -or $NH.Count -ne 56) {
 throw 'Unexpected duplicate packages in snapshot.'
}
Stage 'COMPARE_ASSETS'
$Packages=@(@($OM.Keys)+@($NM.Keys) | Sort-Object -Unique)
Stage 'COMPARE_ASSET_CLASS'
foreach($Package in $Packages) {
 if (-not $OM.ContainsKey($Package) -or -not $NM.ContainsKey($Package) -or
     -not $OH.ContainsKey($Package) -or -not $NH.ContainsKey($Package)) {
  $Totals.package_set++
  Record 'package_set' $Package 'presence' ($OM.ContainsKey($Package)) ($NM.ContainsKey($Package))
  continue
 }
 $L=$OM[$Package]; $R=$NM[$Package]
 Stage 'COMPARE_ASSET_CLASS'
 foreach($F in @('kind','class')) {
  $LP=$L.PSObject.Properties[$F]
  $RP=$R.PSObject.Properties[$F]
  $LV=if($null -eq $LP){'<MISSING>'}else{[string]$LP.Value}
  $RV=if($null -eq $RP){'<MISSING>'}else{[string]$RP.Value}
  if (-not (Equal $LV $RV)) {
   $Totals.class_or_kind++
   Record 'class_or_kind' $Package $F $LV $RV
  }
 }
 Stage 'COMPARE_ASSET_PROPERTIES'
 Compare-Fields $L.properties $R.properties $Package 'property_values'
 Stage 'COMPARE_ASSET_EDGES'
 foreach($F in @('dependencies','referencers')) {
  $LP=$L.PSObject.Properties[$F]
  $RP=$R.PSObject.Properties[$F]
  $LV=if($null -eq $LP){'<MISSING>'}else{EdgeText @($LP.Value)}
  $RV=if($null -eq $RP){'<MISSING>'}else{EdgeText @($RP.Value)}
  if (-not (Equal $LV $RV)) {
   $Totals[$F]=[int]$Totals[$F]+1
   Record $F $Package $F $LV $RV
  }
 }
 Stage 'COMPARE_ASSET_HASHES'
 $OL=$OH[$Package]
 $NL=$NH[$Package]
 if (-not (Equal $OL.sha256 $NL.sha256) -or
     [int64]$OL.bytes -ne [int64]$NL.bytes) {
  $Totals.binary_hashes++
  Record 'binary_hashes' $Package 'sha256' $OL.sha256 $NL.sha256
 }
}
Stage 'COMPARE_PROFILES'
for($I=0;$I -lt 1640;$I++) {
 $Arm='ARM{0:D3}' -f (1+[int][Math]::Floor($I/20))
 $Owner=$Arm+'/N'+(1+($I%20))
 $Count=$Totals.expanded_profile_fields
 Compare-Fields $Old.Manifest.expanded_profiles[$I] $Now.Manifest.expanded_profiles[$I] $Owner 'expanded_profile_fields'
 if($Totals.expanded_profile_fields -gt $Count) { $Totals.changed_profile_rows++ }
}
$Totals.changed_packages=$Affected.Count
Stage 'WRITE_PRIVATE'
$Output=Join-Path $RunDir 'ARM-CLEAN06.4B'
New-Item -ItemType Directory -Path $Output -Force | Out-Null
$Private=[ordered]@{
 schema='ARM-CLEAN06.4B/compare/v1'
 comparison_sha=$ExpectedHead
 original_sha=$OldSha
 current_sha=$NewSha
 original_manifest_sha256=$Old.Hash
 current_manifest_sha256=$Now.Hash
 counts=$Totals
 migration_authorized=$false
 changes=@($Changes.ToArray())
}
$Private | ConvertTo-Json -Depth 9 |
 Set-Content -LiteralPath (Join-Path $Output 'comparison-private.json') -Encoding UTF8
Stage 'CHECK_CLEAN'
$Dirty=@(& git -C $WorktreePath status --porcelain --untracked-files=all)
if($LASTEXITCODE -ne 0 -or $Dirty.Count -ne 0) {
 Stage 'WORKTREE_CHANGED'; throw 'Comparison unexpectedly changed checkout.'
}
Stage 'ISSUE_BUILD'
$Lines=@(
 '# ARM-CLEAN06.4B - comparaison en lecture seule'
 ''
 ('Commit du comparateur : '+$ExpectedHead)
 ('Source 06.0 : '+$OldSha)
 ('Source 06.4A : '+$NewSha)
 ''
 '| Mesure | Ecarts |'
 '|---|---:|'
)
foreach($K in @('package_set','class_or_kind','property_values',
 'dependencies','referencers','binary_hashes','expanded_profile_fields',
 'changed_packages','changed_profile_rows')) {
 $Lines+=('| '+$K+' | '+$Totals[$K]+' |')
}
$Lines+=@(
 ''
 'Le contenu detaille des UPROPERTY, chemins et graphs reste prive.'
 'Aucun nom de sauvegarde ou detail de SaveGame n est diffuse.'
 'La comparaison ne permet aucune migration automatique.'
 'Aucun .uasset, .umap ou .sav modifie.'
)
$Body=Join-Path $Output 'github-issue.md'
Set-Content -LiteralPath $Body -Value $Lines -Encoding UTF8
Stage 'ISSUE_WRITE'
$Title='[ARM-CLEAN06.4B] Dry-run parity '+$ExpectedHead.Substring(0,12)
$Url=@(& gh issue create --repo $GitHubRepository --title $Title --body-file $Body)
if($LASTEXITCODE -ne 0 -or $Url.Count -ne 1 -or
  [string]$Url[0] -notmatch '^https://github\.com/[^/]+/[^/]+/issues/[0-9]+$') {
 throw 'Issue publication failed.'
}
Stage 'SUCCESS'
Write-Host ('[ARM-CLEAN06.4B] COMPARE_OK '+$Url[0])
