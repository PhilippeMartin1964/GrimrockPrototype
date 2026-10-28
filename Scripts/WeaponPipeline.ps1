[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\UE_5.5',
    [ValidateSet('Audit','Plan','Build','Validate','Narrative','PaletteItem')]
    [string]$Action = 'Audit',
    [string]$Archetype,
    [string]$ItemPath,
    [ValidateSet('Items')]
    [string]$PaletteCategory = 'Items',
    [string[]]$Archetypes = @(),
    [switch]$All,
    [ValidateSet('Visual','Item','Palette')]
    [string]$Stage,
    [switch]$Apply
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$Repo = Split-Path -Parent $PSScriptRoot
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Project = Join-Path $Repo 'GrimrockPrototype.uproject'
$Manifest = Join-Path $Repo 'Config\Armement\WeaponProductionManifest.csv'
$ReportDir = Join-Path $Repo 'Saved\ARMEMENT-INDUS'
$VisualCatalogue = 'Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.uasset'
$RpgCatalogue = 'Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.uasset'
$Palette = 'Content/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.uasset'

if (-not (Test-Path -LiteralPath $Editor -PathType Leaf)) { throw "Missing UnrealEditor-Cmd: $Editor" }
if (-not (Test-Path -LiteralPath $Manifest -PathType Leaf)) { throw "Missing manifest: $Manifest" }
if ((& git -C $Repo branch --show-current).Trim() -ne 'master') { throw 'Master only.' }
if ($LASTEXITCODE -ne 0) { throw 'Cannot inspect branch.' }
if ($Archetype -and $Archetypes.Count -gt 0) { throw 'Select Archetype or Archetypes, not both.' }
if ($All -and ($Archetype -or $Archetypes.Count -gt 0)) { throw 'All conflicts with explicit IDs.' }
if ($Action -eq 'Build' -and (-not $Archetype -or $All -or $Archetypes.Count -gt 0)) {
    throw 'Build requires one -Archetype.'
}
if ($Apply -and -not (($Action -eq 'Build' -and $Stage) -or $Action -eq 'Narrative' -or $Action -eq 'PaletteItem')) {
    throw 'Apply requires Build with Stage Visual|Item|Palette, or Narrative.'
}
if ($Stage -and $Action -ne 'Build') { throw 'Stage only applies to Build.' }
if ($ItemPath -and $Action -ne 'PaletteItem') { throw 'ItemPath requires PaletteItem.' }
if ($Action -eq 'Narrative' -and $Apply -and
    ($Archetypes.Count -gt 0 -or (-not $Archetype -and -not $All))) {
    throw 'Narrative -Apply requires exactly one -Archetype or -All; lists are read-only.'
}


$Before = @(& git -C $Repo status --porcelain=v1 --untracked-files=all)
if ($LASTEXITCODE -ne 0) { throw 'git status failed' }
if ($Apply -and ($Stage -eq 'Visual' -or $Stage -eq 'Palette')) {
    $TargetPackage = if ($Stage -eq 'Visual') { $VisualCatalogue } else { $Palette }
    $DirtyTarget = @(& git -C $Repo status --porcelain=v1 -- $TargetPackage)
    if ($DirtyTarget.Count -gt 0) { throw "Target catalogue has local changes; refusing write: $TargetPackage" }
}

function Get-Fingerprint([string]$Relative) {
    $Path = Join-Path $Repo ($Relative.Replace('/', '\'))
    if (Test-Path -LiteralPath $Path -PathType Leaf) {
        return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
    }
    return ''
}
$BeforeHashes = @{}
foreach ($Path in @($RpgCatalogue, $VisualCatalogue, $Palette)) {
    $BeforeHashes[$Path] = Get-Fingerprint $Path
}

if ($Action -eq 'PaletteItem') {
    if (-not $ItemPath -or $Archetype -or $Archetypes.Count -gt 0 -or $All -or $Stage) {
        throw 'PaletteItem requires only ItemPath and optional Apply.'
    }
    $Match = [regex]::Match($ItemPath,
        '^(/Game/GrimrockPrototype/Core/DataAssets/Items/(DA_Item_[A-Za-z0-9_]+))\.(DA_Item_[A-Za-z0-9_]+) for the canonical narrative.
# -Action Narrative alone exports the XLSX (read-only, backward compatible).
# -Archetype ARMxxx audits or applies a single existing DataAsset.
# -All audits/applies active rows one at a time, keeping each package guarded.
if ($Action -eq 'Narrative') {
    $Export = Join-Path $PSScriptRoot 'ExportARMEMENT014Catalogue.ps1'
    & $Export -RequireNarrative
    if ($LASTEXITCODE -ne 0) { throw 'NarrativeV1 export failed.' }
    $NarrativeCsv = Join-Path $ReportDir 'INDUS03_Narrative.csv'
    if (-not (Test-Path -LiteralPath $NarrativeCsv -PathType Leaf)) {
        throw "NarrativeV1 export missing: $NarrativeCsv"
    }
    $Rows = @(Import-Csv -LiteralPath $NarrativeCsv)
    if ($Rows.Count -ne 82 -or @($Rows | Where-Object {
        $_.SchemaVersion -ne 'NarrativeV1'
    }).Count -ne 0) { throw 'NarrativeV1 requires 82 rows.' }
    $Approved = @($Rows | Where-Object {
        $_.EtatValidation -eq 'APPROVED' -and
        -not [string]::IsNullOrWhiteSpace($_.DescriptionCourteFR)
    }).Count
    if ($Apply -and $Approved -ne 82) {
        throw "Narrative -Apply requires 82/82 APPROVED, found $Approved."
    }
    Write-Host "[INDUS05] NARRATIVE Schema=NarrativeV1 Rows=82 Approved=$Approved/82 Apply=$Apply"

    # Historical read-only export remains possible without an archetype selector.
    if (-not $All -and -not $Archetype -and $Archetypes.Count -eq 0) {
        $AfterNarrative = @(& git -C $Repo status --porcelain=v1 --untracked-files=all)
        if ($LASTEXITCODE -ne 0 -or
            @($AfterNarrative | Where-Object { $_ -notin $Before }).Count -ne 0) {
            throw 'Read-only Narrative export unexpectedly changed project files.'
        }
        Write-Host "CSV: $NarrativeCsv"
        return
    }

    $ManifestRows = @(Import-Csv -LiteralPath $Manifest)
    $TargetsNarrative = @()
    if ($All) {
        $TargetsNarrative = @($ManifestRows | Where-Object {
            $_.ProductionState -eq 'Active'
        } | Sort-Object ArchetypeId)
    } else {
        $Ids = @()
        if ($Archetype) { $Ids = @($Archetype) }
        else { $Ids = @($Archetypes | Select-Object -Unique) }
        foreach ($Id in $Ids) {
            $MatchesRow = @($ManifestRows | Where-Object { $_.ArchetypeId -eq $Id })
            if ($MatchesRow.Count -ne 1) { throw "Archetype not uniquely declared in manifest: $Id" }
            $TargetsNarrative += $MatchesRow[0]
        }
    }
    if ($TargetsNarrative.Count -eq 0) { throw 'No declared active archetypes to process.' }
    if ($Apply -and $TargetsNarrative.Count -gt 1 -and -not $All) {
        throw 'Narrative multi-item mutation requires explicit -All.'
    }

    # A single Editor build for this invocation, not one build per weapon.
    & (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipAutomation
    if ($LASTEXITCODE -ne 0) { throw 'Development Editor build failed.' }
    New-Item -ItemType Directory -Force -Path $ReportDir | Out-Null
    $BatchResults = [System.Collections.Generic.List[object]]::new()
    $AllowedChangedItems = [System.Collections.Generic.List[string]]::new()
    foreach ($Spec in $TargetsNarrative) {
        $Id = [string]$Spec.ArchetypeId
        $ObjectPath = [string]$Spec.ItemPath
        $Match = [regex]::Match($ObjectPath, '^(/Game/[A-Za-z0-9_/]+)\.([A-Za-z0-9_]+)$')
        if (-not $Match.Success -or
            -not $Match.Groups[1].Value.StartsWith('/Game/GrimrockPrototype/') -or
            -not $Match.Groups[1].Value.EndsWith('/' + $Match.Groups[2].Value)) {
            throw "Invalid production ItemPath for $Id."
        }
        $RelativeItem = 'Content/' + $Match.Groups[1].Value.Substring('/Game/'.Length) + '.uasset'
        $ItemFile = Join-Path $Repo ($RelativeItem.Replace('/', '\'))
        $NarrativeEntry = @($Rows | Where-Object { $_.ArchetypeId -eq $Id })
        if ($NarrativeEntry.Count -ne 1) { throw "Missing NarrativeV1 entry for $Id" }
        $Review = [string]$NarrativeEntry[0].EtatValidation
        $Status = 'UNKNOWN'
        $Detail = ''
        if ($Spec.ProductionState -ne 'Active') {
            $Status = 'SKIPPED_PLANNED'
        } elseif (-not (Test-Path -LiteralPath $ItemFile -PathType Leaf)) {
            $Status = 'SKIPPED_NOT_CREATED'
        } elseif (@(& git -C $Repo status --porcelain=v1 -- $RelativeItem).Count -gt 0) {
            $Status = 'SKIPPED_DIRTY'
            $Detail = 'Preexisting local changes preserved'
        } else {
            $SingleReport = Join-Path $ReportDir ("INDUS05_Narrative_$Id.csv")
            $Log = Join-Path $ReportDir ("INDUS05_Narrative_$Id.log")
            $ArgsEditor = @(
                $Project, '-run=GridWeaponPipeline', '-Action=NarrativeItem',
                "-Archetype=$Id", "-Manifest=$Manifest",
                "-Narrative=$NarrativeCsv", "-Report=$SingleReport",
                '-Unattended', '-NoSplash', '-NoP4', '-NoSound', '-NullRHI', '-log'
            )
            if ($Apply) { $ArgsEditor += '-Apply' }
            & $Editor @ArgsEditor *> $Log
            $ExitCode = $LASTEXITCODE
            $Messages = @(Select-String -LiteralPath $Log -Pattern '\[INDUS04\.4\]' |
                ForEach-Object { $_.Line })
            if ($ExitCode -ne 0) {
                $Status = 'FAILED'
                $Detail = "Commandlet returned $ExitCode"
            } elseif (@($Messages | Where-Object {
                $_ -match '\[INDUS04\.4\] RESULT Archetype='
            }).Count -ne 1 -or -not (Test-Path -LiteralPath $SingleReport)) {
                $Status = 'FAILED'
                $Detail = 'Missing authoritative report/diagnostics'
            } else {
                $ItemReportRows = @(Import-Csv -LiteralPath $SingleReport)
                if ($ItemReportRows.Count -ne 1) {
                    $Status = 'FAILED'
                    $Detail = 'Expected exactly one item result'
                } else {
                    $Status = [string]$ItemReportRows[0].Status
                    if ($Apply) {
                        $Saves = @($Messages | Where-Object {
                            $_ -match '\[INDUS04\.4\] SAVED NarrativeItem'
                        })
                        $Unchanged = @($Messages | Where-Object {
                            $_ -match '\[INDUS04\.4\] PRESENT_SAME NarrativeItem'
                        })
                        if ($Saves.Count -eq 1 -and $Unchanged.Count -eq 0) {
                            $Status = 'SAVED'
                            $AllowedChangedItems.Add($RelativeItem)
                        } elseif ($Unchanged.Count -eq 1 -and $Saves.Count -eq 0) {
                            $Status = 'PRESENT_SAME'
                        } else {
                            $Status = 'FAILED'
                            $Detail = 'Apply not confirmed as SAVED or PRESENT_SAME'
                        }
                    }
                }
            }
        }
        $BatchResults.Add([pscustomobject]@{
            ArchetypeId=$Id; ProductionState=$Spec.ProductionState
            Review=$Review; Status=$Status; ItemPath=$RelativeItem; Detail=$Detail
        })
        Write-Host "[INDUS05] NARRATIVE RESULT Archetype=$Id Status=$Status Review=$Review $Detail"
    }
    $BatchReport = Join-Path $ReportDir 'INDUS05_Narrative_All.csv'
    $BatchResults | Export-Csv -LiteralPath $BatchReport -NoTypeInformation -Encoding UTF8
    $BatchJson = [IO.Path]::ChangeExtension($BatchReport, '.json')
    $BatchResults | ConvertTo-Json -Depth 5 |
        Set-Content -LiteralPath $BatchJson -Encoding UTF8
    $AfterNarrative = @(& git -C $Repo status --porcelain=v1 --untracked-files=all)
    if ($LASTEXITCODE -ne 0) { throw 'Post-Narrative git status failed.' }
    $NewChanges = @($AfterNarrative | Where-Object { $_ -notin $Before })
    $Unexpected = @($NewChanges | Where-Object {
        $_.Length -lt 4 -or $_.Substring(3).Replace('\','/') -notin $AllowedChangedItems
    })
    if ($Unexpected.Count -ne 0) {
        $Unexpected | ForEach-Object { Write-Host $_ }
        throw 'Unexpected changes outside the selected Narrative Item DataAssets.'
    }
    $Failures = @($BatchResults | Where-Object { $_.Status -eq 'FAILED' })
    if ($Apply -and $Failures.Count -eq 0) {
        & (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipBuild -AutomationFilter 'Grimrock.RPG.ARMEMENT.Industrial'
        if ($LASTEXITCODE -ne 0) {
            throw 'Industrial regression after Narrative apply failed; inspect modified assets.'
        }
    }
    Write-Host "[INDUS05] NARRATIVE SUMMARY Approved=$Approved/82 Targets=$($BatchResults.Count) Saved=$(@($BatchResults | Where-Object { $_.Status -eq 'SAVED' }).Count) Failed=$($Failures.Count)"
    Write-Host "Report: $BatchReport"
    & git -C $Repo status --short
    if ($Failures.Count -gt 0) { throw "$($Failures.Count) narrative item(s) failed: inspect the report." }
    return
}

Write-Host '=== INDUS02: editor build ==='
& (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipAutomation
if ($LASTEXITCODE -ne 0) { throw 'UE editor build failed' }
New-Item -ItemType Directory -Force -Path $ReportDir | Out-Null

$Targets = @('ALL')
if ($Archetype) { $Targets = @($Archetype) }
elseif ($Archetypes.Count -gt 0) { $Targets = @($Archetypes | Select-Object -Unique) }

foreach ($Target in $Targets) {
    if ($Target -ne 'ALL' -and $Target -notmatch '^ARM[0-9]{3}$') {
        throw "Invalid archetype: $Target"
    }
    $Suffix = if ($Target -eq 'ALL') { 'All' } else { $Target }
    $Report = Join-Path $ReportDir ('INDUS02_' + $Action + '_' + $Suffix + '.csv')
    $JsonReport = [IO.Path]::ChangeExtension($Report, '.json')
    $Log = Join-Path $ReportDir ('INDUS02_' + $Action + '_' + $Suffix + '.log')
    $ArgsEditor = @(
        $Project, '-run=GridWeaponPipeline', "-Action=$Action",
        "-Manifest=$Manifest", "-Report=$Report",
        '-Unattended', '-NoSplash', '-NoP4', '-NoSound', '-NullRHI', '-log'
    )
    if ($Target -ne 'ALL') { $ArgsEditor += "-Archetype=$Target" }
    if ($Stage) { $ArgsEditor += "-Stage=$Stage" }
    if ($Apply) { $ArgsEditor += '-Apply' }
    Write-Host "=== INDUS02 $Action $Target (Stage=$Stage, Apply=$Apply) ==="
    Write-Host "Log: $Log"
    & $Editor @ArgsEditor *> $Log
    $Code = $LASTEXITCODE
    $Matches = @(Select-String -LiteralPath $Log -Pattern '\[INDUS02\]' -ErrorAction SilentlyContinue)
    $Lines = @($Matches | ForEach-Object { $_.Line })
    $Lines | ForEach-Object { Write-Host $_ }
    if ($Code -ne 0) {
        Get-Content -LiteralPath $Log -Tail 25
        throw "Commandlet failed for $Target (code $Code)"
    }
    $Results = @($Lines | Where-Object { $_ -match '\[INDUS02\] RESULT Archetype=' })
    $Summaries = @($Lines | Where-Object { $_ -match '\[INDUS02\] SUMMARY Action=' })
    if ($Results.Count -eq 0 -or $Summaries.Count -ne 1) {
        throw 'Commandlet diagnostics missing; refusing silent success.'
    }
    if ($Apply) {
        $Applied = @($Lines | Where-Object {
            $_ -match '\[INDUS02\] (SAVED|PRESENT_SAME) (Visual|Item|Palette) for '
        })
        if ($Applied.Count -ne 1) { throw 'Apply was not explicitly confirmed.' }
    }
    if (-not (Test-Path -LiteralPath $Report -PathType Leaf)) {
        throw "Expected report missing: $Report"
    }
    @(Import-Csv -LiteralPath $Report) | ConvertTo-Json -Depth 6 |
        Set-Content -LiteralPath $JsonReport -Encoding UTF8
    Write-Host "CSV : $Report"
    Write-Host "JSON: $JsonReport"
}

if ($Action -eq 'Validate' -or $Apply) {
    & (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipBuild -AutomationFilter 'Grimrock.RPG.ARMEMENT.Industrial'
    if ($LASTEXITCODE -ne 0) { throw 'TD04.2 industrial tests failed; do not publish.' }
}

foreach ($Path in @($RpgCatalogue, $VisualCatalogue, $Palette)) {
    $Allowed = $Apply -and (($Stage -eq 'Visual' -and $Path -eq $VisualCatalogue) -or
        ($Stage -eq 'Palette' -and $Path -eq $Palette))
    if (-not $Allowed -and (Get-Fingerprint $Path) -ne $BeforeHashes[$Path]) {
        throw "Unexpected package change: $Path"
    }
}
$After = @(& git -C $Repo status --porcelain=v1 --untracked-files=all)
if ($LASTEXITCODE -ne 0) { throw 'Post-command git status failed' }
$New = @($After | Where-Object { $_ -notin $Before })
if ($Apply) {
    if ($Stage -eq 'Visual') {
        $Expected = $VisualCatalogue
    } elseif ($Stage -eq 'Palette') {
        $Expected = $Palette
    } else {
        $Rows = @(Import-Csv -LiteralPath $Manifest | Where-Object { $_.ArchetypeId -eq $Archetype })
        if ($Rows.Count -ne 1) { throw "Manifest ID ambiguous: $Archetype" }
        $LongPath = ($Rows[0].ItemPath -split '\.')[0]
        $Expected = 'Content/' + $LongPath.Substring('/Game/'.Length) + '.uasset'
    }
    $Unexpected = @($New | Where-Object {
        $_.Length -lt 4 -or $_.Substring(3).Replace('\','/') -ne $Expected
    })
    if ($Unexpected.Count -gt 0) {
        $Unexpected | ForEach-Object { Write-Host $_ }
        throw 'Unrelated files changed. Do not stage/commit.'
    }
} elseif ($New.Count -gt 0) {
    $New | ForEach-Object { Write-Host $_ }
    throw 'Read-only operation changed Content or source files.'
}
Write-Host '=== Git status (no auto-stage / commit / push) ==='
& git -C $Repo status --short --untracked-files=all
Write-Host '[OK] INDUS02 completed. PIE acceptance remains separate.'
)
    if (-not $Match.Success -or $Match.Groups[2].Value -cne $Match.Groups[3].Value) {
        throw "Unsupported ItemPath: $ItemPath"
    }
    $Source = 'Content/' + $Match.Groups[1].Value.Substring('/Game/'.Length) + '.uasset'
    $SourceFile = Join-Path $Repo $Source.Replace('/', '\')
    $TargetFile = Join-Path $Repo $Palette.Replace('/', '\')
    foreach ($File in @($SourceFile, $TargetFile)) {
        if (-not (Test-Path -LiteralPath $File -PathType Leaf)) {
            throw "Required package absent: $File"
        }
    }
    if (@(& git -C $Repo diff --cached --name-only).Count -gt 0) {
        throw 'Git index is not empty.'
    }
    # No array-expression argument following '--': native PowerShell binding differs.
    $Dirty = @(& git -C $Repo status --porcelain=v1 -- $Palette $Source)
    if ($LASTEXITCODE -ne 0) { throw 'Failed to inspect Git source/target.' }
    if ($Dirty.Count -gt 0) {
        Write-Host 'The source or palette already has local changes:'
        $Dirty | ForEach-Object { Write-Host $_ }
        throw 'Refusing overwrite. Inspect changes; no reset performed.'
    }
    $SourceHash = (Get-FileHash -LiteralPath $SourceFile -Algorithm SHA256).Hash
    $PaletteHash = (Get-FileHash -LiteralPath $TargetFile -Algorithm SHA256).Hash
    & (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipAutomation
    if ($LASTEXITCODE -ne 0 -or -not $?) { throw 'Unreal Editor build failed.' }
    New-Item -ItemType Directory -Path $ReportDir -Force | Out-Null
    $Log = Join-Path $ReportDir ('INDUS02_PaletteItem_' + $Match.Groups[2].Value + '.log')
    $Args = @($Project,'-run=GridWeaponPipeline','-Action=PaletteItem',
        "-ItemPath=$ItemPath", "-PaletteCategory=$PaletteCategory",
        '-Unattended','-NoSplash','-NoP4','-NoSound','-NullRHI','-log')
    if ($Apply) { $Args += '-Apply' }
    & $Editor @Args *> $Log
    $Code = $LASTEXITCODE
    $Messages = @(Select-String -LiteralPath $Log -Pattern '\[INDUS02\] PALETTE_ITEM' |
        ForEach-Object { $_.Line })
    $Messages | ForEach-Object { Write-Host $_ }
    if ($Code -ne 0) { throw "PaletteItem failed code $Code, log: $Log" }
    $Expected = if ($Apply) { '(SAVED|PRESENT_SAME)' } else { '(DRY_RUN|PRESENT_SAME)' }
    if (@($Messages | Where-Object {
        $_ -match ("\[INDUS02\] PALETTE_ITEM " + $Expected + '\b')
    }).Count -ne 1) {
        throw 'Missing definitive authoring result.'
    }
    if ((Get-FileHash -LiteralPath $SourceFile -Algorithm SHA256).Hash -ne $SourceHash) {
        throw 'Item definition unexpectedly changed.'
    }
    if (-not $Apply -and
        (Get-FileHash -LiteralPath $TargetFile -Algorithm SHA256).Hash -ne $PaletteHash) {
        throw 'Read-only audit unexpectedly changed palette.'
    }
    $After = @(& git -C $Repo status --porcelain=v1 --untracked-files=all)
    if ($LASTEXITCODE -ne 0) { throw 'git status failed.' }
    $Changed = @($After | Where-Object { $_ -notin $Before })
    $Unexpected = @($Changed | Where-Object {
        $_.Length -lt 4 -or $_.Substring(3).Replace('\','/') -cne $Palette
    })
    if ($Unexpected.Count -gt 0) {
        $Unexpected | ForEach-Object { Write-Host $_ }
        throw 'Unexpected file changes, no Git action.'
    }
    if ($Apply) {
        & (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipBuild -AutomationFilter 'Grimrock.RPG.ARMEMENT.LOT02'
        if ($LASTEXITCODE -ne 0 -or -not $?) { throw 'LOT02 test failure, no Git action.' }
    }
    Write-Host "[INDUS02] PALETTE_ITEM COMPLETE Apply=$Apply"
    & git -C $Repo status --short
    return
}

# INDUS05.2: one supported public entry point for the canonical narrative.
# -Action Narrative alone exports the XLSX (read-only, backward compatible).
# -Archetype ARMxxx audits or applies a single existing DataAsset.
# -All audits/applies active rows one at a time, keeping each package guarded.
if ($Action -eq 'Narrative') {
    $Export = Join-Path $PSScriptRoot 'ExportARMEMENT014Catalogue.ps1'
    & $Export -RequireNarrative
    if ($LASTEXITCODE -ne 0) { throw 'NarrativeV1 export failed.' }
    $NarrativeCsv = Join-Path $ReportDir 'INDUS03_Narrative.csv'
    if (-not (Test-Path -LiteralPath $NarrativeCsv -PathType Leaf)) {
        throw "NarrativeV1 export missing: $NarrativeCsv"
    }
    $Rows = @(Import-Csv -LiteralPath $NarrativeCsv)
    if ($Rows.Count -ne 82 -or @($Rows | Where-Object {
        $_.SchemaVersion -ne 'NarrativeV1'
    }).Count -ne 0) { throw 'NarrativeV1 requires 82 rows.' }
    $Approved = @($Rows | Where-Object {
        $_.EtatValidation -eq 'APPROVED' -and
        -not [string]::IsNullOrWhiteSpace($_.DescriptionCourteFR)
    }).Count
    if ($Apply -and $Approved -ne 82) {
        throw "Narrative -Apply requires 82/82 APPROVED, found $Approved."
    }
    Write-Host "[INDUS05] NARRATIVE Schema=NarrativeV1 Rows=82 Approved=$Approved/82 Apply=$Apply"

    # Historical read-only export remains possible without an archetype selector.
    if (-not $All -and -not $Archetype -and $Archetypes.Count -eq 0) {
        $AfterNarrative = @(& git -C $Repo status --porcelain=v1 --untracked-files=all)
        if ($LASTEXITCODE -ne 0 -or
            @($AfterNarrative | Where-Object { $_ -notin $Before }).Count -ne 0) {
            throw 'Read-only Narrative export unexpectedly changed project files.'
        }
        Write-Host "CSV: $NarrativeCsv"
        return
    }

    $ManifestRows = @(Import-Csv -LiteralPath $Manifest)
    $TargetsNarrative = @()
    if ($All) {
        $TargetsNarrative = @($ManifestRows | Where-Object {
            $_.ProductionState -eq 'Active'
        } | Sort-Object ArchetypeId)
    } else {
        $Ids = @()
        if ($Archetype) { $Ids = @($Archetype) }
        else { $Ids = @($Archetypes | Select-Object -Unique) }
        foreach ($Id in $Ids) {
            $MatchesRow = @($ManifestRows | Where-Object { $_.ArchetypeId -eq $Id })
            if ($MatchesRow.Count -ne 1) { throw "Archetype not uniquely declared in manifest: $Id" }
            $TargetsNarrative += $MatchesRow[0]
        }
    }
    if ($TargetsNarrative.Count -eq 0) { throw 'No declared active archetypes to process.' }
    if ($Apply -and $TargetsNarrative.Count -gt 1 -and -not $All) {
        throw 'Narrative multi-item mutation requires explicit -All.'
    }

    # A single Editor build for this invocation, not one build per weapon.
    & (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipAutomation
    if ($LASTEXITCODE -ne 0) { throw 'Development Editor build failed.' }
    New-Item -ItemType Directory -Force -Path $ReportDir | Out-Null
    $BatchResults = [System.Collections.Generic.List[object]]::new()
    $AllowedChangedItems = [System.Collections.Generic.List[string]]::new()
    foreach ($Spec in $TargetsNarrative) {
        $Id = [string]$Spec.ArchetypeId
        $ObjectPath = [string]$Spec.ItemPath
        $Match = [regex]::Match($ObjectPath, '^(/Game/[A-Za-z0-9_/]+)\.([A-Za-z0-9_]+)$')
        if (-not $Match.Success -or
            -not $Match.Groups[1].Value.StartsWith('/Game/GrimrockPrototype/') -or
            -not $Match.Groups[1].Value.EndsWith('/' + $Match.Groups[2].Value)) {
            throw "Invalid production ItemPath for $Id."
        }
        $RelativeItem = 'Content/' + $Match.Groups[1].Value.Substring('/Game/'.Length) + '.uasset'
        $ItemFile = Join-Path $Repo ($RelativeItem.Replace('/', '\'))
        $NarrativeEntry = @($Rows | Where-Object { $_.ArchetypeId -eq $Id })
        if ($NarrativeEntry.Count -ne 1) { throw "Missing NarrativeV1 entry for $Id" }
        $Review = [string]$NarrativeEntry[0].EtatValidation
        $Status = 'UNKNOWN'
        $Detail = ''
        if ($Spec.ProductionState -ne 'Active') {
            $Status = 'SKIPPED_PLANNED'
        } elseif (-not (Test-Path -LiteralPath $ItemFile -PathType Leaf)) {
            $Status = 'SKIPPED_NOT_CREATED'
        } elseif (@(& git -C $Repo status --porcelain=v1 -- $RelativeItem).Count -gt 0) {
            $Status = 'SKIPPED_DIRTY'
            $Detail = 'Preexisting local changes preserved'
        } else {
            $SingleReport = Join-Path $ReportDir ("INDUS05_Narrative_$Id.csv")
            $Log = Join-Path $ReportDir ("INDUS05_Narrative_$Id.log")
            $ArgsEditor = @(
                $Project, '-run=GridWeaponPipeline', '-Action=NarrativeItem',
                "-Archetype=$Id", "-Manifest=$Manifest",
                "-Narrative=$NarrativeCsv", "-Report=$SingleReport",
                '-Unattended', '-NoSplash', '-NoP4', '-NoSound', '-NullRHI', '-log'
            )
            if ($Apply) { $ArgsEditor += '-Apply' }
            & $Editor @ArgsEditor *> $Log
            $ExitCode = $LASTEXITCODE
            $Messages = @(Select-String -LiteralPath $Log -Pattern '\[INDUS04\.4\]' |
                ForEach-Object { $_.Line })
            if ($ExitCode -ne 0) {
                $Status = 'FAILED'
                $Detail = "Commandlet returned $ExitCode"
            } elseif (@($Messages | Where-Object {
                $_ -match '\[INDUS04\.4\] RESULT Archetype='
            }).Count -ne 1 -or -not (Test-Path -LiteralPath $SingleReport)) {
                $Status = 'FAILED'
                $Detail = 'Missing authoritative report/diagnostics'
            } else {
                $ItemReportRows = @(Import-Csv -LiteralPath $SingleReport)
                if ($ItemReportRows.Count -ne 1) {
                    $Status = 'FAILED'
                    $Detail = 'Expected exactly one item result'
                } else {
                    $Status = [string]$ItemReportRows[0].Status
                    if ($Apply) {
                        $Saves = @($Messages | Where-Object {
                            $_ -match '\[INDUS04\.4\] SAVED NarrativeItem'
                        })
                        $Unchanged = @($Messages | Where-Object {
                            $_ -match '\[INDUS04\.4\] PRESENT_SAME NarrativeItem'
                        })
                        if ($Saves.Count -eq 1 -and $Unchanged.Count -eq 0) {
                            $Status = 'SAVED'
                            $AllowedChangedItems.Add($RelativeItem)
                        } elseif ($Unchanged.Count -eq 1 -and $Saves.Count -eq 0) {
                            $Status = 'PRESENT_SAME'
                        } else {
                            $Status = 'FAILED'
                            $Detail = 'Apply not confirmed as SAVED or PRESENT_SAME'
                        }
                    }
                }
            }
        }
        $BatchResults.Add([pscustomobject]@{
            ArchetypeId=$Id; ProductionState=$Spec.ProductionState
            Review=$Review; Status=$Status; ItemPath=$RelativeItem; Detail=$Detail
        })
        Write-Host "[INDUS05] NARRATIVE RESULT Archetype=$Id Status=$Status Review=$Review $Detail"
    }
    $BatchReport = Join-Path $ReportDir 'INDUS05_Narrative_All.csv'
    $BatchResults | Export-Csv -LiteralPath $BatchReport -NoTypeInformation -Encoding UTF8
    $BatchJson = [IO.Path]::ChangeExtension($BatchReport, '.json')
    $BatchResults | ConvertTo-Json -Depth 5 |
        Set-Content -LiteralPath $BatchJson -Encoding UTF8
    $AfterNarrative = @(& git -C $Repo status --porcelain=v1 --untracked-files=all)
    if ($LASTEXITCODE -ne 0) { throw 'Post-Narrative git status failed.' }
    $NewChanges = @($AfterNarrative | Where-Object { $_ -notin $Before })
    $Unexpected = @($NewChanges | Where-Object {
        $_.Length -lt 4 -or $_.Substring(3).Replace('\','/') -notin $AllowedChangedItems
    })
    if ($Unexpected.Count -ne 0) {
        $Unexpected | ForEach-Object { Write-Host $_ }
        throw 'Unexpected changes outside the selected Narrative Item DataAssets.'
    }
    $Failures = @($BatchResults | Where-Object { $_.Status -eq 'FAILED' })
    if ($Apply -and $Failures.Count -eq 0) {
        & (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipBuild -AutomationFilter 'Grimrock.RPG.ARMEMENT.Industrial'
        if ($LASTEXITCODE -ne 0) {
            throw 'Industrial regression after Narrative apply failed; inspect modified assets.'
        }
    }
    Write-Host "[INDUS05] NARRATIVE SUMMARY Approved=$Approved/82 Targets=$($BatchResults.Count) Saved=$(@($BatchResults | Where-Object { $_.Status -eq 'SAVED' }).Count) Failed=$($Failures.Count)"
    Write-Host "Report: $BatchReport"
    & git -C $Repo status --short
    if ($Failures.Count -gt 0) { throw "$($Failures.Count) narrative item(s) failed: inspect the report." }
    return
}

Write-Host '=== INDUS02: editor build ==='
& (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipAutomation
if ($LASTEXITCODE -ne 0) { throw 'UE editor build failed' }
New-Item -ItemType Directory -Force -Path $ReportDir | Out-Null

$Targets = @('ALL')
if ($Archetype) { $Targets = @($Archetype) }
elseif ($Archetypes.Count -gt 0) { $Targets = @($Archetypes | Select-Object -Unique) }

foreach ($Target in $Targets) {
    if ($Target -ne 'ALL' -and $Target -notmatch '^ARM[0-9]{3}$') {
        throw "Invalid archetype: $Target"
    }
    $Suffix = if ($Target -eq 'ALL') { 'All' } else { $Target }
    $Report = Join-Path $ReportDir ('INDUS02_' + $Action + '_' + $Suffix + '.csv')
    $JsonReport = [IO.Path]::ChangeExtension($Report, '.json')
    $Log = Join-Path $ReportDir ('INDUS02_' + $Action + '_' + $Suffix + '.log')
    $ArgsEditor = @(
        $Project, '-run=GridWeaponPipeline', "-Action=$Action",
        "-Manifest=$Manifest", "-Report=$Report",
        '-Unattended', '-NoSplash', '-NoP4', '-NoSound', '-NullRHI', '-log'
    )
    if ($Target -ne 'ALL') { $ArgsEditor += "-Archetype=$Target" }
    if ($Stage) { $ArgsEditor += "-Stage=$Stage" }
    if ($Apply) { $ArgsEditor += '-Apply' }
    Write-Host "=== INDUS02 $Action $Target (Stage=$Stage, Apply=$Apply) ==="
    Write-Host "Log: $Log"
    & $Editor @ArgsEditor *> $Log
    $Code = $LASTEXITCODE
    $Matches = @(Select-String -LiteralPath $Log -Pattern '\[INDUS02\]' -ErrorAction SilentlyContinue)
    $Lines = @($Matches | ForEach-Object { $_.Line })
    $Lines | ForEach-Object { Write-Host $_ }
    if ($Code -ne 0) {
        Get-Content -LiteralPath $Log -Tail 25
        throw "Commandlet failed for $Target (code $Code)"
    }
    $Results = @($Lines | Where-Object { $_ -match '\[INDUS02\] RESULT Archetype=' })
    $Summaries = @($Lines | Where-Object { $_ -match '\[INDUS02\] SUMMARY Action=' })
    if ($Results.Count -eq 0 -or $Summaries.Count -ne 1) {
        throw 'Commandlet diagnostics missing; refusing silent success.'
    }
    if ($Apply) {
        $Applied = @($Lines | Where-Object {
            $_ -match '\[INDUS02\] (SAVED|PRESENT_SAME) (Visual|Item|Palette) for '
        })
        if ($Applied.Count -ne 1) { throw 'Apply was not explicitly confirmed.' }
    }
    if (-not (Test-Path -LiteralPath $Report -PathType Leaf)) {
        throw "Expected report missing: $Report"
    }
    @(Import-Csv -LiteralPath $Report) | ConvertTo-Json -Depth 6 |
        Set-Content -LiteralPath $JsonReport -Encoding UTF8
    Write-Host "CSV : $Report"
    Write-Host "JSON: $JsonReport"
}

if ($Action -eq 'Validate' -or $Apply) {
    & (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipBuild -AutomationFilter 'Grimrock.RPG.ARMEMENT.Industrial'
    if ($LASTEXITCODE -ne 0) { throw 'TD04.2 industrial tests failed; do not publish.' }
}

foreach ($Path in @($RpgCatalogue, $VisualCatalogue, $Palette)) {
    $Allowed = $Apply -and (($Stage -eq 'Visual' -and $Path -eq $VisualCatalogue) -or
        ($Stage -eq 'Palette' -and $Path -eq $Palette))
    if (-not $Allowed -and (Get-Fingerprint $Path) -ne $BeforeHashes[$Path]) {
        throw "Unexpected package change: $Path"
    }
}
$After = @(& git -C $Repo status --porcelain=v1 --untracked-files=all)
if ($LASTEXITCODE -ne 0) { throw 'Post-command git status failed' }
$New = @($After | Where-Object { $_ -notin $Before })
if ($Apply) {
    if ($Stage -eq 'Visual') {
        $Expected = $VisualCatalogue
    } elseif ($Stage -eq 'Palette') {
        $Expected = $Palette
    } else {
        $Rows = @(Import-Csv -LiteralPath $Manifest | Where-Object { $_.ArchetypeId -eq $Archetype })
        if ($Rows.Count -ne 1) { throw "Manifest ID ambiguous: $Archetype" }
        $LongPath = ($Rows[0].ItemPath -split '\.')[0]
        $Expected = 'Content/' + $LongPath.Substring('/Game/'.Length) + '.uasset'
    }
    $Unexpected = @($New | Where-Object {
        $_.Length -lt 4 -or $_.Substring(3).Replace('\','/') -ne $Expected
    })
    if ($Unexpected.Count -gt 0) {
        $Unexpected | ForEach-Object { Write-Host $_ }
        throw 'Unrelated files changed. Do not stage/commit.'
    }
} elseif ($New.Count -gt 0) {
    $New | ForEach-Object { Write-Host $_ }
    throw 'Read-only operation changed Content or source files.'
}
Write-Host '=== Git status (no auto-stage / commit / push) ==='
& git -C $Repo status --short --untracked-files=all
Write-Host '[OK] INDUS02 completed. PIE acceptance remains separate.'
