param(
    [string]$EngineRoot = $env:UE_ROOT,
    [string]$ArchiveRoot,
    [ValidateSet('Development', 'Shipping')]
    [string]$Configuration = 'Shipping'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$RepoRoot = Split-Path -Parent $PSScriptRoot
$ProjectFile = Join-Path $RepoRoot 'GrimrockPrototype.uproject'
$GameTarget = 'GrimrockPrototype'
$Platform = 'Win64'

function Resolve-GrimrockEngineRoot
{
    param([string]$RequestedRoot)

    if ([string]::IsNullOrWhiteSpace($RequestedRoot))
    {
        throw 'Racine Unreal Engine non renseignee. Utilisez -EngineRoot ou definissez la variable d''environnement UE_ROOT.'
    }

    if (-not (Test-Path -LiteralPath $RequestedRoot -PathType Container))
    {
        throw "Racine Unreal Engine introuvable : $RequestedRoot"
    }

    return (Resolve-Path -LiteralPath $RequestedRoot).Path
}

function Assert-FileExists
{
    param(
        [string]$Path,
        [string]$Description
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf))
    {
        throw "$Description introuvable : $Path"
    }
}

function Invoke-GrimrockNativeStep
{
    param(
        [string]$Label,
        [string]$Executable,
        [string[]]$Arguments
    )

    Write-Host ''
    Write-Host "=== $Label ==="
    Write-Host "Executable : $Executable"
    Write-Host ('Arguments  : ' + ($Arguments -join ' '))

    & $Executable @Arguments
    $ExitCode = $LASTEXITCODE
    if ($ExitCode -ne 0)
    {
        throw "$Label a echoue avec le code $ExitCode."
    }
}

Assert-FileExists -Path $ProjectFile -Description 'Projet Unreal'

$ResolvedEngineRoot = Resolve-GrimrockEngineRoot -RequestedRoot $EngineRoot
$RunUAT = Join-Path $ResolvedEngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
Assert-FileExists -Path $RunUAT -Description 'Unreal RunUAT.bat'

if ([string]::IsNullOrWhiteSpace($ArchiveRoot))
{
    $ArchiveRoot = Join-Path $RepoRoot 'Saved\Packaging\TD04'
}

$Timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$SessionName = "TD04-$Configuration-$Timestamp"
$SessionArchivePath = Join-Path $ArchiveRoot $SessionName
New-Item -ItemType Directory -Path $SessionArchivePath -Force | Out-Null

Write-Host 'GrimrockPrototype TD04.3 - Cook / Package Validation'
Write-Host "Repository    : $RepoRoot"
Write-Host "Project       : $ProjectFile"
Write-Host "Engine        : $ResolvedEngineRoot"
Write-Host "Target        : $GameTarget"
Write-Host "Platform      : $Platform"
Write-Host "Configuration : $Configuration"
Write-Host "Archive       : $SessionArchivePath"

$UATArguments = @(
    'BuildCookRun',
    "-project=$ProjectFile",
    '-noP4',
    '-utf8output',
    "-platform=$Platform",
    "-target=$GameTarget",
    "-clientconfig=$Configuration",
    '-build',
    '-cook',
    '-stage',
    '-package',
    '-pak',
    '-archive',
    "-archivedirectory=$SessionArchivePath"
)

Invoke-GrimrockNativeStep -Label "UE5.5.4 $Platform $Configuration BuildCookRun" -Executable $RunUAT -Arguments $UATArguments

$PackagedExecutable = Get-ChildItem -LiteralPath $SessionArchivePath -Recurse -File -Filter "$GameTarget.exe" |
    Select-Object -First 1

if ($null -eq $PackagedExecutable)
{
    Write-Error "Aucun executable package $GameTarget.exe n'a ete trouve sous : $SessionArchivePath"
    exit 2
}

$PakFiles = @(Get-ChildItem -LiteralPath $SessionArchivePath -Recurse -File -Filter '*.pak')
if ($PakFiles.Count -le 0)
{
    Write-Error "Aucun fichier .pak n'a ete produit sous : $SessionArchivePath"
    exit 3
}

$ArchiveFiles = @(Get-ChildItem -LiteralPath $SessionArchivePath -Recurse -File)
$ArchiveBytes = ($ArchiveFiles | Measure-Object -Property Length -Sum).Sum
if ($null -eq $ArchiveBytes)
{
    $ArchiveBytes = 0
}

Write-Host ''
Write-Host '=== Package summary ==='
Write-Host "Target        : $GameTarget"
Write-Host "Platform      : $Platform"
Write-Host "Configuration : $Configuration"
Write-Host "Executable    : $($PackagedExecutable.FullName)"
Write-Host "Pak files     : $($PakFiles.Count)"
Write-Host "Archive files : $($ArchiveFiles.Count)"
Write-Host "Archive bytes : $ArchiveBytes"
Write-Host "Archive       : $SessionArchivePath"
Write-Host '[OK] Cook / package validated.'

Write-Host ''
Write-Host 'TD04.3 validation completed successfully.'
exit 0
