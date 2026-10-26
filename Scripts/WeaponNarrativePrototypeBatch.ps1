# ARMEMENT-INDUS04.4B — one-shot prototype editorial approval and batch import.
# Source authority remains the canonical XLSX. Never silently rewrites combat data.
[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\UE_5.5',
    [switch]$ApproveAll,
    [switch]$Apply
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$Repo = Split-Path -Parent $PSScriptRoot
$Workbook = Join-Path $Repo 'docs\Design\ARMEMENT01_Catalogue_Armes_Boucliers_Niveaux_1_20_ARMEMENT01_2.xlsx'
$Manifest = Join-Path $Repo 'Config\Armement\WeaponProductionManifest.csv'
$ReportDir = Join-Path $Repo 'Saved\ARMEMENT-INDUS'
$Narrative = Join-Path $ReportDir 'INDUS03_Narrative.csv'
$Report = Join-Path $ReportDir 'INDUS04_4B_PrototypeBatch.csv'
$Single = Join-Path $PSScriptRoot 'WeaponNarrativeApply.ps1'
$Export = Join-Path $PSScriptRoot 'ExportARMEMENT014Catalogue.ps1'

if ((& git -C $Repo branch --show-current).Trim() -ne 'master' -or $LASTEXITCODE -ne 0) {
    throw 'Prototype narrative batch requires master.'
}
if (-not (Test-Path -LiteralPath $Workbook -PathType Leaf) -or
    -not (Test-Path -LiteralPath $Single -PathType Leaf) -or
    -not (Test-Path -LiteralPath $Manifest -PathType Leaf)) {
    throw 'Canonical workbook, manifest, or single-item importer missing.'
}
New-Item -ItemType Directory -Force -Path $ReportDir | Out-Null

if ($ApproveAll) {
    # Use installed Excel to preserve the workbook's existing 13 sheets, formats,
    # formulas and cell encoding. Make an isolated backup before any change.
    $Excel = $null
    $Book = $null
    $Sheet = $null
    try {
        $Excel = New-Object -ComObject Excel.Application
        $Excel.Visible = $false
        $Excel.DisplayAlerts = $false
        # Back up the exact original bytes before Excel opens the workbook.
        $BackupDir = Join-Path $ReportDir 'Backups'
        New-Item -ItemType Directory -Force -Path $BackupDir | Out-Null
        $Backup = Join-Path $BackupDir ('Narratif_before_prototype_approval_' +
            (Get-Date -Format 'yyyyMMdd_HHmmss_ffff') + '.xlsx')
        Copy-Item -LiteralPath $Workbook -Destination $Backup -ErrorAction Stop
        $Book = $Excel.Workbooks.Open($Workbook)
        if ($Book.ReadOnly) {
            throw 'The XLSX is read-only or opened elsewhere. Close Excel before approval.'
        }
        $Sheet = $Book.Worksheets.Item('Narratif')
        if ([string]$Sheet.Cells.Item(1, 1).Value2 -cne 'ArchetypeId' -or
            [string]$Sheet.Cells.Item(1, 7).Value2 -cne 'EtatValidation') {
            throw 'Narratif A1/G1 header mismatch; refusing workbook write.'
        }
        $Seen = New-Object 'System.Collections.Generic.HashSet[string]'
        for ($Index = 2; $Index -le 83; $Index++) {
            $Id = [string]$Sheet.Cells.Item($Index, 1).Value2
            $Review = [string]$Sheet.Cells.Item($Index, 7).Value2
            if ($Id -notmatch '^ARM[0-9]{3}$' -or
                [int]$Id.Substring(3) -lt 1 -or [int]$Id.Substring(3) -gt 82 -or
                -not $Seen.Add($Id) -or
                $Review -notin @('DRAFT', 'NEEDS_REVIEW', 'APPROVED') -or
                [string]::IsNullOrWhiteSpace([string]$Sheet.Cells.Item($Index, 2).Value2)) {
                throw "Narratif row $Index is not an eligible, unique, described archetype."
            }
        }
        if ($Seen.Count -ne 82) { throw 'Expected exactly 82 canonical archetypes.' }
        $Sheet.Range('G2:G83').Value2 = 'APPROVED'
        $Book.Save()
        Write-Host "[INDUS04.4B] APPROVED=82 Source=$Workbook Backup=$Backup"
    }
    finally {
        if ($null -ne $Book) {
            $Book.Close($false)
            [void][Runtime.InteropServices.Marshal]::ReleaseComObject($Book)
        }
        if ($null -ne $Sheet) {
            [void][Runtime.InteropServices.Marshal]::ReleaseComObject($Sheet)
        }
        if ($null -ne $Excel) {
            $Excel.Quit()
            [void][Runtime.InteropServices.Marshal]::ReleaseComObject($Excel)
        }
    }
}

# Always re-export the SINGLE canonical workbook (not the historical DRAFT CSV).
& $Export -RequireNarrative
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $Narrative -PathType Leaf)) {
    throw 'NarrativeV1 export failed.'
}
$Rows = @(Import-Csv -LiteralPath $Narrative)
$Approved = @($Rows | Where-Object {
    $_.SchemaVersion -eq 'NarrativeV1' -and
    $_.EtatValidation -eq 'APPROVED' -and
    -not [string]::IsNullOrWhiteSpace($_.DescriptionCourteFR)
})
if ($Rows.Count -ne 82) { throw "Expected 82 narrative rows; found $($Rows.Count)." }
if ($Apply -and $Approved.Count -ne 82) {
    throw "Import refused: $($Approved.Count)/82 APPROVED. Run -ApproveAll (Excel must be closed) first."
}

