[CmdletBinding()]
param(
    [switch]$NoThrow
)

# ARMEMENT-LOT02.11: read-only intake of 11 canonical Meshy imports.
# Presence is not UE validity. Never edits .uasset, manifest, Excel or Git index.
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$Repo = Split-Path -Parent $PSScriptRoot
$Registry = Join-Path $Repo 'docs\Design\ARMEMENT_LOT02_11_ART_INTAKE.csv'
$OutputDir = Join-Path $Repo 'Saved\ARMEMENT-LOT02'
$Report = Join-Path $OutputDir 'LOT02_11_ArtIntake.csv'

$Expected = @('ARM038','ARM048','ARM054','ARM057','ARM058',
              'ARM061','ARM062','ARM067','ARM069','ARM070','ARM081')
$Fields = @('MeshFile','IconFile','MaterialFile',
            'BaseColorFile','NormalFile','OrmFile')
if (-not (Test-Path -LiteralPath $Registry -PathType Leaf)) {
    throw "Missing intake registry: $Registry"
}
$Rows = @(Import-Csv -LiteralPath $Registry)
if ($Rows.Count -ne $Expected.Count) {
    throw "Expected exactly 11 art intake records. Got $($Rows.Count)."
}
$ById = @{}
$UsedPaths = @{}
foreach ($Row in $Rows) {
    if ($Expected -notcontains $Row.ArchetypeId -or $ById.ContainsKey($Row.ArchetypeId)) {
        throw "Duplicate or unapproved intake archetype: $($Row.ArchetypeId)"
    }
    $ById[$Row.ArchetypeId] = $Row
    foreach ($Field in $Fields) {
        $Relative = $Row.$Field
        if ([string]::IsNullOrWhiteSpace($Relative) -or
            $Relative -notmatch '^Content/GrimrockPrototype/[A-Za-z0-9_./-]+\.uasset$' -or
            $Relative.Contains('..') -or $UsedPaths.ContainsKey($Relative)) {
            throw "Invalid or duplicate art path for $($Row.ArchetypeId) : $Field : $Relative"
        }
        $UsedPaths[$Relative] = $Row.ArchetypeId
    }
}
foreach ($Id in $Expected) {
    if (-not $ById.ContainsKey($Id)) { throw "Missing art intake archetype: $Id" }
}

$Results = @(
    foreach ($Id in $Expected) {
        $Row = $ById[$Id]
        $Missing = @()
        $Present = 0
        foreach ($Field in $Fields) {
            $Relative = $Row.$Field
            $Local = Join-Path $Repo ($Relative -replace '/', '\')
            if (Test-Path -LiteralPath $Local -PathType Leaf) {
                $Present++
            } else {
                $Missing += "$Field=$Relative"
            }
        }
        $MaterialRelative = $Row.MaterialFile
        [pscustomobject]@{
            ArchetypeId = $Id
            Slug = $Row.Slug
            Found = $Present
            Required = $Fields.Count
            Status = if ($Missing.Count -eq 0) { 'FILES_PRESENT_UE_UNVERIFIED' } else { 'MISSING_FILES' }
            Missing = $Missing -join ' | '
            Material = $MaterialRelative
        }
    }
)
New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
$Results | Export-Csv -LiteralPath $Report -NoTypeInformation -Encoding UTF8
$Results | Format-Table ArchetypeId,Slug,Found,Required,Status -AutoSize
$NotReady = @($Results | Where-Object { $_.Status -eq 'MISSING_FILES' })
Write-Host "[LOT02.11] Intake report: $Report"
Write-Host ("[LOT02.11] CompleteFilesets={0}/11 Packages={1}/66 Missing={2}" -f
    (11 - $NotReady.Count), (($Results | Measure-Object -Property Found -Sum).Sum),
    (66 - (($Results | Measure-Object -Property Found -Sum).Sum)))
foreach ($Result in $NotReady) {
    Write-Warning ("{0} missing: {1}" -f $Result.ArchetypeId, $Result.Missing)
}
Write-Host '[LOT02.11] File existence only; UE loadability, material slot, texture flags and icon dimensions remain unverified.'
if ($NotReady.Count -gt 0 -and -not $NoThrow) {
    throw "Incomplete Meshy art intake: $($NotReady.Count) weapons. See CSV report."
}
