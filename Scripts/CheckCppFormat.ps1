param(
    [string]$ClangFormatPath
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$ExpectedVersion = '19.1.5'
$RepoRoot = Split-Path -Parent $PSScriptRoot

function Resolve-ClangFormat
{
    param([string]$RequestedPath)

    if (-not [string]::IsNullOrWhiteSpace($RequestedPath))
    {
        if (-not (Test-Path -LiteralPath $RequestedPath -PathType Leaf))
        {
            throw "clang-format introuvable : $RequestedPath"
        }

        return (Resolve-Path -LiteralPath $RequestedPath).Path
    }

    $VsWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path -LiteralPath $VsWhere -PathType Leaf)
    {
        $VsInstallPath = (& $VsWhere -latest -products * -version '[17.0,18.0)' -property installationPath | Select-Object -First 1)
        if (-not [string]::IsNullOrWhiteSpace($VsInstallPath))
        {
            $VsCandidate = Join-Path $VsInstallPath 'VC\Tools\Llvm\x64\bin\clang-format.exe'
            if (Test-Path -LiteralPath $VsCandidate -PathType Leaf)
            {
                return $VsCandidate
            }
        }
    }

    $DefaultVsCandidate = 'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin\clang-format.exe'
    if (Test-Path -LiteralPath $DefaultVsCandidate -PathType Leaf)
    {
        return $DefaultVsCandidate
    }

    $Command = Get-Command clang-format -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($null -ne $Command)
    {
        return $Command.Source
    }

    throw 'clang-format 19.1.5 introuvable. Installez les outils Clang/LLVM de Visual Studio 2022 ou utilisez -ClangFormatPath.'
}

function Assert-ClangFormatVersion
{
    param([string]$ExecutablePath)

    $VersionText = (& $ExecutablePath --version 2>&1 | Out-String).Trim()
    if ($LASTEXITCODE -ne 0)
    {
        throw "Impossible d'interroger clang-format : $VersionText"
    }

    $ExpectedPrefix = "clang-format version $ExpectedVersion"
    if (-not $VersionText.StartsWith($ExpectedPrefix, [System.StringComparison]::OrdinalIgnoreCase))
    {
        throw "Version clang-format incompatible. Attendu : $ExpectedVersion. Detecte : $VersionText"
    }

    Write-Host "clang-format : $VersionText"
}

function Get-FirstPartyCppFiles
{
    $Roots = @(
        'Source\GrimrockPrototype',
        'Source\GrimrockPrototypeEditor',
        'Source\GrimrockLua'
    )

    $Files = foreach ($RelativeRoot in $Roots)
    {
        $AbsoluteRoot = Join-Path $RepoRoot $RelativeRoot
        if (-not (Test-Path -LiteralPath $AbsoluteRoot -PathType Container))
        {
            throw "Perimetre C++ introuvable : $AbsoluteRoot"
        }

        Get-ChildItem -LiteralPath $AbsoluteRoot -Recurse -File | Where-Object {
            $Extension = $_.Extension.ToLowerInvariant()
            ($Extension -eq '.h' -or $Extension -eq '.cpp' -or $Extension -eq '.inl') -and
            $_.Name -notlike '*.generated.h' -and
            $_.FullName -notmatch '[\\/](ThirdParty|Intermediate|Binaries|Saved|DerivedDataCache)[\\/]'
        }
    }

    return $Files | Sort-Object FullName -Unique
}

$ClangFormat = Resolve-ClangFormat -RequestedPath $ClangFormatPath
Assert-ClangFormatVersion -ExecutablePath $ClangFormat

$Files = @(Get-FirstPartyCppFiles)
$InvalidFiles = New-Object System.Collections.Generic.List[string]
$RepoPrefix = $RepoRoot.TrimEnd('\') + '\'

Write-Host "Verification de $($Files.Count) fichiers C++ first-party..."

foreach ($File in $Files)
{
    $Arguments = @('--dry-run', '--Werror', '--style=file')
    if ($File.Extension.ToLowerInvariant() -eq '.inl')
    {
        $Arguments += "--assume-filename=$($File.FullName).cpp"
    }
    $Arguments += $File.FullName

    & $ClangFormat @Arguments 2>&1 | Out-Null
    if ($LASTEXITCODE -ne 0)
    {
        $RelativePath = $File.FullName
        if ($RelativePath.StartsWith($RepoPrefix, [System.StringComparison]::OrdinalIgnoreCase))
        {
            $RelativePath = $RelativePath.Substring($RepoPrefix.Length)
        }

        $InvalidFiles.Add($RelativePath)
        Write-Host "[FORMAT] $RelativePath"
    }
}

if ($InvalidFiles.Count -gt 0)
{
    Write-Error "$($InvalidFiles.Count) fichier(s) ne respectent pas Grimrock C++ Style v1. Executez Scripts\FormatCpp.ps1."
    exit 1
}

Write-Host 'Format C++ conforme.'
exit 0
