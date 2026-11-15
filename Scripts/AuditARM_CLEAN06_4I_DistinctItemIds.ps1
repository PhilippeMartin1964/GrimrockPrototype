<#
 ARM-CLEAN06.4I — distinct ItemId and serialized metadata gap inventory.
 Windows private UE 06.4H report is the immutable input. No asset writes.
#>
[CmdletBinding()]
param(
 [Parameter(Mandatory=$true)][string]$WorktreePath,
 [Parameter(Mandatory=$true)][string]$EngineRoot,
 [Parameter(Mandatory=$true)][string]$StateRoot,
 [Parameter(Mandatory=$true)][string]$RunDir,
 [Parameter(Mandatory=$true)][string]$ExpectedHead,
 [string]$GitHubRepository='PhilippeMartin1964/GrimrockPrototype'
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$PriorSha='aedd0d06a23ba59da9a05b89221979246e8ecbe0'
$SnapshotSha='e0f877d17f39d913c585b55ba18110745c5385b6'
$StagePath=Join-Path $RunDir 'ARMClean064I.stage'
function Stage([string]$S) { Set-Content -LiteralPath $StagePath -Value $S -Encoding ASCII }
function Clean {
 $Files=@(& git -C $WorktreePath status --porcelain --untracked-files=all)
 if($LASTEXITCODE -ne 0 -or $Files.Count -ne 0) {
  Stage 'WORKTREE_CHANGED'
  throw 'Validation worktree modified.'
 }
}
function Hash([string]$Path) {
 return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}
function Find-Private([string]$Label,[string]$File,[string]$Key,[string]$Sha) {
 $Runs=Join-Path $StateRoot 'Runs'
 if(-not (Test-Path -LiteralPath $Runs -PathType Container)) {
  throw 'Private validation runs absent.'
 }
 $Found=New-Object System.Collections.ArrayList
 foreach($R in @(Get-ChildItem -LiteralPath $Runs -Directory -ErrorAction Stop)) {
  $P=Join-Path (Join-Path $R.FullName $Label) $File
  if(-not (Test-Path -LiteralPath $P -PathType Leaf)) {continue}
  $Data=Get-Content -LiteralPath $P -Raw -Encoding UTF8 | ConvertFrom-Json
  $Property=$Data.PSObject.Properties[$Key]
  if($null -ne $Property -and [string]$Property.Value -ceq $Sha) {
   [void]$Found.Add([pscustomobject]@{Path=$P;Data=$Data})
  }
 }
 if($Found.Count -ne 1) { throw 'Expected one exact private source report.' }
 return $Found[0]
}
Stage 'PREFLIGHT'
foreach($Sha in @($ExpectedHead,$PriorSha,$SnapshotSha)) {
 if($Sha -cnotmatch '^[a-f0-9]{40}$') { throw 'Invalid commit SHA.' }
}
$Head=@(& git -C $WorktreePath rev-parse --verify HEAD)
if($LASTEXITCODE -ne 0 -or $Head.Count -ne 1 -or
 ([string]$Head[0]).Trim() -cne $ExpectedHead) {
 Stage 'HEAD_MISMATCH'; throw 'HEAD does not match expected SHA.'
}
$Branch=@(& git -C $WorktreePath symbolic-ref --quiet --short HEAD 2>$null)
if($LASTEXITCODE -eq 0 -and $Branch.Count -gt 0) {
 throw 'A detached validation worktree is required.'
}
$RunRoot=[IO.Path]::GetFullPath($RunDir).TrimEnd('\','/')
$StateAbs=[IO.Path]::GetFullPath($StateRoot).TrimEnd('\','/')
$WorktreeAbs=[IO.Path]::GetFullPath($WorktreePath).TrimEnd('\','/')
if(-not $RunRoot.StartsWith(($StateAbs+'\'),[StringComparison]::OrdinalIgnoreCase) -or
 $RunRoot.StartsWith(($WorktreeAbs+'\'),[StringComparison]::OrdinalIgnoreCase)) {
 throw 'Private output may not be inside the project.'
}
Clean
Stage 'READ_4H'
Stage 'FIND_4H'
$Audit=Find-Private 'ARM-CLEAN06.4H' 'world-reference-private.json' 'schema' 'ARM-CLEAN06.4H/real-references/v1'
Stage 'CHECK_4H_GATE_FILE'
$GatePath=Join-Path (Split-Path -Parent $Audit.Path) 'world-identity-gate-private.json'
if(-not (Test-Path -LiteralPath $GatePath -PathType Leaf)) {
 throw '06.4H private gate missing.'
}
Stage 'VERIFY_4H_RECEIPT'
$Gate=Get-Content -LiteralPath $GatePath -Raw -Encoding UTF8 | ConvertFrom-Json
if($Gate.schema -cne 'ARM-CLEAN06.4H/world-identity-gate/v1' -or
 $Gate.validation_sha -cne $PriorSha -or
 $Gate.source_backup_sha -cne $SnapshotSha -or
 $Gate.report_sha256 -cne (Hash $Audit.Path) -or
 [int]$Gate.candidate_references -ne 95 -or
 $Gate.migration_authorized -ne $false -or
 @($Gate.allowed_binary_writes).Count -ne 0 -or
 @($Gate.allowed_package_deletions).Count -ne 0 -or
 @($Gate.allowed_savegame_writes).Count -ne 0 -or
 @($Audit.Data.reference_rows).Count -ne 95 -or
 [int]$Audit.Data.canonical_definitions -ne 23 -or
 [int]$Audit.Data.levels_scanned -ne 4 -or
 [bool]$Audit.Data.savegame_files_opened) {
 throw '06.4H private evidence or closed migration gate invalid.'
}
Stage 'CHECK_4H_ROWS'
$Rows=@($Audit.Data.reference_rows)
$Candidates=@($Rows | Where-Object {[string]$_.status -ceq 'ID_CANDIDATE_ONLY'})
$Missing=@($Rows | Where-Object {[string]$_.status -ceq 'NO_CANONICAL_ID'})
if($Candidates.Count -ne 55 -or $Missing.Count -ne 40) {
 throw '06.4H classification no longer matches issue 141.'
}
Stage 'CHECK_4H_CATEGORIES'
$Categories=@('LEVEL_LOOSE','LEVEL_SPAWN','LEVEL_RECEPTACLE',
 'PALETTE_DIRECT','DEFINITION_RECEPTACLE')
$Known=@{}
foreach($Cat in $Categories) {$Known[$Cat]=$true}
foreach($Row in $Rows) {
 if(-not $Known.ContainsKey([string]$Row.category)) {
  throw 'Unexpected UE reference family.'
 }
}
Stage 'VERIFY_BACKUP'
$Receipt=Find-Private 'ARM-CLEAN06.4G' 'binary-snapshot-receipt.json' 'validation_sha' $SnapshotSha
$ReceiptGate=Join-Path (Split-Path -Parent $Receipt.Path) 'transaction-gate-private.json'
if(-not (Test-Path -LiteralPath $ReceiptGate -PathType Leaf)) {
 throw '06.4G original transaction gate missing.'
}
$BackupGate=Get-Content -LiteralPath $ReceiptGate -Raw -Encoding UTF8 | ConvertFrom-Json
if($BackupGate.snapshot_receipt_sha256 -cne (Hash $Receipt.Path) -or
 $BackupGate.migration_authorized -ne $false -or
 @($Receipt.Data.source_files).Count -ne 84 -or
 $Receipt.Data.copy_verified -ne $true) {
 throw '06.4G binary snapshot receipt invalid.'
}
$BackupRoot=Join-Path (Split-Path -Parent $Receipt.Path) 'SourceSnapshot'
$Seen=@{}
foreach($Record in @($Receipt.Data.source_files)) {
 $Rel=[string]$Record.relative
 if($Rel -cnotmatch '^Content/GrimrockPrototype/Core/DataAssets/[A-Za-z0-9_/-]+\.uasset$' -or
 $Rel.Contains('..') -or $Seen.ContainsKey($Rel) -or
 [string]$Record.sha256 -cnotmatch '^[a-f0-9]{64}$') {
  throw 'Invalid binary snapshot allowlist.'
 }
 $Seen[$Rel]=$true
 $Original=Join-Path $WorktreePath $Rel.Replace('/','\')
 $Backup=Join-Path $BackupRoot $Rel.Replace('/','\')
 if(-not (Test-Path -LiteralPath $Original -PathType Leaf) -or
 -not (Test-Path -LiteralPath $Backup -PathType Leaf) -or
 (Get-Item -LiteralPath $Original).Length -ne [int64]$Record.bytes -or
 (Get-Item -LiteralPath $Backup).Length -ne [int64]$Record.bytes -or
 (Hash $Original) -cne [string]$Record.sha256 -or
 (Hash $Backup) -cne [string]$Record.sha256) {
  throw 'Original asset or archived backup has changed.'
 }
}
if($Seen.Count -ne 84) {throw 'Expected exactly 84 protected UE binaries.'}
Stage 'UE_IDENTITY_AUDIT'
$Editor=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Project=Join-Path $WorktreePath 'GrimrockPrototype.uproject'
if(-not (Test-Path -LiteralPath $Editor -PathType Leaf) -or
 -not (Test-Path -LiteralPath $Project -PathType Leaf)) {
 throw 'UE5.5.4 editor or project unavailable.'
}
$Output=Join-Path $RunDir 'ARM-CLEAN06.4I'
if(Test-Path -LiteralPath $Output) {
 throw 'Refuse to overwrite a previous private identity audit.'
}
New-Item -ItemType Directory -Path $Output -ErrorAction Stop | Out-Null
$OutputReport=Join-Path $Output 'identity-gap-private.json'
& $Editor $Project '-run=GridArmClean064IIdentityGap' '-unattended' '-NoSplash' '-NoP4' '-NullRHI' "-Input=$($Audit.Path)" "-Output=$OutputReport" | Out-Null
if($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $OutputReport -PathType Leaf)) {
 Stage 'UE_IDENTITY_FAILED'
 throw 'UE Editor-only distinct identity commandlet failed.'
}
Stage 'VERIFY_RESULT'
$Report=Get-Content -LiteralPath $OutputReport -Raw -Encoding UTF8 | ConvertFrom-Json
if($Report.schema -cne 'ARM-CLEAN06.4I/distinct-identity-gap/v1' -or
 [int]$Report.world_references -ne 95 -or
 [int]$Report.canonical_candidate_occurrences -ne 55 -or
 [int]$Report.missing_canonical_occurrences -ne 40 -or
 @($Report.verified_world_references).Count -ne 95 -or
 [bool]$Report.migration_authorized -or
 [int]$Report.legacy_sources_dirty_state_changed -ne 0) {
 throw 'UE distinct-id audit inconsistent.'
}
$MissingIDs=[int]$Report.unique_missing_item_ids
$KnownIDs=[int]$Report.unique_matching_item_ids
if($MissingIDs -lt 1 -or $MissingIDs -gt 40 -or
 $KnownIDs -lt 1 -or $KnownIDs -gt 55 -or
 @($Report.missing_canonical_id_details).Count -ne $MissingIDs -or
 @($Report.existing_canonical_id_details).Count -ne $KnownIDs -or
 [int]$Report.N0_occurrences+[int]$Report.fixed_level_occurrences -ne 95 -or
 [int]$Report.missing_weapon_occurrences+
 [int]$Report.missing_nonweapon_occurrences -ne 40) {
 throw 'Unreconciled unique ID/level/weapon counters.'
}
$S=0
foreach($Item in @($Report.missing_canonical_id_details)) {
 $S+=[int]$Item.world_occurrences
 if([int]$Item.world_occurrences -le 0 -or
   [int]$Item.distinct_legacy_assets -le 0) {
  throw 'Incomplete missing ItemId signature.'
 }
}
if($S -ne 40) {throw 'Missing ItemIds do not account for all 40 references.'}
$S=0
foreach($Item in @($Report.existing_canonical_id_details)) {
 $S+=[int]$Item.world_occurrences
}
if($S -ne 55) {throw 'Existing IDs do not account for 55 references.'}
Stage 'WRITE_GATE'
$GateResult=[ordered]@{
 schema='ARM-CLEAN06.4I/distinct-identity-gate/v1'
 validation_sha=$ExpectedHead
 source_report_sha=$PriorSha
 source_binary_snapshot_sha=$SnapshotSha
 source_report_sha256=(Hash $Audit.Path)
 gap_report_sha256=(Hash $OutputReport)
 references=95
 unique_missing_item_ids=$MissingIDs
 unique_matching_item_ids=$KnownIDs
 source_property_conflict_item_ids=[int]$Report.ids_with_conflicting_legacy_properties
 metadata_mismatch_matching_item_ids=[int]$Report.matching_ids_with_metadata_differences
 migration_authorized=$false
 allowed_binary_writes=@()
 allowed_package_deletions=@()
 allowed_savegame_writes=@()
 next='QUALIFY_UNMAPPED_ITEM_FAMILIES_AND_AUTHOR_EXACT_CANONICAL_DEFINITIONS_IN_FUTURE_TRANSACTION'
}
$GateResult | ConvertTo-Json -Depth 8 |
 Set-Content -LiteralPath (Join-Path $Output 'identity-gate-private.json') -Encoding UTF8
Stage 'CHECK_CLEAN'
Clean
Stage 'ISSUE_BUILD'
$Lines=@(
 '# ARM-CLEAN06.4I - distinct real UE ItemId migration gaps'
 ''
 ('Validated SHA : '+$ExpectedHead)
 ('Protected binary snapshot : '+$SnapshotSha)
 ('Reference inventory SHA : '+$PriorSha)
 ''
 '| Read-only property | Count |'
 '|---|---:|'
 '| Physical world/palette/receptacle references | 95 |'
 '| Existing canonical ID occurrences | 55 |'
 '| Unmapped canonical ID occurrences | 40 |'
 ('| **Distinct missing ItemIds** | **'+$MissingIDs+'** |')
 ('| Distinct already-registered ItemIds | '+$KnownIDs+' |')
 ('| Missing IDs with multiple old definitions | '+$Report.missing_ids_with_multiple_legacy_sources+' |')
 ('| Missing IDs with conflicting legacy UPROPERTY | '+$Report.ids_with_conflicting_legacy_properties+' |')
 ('| Existing IDs with differences in selected metadata | '+$Report.matching_ids_with_metadata_differences+' |')
 ('| Item level N0 occurrences | '+$Report.N0_occurrences+' |')
 ('| Item level fixed N1-N20 occurrences | '+$Report.fixed_level_occurrences+' |')
 ('| Missing-ID weapon references | '+$Report.missing_weapon_occurrences+' |')
 ('| Missing-ID nonweapon references | '+$Report.missing_nonweapon_occurrences+' |')
 '| Original UE binaries / backups SHA-256 reverified | 84 |'
 '| Personal SaveGames read | 0 |'
 '| Binary writes authorized | 0 |'
 ''
 'The complete UPROPERTY exports, item identifiers, asset paths and placement GUIDs remain PRIVATE.'
 'An existing ItemId or matching selected metadata is not gameplay/physics parity.'
 'The signed snapshot and current checkout match. No assets were changed.'
 'Canonical authoring remains blocked until full per-family policy and value checks.'
)
$Issue=Join-Path $Output 'github-issue.md'
Set-Content -LiteralPath $Issue -Value $Lines -Encoding UTF8
Stage 'ISSUE_WRITE'
$Title='[ARM-CLEAN06.4I] Distinct canonical ItemId gaps '+$ExpectedHead.Substring(0,12)
$Link=@(& gh issue create --repo $GitHubRepository --title $Title --body-file $Issue)
if($LASTEXITCODE -ne 0 -or $Link.Count -ne 1 -or
 [string]$Link[0] -notmatch '^https://github\.com/[^/]+/[^/]+/issues/[0-9]+$') {
 throw 'Aggregated GitHub issue publication failed.'
}
Stage 'SUCCESS'
Write-Host ('[ARM-CLEAN06.4I] DISTINCT_IDS_INVENTORIED '+$Link[0])
