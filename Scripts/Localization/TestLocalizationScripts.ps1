[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$Targets = @(
    (Join-Path $PSScriptRoot "RunLocalizationSmoke.ps1"),
    (Join-Path $PSScriptRoot "ApplyEnglishSmokeTranslations.ps1"),
    (Join-Path $PSScriptRoot "TestLocalizationScripts.ps1")
)

$Failed = $false
foreach ($Path in $Targets) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        Write-Error ("Script not found: {0}" -f $Path)
        $Failed = $true
        continue
    }

    $Bytes = [System.IO.File]::ReadAllBytes($Path)
    $NonAsciiCount = @($Bytes | Where-Object { $_ -gt 127 }).Count
    if ($NonAsciiCount -ne 0) {
        Write-Error ("Script must remain ASCII-only for Windows PowerShell 5.1 compatibility: {0}. Non-ASCII bytes: {1}" -f $Path, $NonAsciiCount)
        $Failed = $true
    }
    else {
        Write-Host ("[OK] ASCII-only PowerShell source: {0}" -f $Path)
    }

    $Tokens = $null
    $Errors = $null
    [void][System.Management.Automation.Language.Parser]::ParseFile($Path, [ref]$Tokens, [ref]$Errors)

    if ($Errors.Count -gt 0) {
        foreach ($ParseError in $Errors) {
            Write-Error ("PowerShell parse error in {0} at line {1}, column {2}: {3}" -f $Path, $ParseError.Extent.StartLineNumber, $ParseError.Extent.StartColumnNumber, $ParseError.Message)
        }
        $Failed = $true
    }
    else {
        Write-Host ("[OK] PowerShell syntax: {0}" -f $Path)
    }
}

if ($Failed) {
    exit 1
}

Write-Host "[OK] Localization PowerShell scripts are ASCII-only and parsed successfully."
exit 0
