param(
    [string]$WorkbookPath = 'docs\Design\ARMEMENT01_Catalogue_Armes_Boucliers_Niveaux_1_20_ARMEMENT01_2.xlsx',
    [string]$OutputPath = 'Saved\ARMEMENT01\ARMEMENT01_4_Profiles.csv'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

$RepoRoot = Split-Path -Parent $PSScriptRoot
if (-not [System.IO.Path]::IsPathRooted($WorkbookPath)) {
    $WorkbookPath = Join-Path $RepoRoot $WorkbookPath
}
if (-not [System.IO.Path]::IsPathRooted($OutputPath)) {
    $OutputPath = Join-Path $RepoRoot $OutputPath
}
if (-not (Test-Path -LiteralPath $WorkbookPath -PathType Leaf)) {
    throw "ARMEMENT01.4 : fichier de référence introuvable : $WorkbookPath"
}

function Get-ZipXml {
    param([System.IO.Compression.ZipArchive]$Archive, [string]$EntryPath)
    $Entry = $Archive.GetEntry($EntryPath)
    if ($null -eq $Entry) { throw "XLSX invalide : $EntryPath absent." }
    $Stream = $Entry.Open()
    try {
        $Reader = [System.IO.StreamReader]::new($Stream, [System.Text.Encoding]::UTF8)
        try {
            $Document = [System.Xml.XmlDocument]::new()
            $Document.LoadXml($Reader.ReadToEnd())
            # Preserve the XmlDocument as one object when returned through the pipeline.
            return ,$Document
        } finally { $Reader.Dispose() }
    } finally { $Stream.Dispose() }
}

function New-XlsxNamespace {
    param([System.Xml.XmlDocument]$Document)
    $Manager = [System.Xml.XmlNamespaceManager]::new($Document.NameTable)
    $Manager.AddNamespace('x', 'http://schemas.openxmlformats.org/spreadsheetml/2006/main')
    # XmlNamespaceManager is IEnumerable; prevent PowerShell from unrolling it.
    return ,$Manager
}

function Get-WorksheetRows {
    param(
        [System.IO.Compression.ZipArchive]$Archive,
        [string]$SheetEntry,
        [string[]]$Shared
    )
    $Xml = Get-ZipXml $Archive $SheetEntry
    $Ns = New-XlsxNamespace $Xml
    $Rows = [System.Collections.Generic.List[object]]::new()
    foreach ($Row in $Xml.SelectNodes('//x:sheetData/x:row', $Ns)) {
        $Record = @{}
        foreach ($Cell in $Row.SelectNodes('./x:c', $Ns)) {
            $Address = $Cell.GetAttribute('r')
            if ($Address -notmatch '^([A-Z]+)[0-9]+$') { continue }
            $Column = $Matches[1]
            $ValueNode = $Cell.SelectSingleNode('./x:v', $Ns)
            if ($Cell.GetAttribute('t') -eq 'inlineStr') {
                $Texts = $Cell.SelectNodes('.//x:is//x:t', $Ns)
                $Value = ($Texts | ForEach-Object { $_.InnerText }) -join ''
            } elseif ($null -eq $ValueNode) {
                $Value = ''
            } elseif ($Cell.GetAttribute('t') -eq 's') {
                $Index = [int]::Parse($ValueNode.InnerText)
                if ($Index -lt 0 -or $Index -ge $Shared.Count) {
                    throw "Index sharedStrings invalide à $SheetEntry $Address : $Index"
                }
                $Value = $Shared[$Index]
            } else {
                # Formulas are read from their stored cached result, never re-evaluated.
                $Value = $ValueNode.InnerText
            }
            $Record[$Column] = [string]$Value
        }
        $Rows.Add([PSCustomObject]@{ Number = [int]$Row.GetAttribute('r'); Cells = $Record })
    }
    return $Rows.ToArray()
}

function Get-RequiredCell {
    param([hashtable]$Cells, [string]$Column, [string]$Context)
    if (-not $Cells.ContainsKey($Column) -or
        [string]::IsNullOrWhiteSpace([string]$Cells[$Column])) {
        throw "Valeur XLSX manquante : $Context colonne $Column (vérifier le cache de formules)."
    }
    return [string]$Cells[$Column]
}

function Assert-Int {
    param([string]$Value, [string]$Context)
    $Result = 0
    if (-not [int]::TryParse($Value, [ref]$Result)) {
        throw "Entier invalide $Context : '$Value'"
    }
    return $Result
}

$Export = [System.Collections.Generic.List[object]]::new()
$Zip = [System.IO.Compression.ZipFile]::OpenRead($WorkbookPath)
try {
    $Workbook = Get-ZipXml $Zip 'xl/workbook.xml'
    $WorkbookNs = New-XlsxNamespace $Workbook
    $Names = @($Workbook.SelectNodes('//x:sheets/x:sheet', $WorkbookNs) | ForEach-Object { $_.GetAttribute('name') })
    $ExpectedSheets = @(
        'Lisez-moi', 'Catalogue', 'Niveaux 1-20', 'Raretés', 'Classes',
        'Améliorations', 'Uniques', 'Paramètres', 'Implémentation UE5',
        'Affixes', 'Pénalités de niveau', 'Contrat synthèse'
    )
    # Exact sheet-order contract: report the actual names on any mismatch.
    # This script is stored WITH a UTF-8 BOM for Windows PowerShell 5.1.
    $ValidStructure = $Names.Count -eq $ExpectedSheets.Count
    if ($ValidStructure) {
        for ($I = 0; $I -lt $ExpectedSheets.Count; $I++) {
            if (-not [string]::Equals($Names[$I], $ExpectedSheets[$I],
                    [System.StringComparison]::Ordinal)) {
                $ValidStructure = $false
                break
            }
        }
    }
    if (-not $ValidStructure) {
        throw ("Les 12 onglets attendus ne correspondent pas. Reels : " +
            ($Names -join ' | ') + " ; Attendus : " + ($ExpectedSheets -join ' | '))
    }

    $StringsXml = Get-ZipXml $Zip 'xl/sharedStrings.xml'
    $StringNs = New-XlsxNamespace $StringsXml
    $Shared = @($StringsXml.SelectNodes('//x:si', $StringNs) | ForEach-Object {
        ($_.SelectNodes('.//x:t', $StringNs) | ForEach-Object { $_.InnerText }) -join ''
    })

    # The 12-sheet contract fixes Catalogue as sheet 2, Levels as sheet 3.
    $CatalogueRows = @(Get-WorksheetRows $Zip 'xl/worksheets/sheet2.xml' $Shared |
        Where-Object { $_.Number -ge 4 -and $_.Number -le 85 })
    $LevelRows = @(Get-WorksheetRows $Zip 'xl/worksheets/sheet3.xml' $Shared |
        Where-Object { $_.Number -ge 4 -and $_.Number -le 1643 })
    if ($CatalogueRows.Count -ne 82 -or $LevelRows.Count -ne 1640) {
        throw "Catalogue incomplet : $($CatalogueRows.Count) archétypes / $($LevelRows.Count) niveaux."
    }

    $MetaById = @{}
    foreach ($Row in $CatalogueRows) {
        $Id = Get-RequiredCell $Row.Cells 'A' "Catalogue:$($Row.Number)"
        if ($MetaById.ContainsKey($Id)) { throw "Archétype en doublon : $Id" }
        $MetaById[$Id] = $Row.Cells
    }

    $Attributes = @{
        'Force' = 'Strength'; 'Dextérité' = 'Dexterity';
        'Constitution' = 'Constitution'; 'Intelligence' = 'Intelligence';
        'Sagesse' = 'Wisdom'; 'Charisme' = 'Charisma'
    }
    $PassiveCount = 0
    foreach ($Row in $LevelRows) {
        $C = $Row.Cells
        $Context = "Niveaux 1-20:$($Row.Number)"
        $Id = Get-RequiredCell $C 'A' $Context
        if (-not $MetaById.ContainsKey($Id)) { throw "$Context : $Id absent du catalogue." }
        $M = $MetaById[$Id]
        $Attr = Get-RequiredCell $M 'M' "Catalogue:$Id"
        if (-not $Attributes.ContainsKey($Attr)) { throw "Attribut non reconnu : $Attr" }
        $Level = Assert-Int (Get-RequiredCell $C 'C' $Context) "$Context niveau"
        $DiceCount = Assert-Int (Get-RequiredCell $C 'M' $Context) "$Context dés"
        $DiceSides = Assert-Int (Get-RequiredCell $C 'N' $Context) "$Context faces"
        $Flat = Assert-Int (Get-RequiredCell $C 'O' $Context) "$Context K"
        $Min = Assert-Int (Get-RequiredCell $C 'E' $Context) "$Context Min"
        $Max = Assert-Int (Get-RequiredCell $C 'F' $Context) "$Context Max"
        if ($Level -lt 1 -or $Level -gt 20) { throw "$Context : niveau hors plage" }
        if ($DiceCount -eq 0) {
            if ($DiceSides -ne 0 -or $Flat -ne 0 -or $Min -ne 0 -or $Max -ne 0) {
                throw "$Context : profil passif incohérent."
            }
            $PassiveCount++
        } else {
            if ($DiceCount -lt 1 -or $DiceCount -gt 32 -or
                $DiceSides -notin @(4,6,8,10,12,20) -or $Flat -lt 0 -or
                $Min -ne ($DiceCount + $Flat) -or $Max -ne ($DiceCount * $DiceSides + $Flat)) {
                throw "$Context : NdM+K invalide."
            }
        }

        $Export.Add([PSCustomObject][ordered]@{
            ArchetypeId = $Id
            ItemLevel = $Level
            DisplayName = (Get-RequiredCell $M 'B' "Catalogue:$Id")
            Family = (Get-RequiredCell $M 'C' "Catalogue:$Id")
            HandUsage = (Get-RequiredCell $M 'D' "Catalogue:$Id")
            AccuracyBonus = (Get-RequiredCell $M 'L' "Catalogue:$Id")
            DamageType = (Get-RequiredCell $M 'N' "Catalogue:$Id")
            PhysicalSubtype = (Get-RequiredCell $M 'O' "Catalogue:$Id")
            ScalingAttribute = $Attributes[$Attr]
            DiceCount = $DiceCount
            DiceSides = $DiceSides
            FlatBonus = $Flat
            PhysicalArmor = (Get-RequiredCell $C 'G' $Context)
            MagicalArmor = (Get-RequiredCell $C 'H' $Context)
            ActionPointCost = (Get-RequiredCell $C 'I' $Context)
            RangeCells = (Get-RequiredCell $C 'J' $Context)
            WeightKg = (Get-RequiredCell $C 'K' $Context)
            MinDamage = $Min
            MaxDamage = $Max
        })
    }
    if ($PassiveCount -ne 260) {
        throw "Nombre inattendu de profils passifs : $PassiveCount au lieu de 260."
    }
    $Keys = @($Export | ForEach-Object { "$($_.ArchetypeId):$($_.ItemLevel)" })
    if (@($Keys | Select-Object -Unique).Count -ne 1640) {
        throw 'Doublon(s) dans les couples (ArchetypeId, ItemLevel).'
    }
} finally { $Zip.Dispose() }

$Directory = Split-Path -Parent $OutputPath
[System.IO.Directory]::CreateDirectory($Directory) | Out-Null
$Export.ToArray() | Export-Csv -LiteralPath $OutputPath -NoTypeInformation -Delimiter ',' -Encoding UTF8
Write-Host "[ARMEMENT01.4] Export validé : 82 archétypes, 1640 profils, 260 sans attaque."
Write-Host "Source : $WorkbookPath"
Write-Host "CSV    : $OutputPath"
