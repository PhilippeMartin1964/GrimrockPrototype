<#
.SYNOPSIS
  ARM-CLEAN03.1 — Audit strict de la compressibilite des 82 progressions N1-N20.
.DESCRIPTION
  Analyse le CSV canonique issu de ExportARMEMENT014Catalogue.ps1.
  Pour chaque statistique, cherche la meilleure regle :
      Valeur(L) = N1 + floor((L - 1) / StepEvery) * StepAmount
  + exceptions ponctuelles absolues. Reconstitue et compare toutes les valeurs.
  Lecture seule sur les DataAssets et sur le classeur ; le rapport va dans Saved/.
  Ne convertit PAS le .uasset avant validation UE locale.
.EXAMPLE
  .\Scripts\AnalyzeARMWeaponProgression.ps1 -ExportFirst
#>
[CmdletBinding()]
param(
    [string] $SourceCsv = '',
    [string] $ReportCsv = '',
    [switch] $ExportFirst
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if ([string]::IsNullOrWhiteSpace($SourceCsv)) {
    $SourceCsv = Join-Path $Root 'Saved\ARMEMENT01\ARMEMENT01_4_Profiles.csv'
}
if ([string]::IsNullOrWhiteSpace($ReportCsv)) {
    $ReportCsv = Join-Path $Root 'Saved\ARMEMENT01\CLEAN03_GrowthRules.csv'
}
if ($ExportFirst) {
    & (Join-Path $PSScriptRoot 'ExportARMEMENT014Catalogue.ps1')
    # The exporter is a PowerShell script: let terminating errors propagate.
    # LASTEXITCODE is only defined for external processes and is not applicable.
}
if (-not (Test-Path -LiteralPath $SourceCsv -PathType Leaf)) {
    throw "CLEAN03: CSV absent : $SourceCsv. Lancez d'abord -ExportFirst."
}

$Culture = [System.Globalization.CultureInfo]::InvariantCulture
$Stats = @('DiceCount', 'DiceSides', 'FlatBonus', 'PhysicalArmor', 'MagicalArmor')
$Invariants = @('DisplayName', 'Family', 'HandUsage', 'AccuracyBonus',
    'DamageType', 'PhysicalSubtype', 'ScalingAttribute', 'ActionPointCost',
    'RangeCells', 'WeightKg')
$Rows = @(Import-Csv -LiteralPath $SourceCsv -Encoding UTF8)
if ($Rows.Count -ne 1640) { throw "CLEAN03: 1640 lignes attendues ; trouvees : $($Rows.Count)." }

function Read-Number([object]$Row, [string]$Column) {
    return [int]::Parse([string]$Row.$Column, $Culture)
}

function Build-Rule([object[]]$LevelRows, [string]$Stat) {
    $Baseline = Read-Number $LevelRows[0] $Stat
    $Best = $null
    for ($Interval = 1; $Interval -le 20; ++$Interval) {
        $Candidates = New-Object 'System.Collections.Generic.HashSet[int]'
        [void] $Candidates.Add(0)
        for ($Index = 1; $Index -lt 20; ++$Index) {
            $Steps = [int][math]::Floor($Index / $Interval)
            if ($Steps -eq 0) { continue }
            $Diff = (Read-Number $LevelRows[$Index] $Stat) - $Baseline
            if (($Diff % $Steps) -eq 0) {
                [void] $Candidates.Add([int]($Diff / $Steps))
            }
        }
        foreach ($Amount in $Candidates) {
            $Exceptions = New-Object System.Collections.ArrayList
            for ($Index = 1; $Index -lt 20; ++$Index) {
                $Level = $Index + 1
                $Expected = $Baseline + [int][math]::Floor($Index / $Interval) * $Amount
                $Actual = Read-Number $LevelRows[$Index] $Stat
                if ($Expected -ne $Actual) {
                    [void] $Exceptions.Add([pscustomobject]@{ Level = $Level; Value = $Actual })
                }
            }
            $Candidate = [pscustomobject]@{
                BaseValue = $Baseline
                StepEveryLevels = $Interval
                StepAmount = [int]$Amount
                Exceptions = @($Exceptions.ToArray())
            }
            if ($null -eq $Best -or
                $Candidate.Exceptions.Count -lt $Best.Exceptions.Count -or
                ($Candidate.Exceptions.Count -eq $Best.Exceptions.Count -and
                 [math]::Abs($Candidate.StepAmount) -lt [math]::Abs($Best.StepAmount))) {
                $Best = $Candidate
            }
        }
    }
    if ($null -eq $Best) { throw "CLEAN03: aucune regle possible pour $Stat." }
    return $Best
}

$Output = New-Object System.Collections.ArrayList
$ExceptionsTotal = 0
$Seen = New-Object 'System.Collections.Generic.HashSet[string]'
$Groups = @($Rows | Group-Object -Property ArchetypeId | Sort-Object -Property Name)
if ($Groups.Count -ne 82) { throw "CLEAN03: 82 archetypes attendus ; trouves : $($Groups.Count)." }

foreach ($Group in $Groups) {
    $Id = [string]$Group.Name
    if ($Id -notmatch '^ARM(0[0-7][0-9]|08[0-2])$') {
        throw "CLEAN03: identifiant inconnu $Id."
    }
    $Ordered = @($Group.Group | Sort-Object { [int]$_.ItemLevel })
    if ($Ordered.Count -ne 20) { throw "CLEAN03: $Id n'a pas 20 niveaux." }
    for ($Index = 0; $Index -lt 20; ++$Index) {
        if ((Read-Number $Ordered[$Index] 'ItemLevel') -ne ($Index + 1)) {
            throw "CLEAN03: niveau absent/duplique pour $Id."
        }
        $Key = "$Id/$($Index + 1)"
        if (-not $Seen.Add($Key)) { throw "CLEAN03: doublon $Key." }
        foreach ($Property in $Invariants) {
            if ([string]$Ordered[$Index].$Property -cne [string]$Ordered[0].$Property) {
                throw "CLEAN03: invariant $Property differe a $Key."
            }
        }
    }
    foreach ($Stat in $Stats) {
        $Rule = Build-Rule $Ordered $Stat
        $ExceptionsTotal += $Rule.Exceptions.Count
        $ByLevel = @{}
        foreach ($E in $Rule.Exceptions) { $ByLevel[[int]$E.Level] = [int]$E.Value }
        for ($Index = 0; $Index -lt 20; ++$Index) {
            $Level = $Index + 1
            $Calculated = $Rule.BaseValue +
                [int][math]::Floor($Index / $Rule.StepEveryLevels) * $Rule.StepAmount
            if ($ByLevel.ContainsKey($Level)) { $Calculated = $ByLevel[$Level] }
            $Actual = Read-Number $Ordered[$Index] $Stat
            if ($Calculated -ne $Actual) {
                throw "CLEAN03: non-parite $Id N$Level $Stat : $Calculated != $Actual."
            }
        }
        [void] $Output.Add([pscustomobject]@{
            ArchetypeId = $Id
            Stat = $Stat
            BaseValue = $Rule.BaseValue
            StepEveryLevels = $Rule.StepEveryLevels
            StepAmount = $Rule.StepAmount
            ExceptionsCount = $Rule.Exceptions.Count
            ExceptionsJson = (ConvertTo-Json -InputObject @($Rule.Exceptions) -Compress -Depth 4)
        })
    }
}

$Folder = Split-Path -Parent $ReportCsv
if (-not (Test-Path -LiteralPath $Folder)) {
    New-Item -Path $Folder -ItemType Directory -Force | Out-Null
}
$Output | Export-Csv -LiteralPath $ReportCsv -NoTypeInformation -Encoding UTF8
$NaiveScalarValues = 82 * 20 * $Stats.Count
$CompactRuleNumbers = 82 * $Stats.Count * 3 + $ExceptionsTotal * 2
Write-Host ("[ARM-CLEAN03] Parite exacte : {0} profils, {1} series, {2} valeurs scalaires." -f 1640, $Output.Count, $NaiveScalarValues)
Write-Host ("[ARM-CLEAN03] Exceptions : {0}; nombres modelises : {1} (estimation structurelle, pas taille uasset)." -f $ExceptionsTotal, $CompactRuleNumbers)
Write-Host "[ARM-CLEAN03] Rapport : $ReportCsv"
