[CmdletBinding()]
param(
    [string]$PoPath,
    [string]$CsvPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$ProjectRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..\..")).Path

if ([string]::IsNullOrWhiteSpace($PoPath)) {
    $PoPath = Join-Path $ProjectRoot "Content\Localization\Game\en\Game.po"
}

if ([string]::IsNullOrWhiteSpace($CsvPath)) {
    $CsvPath = Join-Path $ProjectRoot "Localization\Source\UI.csv"
}

if (-not (Test-Path -LiteralPath $PoPath -PathType Leaf)) {
    throw ("English PO file not found: {0}" -f $PoPath)
}

if (-not (Test-Path -LiteralPath $CsvPath -PathType Leaf)) {
    throw ("UI localization CSV not found: {0}" -f $CsvPath)
}

function ConvertTo-PoQuoted {
    param([Parameter(Mandatory = $true)][string]$Value)

    return $Value.Replace("\", "\\").Replace('"', '\"').Replace([string][char]13, "\r").Replace([string][char]10, "\n")
}

# Keep this script ASCII-only for Windows PowerShell 5.1 compatibility.
# French source text is read from UI.csv as UTF-8 using stable keys.
$TranslationsByKey = [ordered]@{
    "CharacterCreation.Gender.Female" = "Female"
    "CharacterCreation.Gender.Male" = "Male"
    "CharacterCreation.Identity.Title" = "Character Identity"
    "CharacterCreation.Wizard.Step.Summary" = "Summary"
    "CharacterCreation.Summary.State.Ready.Create" = "Ready to create character."
    "Status.Control.BlockTranslation" = "Grid movement is blocked."
    "Status.Duration.Rounds" = "{Count} {Count}|plural(one=round,other=rounds)"
    "Status.Duration.Turns" = "{Count} {Count}|plural(one=turn,other=turns)"
    "Status.Tooltip.Duration" = "Duration: {Duration}"
    "Status.Tooltip.PeriodicDamage" = "Periodic damage active."
    "Interaction.Button.Push" = "Push button"
    "Interaction.Door.Open" = "Open door"
    "Interaction.Item.Take" = "Take"
    "Interaction.Lock.Locked" = "The lock is locked."
    "Interaction.Readable.Read" = "Read"
    "Interaction.Receptacle.PlaceItem" = "Place item"
    "Combat.Action.Unavailable.StatusBlocksSpells" = "A status effect prevents spell use."
    "Combat.EndTurn.EnemyTurn" = "Enemy turn"
    "Combat.Hotbar.EmptyTooltip" = "Drop an inventory item, spell, or ability here."
    "Combat.Hotbar.Tooltip.Cost" = "Cost: {Cost}"
    "Combat.Initiative.Round" = "ROUND {Round}"
    "Combat.Resource.ActionPoints" = "AP {Current} / {Maximum}"
    "Combat.State.Active" = "ACTIVE"
    "Combat.Targeting.Invalid.OutOfRange" = "This cell is out of range."
    "Inventory.Bag.Weight" = "Weight: {Current} / {Maximum}"
    "Progression.Skill.AllocationFailed" = "Skill allocation failed."
    "Progression.Skill.CancellationFailed" = "Skill allocation cancellation failed."
    "SaveLoad.Slot.Available" = "Available"
    "SaveLoad.Slot.Numbered" = "Save {Index}"
    "SaveLoad.Slot.Primary" = "Primary save"
    "SaveLoad.Slot.Unavailable" = "Unavailable"
    "Spellbook.Fallback.DefinitionUnavailable" = "Spell definition unavailable."
}

$Utf8NoBom = New-Object System.Text.UTF8Encoding($false)
$CsvText = [System.IO.File]::ReadAllText($CsvPath, [System.Text.Encoding]::UTF8)
$CsvRows = @($CsvText | ConvertFrom-Csv)
$PoContent = [System.IO.File]::ReadAllText($PoPath, [System.Text.Encoding]::UTF8)

foreach ($Pair in $TranslationsByKey.GetEnumerator()) {
    $Key = [string]$Pair.Key
    $EnglishText = [string]$Pair.Value
    $Rows = @($CsvRows | Where-Object { $_.Key -eq $Key })

    if ($Rows.Count -ne 1) {
        throw ("Expected exactly one UI.csv row for key '{0}'; found {1}. File: {2}" -f $Key, $Rows.Count, $CsvPath)
    }

    $SourceText = [string]$Rows[0].SourceString
    if ([string]::IsNullOrWhiteSpace($SourceText)) {
        throw ("UI.csv source text is empty for key '{0}'." -f $Key)
    }

    $Source = ConvertTo-PoQuoted -Value $SourceText
    $Translation = ConvertTo-PoQuoted -Value $EnglishText
    $Pattern = '(?ms)(^msgid "' + [regex]::Escape($Source) + '"\r?\n(?:(?!\r?\n\r?\n).)*?^msgstr )"(?:\\.|[^"\\])*"'
    $Matches = [regex]::Matches($PoContent, $Pattern)

    if ($Matches.Count -ne 1) {
        throw ("Expected exactly one PO entry for key '{0}' (source length {1}); found {2}. File: {3}" -f $Key, $SourceText.Length, $Matches.Count, $PoPath)
    }

    $PoContent = [regex]::Replace(
        $PoContent,
        $Pattern,
        [System.Text.RegularExpressions.MatchEvaluator]{
            param($Match)
            return $Match.Groups[1].Value + '"' + $Translation + '"'
        },
        1)
}

foreach ($Pair in $TranslationsByKey.GetEnumerator()) {
    $Key = [string]$Pair.Key
    $EnglishText = [string]$Pair.Value
    $Rows = @($CsvRows | Where-Object { $_.Key -eq $Key })
    $SourceText = [string]$Rows[0].SourceString
    $Source = ConvertTo-PoQuoted -Value $SourceText
    $Translation = ConvertTo-PoQuoted -Value $EnglishText
    $ValidationPattern = '(?ms)^msgid "' + [regex]::Escape($Source) + '"\r?\n(?:(?!\r?\n\r?\n).)*?^msgstr "' + [regex]::Escape($Translation) + '"'

    if (-not [regex]::IsMatch($PoContent, $ValidationPattern)) {
        throw ("English PO validation failed for key '{0}'." -f $Key)
    }
}

$Directory = Split-Path -Parent $PoPath
$TempPath = Join-Path $Directory ((Split-Path -Leaf $PoPath) + ".tmp")
try {
    [System.IO.File]::WriteAllText($TempPath, $PoContent, $Utf8NoBom)
    Move-Item -LiteralPath $TempPath -Destination $PoPath -Force
}
finally {
    if (Test-Path -LiteralPath $TempPath) {
        Remove-Item -LiteralPath $TempPath -Force -ErrorAction SilentlyContinue
    }
}

Write-Host ("[OK] English smoke translations applied from stable UI.csv keys: {0}" -f $PoPath)
