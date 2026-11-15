<#
 ARM-CLEAN06.4G: immutable, private binary snapshot and sandbox-only
 restoration drill. Unreal content remains READ-ONLY; no SavePackage.
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
$StageFile=Join-Path $RunDir 'ARMClean064G.stage'
$BaselineSha='27563be7bee162e86b5f5f9ac29704c1b50aa35c'
$BaselineHash='c00c395cc19645e9f798df5c4a7c38d94aa9d1ecb1e8487ca27d07f864c2b1f9'
$ProvenanceSha='ec43ca762860533b0de43269f346a48dc25983ae'
function Stage([string]$Name) {Set-Content -LiteralPath $StageFile -Value $Name -Encoding ASCII}
function Clean {
 $State=@(& git -C $WorktreePath status --porcelain --untracked-files=all)
 if($LASTEXITCODE -ne 0 -or $State.Count -ne 0) {
  Stage 'WORKTREE_CHANGED'; throw 'Validation worktree is not clean.'
 }
}
function Same([object]$A,[object]$B) {
 return [string]::Equals([string]$A,[string]$B,[StringComparison]::Ordinal)
}
function Read-Private([string]$Label,[string]$File,[string]$Key,[string]$Sha) {
 $Root=Join-Path $StateRoot 'Runs'
 if(-not (Test-Path -LiteralPath $Root -PathType Container)) {throw 'Private proof archive absent.'}
 $Found=New-Object System.Collections.ArrayList
 foreach($D in @(Get-ChildItem -LiteralPath $Root -Directory -ErrorAction Stop)) {
  $Path=Join-Path (Join-Path $D.FullName $Label) $File
  if(-not (Test-Path -LiteralPath $Path -PathType Leaf)) {continue}
  $Data=Get-Content -LiteralPath $Path -Raw -Encoding UTF8 | ConvertFrom-Json
  $P=$Data.PSObject.Properties[$Key]
  if($null -ne $P -and [string]$P.Value -ceq $Sha) {
   [void]$Found.Add([pscustomobject]@{Path=$Path;Data=$Data})
  }
 }
 if($Found.Count -ne 1) {throw 'Expected exactly one original private evidence file.'}
 return $Found[0]
}
function Map([object]$Value) {
 $M=@{}
 foreach($P in @($Value.PSObject.Properties)) {$M.Add([string]$P.Name,[string]$P.Value)}
 return $M
}
function Identical-Fields([object]$Before,[object]$After) {
 $A=Map $Before;$B=Map $After
 if($A.Count -ne $B.Count) {return $false}
 foreach($Key in @($A.Keys)) {
  if(-not $B.ContainsKey($Key) -or -not (Same $A[$Key] $B[$Key])) {return $false}
 }
 return $true
}
function Edges([object[]]$InputValues) {
 $Ordered=@($InputValues | ForEach-Object {[string]$_} | Sort-Object -CaseSensitive)
 return [string]::Join([string][char]31,[string[]]$Ordered)
}
function File-Hash([string]$Path) {
 return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}
