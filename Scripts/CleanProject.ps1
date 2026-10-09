<#
.SYNOPSIS
Nettoie les artefacts locaux regenerables de GrimrockPrototype.

.DESCRIPTION
Le script fonctionne en PREVIEW par defaut. Aucune suppression n'est effectuee
tant que -Apply n'est pas fourni.

Le nettoyage courant cible uniquement des sous-dossiers regenerables de Saved.
Des groupes plus agressifs sont disponibles explicitement pour les anciens
rapports/packages, les artefacts C++/Visual Studio, DerivedDataCache et .tmp.

Le script utilise une liste blanche interne. Il ne derive jamais ses suppressions
de .gitignore et ne lance jamais git clean.

.EXAMPLE
.\Scripts\CleanProject.ps1

.EXAMPLE
.\Scripts\CleanProject.ps1 -Apply

.EXAMPLE
.\Scripts\CleanProject.ps1 -ValidationArtifacts -Apply

.EXAMPLE
.\Scripts\CleanProject.ps1 -BuildArtifacts -Apply

.EXAMPLE
.\Scripts\CleanProject.ps1 -Full
.\Scripts\CleanProject.ps1 -Full -Apply
#>

[CmdletBinding()]
param(
    [switch]$Apply,
    [switch]$ValidationArtifacts,
    [switch]$BuildArtifacts,
    [switch]$DerivedDataCache,
    [switch]$IncludeTmp,
    [switch]$Full
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$RepoRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$ProjectFile = Join-Path $RepoRoot 'GrimrockPrototype.uproject'

if (-not (Test-Path -LiteralPath $ProjectFile -PathType Leaf))
{
    throw "Projet GrimrockPrototype introuvable sous : $RepoRoot"
}

$DoValidationArtifacts = $ValidationArtifacts -or $Full
$DoBuildArtifacts = $BuildArtifacts -or $Full
$DoDerivedDataCache = $DerivedDataCache -or $Full
$DoTmp = $IncludeTmp -or $Full

function Format-ByteSize
{
    param([long]$Bytes)

    if ($Bytes -ge 1TB)
    {
        return ('{0:N2} TiB' -f ($Bytes / 1TB))
    }

    if ($Bytes -ge 1GB)
    {
        return ('{0:N2} GiB' -f ($Bytes / 1GB))
    }

    if ($Bytes -ge 1MB)
    {
        return ('{0:N2} MiB' -f ($Bytes / 1MB))
    }

    if ($Bytes -ge 1KB)
    {
        return ('{0:N2} KiB' -f ($Bytes / 1KB))
    }

    return "$Bytes B"
}

function Get-DirectorySize
{
    param([string]$Path)

    if (-not (Test-Path -LiteralPath $Path -PathType Container))
    {
        return 0L
    }

    $Measure = Get-ChildItem -LiteralPath $Path -File -Recurse -Force -ErrorAction SilentlyContinue |
        Measure-Object -Property Length -Sum

    if ($null -eq $Measure.Sum)
    {
        return 0L
    }

    return [long]$Measure.Sum
}

function New-CleanupTarget
{
    param(
        [string]$RelativePath,
        [string]$Category,
        [string]$Description
    )

    $FullPath = [System.IO.Path]::GetFullPath((Join-Path $RepoRoot $RelativePath))
    $RootPrefix = $RepoRoot.TrimEnd('\', '/') + [System.IO.Path]::DirectorySeparatorChar

    if (-not $FullPath.StartsWith($RootPrefix, [System.StringComparison]::OrdinalIgnoreCase))
    {
        throw "Chemin de nettoyage hors depot refuse : $FullPath"
    }

    if (-not (Test-Path -LiteralPath $FullPath -PathType Container))
    {
        return $null
    }

    $Item = Get-Item -LiteralPath $FullPath -Force
    if (($Item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0)
    {
        throw "Point de reanalyse/jonction refuse pour securite : $FullPath"
    }

    return [PSCustomObject]@{
        RelativePath = $RelativePath
        FullPath = $FullPath
        Category = $Category
        Description = $Description
        Bytes = Get-DirectorySize -Path $FullPath
    }
}

$TargetDefinitions = @(
    @{ Path = 'Saved\Logs'; Category = 'Routine'; Description = 'Logs Unreal locaux' },
    @{ Path = 'Saved\Crashes'; Category = 'Routine'; Description = 'Rapports de crash locaux' },
    @{ Path = 'Saved\Cooked'; Category = 'Routine'; Description = 'Donnees cooked regenerables' },
    @{ Path = 'Saved\StagedBuilds'; Category = 'Routine'; Description = 'Staging regenerable' },
    @{ Path = 'Saved\Temp'; Category = 'Routine'; Description = 'Temporaires Unreal' },
    @{ Path = 'Saved\ShaderDebugInfo'; Category = 'Routine'; Description = 'Debug shaders regenerable' }
)

if ($DoValidationArtifacts)
{
    $TargetDefinitions += @(
        @{ Path = 'Saved\Automation'; Category = 'Validation'; Description = 'Rapports Automation locaux' },
        @{ Path = 'Saved\Packaging'; Category = 'Validation'; Description = 'Archives de packaging locales' }
    )
}

if ($DoBuildArtifacts)
{
    $TargetDefinitions += @(
        @{ Path = 'Binaries'; Category = 'Build'; Description = 'Binaires UE regenerables' },
        @{ Path = 'Intermediate'; Category = 'Build'; Description = 'Intermediaires UE regenerables' },
        @{ Path = '.vs'; Category = 'Build'; Description = 'Cache Visual Studio local' }
    )
}

if ($DoDerivedDataCache)
{
    $TargetDefinitions += @(
        @{ Path = 'DerivedDataCache'; Category = 'Cache'; Description = 'Derived Data Cache local' }
    )
}

if ($DoTmp)
{
    $TargetDefinitions += @(
        @{ Path = '.tmp'; Category = 'Local'; Description = 'Dossier temporaire local explicite' }
    )
}

$Targets = @()
foreach ($Definition in $TargetDefinitions)
{
    $Target = New-CleanupTarget -RelativePath $Definition.Path -Category $Definition.Category -Description $Definition.Description
    if ($null -ne $Target)
    {
        $Targets += $Target
    }
}

Write-Host ''
Write-Host 'GrimrockPrototype - Project Cleanup'
Write-Host "Repository : $RepoRoot"
Write-Host ('Mode       : ' + $(if ($Apply) { 'APPLY' } else { 'PREVIEW' }))
Write-Host ''

if ($Targets.Count -eq 0)
{
    Write-Host '[OK] Aucun dossier cible present.'
    exit 0
}

$Targets |
    Select-Object Category, RelativePath, @{ Name = 'Size'; Expression = { Format-ByteSize $_.Bytes } }, Description |
    Format-Table -AutoSize

$TotalBytes = [long](($Targets | Measure-Object -Property Bytes -Sum).Sum)
Write-Host ''
Write-Host ("Total cible : {0}" -f (Format-ByteSize $TotalBytes))

if (-not $Apply)
{
    Write-Host ''
    Write-Host '[PREVIEW] Aucune suppression effectuee.'
    Write-Host 'Relancez avec -Apply pour supprimer les cibles affichees.'
    Write-Host 'Options : -ValidationArtifacts, -BuildArtifacts, -DerivedDataCache, -IncludeTmp, -Full.'
    exit 0
}

$BlockedProcessNames = @(
    'UnrealEditor',
    'UnrealEditor-Cmd',
    'UnrealBuildTool',
    'AutomationTool',
    'MSBuild',
    'ShaderCompileWorker'
)

if ($DoBuildArtifacts)
{
    $BlockedProcessNames += 'devenv'
}

$Running = Get-Process -ErrorAction SilentlyContinue |
    Where-Object { $BlockedProcessNames -contains $_.ProcessName } |
    Select-Object -ExpandProperty ProcessName -Unique

if ($Running)
{
    $ContextSuffix = if ($DoBuildArtifacts) { '/Visual Studio' } else { '' }
    throw ('Nettoyage refuse pendant execution de : ' + ($Running -join ', ') + '. Fermez Unreal/build' + $ContextSuffix + ' puis relancez.')
}

$Git = Get-Command git -CommandType Application -ErrorAction SilentlyContinue
if ($null -eq $Git)
{
    throw 'git est requis en mode -Apply pour verifier qu''aucun fichier versionne ne sera supprime.'
}

foreach ($Target in $Targets)
{
    $GitPath = $Target.RelativePath.Replace('\', '/')
    $Tracked = @(& $Git.Source -C $RepoRoot ls-files -- $GitPath)

    if ($LASTEXITCODE -ne 0)
    {
        throw "git ls-files a echoue pour : $GitPath"
    }

    if ($Tracked.Count -gt 0)
    {
        $TrackedText = ($Tracked | ForEach-Object { "  $_" }) -join [Environment]::NewLine
        throw ("Nettoyage refuse : le dossier cible contient des fichiers versionnes : {0}{1}{2}" -f $Target.RelativePath, [Environment]::NewLine, $TrackedText)
    }
}

Write-Host ''
foreach ($Target in $Targets)
{
    Write-Host ("[DELETE] {0} ({1})" -f $Target.RelativePath, (Format-ByteSize $Target.Bytes))
    Remove-Item -LiteralPath $Target.FullPath -Recurse -Force
}

Write-Host ''
Write-Host ("[OK] Nettoyage termine. Volume supprime estime : {0}" -f (Format-ByteSize $TotalBytes))
Write-Host 'Conserves volontairement : Content, Source, Config, Build, Plugins, ThirdParty, docs, Scripts, Design, Meshes, Textures, Sounds, Fonts, Saved\Autosaves, Saved\Config, Saved\SaveGames et Saved\Screenshots.'
