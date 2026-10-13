param(
    [string]$PoPath
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($PoPath)) {
    $ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
    $PoPath = Join-Path $ProjectRoot "Content\Localization\Game\en\Game.po"
}

if (-not (Test-Path -LiteralPath $PoPath)) {
    throw "English PO file not found: $PoPath"
}

function ConvertTo-PoQuoted {
    param([string]$Value)
    return $Value.Replace("\", "\\").Replace('"', '\"').Replace([string][char]13, "\r").Replace([string][char]10, "\n")
}

$Translations = [ordered]@{
    "Féminin" = "Female"
    "Masculin" = "Male"
    "Identité du personnage" = "Character Identity"
    "Résumé" = "Summary"
    "Prêt à créer le personnage." = "Ready to create character."
}

$Utf8 = New-Object System.Text.UTF8Encoding($false)
$Content = [System.IO.File]::ReadAllText($PoPath, [System.Text.Encoding]::UTF8)

foreach ($Pair in $Translations.GetEnumerator()) {
    $Source = ConvertTo-PoQuoted $Pair.Key
    $Translation = ConvertTo-PoQuoted $Pair.Value
    $Pattern = '(?m)^msgid "' + [regex]::Escape($Source) + '"\r?\nmsgstr "(?:\\.|[^"\\])*"$'
    $Matches = [regex]::Matches($Content, $Pattern)

    if ($Matches.Count -ne 1) {
        throw "Expected exactly one PO entry for '$($Pair.Key)', found $($Matches.Count)."
    }

    $Replacement = 'msgid "' + $Source + '"' + [Environment]::NewLine + 'msgstr "' + $Translation + '"'
    $Content = [regex]::Replace(
        $Content,
        $Pattern,
        [System.Text.RegularExpressions.MatchEvaluator]{ param($Match) $Replacement },
        1)
}

[System.IO.File]::WriteAllText($PoPath, $Content, $Utf8)
Write-Host "[OK] English localization smoke translations applied to $PoPath"