Stage 'PREFLIGHT'
foreach($Sha in @($ExpectedHead,$ProvenanceSha,$BaselineSha)) {
 if($Sha -cnotmatch '^[a-f0-9]{40}$') {throw 'Malformed pinned commit.'}
}
$Head=@(& git -C $WorktreePath rev-parse --verify HEAD)
if($LASTEXITCODE -ne 0 -or $Head.Count -ne 1 -or
 ([string]$Head[0]).Trim() -cne $ExpectedHead) {
 Stage 'HEAD_MISMATCH';throw 'Wrong checkout commit.'
}
$Detached=@(& git -C $WorktreePath symbolic-ref --quiet --short HEAD 2>$null)
if($LASTEXITCODE -eq 0 -and $Detached.Count -gt 0) {
 throw 'Only a detached validation worktree may be used.'
}
$WorktreeFull=[IO.Path]::GetFullPath($WorktreePath).TrimEnd('\','/')
$RunFull=[IO.Path]::GetFullPath($RunDir).TrimEnd('\','/')
$StateFull=[IO.Path]::GetFullPath($StateRoot).TrimEnd('\','/')
if(-not $RunFull.StartsWith($StateFull+[IO.Path]::DirectorySeparatorChar,
   [StringComparison]::OrdinalIgnoreCase) -or
   $RunFull.StartsWith($WorktreeFull+[IO.Path]::DirectorySeparatorChar,
   [StringComparison]::OrdinalIgnoreCase) -or
   $RunFull -ceq $WorktreeFull) {
 throw 'Refuse to write snapshot inside the project or outside private validation state.'
}
Clean
Stage 'PROVENANCE'
$P=Read-Private 'ARM-CLEAN06.4F' 'three-way-private.json' 'validation_sha' $ProvenanceSha
if($P.Data.schema -cne 'ARM-CLEAN06.4F/three-way/v1' -or
 $P.Data.source_parity_verified -ne $true -or
 [int]$P.Data.changed_packages_06_4A -ne 4 -or
 [int]$P.Data.preserved_post_audio_fields -ne 8 -or
 [int]$P.Data.later_property_drifts -ne 0 -or
 [int]$P.Data.later_dependency_drifts -ne 0 -or
 [int]$P.Data.later_profile_drifts -ne 0 -or
 [bool]$P.Data.migration_authorized -or
 @($P.Data.allowed_binary_writes).Count -ne 0 -or
 @($P.Data.allowed_package_deletions).Count -ne 0 -or
 @($P.Data.allowed_savegame_writes).Count -ne 0) {
 throw 'Source provenance not stable or prior conversion gate unlocked.'
}
$Base=Read-Private 'ARM-CLEAN06.4A' 'asset-hashes.json' 'git_sha' $BaselineSha
$Receipt=$Base.Data
$ManifestPath=Join-Path (Split-Path -Parent $Base.Path) 'asset-baseline.json'
if($Receipt.schema -cne 'ARM-CLEAN06.4A/asset-hashes/v1' -or
 $Receipt.manifest_sha256 -cne $BaselineHash -or
 (File-Hash $ManifestPath) -cne $BaselineHash -or
 @($Receipt.assets).Count -ne 56 -or
 $null -eq $Receipt.PSObject.Properties['canonical_item_system']) {
 throw 'Verified UE5.5.4 source capture is absent or inconsistent.'
}
$Manifest=Get-Content -LiteralPath $ManifestPath -Raw -Encoding UTF8 | ConvertFrom-Json
if($Manifest.schema -cne 'ARM-CLEAN06.0/read-only/v1' -or
 @($Manifest.assets).Count -ne 56 -or
 @($Manifest.expanded_profiles).Count -ne 1640 -or
 [int]$Manifest.counts.passive_profiles -ne 260) {
 throw 'Invalid historical UE asset inventory.'
}
Stage 'BUILD_ALLOWLIST'
Stage 'CHECK_TRACKED'
$Tracked=@{}
$GitFiles=@(& git -C $WorktreePath ls-files -- 'Content/GrimrockPrototype/Core/DataAssets')
if($LASTEXITCODE -ne 0) {throw 'Cannot enumerate tracked UE source files.'}
foreach($Line in $GitFiles) {$Tracked[[string]$Line]=$true}
Stage 'BUILD_LEGACY'
$SourceEntries=New-Object System.Collections.ArrayList
$Unique=@{}
foreach($Record in @($Receipt.assets)) {
 $Package=[string]$Record.package
 if($Package -cnotmatch '^/Game/GrimrockPrototype/Core/DataAssets/[A-Za-z0-9_/-]+$' -or
 $Package.Contains('..')) {throw 'Untrusted historical package identifier.'}
 $Rel='Content/'+$Package.Substring('/Game/'.Length)+'.uasset'
 if(-not $Tracked.ContainsKey($Rel) -or $Unique.ContainsKey($Rel) -or
 $Record.sha256 -cnotmatch '^[a-f0-9]{64}$') {
  throw 'Historical allowlist cannot be frozen.'
 }
 $Unique[$Rel]=$true
 [void]$SourceEntries.Add([pscustomobject]@{
   relative=$Rel
   category='legacy'
   expected_sha256=[string]$Record.sha256
   expected_bytes=[int64]$Record.bytes
 })
}
if($SourceEntries.Count -ne 56) {throw 'Historical package count not 56.'}
Stage 'CHECK_CANONICAL_DISCOVERY'
$CanonicalRoot='Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/'
$Canon=@($GitFiles | Where-Object {
 $_.StartsWith($CanonicalRoot,[StringComparison]::Ordinal) -and
 $_.EndsWith('.uasset',[StringComparison]::OrdinalIgnoreCase)
} | Sort-Object -CaseSensitive)
$AllowedRoot=@(
 ($CanonicalRoot+'DA_ItemSystem.uasset'),
 ($CanonicalRoot+'DA_ActionCatalogue.uasset'),
 ($CanonicalRoot+'DA_ActionPresentationCatalogue.uasset'),
 ($CanonicalRoot+'DA_CanonicalWeaponCatalogue.uasset'),
 ($CanonicalRoot+'DA_CanonicalWeaponVisualCatalogue.uasset')
)
Stage 'CHECK_CANONICAL_NAMES'
$Top=@($Canon | Where-Object { $_ -notmatch '/Definitions/' })
$Defs=@($Canon | Where-Object { $_ -match '^Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/DA_Canonical_[A-Za-z0-9_]+\.uasset$' })
if($Canon.Count -ne 28 -or $Top.Count -ne 5 -or $Defs.Count -ne 23) {
 throw 'Canonical content changed from expected 5 root/catalogues + 23 definitions.'
}
foreach($R in $AllowedRoot) {
 if($Top -cnotcontains $R) {throw 'Canonical root/catalogue missing.'}
}
foreach($Rel in $Canon) {
 if($Unique.ContainsKey($Rel) -or
   ($AllowedRoot -cnotcontains $Rel -and $Defs -cnotcontains $Rel)) {
  throw 'Canonical package outside frozen allowlist.'
 }
 $Unique[$Rel]=$true
 $ExpectedHash=$null
 $ExpectedBytes=0
 if($Rel -ceq ($CanonicalRoot+'DA_ItemSystem.uasset')) {
  $ExpectedHash=[string]$Receipt.canonical_item_system.sha256
  $ExpectedBytes=[int64]$Receipt.canonical_item_system.bytes
 }
 [void]$SourceEntries.Add([pscustomobject]@{
  relative=$Rel
  category='canonical'
  expected_sha256=$ExpectedHash
  expected_bytes=$ExpectedBytes
 })
}
Stage 'CHECK_FINAL_SCOPE'
if($SourceEntries.Count -ne 84 -or $Unique.Count -ne 84) {
 throw 'Binary allowlist must contain 84 unique sources.'
}
Stage 'CHECK_OUTPUT_DIR'
$Output=Join-Path $RunDir 'ARM-CLEAN06.4G'
if(Test-Path -LiteralPath $Output) {
 throw 'Refuse overwriting an earlier private binary snapshot.'
}
New-Item -ItemType Directory -Path $Output -ErrorAction Stop | Out-Null
Stage 'LIVE_UNREAL'
$Editor=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Project=Join-Path $WorktreePath 'GrimrockPrototype.uproject'
if(-not (Test-Path -LiteralPath $Editor -PathType Leaf) -or
 -not (Test-Path -LiteralPath $Project -PathType Leaf)) {
 throw 'UE5.5.4 commandlet or project missing.'
}
$LivePath=Join-Path $Output 'live-baseline.json'
& $Editor $Project '-run=GridArmClean06Baseline' '-unattended' '-NoSplash' '-NoP4' '-NullRHI' "-Output=$LivePath" | Out-Null
if($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $LivePath -PathType Leaf)) {
 Stage 'LIVE_UNREAL_FAILED';throw 'Real Unreal baseline export failed.'
}
Stage 'COMPARE_LIVE'
$Live=Get-Content -LiteralPath $LivePath -Raw -Encoding UTF8 | ConvertFrom-Json
if($Live.schema -cne $Manifest.schema -or
 @($Live.assets).Count -ne 56 -or
 @($Live.expanded_profiles).Count -ne 1640 -or
 [int]$Live.counts.passive_profiles -ne 260) {
 throw 'Current Unreal inventory cardinalities differ.'
}
$OldIndex=@{}
$LiveIndex=@{}
foreach($A in @($Manifest.assets)) {$OldIndex.Add([string]$A.package,$A)}
foreach($A in @($Live.assets)) {$LiveIndex.Add([string]$A.package,$A)}
if($OldIndex.Count -ne 56 -or $LiveIndex.Count -ne 56) {
 throw 'Duplicate or missing package after Unreal recapture.'
}
foreach($Name in @($OldIndex.Keys)) {
 if(-not $LiveIndex.ContainsKey($Name)) {throw 'UE package missing.'}
 $A=$OldIndex[$Name];$B=$LiveIndex[$Name]
 if($A.class -cne $B.class -or $A.kind -cne $B.kind -or
 $A.object -cne $B.object -or
 -not (Identical-Fields $A.properties $B.properties) -or
 (Edges @($A.dependencies)) -cne (Edges @($B.dependencies)) -or
 (Edges @($A.referencers)) -cne (Edges @($B.referencers))) {
  throw 'Historical UE asset fields or graph changed since 06.4A.'
 }
}
for($I=0;$I -lt 1640;$I++) {
 if(-not (Identical-Fields $Manifest.expanded_profiles[$I] $Live.expanded_profiles[$I])) {
  throw 'N1-N20 ARM profile value changed.'
 }
}
Stage 'VERIFY_SOURCE_HASHES'
$Verified=New-Object System.Collections.ArrayList
$TotalBytes=[int64]0
foreach($Entry in @($SourceEntries.ToArray())) {
 $Rel=[string]$Entry.relative
 if($Rel -cnotmatch '^Content/GrimrockPrototype/Core/DataAssets/[A-Za-z0-9_/-]+\.uasset$' -or
 $Rel.Contains('..')) {throw 'Unsafe content path.'}
 $Source=Join-Path $WorktreePath $Rel.Replace('/','\')
 if(-not (Test-Path -LiteralPath $Source -PathType Leaf)) {throw 'Binary missing.'}
 $File=Get-Item -LiteralPath $Source -ErrorAction Stop
 if(($File.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0 -or
 $File.Length -le 200) {throw 'Symlink or Git LFS pointer rejected.'}
 $Hash=File-Hash $Source
 if(($null -ne $Entry.expected_sha256 -and
  -not (Same $Hash $Entry.expected_sha256)) -or
  ($Entry.expected_bytes -gt 0 -and $Entry.expected_bytes -ne $File.Length)) {
  throw 'UE binary differs from verified 06.4A snapshot.'
 }
 $TotalBytes+=$File.Length
 [void]$Verified.Add([pscustomobject]@{
  relative=$Rel
  category=$Entry.category
  bytes=$File.Length
  sha256=$Hash
 })
}
Stage 'COPY_PRIVATE_SNAPSHOT'
$BackupRoot=Join-Path $Output 'SourceSnapshot'
New-Item -ItemType Directory -Path $BackupRoot -ErrorAction Stop | Out-Null
foreach($Entry in @($Verified.ToArray())) {
 $Rel=[string]$Entry.relative
 $Src=Join-Path $WorktreePath $Rel.Replace('/','\')
 $Dst=Join-Path $BackupRoot $Rel.Replace('/','\')
 $Dir=Split-Path -Parent $Dst
 if(-not (Test-Path -LiteralPath $Dir -PathType Container)) {
  New-Item -ItemType Directory -Force -Path $Dir | Out-Null
 }
 Copy-Item -LiteralPath $Src -Destination $Dst -ErrorAction Stop
 if((Get-Item -LiteralPath $Dst).Length -ne [int64]$Entry.bytes -or
  -not (Same (File-Hash $Dst) $Entry.sha256) -or
  -not (Same (File-Hash $Src) $Entry.sha256)) {
  throw 'Private backup copy differs from original binary.'
 }
}
Stage 'SANDBOX_RESTORE_DRILL'
$Drill=Join-Path $Output 'RestoreDrill'
New-Item -ItemType Directory -Path $Drill -ErrorAction Stop | Out-Null
foreach($Entry in @($Verified.ToArray())) {
 $Rel=[string]$Entry.relative
 $Backup=Join-Path $BackupRoot $Rel.Replace('/','\')
 $TestDst=Join-Path $Drill $Rel.Replace('/','\')
 $TestDir=Split-Path -Parent $TestDst
 if(-not (Test-Path -LiteralPath $TestDir -PathType Container)) {
  New-Item -ItemType Directory -Force -Path $TestDir | Out-Null
 }
 Copy-Item -LiteralPath $Backup -Destination $TestDst -ErrorAction Stop
 if((Get-Item -LiteralPath $TestDst).Length -ne [int64]$Entry.bytes -or
  -not (Same (File-Hash $TestDst) $Entry.sha256)) {
  throw 'Restored sandbox copy does not match preflight SHA-256.'
 }
}
# The drill directory is private and contained entirely in this run.
# Never delete or overwrite anything in the project / user SaveGames.
Remove-Item -LiteralPath $Drill -Recurse -Force -ErrorAction Stop
Stage 'WRITE_RECEIPTS'
$ReceiptOut=[ordered]@{
 schema='ARM-CLEAN06.4G/private-binary-snapshot/v1'
 validation_sha=$ExpectedHead
 baseline_sha=$BaselineSha
 proven_audio_sha=$ProvenanceSha
 current_unreal_manifest_sha256=(File-Hash $LivePath)
 binary_file_count=$Verified.Count
 legacy_file_count=56
 canonical_file_count=28
 total_bytes=$TotalBytes
 source_files=@($Verified.ToArray())
 copy_verified=$true
 sandbox_restore_drill_verified=$true
}
$ReceiptOut | ConvertTo-Json -Depth 8 |
 Set-Content -LiteralPath (Join-Path $Output 'binary-snapshot-receipt.json') -Encoding UTF8
$Contract=[ordered]@{
 schema='ARM-CLEAN06.4G/transaction-gate/v1'
 validation_sha=$ExpectedHead
 mode='PRECONVERSION_READ_ONLY'
 provenance='06.0+06.2A+06.4A+06.4E+06.4F'
 source_file_count=$Verified.Count
 snapshot_receipt_sha256=(File-Hash (Join-Path $Output 'binary-snapshot-receipt.json'))
 backup_is_private=$true
 project_restoration_executed=$false
 permitted_restore_path='PRIVATE_SANDBOX_ONLY'
 savegames_in_snapshot=$false
 restoration_requires_separate_review=$true
 migration_authorized=$false
 allowed_binary_writes=@()
 allowed_package_deletions=@()
 allowed_savegame_writes=@()
 outstanding_requirements=@(
  'Exact per-family source-to-canonical field mapping',
  'Level, receptacle, palette and starter-kit reference parity',
  'Physical projectile and throw policy parity',
  'SaveGame schema and backup plan for real saves',
  'Unreal asset-authoring transaction with explicit allowlist',
  'Pre/post AssetRegistry reference scan and rollback after failed validation',
  'PIE/Chaos and Cook/Shipping acceptance'
 )
}
$Contract | ConvertTo-Json -Depth 8 |
 Set-Content -LiteralPath (Join-Path $Output 'transaction-gate-private.json') -Encoding UTF8
Stage 'FINAL_CLEAN'
Clean
Stage 'ISSUE_BUILD'
$Lines=@(
 '# ARM-CLEAN06.4G - private binary backup and rollback simulation'
 ''
 ('Commit validation : '+$ExpectedHead)
 ('Source provenance : '+$ProvenanceSha)
 ('Verified UE reference state : '+$BaselineSha)
 ''
 '| Audit/protection | Result |'
 '|---|---:|'
 '| Legacy UE assets opened by the real commandlet | 56 |'
 '| ARM N1-N20 profiles rechecked | 1640 |'
 '| Old .uasset copied to private snapshot | 56 |'
 '| CanonicalStage .uasset copied to private snapshot | 28 |'
 '| Total files verified by SHA-256 | 84 |'
 '| Restores successfully simulated in private sandbox | 84 |'
 '| Project or SaveGame files modified | 0 |'
 '| Project restoration executed | 0 |'
 '| Binary writes authorized | 0 |'
 ''
 'Every source hash was checked before and after backup.'
 'The full backup, hashes, paths and restore contract stay PRIVATE on Windows.'
 'The restore drill writes only within private Runs, NEVER in the project.'
 'No inference of cutover readiness: migration remains blocked.'
)
$IssueBody=Join-Path $Output 'github-issue.md'
Set-Content -LiteralPath $IssueBody -Value $Lines -Encoding UTF8
Stage 'ISSUE_WRITE'
$Title='[ARM-CLEAN06.4G] Private binary safety snapshot '+$ExpectedHead.Substring(0,12)
$Url=@(& gh issue create --repo $GitHubRepository --title $Title --body-file $IssueBody)
if($LASTEXITCODE -ne 0 -or $Url.Count -ne 1 -or
 [string]$Url[0] -notmatch '^https://github\.com/[^/]+/[^/]+/issues/[0-9]+$') {
 throw 'Sanitized snapshot issue could not be published.'
}
Stage 'SUCCESS'
Write-Host ('[ARM-CLEAN06.4G] PRIVATE_SNAPSHOT_OK '+$Url[0])
