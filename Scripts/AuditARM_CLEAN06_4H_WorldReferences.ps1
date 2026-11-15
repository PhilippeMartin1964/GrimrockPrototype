<#
 ARM-CLEAN06.4H — read actual UE levels, palette and receptacle definitions.
 All IDs and reference paths stay private. No binary writes and no SaveGame access.
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
$SnapshotSha='e0f877d17f39d913c585b55ba18110745c5385b6'
$StagePath=Join-Path $RunDir 'ARMClean064H.stage'
function Stage([string]$Name) {Set-Content -LiteralPath $StagePath -Value $Name -Encoding ASCII}
function Clean {
 $Result=@(& git -C $WorktreePath status --porcelain --untracked-files=all)
 if($LASTEXITCODE -ne 0 -or $Result.Count -ne 0) {
  Stage 'WORKTREE_CHANGED';throw 'Validation checkout modified or contains untracked files.'
 }
}
function Hash([string]$File) {
 return (Get-FileHash -LiteralPath $File -Algorithm SHA256).Hash.ToLowerInvariant()
}
Stage 'PREFLIGHT'
foreach($Sha in @($ExpectedHead,$SnapshotSha)) {
 if($Sha -cnotmatch '^[a-f0-9]{40}$') {throw 'Malformed commit SHA.'}
}
$Head=@(& git -C $WorktreePath rev-parse --verify HEAD)
if($LASTEXITCODE -ne 0 -or $Head.Count -ne 1 -or
 ([string]$Head[0]).Trim() -cne $ExpectedHead) {
 Stage 'HEAD_MISMATCH';throw 'Unexpected detached worktree HEAD.'
}
$Branch=@(& git -C $WorktreePath symbolic-ref --quiet --short HEAD 2>$null)
if($LASTEXITCODE -eq 0 -and $Branch.Count -gt 0) {
 throw 'Validation checkout must have detached HEAD.'
}
$RunAbsolute=[IO.Path]::GetFullPath($RunDir).TrimEnd('\','/')
$WorktreeAbsolute=[IO.Path]::GetFullPath($WorktreePath).TrimEnd('\','/')
$StateAbsolute=[IO.Path]::GetFullPath($StateRoot).TrimEnd('\','/')
if(-not $RunAbsolute.StartsWith(($StateAbsolute+'\'),[StringComparison]::OrdinalIgnoreCase) -or
 $RunAbsolute.StartsWith(($WorktreeAbsolute+'\'),[StringComparison]::OrdinalIgnoreCase)) {
 throw 'Private validation destination is outside state directory or within project.'
}
Clean
Stage 'READ_SNAPSHOT'
$History=Join-Path $StateRoot 'Runs'
if(-not (Test-Path -LiteralPath $History -PathType Container)) {
 throw 'Private 06.4G snapshot history absent.'
}
$Found=New-Object System.Collections.ArrayList
foreach($Run in @(Get-ChildItem -LiteralPath $History -Directory -ErrorAction Stop)) {
 $ReceiptPath=Join-Path (Join-Path $Run.FullName 'ARM-CLEAN06.4G') 'binary-snapshot-receipt.json'
 $GatePath=Join-Path (Join-Path $Run.FullName 'ARM-CLEAN06.4G') 'transaction-gate-private.json'
 if(-not (Test-Path -LiteralPath $ReceiptPath -PathType Leaf) -or
    -not (Test-Path -LiteralPath $GatePath -PathType Leaf)) {continue}
 $Receipt=Get-Content -LiteralPath $ReceiptPath -Raw -Encoding UTF8 | ConvertFrom-Json
 if($Receipt.validation_sha -cne $SnapshotSha) {continue}
 [void]$Found.Add([pscustomobject]@{ReceiptPath=$ReceiptPath; GatePath=$GatePath; Receipt=$Receipt})
}
if($Found.Count -ne 1) {
 throw 'Expected a single verified private backup from 06.4G.'
}
$Record=$Found[0]
$Gate=Get-Content -LiteralPath $Record.GatePath -Raw -Encoding UTF8 | ConvertFrom-Json
if($Record.Receipt.schema -cne 'ARM-CLEAN06.4G/private-binary-snapshot/v1' -or
 $Record.Receipt.binary_file_count -ne 84 -or
 $Record.Receipt.legacy_file_count -ne 56 -or
 $Record.Receipt.canonical_file_count -ne 28 -or
 $Record.Receipt.copy_verified -ne $true -or
 $Record.Receipt.sandbox_restore_drill_verified -ne $true -or
 $Gate.schema -cne 'ARM-CLEAN06.4G/transaction-gate/v1' -or
 $Gate.validation_sha -cne $SnapshotSha -or
 $Gate.snapshot_receipt_sha256 -cne (Hash $Record.ReceiptPath) -or
 $Gate.migration_authorized -ne $false -or
 @($Gate.allowed_binary_writes).Count -ne 0 -or
 @($Gate.allowed_package_deletions).Count -ne 0 -or
 @($Gate.allowed_savegame_writes).Count -ne 0) {
 throw '06.4G backup evidence is not valid or gate is open.'
}
Stage 'REHASH_SNAPSHOT'
$BackupDir=Join-Path (Split-Path -Parent $Record.ReceiptPath) 'SourceSnapshot'
$Seen=@{}
foreach($Asset in @($Record.Receipt.source_files)) {
 $Rel=[string]$Asset.relative
 if($Rel -cnotmatch '^Content/GrimrockPrototype/Core/DataAssets/[A-Za-z0-9_/-]+\.uasset$' -or
 $Rel.Contains('..') -or $Seen.ContainsKey($Rel) -or
 [string]$Asset.sha256 -cnotmatch '^[a-f0-9]{64}$') {
  throw 'Private backup allowlist malformed.'
 }
 $Seen[$Rel]=$true
 $Backup=Join-Path $BackupDir $Rel.Replace('/','\')
 $Source=Join-Path $WorktreePath $Rel.Replace('/','\')
 if(-not (Test-Path -LiteralPath $Backup -PathType Leaf) -or
    -not (Test-Path -LiteralPath $Source -PathType Leaf) -or
    (Get-Item -LiteralPath $Backup).Length -ne [int64]$Asset.bytes -or
    (Get-Item -LiteralPath $Source).Length -ne [int64]$Asset.bytes -or
    (Hash $Backup) -cne [string]$Asset.sha256 -or
    (Hash $Source) -cne [string]$Asset.sha256) {
  throw 'Source asset or its private backup has drifted.'
 }
}
if($Seen.Count -ne 84) {throw 'Closed inventory is not 84 unique assets.'}
Stage 'UNREAL_AUDIT'
$Editor=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Project=Join-Path $WorktreePath 'GrimrockPrototype.uproject'
if(-not (Test-Path -LiteralPath $Editor -PathType Leaf) -or
 -not (Test-Path -LiteralPath $Project -PathType Leaf)) {
 throw 'Unreal 5.5.4 editor or project missing.'
}
$Output=Join-Path $RunDir 'ARM-CLEAN06.4H'
if(Test-Path -LiteralPath $Output) {throw 'Refuse to overwrite previous private report.'}
New-Item -ItemType Directory -Path $Output -ErrorAction Stop | Out-Null
$ReportPath=Join-Path $Output 'world-reference-private.json'
& $Editor $Project '-run=GridArmClean064HReferenceAudit' '-unattended' '-NoSplash' '-NoP4' '-NullRHI' "-Input=$($Record.ReceiptPath)" "-Output=$ReportPath" | Out-Null
if($LASTEXITCODE -ne 0 -or
   -not (Test-Path -LiteralPath $ReportPath -PathType Leaf)) {
 Stage 'UNREAL_AUDIT_FAILED'
 throw 'Real UE reference audit commandlet failed.'
}
Stage 'VERIFY_RESULT'
$Report=Get-Content -LiteralPath $ReportPath -Raw -Encoding UTF8 | ConvertFrom-Json
if($Report.schema -cne 'ARM-CLEAN06.4H/real-references/v1' -or
 [int]$Report.levels_scanned -ne 4 -or
 [int]$Report.canonical_definitions -ne 23 -or
 [int]$Report.savegame_legacy_pointer_fields -ne 2 -or
 $Report.savegame_files_opened -ne $false -or
 $Report.migration_authorized -ne $false) {
 throw 'Actual UE reference report lacks required source invariants.'
}
$Categories=@('LEVEL_LOOSE','LEVEL_SPAWN','LEVEL_RECEPTACLE',
 'PALETTE_DIRECT','DEFINITION_RECEPTACLE')
$Totals=[ordered]@{}
foreach($Category in $Categories) {
 $Obj=$Report.reference_counts.PSObject.Properties[$Category]
 $D=[ordered]@{
  total=0; id_candidates=0; unmapped=0; empty_ids=0
  missing_definition=0; invalid_level_or_quantity=0; archetype_mismatch=0
 }
 if($null -ne $Obj) {
  foreach($Name in @($D.Keys)) {$D[$Name]=[int]$Obj.Value.$Name}
 }
 $Parts=0
 foreach($Name in @('id_candidates','unmapped','empty_ids','missing_definition',
  'invalid_level_or_quantity','archetype_mismatch')) {
  if($D[$Name] -lt 0) {throw 'Negative reference count.'}
  $Parts+=$D[$Name]
 }
 if($D['total'] -ne $Parts) {throw 'Reference classification is incomplete.'}
 $Totals[$Category]=$D
}
$TotalRefs=0
foreach($Cat in $Categories) {$TotalRefs+=$Totals[$Cat]['total']}
if(@($Report.reference_rows).Count -ne $TotalRefs) {
 throw 'Private reference rows do not match aggregate counts.'
}
Stage 'WRITE_GATE'
$GateOut=[ordered]@{
 schema='ARM-CLEAN06.4H/world-identity-gate/v1'
 validation_sha=$ExpectedHead
 source_backup_sha=$SnapshotSha
 report_sha256=(Hash $ReportPath)
 audited_levels=4
 scanned_world_definitions=[int]$Report.world_definitions_scanned
 candidate_references=$TotalRefs
 reference_counts=$Totals
 duplicate_or_invalid_guids=[int]$Report.invalid_or_duplicate_placement_guids
 savegame_schema_legacy_pointers=2
 real_saves_opened=0
 migration_authorized=$false
 allowed_binary_writes=@()
 allowed_package_deletions=@()
 allowed_savegame_writes=@()
 required_next='COVER_WORLD_DEFINITIONS_OUTSIDE_ROOT_AND_PROVE_ITEM_IDS_LEVELS_AND_PHYSICS'
 }
$GateOut | ConvertTo-Json -Depth 9 |
 Set-Content -LiteralPath (Join-Path $Output 'world-identity-gate-private.json') -Encoding UTF8
Stage 'CHECK_CLEAN'
Clean
Stage 'ISSUE_BUILD'
$Lines=@(
 '# ARM-CLEAN06.4H - UE World and SaveGame legacy identity scan'
 ''
 ('Validated SHA : '+$ExpectedHead)
 ('Verified private backup SHA : '+$SnapshotSha)
 ''
 '| Read-only source | Count |'
 '|---|---:|'
 '| Real grid levels loaded | 4 |'
 ('| Palette entries inspected | '+$Report.palette_entries+' |')
 ('| World-object definitions inspected | '+$Report.world_definitions_scanned+' |')
 ('| Receptacle default items inspected | '+$Report.definition_receptacle_entries+' |')
 '| Canonical registry definitions | 23 |'
 ('| Invalid or duplicated placement GUIDs observed | '+$Report.invalid_or_duplicate_placement_guids+' |')
 '| Legacy SaveGame schema object-pointer fields | 2 |'
 '| Private SaveGames opened | 0 |'
 ''
 '| UE item reference family | Total | Candidate IDs | Unmapped IDs | Missing/invalid/ambiguous |'
 '|---|---:|---:|---:|---:|'
)
foreach($Cat in $Categories) {
 $C=$Totals[$Cat]
 $Problems=$C['total']-$C['id_candidates']-$C['unmapped']
 $Lines+=('| '+$Cat+' | '+$C['total']+' | '+$C['id_candidates']+
  ' | '+$C['unmapped']+' | '+$Problems+' |')
}
$Lines+=@(
 ''
 'Candidate ID means identity only, not value/physics/gameplay equivalence.'
 'All individual ItemIds, object paths and placement GUIDs remain PRIVATE on Windows.'
 'World-object definition scan is limited to the configured DataAssets hierarchy.'
 'SaveGame results describe reflected C++ fields, not contents of real .sav files.'
 'The 06.4G 84 binaries and private backups were rehashed without changes.'
 'Migration is BLOCKED and zero binary or SaveGame writes are authorized.'
)
$Body=Join-Path $Output 'github-issue.md'
Set-Content -LiteralPath $Body -Value $Lines -Encoding UTF8
Stage 'ISSUE_WRITE'
$Title='[ARM-CLEAN06.4H] Real world item-reference inventory '+$ExpectedHead.Substring(0,12)
$Link=@(& gh issue create --repo $GitHubRepository --title $Title --body-file $Body)
if($LASTEXITCODE -ne 0 -or $Link.Count -ne 1 -or
 [string]$Link[0] -notmatch '^https://github\.com/[^/]+/[^/]+/issues/[0-9]+$') {
 throw 'Could not publish aggregate-only reference inventory.'
}
Stage 'SUCCESS'
Write-Host ('[ARM-CLEAN06.4H] INVENTORY_DONE '+$Link[0])
