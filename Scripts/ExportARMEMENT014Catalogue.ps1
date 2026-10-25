param(
    [string]$WorkbookPath = 'docs\Design\ARMEMENT01_Catalogue_Armes_Boucliers_Niveaux_1_20_ARMEMENT01_2.xlsx',
    [string]$OutputPath = 'Saved\ARMEMENT01\ARMEMENT01_4_Profiles.csv',
    [string]$NarrativeOutputPath = 'Saved\ARMEMENT-INDUS\INDUS03_Narrative.csv',
    [switch]$RequireNarrative
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
if (-not [System.IO.Path]::IsPathRooted($NarrativeOutputPath)) {
    $NarrativeOutputPath = Join-Path $RepoRoot $NarrativeOutputPath
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
$NarrativeExport = [System.Collections.Generic.List[object]]::new()
$NarrativeSheetPresent = $false
$Zip = [System.IO.Compression.ZipFile]::OpenRead($WorkbookPath)
try {
    $Workbook = Get-ZipXml $Zip 'xl/workbook.xml'
    $WorkbookNs = New-XlsxNamespace $Workbook
    $SheetNodes = @($Workbook.SelectNodes('//x:sheets/x:sheet', $WorkbookNs))
    $Names = @($SheetNodes | ForEach-Object { $_.GetAttribute('name') })
    $ExpectedSheets = @(
        'Lisez-moi', 'Catalogue', 'Niveaux 1-20', 'Raretés', 'Classes',
        'Améliorations', 'Uniques', 'Paramètres', 'Implémentation UE5',
        'Affixes', 'Pénalités de niveau', 'Contrat synthèse'
    )
    # Exact sheet-order contract: report the actual names on any mismatch.
    # This script is stored WITH a UTF-8 BOM for Windows PowerShell 5.1.
    $ValidStructure = $Names.Count -eq 12 -or $Names.Count -eq 13
    if ($ValidStructure) {
        for ($I = 0; $I -lt $ExpectedSheets.Count; $I++) {
            if (-not [string]::Equals($Names[$I], $ExpectedSheets[$I],
                    [System.StringComparison]::Ordinal)) {
                $ValidStructure = $false
                break
            }
        }
    }
    # Version V1: legacy 12 tabs, or exactly those 12 plus a final Narratif.
    if ($ValidStructure -and $Names.Count -eq 13 -and $Names[12] -cne 'Narratif') {
        $ValidStructure = $false
    }
    if (-not $ValidStructure) {
        throw ("Classeur invalide : 12 onglets canoniques, ou 12 + Narratif en derniere position. Reels : " +
            ($Names -join ' | '))
    }
    $NarrativeSheetPresent = $Names.Count -eq 13
    if ($RequireNarrative -and -not $NarrativeSheetPresent) {
        throw 'INDUS03 : onglet Narratif requis mais absent.'
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

    # NarrativeV1: one row per archetype, no changes to the 19 combat columns.
    if ($NarrativeSheetPresent) {
        $Rid = $SheetNodes[12].GetAttribute('id',
            'http://schemas.openxmlformats.org/officeDocument/2006/relationships')
        $Rels = Get-ZipXml $Zip 'xl/_rels/workbook.xml.rels'
        $Rns = [System.Xml.XmlNamespaceManager]::new($Rels.NameTable)
        $Rns.AddNamespace('r', 'http://schemas.openxmlformats.org/package/2006/relationships')
        $Target = $null
        foreach ($Rel in $Rels.SelectNodes('/r:Relationships/r:Relationship', $Rns)) {
            if ($Rel.GetAttribute('Id') -eq $Rid) {
                $Target = $Rel.GetAttribute('Target')
                break
            }
        }
        if ([string]::IsNullOrWhiteSpace($Target) -or $Target.Contains('..') -or $Target.Contains('\')) {
            throw 'INDUS03 : chemin de feuille Narratif invalide.'
        }
        $SheetPath = if ($Target.StartsWith('/')) {
            $Target.TrimStart('/')
        } elseif ($Target.StartsWith('xl/')) {
            $Target
        } else { 'xl/' + $Target }
        if (-not $SheetPath.StartsWith('xl/worksheets/')) {
            throw 'INDUS03 : la relation ne pointe pas sur une feuille de calcul.'
        }
        $AllNarrativeRows = @(Get-WorksheetRows $Zip $SheetPath $Shared)
        $HeaderRow = @($AllNarrativeRows | Where-Object { $_.Number -eq 1 })
        if ($HeaderRow.Count -ne 1) { throw 'INDUS03 : en-tete Narratif absent.' }
        $ExpectedColumns = @('ArchetypeId', 'DescriptionCourteFR', 'LoreFR',
            'ParticulariteFR', 'EtatParticularite', 'NotesConception', 'EtatValidation')
        $ColumnKeys = @('A','B','C','D','E','F','G')
        for ($Index = 0; $Index -lt $ColumnKeys.Count; $Index++) {
            if ([string]$HeaderRow[0].Cells[$ColumnKeys[$Index]] -cne $ExpectedColumns[$Index]) {
                throw "INDUS03 : en-tete Narratif invalide en $($ColumnKeys[$Index]) ; attendu $($ExpectedColumns[$Index])."
            }
        }
        $ExtraRows = @($AllNarrativeRows | Where-Object {
            $_.Number -gt 83 -and -not [string]::IsNullOrWhiteSpace([string]$_.Cells['A'])
        })
        if ($ExtraRows.Count -gt 0) {
            throw 'INDUS03 : ligne ArchetypeId inattendue apres la ligne 83.'
        }
        $RowsNarratif = @($AllNarrativeRows |
            Where-Object { $_.Number -ge 2 -and $_.Number -le 83 })
        if ($RowsNarratif.Count -ne 82) {
            throw "INDUS03 : Narratif contient $($RowsNarratif.Count) lignes, 82 attendues."
        }
        $SeenNarratif = @{}
        foreach ($R in $RowsNarratif) {
            $C = $R.Cells
            $Id = Get-RequiredCell $C 'A' "Narratif:$($R.Number)"
            if (-not $MetaById.ContainsKey($Id) -or $SeenNarratif.ContainsKey($Id)) {
                throw "INDUS03 : ID inconnu ou duplique : $Id"
            }
            $SeenNarratif[$Id] = $true
            $Short = [string]$C['B']
            $Lore = [string]$C['C']
            $Feature = [string]$C['D']
            $FeatureState = Get-RequiredCell $C 'E' "Narratif:$Id"
            $Notes = [string]$C['F']
            $Review = Get-RequiredCell $C 'G' "Narratif:$Id"
            if ($FeatureState -notin @('UNDOCUMENTED','PLANNED','IMPLEMENTED','NOT_APPLICABLE') -or
                $Review -notin @('DRAFT','NEEDS_REVIEW','APPROVED')) {
                throw "INDUS03 : etat narratif invalide pour $Id."
            }
            if ($Short.Length -gt 300 -or $Lore.Length -gt 4000 -or
                $Feature.Length -gt 300 -or $Notes.Length -gt 2000) {
                throw "INDUS03 : longueur narrative invalide pour $Id."
            }
            if ($Review -eq 'APPROVED' -and [string]::IsNullOrWhiteSpace($Short)) {
                throw "INDUS03 : $Id APPROVED sans DescriptionCourteFR."
            }
            if ($FeatureState -eq 'IMPLEMENTED' -and [string]::IsNullOrWhiteSpace($Feature)) {
                throw "INDUS03 : $Id IMPLEMENTED sans ParticulariteFR."
            }
            $NarrativeExport.Add([PSCustomObject][ordered]@{
                SchemaVersion = 'NarrativeV1'
                ArchetypeId = $Id
                DescriptionCourteFR = $Short
                LoreFR = $Lore
                ParticulariteFR = $Feature
                EtatParticularite = $FeatureState
                NotesConception = $Notes
                EtatValidation = $Review
            })
        }
        if ($SeenNarratif.Count -ne $MetaById.Count) {
            throw "INDUS03 : couverture incomplete : $($SeenNarratif.Count)/$($MetaById.Count)."
        }
    }
} finally { $Zip.Dispose() }

$Directory = Split-Path -Parent $OutputPath
[System.IO.Directory]::CreateDirectory($Directory) | Out-Null
$Export.ToArray() | Export-Csv -LiteralPath $OutputPath -NoTypeInformation -Delimiter ',' -Encoding UTF8
Write-Host "[ARMEMENT01.4] Export validé : 82 archétypes, 1640 profils, 260 sans attaque."
Write-Host "Source : $WorkbookPath"
Write-Host "CSV    : $OutputPath"

if ($NarrativeSheetPresent) {
    [System.IO.Directory]::CreateDirectory((Split-Path -Parent $NarrativeOutputPath)) | Out-Null
    $NarrativeExport.ToArray() | Export-Csv -LiteralPath $NarrativeOutputPath -NoTypeInformation -Delimiter ',' -Encoding UTF8
    Write-Host "[ARMEMENT-INDUS03] NarrativeV1 valide : 82 archétypes, 0 statistique de combat modifiée."
    Write-Host "Narratif : $NarrativeOutputPath"
} else {
    Write-Host '[ARMEMENT-INDUS03] Classeur legacy 12 onglets valide ; narratif non encore present.'
}
