param(
    [Parameter(Mandatory = $false)]
    [string]$ClangFormatPath
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$RepositoryRoot = Split-Path -Parent $PSScriptRoot
$DiagnosticsDir = Join-Path $RepositoryRoot "Saved\Diagnostics\TD07"
$ReportPath = Join-Path $DiagnosticsDir "TD07_7_HygieneAudit.txt"
$FormatScript = Join-Path $PSScriptRoot "CheckCppFormat.ps1"

function Get-FirstPartyCppFiles
{
    $Roots = @(
        "Source\GrimrockPrototype",
        "Source\GrimrockPrototypeEditor",
        "Source\GrimrockLua"
    )

    $Files = foreach ($RelativeRoot in $Roots)
    {
        $AbsoluteRoot = Join-Path $RepositoryRoot $RelativeRoot
        if (-not (Test-Path -LiteralPath $AbsoluteRoot -PathType Container))
        {
            throw "First-party source root not found: $AbsoluteRoot"
        }

        Get-ChildItem -LiteralPath $AbsoluteRoot -Recurse -File | Where-Object {
            $Extension = $_.Extension.ToLowerInvariant()
            ($Extension -eq ".h" -or $Extension -eq ".cpp" -or $Extension -eq ".inl") -and
            $_.Name -notlike "*.generated.h" -and
            $_.FullName -notmatch "[\\/](ThirdParty|Intermediate|Binaries|Saved|DerivedDataCache)[\\/]"
        }
    }

    return $Files | Sort-Object FullName -Unique
}

function Get-RelativePath
{
    param([string]$FullName)

    $Prefix = $RepositoryRoot.TrimEnd("\") + "\"
    if ($FullName.StartsWith($Prefix, [System.StringComparison]::OrdinalIgnoreCase))
    {
        return $FullName.Substring($Prefix.Length)
    }

    return $FullName
}

New-Item -ItemType Directory -Force -Path $DiagnosticsDir | Out-Null

$Files = @(Get-FirstPartyCppFiles)
$LogRows = foreach ($File in $Files)
{
    $Text = Get-Content -LiteralPath $File.FullName -Raw
    $Count = ([regex]::Matches($Text, "UE_LOG\(LogTemp")).Count
    if ($Count -gt 0)
    {
        [pscustomobject]@{
            Path  = Get-RelativePath -FullName $File.FullName
            Count = $Count
        }
    }
}

$LogRows = @($LogRows | Sort-Object Count -Descending, Path)
$TotalLogTempCalls = ($LogRows | Measure-Object -Property Count -Sum).Sum
if ($null -eq $TotalLogTempCalls)
{
    $TotalLogTempCalls = 0
}

$ActivationRow = $LogRows | Where-Object { $_.Path -eq "Source\GrimrockPrototype\Private\Runtime\GridActivationComponent.cpp" } | Select-Object -First 1
$ActivationCount = if ($null -ne $ActivationRow) { $ActivationRow.Count } else { 0 }

$CurrentExe = (Get-Process -Id $PID).Path
$FormatArgs = @(
    "-NoProfile",
    "-ExecutionPolicy", "Bypass",
    "-File", $FormatScript
)
if (-not [string]::IsNullOrWhiteSpace($ClangFormatPath))
{
    $FormatArgs += @("-ClangFormatPath", $ClangFormatPath)
}

$FormatOutput = (& $CurrentExe @FormatArgs 2>&1 | Out-String).TrimEnd()
$FormatExitCode = $LASTEXITCODE

$Lines = [System.Collections.Generic.List[string]]::new()
$Lines.Add("GrimrockPrototype TD07.7 - Targeted Log / Formatting Hygiene Audit")
$Lines.Add("First-party files scanned: $($Files.Count)")
$Lines.Add("Files containing UE_LOG(LogTemp): $($LogRows.Count)")
$Lines.Add("Total UE_LOG(LogTemp) calls: $TotalLogTempCalls")
$Lines.Add("GridActivationComponent UE_LOG(LogTemp): $ActivationCount")
$Lines.Add("")
$Lines.Add("LogTemp by file:")
foreach ($Row in $LogRows)
{
    $Lines.Add("$($Row.Count) | $($Row.Path)")
}
$Lines.Add("")
$Lines.Add("clang-format global baseline:")
$Lines.Add("ExitCode: $FormatExitCode")
$Lines.Add($FormatOutput)

$Lines | Set-Content -LiteralPath $ReportPath -Encoding UTF8
$Lines | ForEach-Object { Write-Host $_ }

Write-Host ""
Write-Host "Report: $ReportPath"
Write-Host ""
Write-Host "TD07.7 characterization completed. This script does not modify source files."
exit 0
