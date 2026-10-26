# ARMEMENT-INDUS04.4: one approved narrative description -> one ItemDefinition package.
# Never approves source data and never touches UMG/StringTables or RPG catalogues.
[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\UE_5.5',
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^ARM0(?:[0-7][0-9]|8[0-2])$')]
    [string]$Archetype,
    [switch]$Apply
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$Repo = Split-Path -Parent $PSScriptRoot
$ManifestPath = Join-Path $Repo 'Config\Armement\WeaponProductionManifest.csv'
$CsvPath = Join-Path $Repo 'Saved\ARMEMENT-INDUS\INDUS03_Narrative.csv'
$ReportDir = Join-Path $Repo 'Saved\ARMEMENT-INDUS'
$Report = Join-Path $ReportDir ('INDUS04_4_Narrative_' + $Archetype + '.csv')
$Log = Join-Path $ReportDir ('INDUS04_4_Narrative_' + $Archetype + '.log')
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Project = Join-Path $Repo 'GrimrockPrototype.uproject'

if ((& git -C $Repo branch --show-current).Trim() -ne 'master' -or $LASTEXITCODE -ne 0) {
    throw 'ARMEMENT-INDUS04.4 requires master.'
}
if (-not (Test-Path -LiteralPath $Editor -PathType Leaf)) {
    throw "Missing UnrealEditor-Cmd.exe: $Editor"
}
$Specs = @(Import-Csv -LiteralPath $ManifestPath |
    Where-Object { $_.ArchetypeId -eq $Archetype })
if ($Specs.Count -ne 1) { throw "No unique production manifest entry for $Archetype" }
$ItemPath = [string]$Specs[0].ItemPath
$PathMatch = [regex]::Match($ItemPath, '^(/Game/[A-Za-z0-9_/]+)\.([A-Za-z0-9_]+)$')
if (-not $ItemPath.StartsWith('/Game/GrimrockPrototype/') -or $ItemPath.Contains('..') -or
    $ItemPath.Contains('\') -or -not $PathMatch.Success -or
    -not $PathMatch.Groups[1].Value.EndsWith('/' + $PathMatch.Groups[2].Value)) {
    throw 'Unsafe ItemPath in manifest.'
}
$RelativeItem = 'Content/' + $PathMatch.Groups[1].Value.Substring('/Game/'.Length) + '.uasset'
$ItemFile = Join-Path $Repo ($RelativeItem.Replace('/', '\'))
if (-not (Test-Path -LiteralPath $ItemFile -PathType Leaf)) {
    throw "Item DataAsset has not been authored yet: $RelativeItem"
}
$Before = @(& git -C $Repo status --porcelain=v1 --untracked-files=all)
if ($LASTEXITCODE -ne 0) { throw 'Cannot read git status' }
$DirtyItem = @(& git -C $Repo status --porcelain=v1 -- $RelativeItem)
if ($Apply -and $DirtyItem.Count -ne 0) {
    throw "Refusing to overwrite a locally modified DataAsset: $RelativeItem"
}
$HashBefore = (Get-FileHash -LiteralPath $ItemFile -Algorithm SHA256).Hash

# Always regenerate from the canonical 13-sheet workbook; DRAFT is not authorization.
& (Join-Path $PSScriptRoot 'ExportARMEMENT014Catalogue.ps1') -RequireNarrative
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $CsvPath -PathType Leaf)) {
    throw 'Canonical NarrativeV1 export failed.'
}
$Rows = @(Import-Csv -LiteralPath $CsvPath)
if ($Rows.Count -ne 82 -or @($Rows | Where-Object { $_.SchemaVersion -ne 'NarrativeV1' }).Count -ne 0) {
    throw 'Canonical NarrativeV1 requires 82 valid rows.'
}
$MatchesRow = @($Rows | Where-Object { $_.ArchetypeId -eq $Archetype })
if ($MatchesRow.Count -ne 1) { throw "Narrative row missing/ambiguous for $Archetype" }
$Row = $MatchesRow[0]
if ($Apply -and ($Row.EtatValidation -ne 'APPROVED' -or
        [string]::IsNullOrWhiteSpace($Row.DescriptionCourteFR))) {
    throw "$Archetype is $($Row.EtatValidation): approve its short description in the XLSX first. No write."
}

# Build is mandatory for commandlet changes to be recognized by the local editor.
& (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipAutomation
if ($LASTEXITCODE -ne 0) { throw 'Development Editor build failed.' }

New-Item -ItemType Directory -Force -Path $ReportDir | Out-Null
$Params = @(
    $Project, '-run=GridWeaponPipeline', '-Action=NarrativeItem',
    "-Archetype=$Archetype", "-Manifest=$ManifestPath",
    "-Narrative=$CsvPath", "-Report=$Report",
    '-Unattended', '-NoSplash', '-NoP4', '-NoSound', '-NullRHI', '-log'
)
if ($Apply) { $Params += '-Apply' }
& $Editor @Params *> $Log
$Exit = $LASTEXITCODE
$Lines = @(Select-String -LiteralPath $Log -Pattern '\[INDUS04\.4\]' |
    ForEach-Object { $_.Line })
$Lines | ForEach-Object { Write-Host $_ }
if ($Exit -ne 0) {
    Get-Content -LiteralPath $Log -Tail 30
    throw "Narrative commandlet refused $Archetype (code $Exit)."
}
if (-not (Test-Path -LiteralPath $Report -PathType Leaf) -or
    @($Lines | Where-Object { $_ -match '\[INDUS04\.4\] RESULT Archetype=' }).Count -ne 1) {
    throw 'Narrative authoring did not produce exactly one report.'
}
$After = @(& git -C $Repo status --porcelain=v1 --untracked-files=all)
if ($LASTEXITCODE -ne 0) { throw 'Post-run git status failed.' }
$New = @($After | Where-Object { $_ -notin $Before })
$Unexpected = @($New | Where-Object {
    $_.Length -lt 4 -or $_.Substring(3).Replace('\','/') -ne $RelativeItem
})
if ($Unexpected.Count -ne 0) {
    $Unexpected | ForEach-Object { Write-Host $_ }
    throw 'Unrelated tracked/untracked files changed: do not commit.'
}
if (-not $Apply -and (Get-FileHash -LiteralPath $ItemFile -Algorithm SHA256).Hash -ne $HashBefore) {
    throw 'Dry-run unexpectedly modified the item.'
}
if ($Apply) {
    $Applied = @($Lines | Where-Object {
        $_ -match '\[INDUS04\.4\] (SAVED|PRESENT_SAME) NarrativeItem for '
    })
    if ($Applied.Count -ne 1) { throw 'Apply not confirmed by commandlet.' }
}
Write-Host "Report: $Report"
Write-Host "=== Git status (never auto-stage or commit) ==="
& git -C $Repo status --short
Write-Host "[OK] INDUS04.4 completed; PIE and explicit asset commit still required."
