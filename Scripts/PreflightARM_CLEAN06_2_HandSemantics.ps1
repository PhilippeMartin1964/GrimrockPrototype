# ARM-CLEAN06.2E — read-only native semantic inventory in a detached worktree.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$WorktreePath,
    [Parameter(Mandatory=$true)][string]$EngineRoot,
    [Parameter(Mandatory=$true)][string]$ExpectedHead,
    [Parameter(Mandatory=$true)][string]$RunDir,
    [string]$GitHubRepository='PhilippeMartin1964/GrimrockPrototype'
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$Subject='fix(ARM-CLEAN06.2E.1): preserve conflicting IDs and diagnose native census'
$Stage=Join-Path $RunDir 'ARMClean062E.stage'
$OutDir=Join-Path $RunDir 'ARM-CLEAN06.2E'
New-Item -ItemType Directory -Path $OutDir -Force | Out-Null
function Set-Stage([string]$Value) {
    Set-Content -LiteralPath $Stage -Value $Value -Encoding ASCII
}
function Git-One([string[]]$GitArgs) {
    $Lines=@(& git -C $WorktreePath @GitArgs)
    if ($LASTEXITCODE -ne 0 -or $Lines.Count -ne 1) { throw 'Git lookup failed.' }
    return ([string]$Lines[0]).Trim()
}
Set-Stage 'PREFLIGHT'
if ($ExpectedHead -notmatch '^[a-f0-9]{40}$' -or
    (Git-One -GitArgs @('rev-parse','HEAD')) -cne $ExpectedHead -or
    (Git-One -GitArgs @('log','-1','--format=%s')) -cne $Subject) {
    throw 'SHA / exact semantic audit trigger mismatch.'
}
if ((Git-One -GitArgs @('remote','get-url','origin')) -notmatch
    '(?i)github\.com[:/]PhilippeMartin1964/GrimrockPrototype(\.git)?$') {
    throw 'Unexpected origin.'
}
$Branch=@(& git -C $WorktreePath branch --show-current)
if ($LASTEXITCODE -ne 0 -or
    @($Branch | Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) }).Count -gt 0) {
    throw 'Detached validation worktree is mandatory.'
}
$Dirty=@(& git -C $WorktreePath status --porcelain --untracked-files=all)
if ($LASTEXITCODE -ne 0 -or $Dirty.Count -ne 0) { throw 'Read-only worktree not clean.' }
$Editor=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Project=Join-Path $WorktreePath 'GrimrockPrototype.uproject'
if (-not (Test-Path -LiteralPath $Editor -PathType Leaf) -or
    -not (Test-Path -LiteralPath $Project -PathType Leaf)) {
    throw 'Unreal Editor or project not found.'
}
foreach ($Value in @($Editor,$Project,$OutDir)) {
    if ($Value.Contains('"')) { throw 'Unsupported quoted path.' }
}
Set-Stage 'COMMANDLET'
$ResultPath=Join-Path $OutDir 'hand-semantics-private.json'
$Args='"'+$Project+'" -run=GridArmClean06HandSemantics -unattended -NoSplash -NoP4 -NullRHI "-Output='+$ResultPath+'"'
$Stdout=Join-Path $OutDir 'Unreal.stdout.log'
$Stderr=Join-Path $OutDir 'Unreal.stderr.log'
$Proc=Start-Process -FilePath $Editor -ArgumentList $Args -WorkingDirectory $WorktreePath -WindowStyle Hidden -PassThru -Wait -RedirectStandardOutput $Stdout -RedirectStandardError $Stderr -ErrorAction Stop
if ($Proc.ExitCode -ne 0) {
    # Strict whitelist of diagnostics inferred from the PRIVATE native logs.
    # No source lines, user paths, asset IDs or stack traces are ever exported.
    $Category='NATIVE_PROCESS'
    foreach ($LogPath in @($Stdout,$Stderr)) {
        if (-not (Test-Path -LiteralPath $LogPath -PathType Leaf)) { continue }
        foreach ($Line in @(Get-Content -LiteralPath $LogPath -Tail 3000 -ErrorAction SilentlyContinue)) {
            if ($Line -match '\[ARM-CLEAN06\.2E\] Explicit -Output=') { $Category='CMD_ARGUMENT'; break }
            if ($Line -match '\[ARM-CLEAN06\.2E\] Source catalogue invalid') { $Category='SOURCE_CATALOGUE'; break }
            if ($Line -match '\[ARM-CLEAN06\.2E\] Invalid source ARM identity') { $Category='SOURCE_ARCHETYPE'; break }
            if ($Line -match '\[ARM-CLEAN06\.2E\] Missing/duplicate real ItemDefinitionId') { $Category='ITEM_IDS'; break }
            if ($Line -match '\[ARM-CLEAN06\.2E\] Concrete source count changed') { $Category='ASSET_COUNT'; break }
            if ($Line -match '\[ARM-CLEAN06\.2E\] Private JSON output failed') { $Category='OUTPUT_JSON'; break }
        }
        if ($Category -ne 'NATIVE_PROCESS') { break }
    }
    if ($Category -eq 'NATIVE_PROCESS' -and $Proc.ExitCode -eq 255) {
        $Category='UNREAL_EXIT_255'
    }
    Set-Stage $Category
    throw 'Native UE semantic audit failed; sanitized stage recorded.'
}
if (-not (Test-Path -LiteralPath $ResultPath -PathType Leaf)) {
    Set-Stage 'REPORT_ABSENT'
    throw 'Private UE semantic audit absent.'
}
Set-Stage 'REPORT'
$Report=Get-Content -LiteralPath $ResultPath -Raw -Encoding UTF8 | ConvertFrom-Json
if ($Report.schema -cne 'ARM-CLEAN06.2E/semantic-census/read-only/v1' -or
    [int]$Report.archetypes -ne 82 -or
    [int]$Report.concrete_weapon_assets -ne 24 -or
    [int]$Report.unique_nonempty_item_ids -lt 0 -or
    [int]$Report.missing_item_ids -lt 0 -or
    [int]$Report.duplicate_item_ids -lt 0 -or
    ([int]$Report.unique_nonempty_item_ids + [int]$Report.missing_item_ids + [int]$Report.duplicate_item_ids) -ne 24 -or
    [int]$Report.source_hand_label_variants -ne 3 -or
    @($Report.source_archetype_rows).Count -ne 82 -or
    @($Report.concrete_weapon_rows).Count -ne 24 -or
    [int]$Report.concrete_joined -lt 0 -or
    [int]$Report.concrete_joined -gt 24 -or
    [int]$Report.concrete_hand_policy_proven -gt [int]$Report.concrete_joined -or
    [int]$Report.concrete_armament_kind_proven -gt [int]$Report.concrete_joined -or
    [bool]$Report.binary_migration_authorized -or
    [bool]$Report.any_uasset_saved) {
    throw 'Semantic census schema or safety invariants invalid.'
}
Set-Stage 'CHECK_CLEAN'
$After=@(& git -C $WorktreePath status --porcelain --untracked-files=all)
if ($LASTEXITCODE -ne 0 -or $After.Count -ne 0) {
    throw 'Native semantic audit modified tracked or untracked project files.'
}
Set-Stage 'ISSUE'
$Markdown=@(
    '# ARM-CLEAN06.2E — audit natif des mains et familles',
    '',
    ('SHA verifie : '+$ExpectedHead),
    '',
    '| Controle | Resultat |',
    '|---|---:|',
    '| Archetypes source | 82 |',
    '| Defintions concretes chargees par UE | 24 |',
    ('| ItemId uniques et presents | '+[int]$Report.unique_nonempty_item_ids+' |'),
    ('| ItemId absents | '+[int]$Report.missing_item_ids+' |'),
    ('| ItemId dupliques | '+[int]$Report.duplicate_item_ids+' |'),
    ('| Jointures catalogue valides | '+[int]$Report.concrete_joined+' |'),
    ('| Regles de mains prouvables | '+[int]$Report.concrete_hand_policy_proven+' |'),
    ('| Types offensif/bouclier prouvables | '+[int]$Report.concrete_armament_kind_proven+' |'),
    ('| Conflits semantiques releves | '+[int]$Report.semantic_conflicts+' |'),
    ('| Types de familles source | '+[int]$Report.source_family_variants+' |'),
    '| Libelles HandUsage source differents | 3 |',
    ('| Archetypes sans definition concrete | '+[int]$Report.source_archetypes_without_any_concrete_definition+' |'),
    '| .uasset modifies | 0 |',
    '',
    '**Inventaire seulement : conversion binaire NON autorisee.**',
    'Les ItemId absents et dupliques sont des BLOQUEURS de conversion, pas une autorisation de fusion.',
    'La conversion des 82 archetypes ne peut etre deduite des seules 24 definitions.',
    'Les libelles sources exacts, identites, slots et chemins restent dans le JSON prive.',
    'Les valeurs HandUsage du classeur sont informatives dans le catalogue historique.',
    'Aucune modification de Sword/RatGiant, sauvegarde ou lecteur runtime.'
)
$BodyPath=Join-Path $OutDir 'issue.md'
Set-Content -LiteralPath $BodyPath -Value $Markdown -Encoding UTF8
$Title='[ARM-CLEAN06.2E] Hand semantics '+$ExpectedHead.Substring(0,12)
$Issue=@(& gh issue create --repo $GitHubRepository --title $Title --body-file $BodyPath)
if ($LASTEXITCODE -ne 0 -or $Issue.Count -ne 1) {
    throw 'Filtered GitHub issue publication failed.'
}
Set-Stage 'SUCCESS'
Write-Host ('[ARM-CLEAN06.2E] SEMANTIC_CENSUS_OK '+$Issue[0])
