# ARM-CLEAN06.2A — read-only, lossless-source migration preflight.
# Runs only in VALIDATION-AUTO01's detached worktree; NEVER saves an Unreal asset.
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
$Trigger='chore(ARM-CLEAN06.2A): audit post-audio assets before canonical migration'
$BaselineSha='c66bf68aa1a42eb8d9c47eb9b9336214d0bccf95'
$BaselineManifestSha256='03c3b09c1430fc8938e4c4bf1cde6fb229334aeda2348ed902c359c4edca6940'
$UserAudioSha='a60031db0d05aa444a7453f061e7463bd4e0b4d5'
$StagePath=Join-Path $RunDir 'ARMClean062A.stage'
$OutputDir=Join-Path $RunDir 'ARM-CLEAN06.2A'
New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
function Stage([string]$Value) { Set-Content -LiteralPath $StagePath -Value $Value -Encoding ASCII }
function Invoke-GitChecked([string[]]$CommandArgs) {
    $Lines=@(& git -C $WorktreePath @CommandArgs)
    if ($LASTEXITCODE -ne 0) { throw ('Git failed: '+$CommandArgs[0]) }
    return @($Lines)
}
function One([string[]]$CommandArgs) {
    $Lines=@(Invoke-GitChecked -CommandArgs $CommandArgs)
    if ($Lines.Count -ne 1) { throw 'Git response cardinality invalid.' }
    return ([string]$Lines[0]).Trim()
}
function AssertCounts($Manifest) {
    if ($Manifest.schema -cne 'ARM-CLEAN06.0/read-only/v1' -or
        @($Manifest.assets).Count -ne 56 -or
        @($Manifest.expanded_profiles).Count -ne 1640 -or
        [int]$Manifest.counts.weapon -ne 24 -or
        [int]$Manifest.counts.item -ne 24 -or
        [int]$Manifest.counts.level -ne 4 -or
        [int]$Manifest.counts.dungeon -ne 1 -or
        [int]$Manifest.counts.palette -ne 1 -or
        [int]$Manifest.counts.combat_catalogue -ne 1 -or
        [int]$Manifest.counts.visual_catalogue -ne 1 -or
        [int]$Manifest.counts.passive_profiles -ne 260) {
        throw 'UE asset inventory cardinality differs from ARM-CLEAN06.0.'
    }
}
function Props($Record) {
    $Map=@{}
    foreach ($Property in @($Record.properties.PSObject.Properties)) {
        $Map[$Property.Name]=[string]$Property.Value
    }
    return $Map
}
function FieldDelta($Before,$After) {
    $A=Props $Before
    $B=Props $After
    $Keys=@($A.Keys)+@($B.Keys) | Sort-Object -Unique
    $Changes=@()
    foreach ($Key in $Keys) {
        if (-not $A.ContainsKey($Key) -or -not $B.ContainsKey($Key) -or
            $A[$Key] -cne $B[$Key]) {
            $Changes+=([string]$Key)
        }
    }
    return @($Changes)
}
try {
    Stage 'PREFLIGHT'
    if ($ExpectedHead -notmatch '^[0-9a-f]{40}$') { throw 'Invalid SHA.' }
    if (-not (Test-Path -LiteralPath (Join-Path $WorktreePath '.git'))) {
        throw 'Detached validation worktree not found.'
    }
    if ((One -CommandArgs @('rev-parse','HEAD')) -cne $ExpectedHead -or
        (One -CommandArgs @('log','-1','--format=%s')) -cne $Trigger) {
        throw 'Current checkout or exact one-shot trigger mismatch.'
    }
    $Branch=@(Invoke-GitChecked -CommandArgs @('branch','--show-current'))
    if (@($Branch | Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) }).Count -gt 0) {
        throw 'Preflight requires detached HEAD.'
    }
    if (@(Invoke-GitChecked -CommandArgs @('status','--porcelain','--untracked-files=all')).Count -gt 0) {
        throw 'Worktree not clean before inspection.'
    }
    & git -C $WorktreePath merge-base --is-ancestor $UserAudioSha $ExpectedHead | Out-Null
    if ($LASTEXITCODE -ne 0) { throw 'User Sword and RatGiant commit is not an ancestor.' }
    if ((One -CommandArgs @('remote','get-url','origin')) -notmatch
        '(?i)github\.com[:/]PhilippeMartin1964/GrimrockPrototype(\.git)?$') {
        throw 'Unexpected origin.'
    }
    Stage 'BASELINE'
    $RunsRoot=Split-Path -Parent $RunDir
    $BaselineDirs=@(Get-ChildItem -LiteralPath $RunsRoot -Directory -Filter 'c66bf68aa1a4-*' -ErrorAction Stop |
        Sort-Object Name -Descending)
    if ($BaselineDirs.Count -eq 0) { throw 'Private ARM-CLEAN06.0 capture unavailable.' }
    $BaselinePath=Join-Path $BaselineDirs[0].FullName 'ARM-CLEAN06.0\asset-baseline.json'
    if (-not (Test-Path -LiteralPath $BaselinePath -PathType Leaf)) {
        throw 'Private ARM-CLEAN06.0 manifest is missing.'
    }
    $Hash=(Get-FileHash -LiteralPath $BaselinePath -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($Hash -cne $BaselineManifestSha256) {
        throw 'Private baseline checksum is not the verified ARM-CLEAN06.0 snapshot.'
    }
    $Before=Get-Content -LiteralPath $BaselinePath -Raw -Encoding UTF8 | ConvertFrom-Json
    AssertCounts $Before

    Stage 'AUDIO_DELTA'
    $AudioAssets=@(
        'Content/GrimrockPrototype/Audio/Weapons/Sword/Swing/SFX_Sword_Swing_Standard_01.uasset',
        'Content/GrimrockPrototype/Audio/Weapons/Sword/Swing/SFX_Sword_Swing_Standard_02.uasset',
        'Content/GrimrockPrototype/Audio/Weapons/Sword/Swing/SFX_Sword_Swing_Standard_03.uasset',
        'Content/GrimrockPrototype/Audio/Weapons/Sword/Swing/SFX_Sword_Swing_Standard_04.uasset',
        'Content/GrimrockPrototype/Monsters/RatGiant/Audio/S_GiantRat_Attack.uasset',
        'Content/GrimrockPrototype/Monsters/RatGiant/Audio/S_GiantRat_ImpactHit.uasset',
        'Content/GrimrockPrototype/Monsters/RatGiant/Data/DA_MON_RatGiant.uasset'
    )
    foreach ($Relative in $AudioAssets) {
        $File=Join-Path $WorktreePath $Relative.Replace('/', '\')
        if (-not (Test-Path -LiteralPath $File -PathType Leaf) -or
            (Get-Item -LiteralPath $File).Length -lt 1024) {
            throw 'Post-baseline Sword or RatGiant source binary is missing or is only an LFS pointer.'
        }
    }
    Stage 'CURRENT_EXPORT'
    $Editor=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
    $Project=Join-Path $WorktreePath 'GrimrockPrototype.uproject'
    if (-not (Test-Path -LiteralPath $Editor -PathType Leaf) -or
        -not (Test-Path -LiteralPath $Project -PathType Leaf)) {
        throw 'Unreal Editor or project unavailable.'
    }
    foreach ($Path in @($Project,$OutputDir)) {
        if ($Path.Contains('"')) { throw 'Unexpected quote in path.' }
    }
    $CurrentPath=Join-Path $OutputDir 'post-audio-baseline.json'
    $Stdout=Join-Path $OutputDir 'Unreal.stdout.log'
    $Stderr=Join-Path $OutputDir 'Unreal.stderr.log'
    $Arguments='"'+$Project+'" -run=GridArmClean06Baseline -unattended -NoSplash -NoP4 -NullRHI "-Output='+$CurrentPath+'"'
    $Process=Start-Process -FilePath $Editor -ArgumentList $Arguments -WorkingDirectory $WorktreePath -Wait -PassThru -WindowStyle Hidden -RedirectStandardOutput $Stdout -RedirectStandardError $Stderr -ErrorAction Stop
    if ($Process.ExitCode -ne 0) { throw 'Read-only UE baseline commandlet failed on post-audio assets.' }
    if (-not (Test-Path -LiteralPath $CurrentPath -PathType Leaf)) {
        throw 'Current UE manifest not produced.'
    }
    $After=Get-Content -LiteralPath $CurrentPath -Raw -Encoding UTF8 | ConvertFrom-Json
    AssertCounts $After

    Stage 'DIFF'
    $BeforeByPackage=@{}
    foreach ($Asset in @($Before.assets)) {
        $BeforeByPackage[[string]$Asset.package]=$Asset
    }
    $Changed=New-Object System.Collections.ArrayList
    foreach ($Asset in @($After.assets)) {
        $Package=[string]$Asset.package
        if (-not $BeforeByPackage.ContainsKey($Package)) {
            throw 'A package was introduced or renamed since the verified baseline.'
        }
        $Old=$BeforeByPackage[$Package]
        if ([string]$Old.class -cne [string]$Asset.class -or
            [string]$Old.kind -cne [string]$Asset.kind) {
            throw 'An existing serialized UE class or asset kind changed unexpectedly.'
        }
        $ChangedFields=@(FieldDelta -Before $Old -After $Asset)
        if ($ChangedFields.Count -gt 0) {
            [void]$Changed.Add([pscustomobject]@{
                package=$Package
                kind=[string]$Asset.kind
                property_names=@($ChangedFields)
                property_count=$ChangedFields.Count
            })
        }
    }
    if ($BeforeByPackage.Count -ne @($After.assets).Count) {
        throw 'A package was removed from the verified baseline.'
    }
    $ProfileMismatches=New-Object System.Collections.ArrayList
    for ($Index=0; $Index -lt 1640; $Index++) {
        $Old=$Before.expanded_profiles[$Index]
        $Current=$After.expanded_profiles[$Index]
        $A=@{}
        foreach ($Prop in $Old.PSObject.Properties) { $A[$Prop.Name]=[string]$Prop.Value }
        $B=@{}
        foreach ($Prop in $Current.PSObject.Properties) { $B[$Prop.Name]=[string]$Prop.Value }
        $Differences=@()
        foreach ($Key in @($A.Keys)+@($B.Keys) | Sort-Object -Unique) {
            if (-not $A.ContainsKey($Key) -or -not $B.ContainsKey($Key) -or
                $A[$Key] -cne $B[$Key]) { $Differences+=([string]$Key) }
        }
        if ($Differences.Count -gt 0) {
            [void]$ProfileMismatches.Add([pscustomobject]@{
                index=$Index
                before_id=[string]$Old.ArchetypeId
                current_id=[string]$Current.ArchetypeId
                level=[string]$Current.ItemLevel
                property_names=@($Differences)
            })
        }
    }
    $ChangedWeapons=@($Changed | Where-Object { $_.kind -eq 'weapon' })
    $Report=[ordered]@{
        schema='ARM-CLEAN06.2A/source-parity/v1'
        source_baseline_sha=$BaselineSha
        user_audio_sha=$UserAudioSha
        audited_head=$ExpectedHead
        baseline_sha256=$Hash
        current_manifest_sha256=(Get-FileHash -LiteralPath $CurrentPath -Algorithm SHA256).Hash.ToLowerInvariant()
        inventoried_assets=56
        legacy_weapon_definitions=24
        legacy_item_definitions=24
        compared_profiles=1640
        passive_profiles=260
        profile_mismatches=$ProfileMismatches.Count
        changed_weapon_definitions=$ChangedWeapons.Count
        changed_assets=@($Changed.ToArray())
        mismatched_profiles=@($ProfileMismatches.ToArray())
        user_audio_binaries_checked=$AudioAssets.Count
        assets_saved=0
    }
    $ReportPath=Join-Path $OutputDir 'source-parity.json'
    $Report | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $ReportPath -Encoding UTF8
    if ($ProfileMismatches.Count -gt 0) {
        Stage 'PROFILE_PARITY_FAILED'
        throw 'Expanded combat profiles differ from immutable ARM-CLEAN06.0 source.'
    }
    # A changed catalogue, items, or Sword presentation is an observation,
    # NEVER silently discarded or overwritten during later conversion.
    Stage 'CLEAN_CHECK'
    if (@(Invoke-GitChecked -CommandArgs @('status','--porcelain','--untracked-files=all')).Count -gt 0) {
        throw 'Read-only UE export modified the detached validation worktree.'
    }
    Stage 'ISSUE'
    $ChangedNames=@($ChangedWeapons | ForEach-Object {
        [IO.Path]::GetFileName([string]$_.package)
    })
    $PublicLines=@(
        '# ARM-CLEAN06.2A — audit des sources après ajouts Sword/RatGiant',
        '',
        ('SHA inspecté : '+$ExpectedHead),
        ('Baseline vérifiée : '+$BaselineSha),
        ('SHA256 baseline : '+$Hash),
        '',
        '| Contrôle | Résultat |',
        '|---|---:|',
        '| Assets UE comparés | 56 |',
        '| Profils numériques comparés | 1640 |',
        '| Profils passifs | 260 |',
        ('| Écarts des profils ARM | '+$ProfileMismatches.Count+' |'),
        ('| Définitions armes modifiées depuis baseline | '+$ChangedWeapons.Count+' |'),
        ('| Binaires Sword / RatGiant préservés | '+$AudioAssets.Count+' |'),
        '| Assets UE modifiés par l audit | 0 |',
        '',
        ('Weapons modifiés : '+($ChangedNames -join ', ')),
        '',
        'Rapport complet, propriétés modifiées et données sérialisées restent privés.',
        'Aucune conversion .uasset/.umap ni SaveGame dans cet audit.'
    )
    $IssueBody=Join-Path $OutputDir 'github-issue.md'
    Set-Content -LiteralPath $IssueBody -Value $PublicLines -Encoding UTF8
    $Title='[ARM-CLEAN06.2A] Source parity '+$ExpectedHead.Substring(0,12)
    $Issue=@(& gh issue create --repo $GitHubRepository --title $Title --body-file $IssueBody)
    if ($LASTEXITCODE -ne 0 -or $Issue.Count -ne 1) { throw 'Audit complete, but GitHub issue publication failed.' }
    Stage 'SUCCESS'
    Write-Host ('[ARM-CLEAN06.2A] SOURCE_PARITY_OK; '+$Issue[0])
    exit 0
}
catch {
    if (-not (Test-Path -LiteralPath $StagePath)) { Stage 'FAILED' }
    throw
}
