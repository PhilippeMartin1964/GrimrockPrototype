[CmdletBinding()]
param(
    [string]$PoPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($PoPath)) {
    $ProjectRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..\..")).Path
    $PoPath = Join-Path $ProjectRoot "Content\Localization\Game\en\Game.po"
}

if (-not (Test-Path -LiteralPath $PoPath -PathType Leaf)) {
    throw ("English PO file not found: {0}" -f $PoPath)
}

function ConvertTo-PoQuoted {
    param([Parameter(Mandatory = $true)][string]$Value)

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
    $Source = ConvertTo-PoQuoted -Value ([string]$Pair.Key)
    $Translation = ConvertTo-PoQuoted -Value ([string]$Pair.Value)

    $Pattern = '(?ms)(^msgid "' + [regex]::Escape($Source) + '"\r?\n(?:(?!\r?\n\r?\n).)*?^msgstr )"(?:\\.|[^"\\])*"'
    $Matches = [regex]::Matches($Content, $Pattern)

    if ($Matches.Count -ne 1) {
        throw ("Expected exactly one PO entry for '{0}'; found {1}. File: {2}" -f $Pair.Key, $Matches.Count, $PoPath)
    }

    $Content = [regex]::Replace(
        $Content,
        $Pattern,
        [System.Text.RegularExpressions.MatchEvaluator]{
            param($Match)
            return $Match.Groups[1].Value + '"' + $Translation + '"'
        },
        1)
}

foreach ($Pair in $Translations.GetEnumerator()) {
    $Source = ConvertTo-PoQuoted -Value ([string]$Pair.Key)
    $Translation = ConvertTo-PoQuoted -Value ([string]$Pair.Value)
    $ValidationPattern = '(?ms)^msgid "' + [regex]::Escape($Source) + '"\r?\n(?:(?!\r?\n\r?\n).)*?^msgstr "' + [regex]::Escape($Translation) + '"'

    if (-not [regex]::IsMatch($Content, $ValidationPattern)) {
        throw ("English PO validation failed for source '{0}'." -f $Pair.Key)
    }
}

$Directory = Split-Path -Parent $PoPath
$TempPath = Join-Path $Directory ((Split-Path -Leaf $PoPath) + ".tmp")
try {
    [System.IO.File]::WriteAllText($TempPath, $Content, $Utf8)
    Move-Item -LiteralPath $TempPath -Destination $PoPath -Force
}
finally {
    if (Test-Path -LiteralPath $TempPath) {
        Remove-Item -LiteralPath $TempPath -Force -ErrorAction SilentlyContinue
    }
}

Write-Host ("[OK] English localization smoke translations applied and verified: {0}" -f $PoPath)
