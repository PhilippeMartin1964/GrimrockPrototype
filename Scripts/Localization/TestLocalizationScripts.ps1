[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$Targets = @(
    (Join-Path $PSScriptRoot "RunLocalizationSmoke.ps1"),
    (Join-Path $PSScriptRoot "ApplyEnglishSmokeTranslations.ps1")
)

$Failed = $false
foreach ($Path in $Targets) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        Write-Error ("Script not found: {0}" -f $Path)
        $Failed = $true
        continue
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

Write-Host "[OK] Localization PowerShell scripts parsed successfully."
exit 0
