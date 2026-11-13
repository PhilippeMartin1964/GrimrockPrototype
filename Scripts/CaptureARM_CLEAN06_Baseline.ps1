<#
.SYNOPSIS
  ARM-CLEAN06.0: read-only baseline of real Unreal DataAssets.
.DESCRIPTION
  Runs in the isolated Windows validation worktree, saves private JSON and
  SHA-256 receipts, publishes a sanitized GitHub issue and never commits.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$WorktreePath,
    [Parameter(Mandatory = $true)][string]$EngineRoot,
    [Parameter(Mandatory = $true)][string]$ExpectedHead,
    [Parameter(Mandatory = $true)][string]$RunDir,
    [string]$DevelopmentRoot = '',
    [string]$GitHubRepository = 'PhilippeMartin1964/GrimrockPrototype'
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$StagePath = Join-Path $RunDir 'ARMClean060.stage'
function Set-Stage([string]$Name) {
    Set-Content -LiteralPath $StagePath -Value $Name -Encoding ASCII
}
Set-Stage 'PREFLIGHT'
if ($ExpectedHead -notmatch '^[a-f0-9]{40}$') { throw 'Invalid baseline commit SHA.' }
Set-Stage 'CHECK_HEAD'
$GitHeadLines = @(& git -C $WorktreePath rev-parse --verify HEAD)
$GitHeadExit = $LASTEXITCODE
if ($GitHeadExit -ne 0 -or $GitHeadLines.Count -ne 1) {
    Set-Stage 'HEAD_QUERY_FAILED'
    throw 'Unable to query HEAD from detached worktree.'
}
$Head = ([string]$GitHeadLines[0]).Trim()
if ($Head -cne $ExpectedHead) {
    Set-Stage 'HEAD_MISMATCH'
    throw 'Worktree HEAD differs from expected validated SHA.'
}
Set-Stage 'CHECK_WORKTREE'
$Before = @(& git -C $WorktreePath status --porcelain --untracked-files=all)
if ($LASTEXITCODE -ne 0 -or $Before.Count -gt 0) { throw 'Validation worktree not clean.' }
Set-Stage 'CHECK_ENGINE'
$Project = Join-Path $WorktreePath 'GrimrockPrototype.uproject'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if (-not (Test-Path -LiteralPath $Editor -PathType Leaf) -or
    -not (Test-Path -LiteralPath $Project -PathType Leaf)) { throw 'Unreal Editor or project missing.' }
