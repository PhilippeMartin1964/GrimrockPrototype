param(
    [string]$WorkbookPath = 'docs\Design\ARMEMENT01_Catalogue_Armes_Boucliers_Niveaux_1_20_ARMEMENT01_2.xlsx',
    [string]$AffinityOutputPath = 'Saved\ARMEMENT01\ARMEMENT01_9_5_2_Affinities.csv',
    [string]$ClassOutputPath = 'Saved\ARMEMENT01\ARMEMENT01_9_5_2_ClassPreferences.csv'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$RepoRoot = Split-Path -Parent $PSScriptRoot

# Reuse the proven ARMEMENT01.4 XLSX reader instead of creating a second
# independent workbook interpretation. Also regenerates normalized profiles.
. (Join-Path $PSScriptRoot 'ExportARMEMENT014Catalogue.ps1') -WorkbookPath $WorkbookPath

foreach ($Name in @('AffinityOutputPath', 'ClassOutputPath')) {
    $Value = Get-Variable -Name $Name -ValueOnly
    if (-not [System.IO.Path]::IsPathRooted($Value)) {
        Set-Variable -Name $Name -Value (Join-Path $RepoRoot $Value)
    }
}

$ClassIds = @{
    'Guerrier'='Warrior'; 'Voleur'='Rogue'; 'Rôdeur'='Ranger'
    'Mage'='Mage'; 'Prêtre'='Priest'; 'Alchimiste'='Alchemist'
}
$Archive = [System.IO.Compression.ZipFile]::OpenRead($WorkbookPath)
try {
    $StringsXml = Get-ZipXml $Archive 'xl/sharedStrings.xml'
    $StringNs = New-XlsxNamespace $StringsXml
    $Shared = @($StringsXml.SelectNodes('//x:si', $StringNs) | ForEach-Object {
        ($_.SelectNodes('.//x:t', $StringNs) | ForEach-Object { $_.InnerText }) -join ''
    })

    $CatalogueRows = @(Get-WorksheetRows $Archive 'xl/worksheets/sheet2.xml' $Shared |
        Where-Object { $_.Number -ge 4 -and $_.Number -le 85 })
    if ($CatalogueRows.Count -ne 82) { throw 'ARMEMENT01.9.5.2 : 82 archétypes requis.' }
    $Affinities = [System.Collections.Generic.List[object]]::new()
    $SeenArchetypes = @{}
    foreach ($Row in $CatalogueRows) {
        $Id = Get-RequiredCell $Row.Cells 'A' "Catalogue:$($Row.Number)"
        if ($SeenArchetypes.ContainsKey($Id)) { throw "Archétype dupliqué : $Id" }
        $SeenArchetypes[$Id] = $true
        $PrimaryLabel = Get-RequiredCell $Row.Cells 'P' "Catalogue:$Id"
        if (-not $ClassIds.ContainsKey($PrimaryLabel)) {
            throw "Classe principale inconnue pour $Id : $PrimaryLabel"
        }
        $SecondaryRaw = [string]$Row.Cells['Q']
        $SecondaryIds = [System.Collections.Generic.List[string]]::new()
        foreach ($Fragment in @($SecondaryRaw -split '\s*/\s*')) {
            if ([string]::IsNullOrWhiteSpace($Fragment)) { continue }
            # Keep the exact source qualifier, e.g. Prêtre (marteaux),
            # separately; it must not become an equip restriction.
            $Label = ($Fragment -replace '\s*\([^)]*\)\s*$', '').Trim()
            if (-not $ClassIds.ContainsKey($Label)) {
                throw "Classe secondaire inconnue pour $Id : $Fragment"
            }
            $ClassId = $ClassIds[$Label]
            if ($ClassId -eq $ClassIds[$PrimaryLabel] -or $SecondaryIds.Contains($ClassId)) {
                throw "Affinité secondaire invalide pour $Id : $Fragment"
            }
            $SecondaryIds.Add($ClassId)
        }
        $Affinities.Add([PSCustomObject][ordered]@{
            ArchetypeId = $Id
            PrimaryClassId = $ClassIds[$PrimaryLabel]
            SecondaryClassIds = ($SecondaryIds -join ';')
            SecondarySourceLabel = $SecondaryRaw
        })
    }

    $ClassesRows = @(Get-WorksheetRows $Archive 'xl/worksheets/sheet5.xml' $Shared |
        Where-Object { $_.Number -ge 4 -and $_.Number -le 9 })
    if ($ClassesRows.Count -ne 6) {
        throw "ARMEMENT01.9.5.2 : 6 classes requises, reçu $($ClassesRows.Count)"
    }
    $Preferences = [System.Collections.Generic.List[object]]::new()
    $SeenClasses = @{}
    foreach ($Row in $ClassesRows) {
        $Name = Get-RequiredCell $Row.Cells 'A' "Classes:$($Row.Number)"
        if (-not $ClassIds.ContainsKey($Name) -or $SeenClasses.ContainsKey($Name)) {
            throw "Classe inconnue ou dupliquée : $Name"
        }
        $SeenClasses[$Name] = $true
        $Preferences.Add([PSCustomObject][ordered]@{
            ClassId = $ClassIds[$Name]
            Branches = (Get-RequiredCell $Row.Cells 'B' "Classes:$Name")
            PriorityWeapons = (Get-RequiredCell $Row.Cells 'C' "Classes:$Name")
            Examples = (Get-RequiredCell $Row.Cells 'D' "Classes:$Name")
            Alternatives = (Get-RequiredCell $Row.Cells 'E' "Classes:$Name")
            DominantAttributes = (Get-RequiredCell $Row.Cells 'F' "Classes:$Name")
            Synergies = (Get-RequiredCell $Row.Cells 'G' "Classes:$Name")
        })
    }
} finally {
    $Archive.Dispose()
}

[System.IO.Directory]::CreateDirectory((Split-Path -Parent $AffinityOutputPath)) | Out-Null
[System.IO.Directory]::CreateDirectory((Split-Path -Parent $ClassOutputPath)) | Out-Null
$Affinities.ToArray() | Export-Csv -LiteralPath $AffinityOutputPath -NoTypeInformation -Encoding UTF8
$Preferences.ToArray() | Export-Csv -LiteralPath $ClassOutputPath -NoTypeInformation -Encoding UTF8
Write-Host "[ARMEMENT01.9.5.2] Source XLSX : $WorkbookPath"
Write-Host "[OK] Affinités 82 archetypes et préférences de 6 classes générées."
Write-Host "Affinités : $AffinityOutputPath"
Write-Host "Classes   : $ClassOutputPath"
