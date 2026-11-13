# ARM-CLEAN06.2C — read-only, numerically exact compact-catalogue projection.
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
$Subject='chore(ARM-CLEAN06.2C): prove transient compact catalogue numeric parity'
$OutDir=Join-Path $RunDir 'ARM-CLEAN06.2C'
$Stage=Join-Path $RunDir 'ARMClean062C.stage'
New-Item -ItemType Directory -Path $OutDir -Force | Out-Null
function Stage([string]$Value) { Set-Content -LiteralPath $Stage -Value $Value -Encoding ASCII }
function Git-One([string[]]$GitArgs) {
    $Lines=@(& git -C $WorktreePath @GitArgs)
    if ($LASTEXITCODE -ne 0 -or $Lines.Count -ne 1) { throw 'Git lookup failed.' }
    return ([string]$Lines[0]).Trim()
}
try {
    Stage 'PREFLIGHT'
    if ($ExpectedHead -notmatch '^[a-f0-9]{40}$' -or
        (Git-One -GitArgs @('rev-parse','HEAD')) -cne $ExpectedHead -or
        (Git-One -GitArgs @('log','-1','--format=%s')) -cne $Subject) {
        throw 'SHA or trigger mismatch.'
    }
    if ((Git-One -GitArgs @('remote','get-url','origin')) -notmatch
        '(?i)github\.com[:/]PhilippeMartin1964/GrimrockPrototype(\.git)?$') {
        throw 'Unexpected GitHub origin.'
    }
    $Branch=@(& git -C $WorktreePath branch --show-current)
    if ($LASTEXITCODE -ne 0 -or
        @($Branch | Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) }).Count -gt 0) {
        throw 'Detached HEAD required.'
    }
    $Dirty=@(& git -C $WorktreePath status --porcelain --untracked-files=all)
    if ($LASTEXITCODE -ne 0 -or $Dirty.Count -gt 0) { throw 'Source worktree is not clean.' }
    $Editor=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
    $Project=Join-Path $WorktreePath 'GrimrockPrototype.uproject'
    if (-not (Test-Path -LiteralPath $Editor -PathType Leaf) -or
        -not (Test-Path -LiteralPath $Project -PathType Leaf)) {
        throw 'Unreal Engine or project missing.'
    }
    foreach ($Value in @($Editor,$Project,$OutDir)) {
        if ($Value.Contains('"')) { throw 'Unsupported quoted path.' }
    }
    Stage 'COMMANDLET'
    $ResultPath=Join-Path $OutDir 'numeric-projection-private.json'
    $Args='"'+$Project+'" -run=GridArmClean06CatalogueProjection -unattended -NoSplash -NoP4 -NullRHI "-Output='+$ResultPath+'"'
    $Proc=Start-Process -FilePath $Editor -ArgumentList $Args -WorkingDirectory $WorktreePath -WindowStyle Hidden -PassThru -Wait -RedirectStandardOutput (Join-Path $OutDir 'Unreal.stdout.log') -RedirectStandardError (Join-Path $OutDir 'Unreal.stderr.log') -ErrorAction Stop
    if ($Proc.ExitCode -ne 0) { throw 'Numeric projection rejected by native Unreal.' }
    if (-not (Test-Path -LiteralPath $ResultPath -PathType Leaf)) { throw 'Private report absent.' }
    Stage 'REPORT'
    $Report=Get-Content -LiteralPath $ResultPath -Raw -Encoding UTF8 | ConvertFrom-Json
    if ($Report.schema -cne 'ARM-CLEAN06.2C/transient-numeric-only/v1' -or
        [int]$Report.source_compact_archetypes -ne 82 -or
        [int]$Report.compared_profiles -ne 1640 -or
        [int]$Report.passive_profiles -ne 260 -or
        [int]$Report.numeric_mismatches -ne 0 -or
        @($Report.unmapped_semantics).Count -ne 82 -or
        [bool]$Report.authoring_or_binary_migration_authorized -or
        [bool]$Report.any_binary_writes) {
        throw 'Lossless numeric report failed invariant checks.'
    }
    Stage 'CHECK_CLEAN'
    $After=@(& git -C $WorktreePath status --porcelain --untracked-files=all)
    if ($LASTEXITCODE -ne 0 -or $After.Count -gt 0) {
        throw 'Unexpected binary or source modification by read-only commandlet.'
    }
    Stage 'ISSUE'
    $Markdown=@(
        '# ARM-CLEAN06.2C — preuve numérique UE sur catalogue compact',
        '',
        ('Commit source : '+$ExpectedHead),
        '',
        '| Contrôle | Résultat |',
        '|---|---:|',
        '| Archétypes ARM | 82 |',
        '| Profils N1 à N20 contrôlés | 1640 |',
        '| Profils passifs | 260 |',
        '| Écarts numériques | 0 |',
        ('| Métadonnées de classes (rows) | '+[int]$Report.source_class_affinity_rows+' |'),
        ('| Classes source (prefs) | '+[int]$Report.source_class_preferences+' |'),
        ('| Variantes de HandUsage | '+[int]$Report.distinct_hand_usages+' |'),
        '| .uasset ou .umap sauvegardés | 0 |',
        '',
        '**Ce résultat prouve seulement la parité numérique en mémoire.**',
        'Type exact du passif, politique de mains, classe, action, munitions,',
        'SFX/VFX et liens vers les définitions concrètes ne sont PAS convertis.',
        'Les valeurs textuelles et les autres données sources restent privées.',
        'Aucun asset canonique binaire publié et aucune suppression legacy.'
    )
    $Body=Join-Path $OutDir 'github-issue.md'
    Set-Content -LiteralPath $Body -Value $Markdown -Encoding UTF8
    $Title='[ARM-CLEAN06.2C] Numeric projection '+$ExpectedHead.Substring(0,12)
    $Issue=@(& gh issue create --repo $GitHubRepository --title $Title --body-file $Body)
    if ($LASTEXITCODE -ne 0 -or $Issue.Count -ne 1) { throw 'GitHub issue creation failed.' }
    Stage 'SUCCESS'
    Write-Host ('[ARM-CLEAN06.2C] NUMERIC_PARITY_OK '+$Issue[0])
    exit 0
}
catch { throw }