$Entries = @(Import-Csv -LiteralPath $Manifest)
$Active = @($Entries | Where-Object { $_.ProductionState -eq 'Active' } |
    Sort-Object ArchetypeId)
$Output = [System.Collections.Generic.List[object]]::new()
foreach ($Entry in $Active) {
    $Id = [string]$Entry.ArchetypeId
    $Row = @($Rows | Where-Object { $_.ArchetypeId -eq $Id })
    if ($Row.Count -ne 1) { throw "Narrative row missing/duplicated for $Id." }
    $Path = [string]$Entry.ItemPath
    $Match = [regex]::Match($Path, '^(/Game/[A-Za-z0-9_/]+)\.([A-Za-z0-9_]+)$')
    if (-not $Match.Success -or
        -not $Match.Groups[1].Value.StartsWith('/Game/GrimrockPrototype/') -or
        -not $Match.Groups[1].Value.EndsWith('/' + $Match.Groups[2].Value)) {
        throw "Invalid item object path for $Id."
    }
    $ItemRelative = 'Content/' + $Match.Groups[1].Value.Substring('/Game/'.Length) + '.uasset'
    $ItemFile = Join-Path $Repo ($ItemRelative.Replace('/', '\'))
    $Status = 'READY_FOR_AUDIT'
    $Detail = ''
    if (-not (Test-Path -LiteralPath $ItemFile -PathType Leaf)) {
        $Status = 'SKIPPED_NOT_CREATED'
    }
    elseif (@(& git -C $Repo status --porcelain=v1 -- $ItemRelative).Count -ne 0) {
        # Includes ARM031 already imported by the user: never overwrite it.
        $Status = 'SKIPPED_DIRTY'
        $Detail = 'Local asset modification preserved; inspect/commit separately'
    }
    elseif ($Row[0].EtatValidation -ne 'APPROVED') {
        $Status = 'SKIPPED_REVIEW'
    }
    elseif ($Apply) {
        try {
            # Existing strict importer: verifies item identity and existing text,
            # writes at most ONE approved DataAsset per commandlet invocation.
            & $Single -EngineRoot $EngineRoot -Archetype $Id -Apply | Out-Null
            if ($LASTEXITCODE -ne 0) {
                throw "Importer exit code $LASTEXITCODE"
            }
            $ItemLog = Join-Path $ReportDir ('INDUS04_4_Narrative_' + $Id + '.log')
            $Outcome = @(Select-String -LiteralPath $ItemLog -Pattern '\[INDUS04\.4\] (SAVED|PRESENT_SAME) NarrativeItem for ' |
                ForEach-Object { $_.Matches[0].Groups[1].Value })
            if ($Outcome.Count -ne 1) {
                throw "Could not distinguish SAVED/PRESENT_SAME in $ItemLog"
            }
            $Status = if ($Outcome[0] -eq 'SAVED') { 'SAVED' } else { 'PRESENT_SAME' }
        }
        catch {
            $Status = 'FAILED'
            $Detail = $_.Exception.Message
        }
    }
    $Output.Add([pscustomobject]@{
        ArchetypeId = $Id
        Review = $Row[0].EtatValidation
        Status = $Status
        ItemPath = $ItemRelative
        Detail = $Detail
    })
    Write-Host "[INDUS04.4B] $Id : $Status $Detail"
}
$Output | Export-Csv -LiteralPath $Report -NoTypeInformation -Encoding UTF8
$Failures = @($Output | Where-Object { $_.Status -eq 'FAILED' })
$Dirty = @($Output | Where-Object { $_.Status -eq 'SKIPPED_DIRTY' })
$Saved = @($Output | Where-Object { $_.Status -eq 'SAVED' })
$Unchanged = @($Output | Where-Object { $_.Status -eq 'PRESENT_SAME' })
Write-Host "[INDUS04.4B] SUMMARY Approved=$($Approved.Count)/82 Active=$($Active.Count) Saved=$($Saved.Count) PresentSame=$($Unchanged.Count) PreservedDirty=$($Dirty.Count) Failed=$($Failures.Count)"
Write-Host "Report : $Report"
Write-Host '=== Existing local modifications preserved — do not git add -A ==='
& git -C $Repo status --short
if ($Failures.Count -ne 0) {
    throw "$($Failures.Count) active narrative items failed; inspect report and the individual import logs."
}
if ($Apply -and $Approved.Count -ne 82) { throw 'All narrative rows not approved.' }
Write-Host '[OK] INDUS04.4B completed; binary asset publication and PIE remain explicit.'