$OutputDir = Join-Path $RunDir 'ARM-CLEAN06.0'
New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
$ManifestPath = Join-Path $OutputDir 'asset-baseline.json'
Set-Stage 'COMMANDLET'
& $Editor $Project '-run=GridArmClean06Baseline' '-unattended' '-NoSplash' '-NoP4' '-NullRHI' "-Output=$ManifestPath" | Out-Null
if ($LASTEXITCODE -ne 0) { Set-Stage 'COMMANDLET_FAILED'; throw 'Read-only UE asset export failed.' }
if (-not (Test-Path -LiteralPath $ManifestPath -PathType Leaf)) { throw 'Asset manifest not produced.' }
Set-Stage 'VERIFY'
Set-Stage 'READ_JSON'
$Manifest = Get-Content -LiteralPath $ManifestPath -Raw -Encoding UTF8 | ConvertFrom-Json
Set-Stage 'CHECK_COUNTS'
$Counts = $Manifest.counts
if ($Manifest.schema -cne 'ARM-CLEAN06.0/read-only/v1' -or
    [int]$Counts.weapon -ne 24 -or [int]$Counts.item -ne 24 -or
    [int]$Counts.level -ne 4 -or [int]$Counts.dungeon -ne 1 -or
    [int]$Counts.palette -ne 1 -or [int]$Counts.combat_catalogue -ne 1 -or
    [int]$Counts.visual_catalogue -ne 1 -or
    [int]$Counts.profiles -ne 1640 -or [int]$Counts.passive_profiles -ne 260 -or
    @($Manifest.assets).Count -ne 56 -or @($Manifest.expanded_profiles).Count -ne 1640) {
    throw 'Baseline cardinality or profile invariants mismatch.'
}
Set-Stage 'BINARY_HASH'
$Hashes = New-Object System.Collections.ArrayList
foreach ($Asset in @($Manifest.assets)) {
    $Package = [string]$Asset.package
    if (-not $Package.StartsWith('/Game/GrimrockPrototype/Core/DataAssets/')) {
        throw 'Package outside authorized inventory.'
    }
    $Relative = $Package.Substring('/Game/'.Length).Replace('/', '\') + '.uasset'
    $File = Join-Path (Join-Path $WorktreePath 'Content') $Relative
    if (-not (Test-Path -LiteralPath $File -PathType Leaf)) { throw 'Missing asset binary.' }
    $Size = (Get-Item -LiteralPath $File).Length
    if ($Size -le 200) { throw 'Git LFS pointer instead of real Unreal binary.' }
    [void]$Hashes.Add([pscustomobject]@{
        package = $Package
        bytes = $Size
        sha256 = (Get-FileHash -LiteralPath $File -Algorithm SHA256).Hash.ToLowerInvariant()
        dependencies = @($Asset.dependencies).Count
        referencers = @($Asset.referencers).Count
    })
}
Set-Stage 'SAVE_HASH'
$SaveHashes = New-Object System.Collections.ArrayList
if (-not [string]::IsNullOrWhiteSpace($DevelopmentRoot)) {
    $SaveDir = Join-Path $DevelopmentRoot 'Saved\SaveGames'
    if (Test-Path -LiteralPath $SaveDir -PathType Container) {
        foreach ($Save in @(Get-ChildItem -LiteralPath $SaveDir -File -Recurse -Filter '*.sav' -ErrorAction Stop)) {
            [void]$SaveHashes.Add([pscustomobject]@{
                name = $Save.Name
                bytes = $Save.Length
                sha256 = (Get-FileHash -LiteralPath $Save.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
            })
        }
    }
}
Set-Stage 'WRITE_RECEIPT'
$Receipt = [ordered]@{
    schema = 'ARM-CLEAN06.0/asset-hashes/v1'
    git_sha = $ExpectedHead
    manifest_sha256 = (Get-FileHash -LiteralPath $ManifestPath -Algorithm SHA256).Hash.ToLowerInvariant()
    captured_utc = [DateTime]::UtcNow.ToString('o')
    assets = @($Hashes.ToArray())
    savegames = @($SaveHashes.ToArray())
}
$ReceiptPath = Join-Path $OutputDir 'asset-hashes.json'
$Receipt | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $ReceiptPath -Encoding UTF8
Set-Stage 'CHECK_CLEAN'
$After = @(& git -C $WorktreePath status --porcelain --untracked-files=all)
if ($LASTEXITCODE -ne 0 -or $After.Count -gt 0) {
    Set-Stage 'WORKTREE_CHANGED'
    throw 'Audit changed a tracked or untracked project file.'
}

# Public issue: sanitized summary only, no serialized texts or SaveGame details.
Set-Stage 'ISSUE_BUILD'
# Counts were already checked against the actual UE manifest before this phase.
# Use a deliberately small fixed-format public receipt. Full content stays private.
$ManifestHash = [string]$Receipt['manifest_sha256']
$BodyLines = @(
    '# ARM-CLEAN06.0 - baseline UE 5.5.4 en lecture seule',
    '',
    ('Commit source : ' + $ExpectedHead),
    '',
    '| Mesure | Resultat |',
    '|---|---:|',
    '| Weapon definitions | 24 |',
    '| Other item definitions | 24 |',
    '| Weapon catalogues | 2 |',
    '| Level assets | 4 |',
    '| Dungeon assets | 1 |',
    '| Object palettes | 1 |',
    '| Weapon profiles | 1640 |',
    '| Passive profiles | 260 |',
    '',
    ('SHA-256 du manifeste UE : ' + $ManifestHash),
    '',
    'Les UPROPERTY, les graphes de references, les 1640 profils',
    'et les 56 SHA-256 binaires sont conserves dans le dossier',
    'prive du validateur Windows (Runs/ARM-CLEAN06.0).',
    'Les sauvegardes personnelles ne sont jamais transmises.',
    'Aucun fichier de projet ou asset Unreal modifie.'
)
Set-Stage 'ISSUE_WRITE'
$IssueBody = Join-Path $OutputDir 'github-issue.md'
Set-Content -LiteralPath $IssueBody -Value $BodyLines -Encoding UTF8
Set-Stage 'PUBLICATION'
$Title = '[ARM-CLEAN06.0] Baseline read-only ' + $ExpectedHead.Substring(0,12)
$IssueUrl = @(& gh issue create --repo $GitHubRepository --title $Title --body-file $IssueBody)
if ($LASTEXITCODE -ne 0 -or @($IssueUrl).Count -ne 1 -or
    [string]$IssueUrl[0] -notmatch '^https://github\.com/[^/]+/[^/]+/issues/[0-9]+$') {
    throw 'Unable to publish GitHub baseline issue.'
}
Set-Stage 'SUCCESS'
Write-Host ('[ARM-CLEAN06.0] BASELINE_OK: ' + $IssueUrl[0])
Write-Host ('[ARM-CLEAN06.0] Full private report: ' + $OutputDir)
