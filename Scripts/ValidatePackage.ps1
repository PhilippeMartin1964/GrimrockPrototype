[CmdletBinding()]
param(
    [string]$EngineRoot = $env:UE_ROOT,
    [string]$ArchiveRoot,
    [string]$Configuration = 'Shipping',
    [switch]$Help
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$RepoRoot = Split-Path -Parent $PSScriptRoot
$ProjectFile = Join-Path $RepoRoot 'GrimrockPrototype.uproject'
$GameTarget = 'GrimrockPrototype'
$Platform = 'Win64'
$ValidConfigurations = @('Development', 'Shipping')

function Show-GrimrockPackageUsage
{
    Write-Host @'
GrimrockPrototype TD04.3 - Cook / Package Validation

Usage:
  .\Scripts\ValidatePackage.ps1 -EngineRoot <UE_ROOT> [-Configuration Shipping|Development] [-ArchiveRoot <path>]
  .\Scripts\ValidatePackage.ps1 -Help

Parameters:
  -EngineRoot       Racine Unreal Engine contenant Engine\Build\BatchFiles\RunUAT.bat.
                    Peut etre omis si UE_ROOT est defini.
  -Configuration    Shipping (defaut) ou Development.
  -ArchiveRoot      Dossier parent des archives.
                    Defaut : <Repo>\Saved\Packaging\TD04
  -Help             Affiche cette aide sans lancer de build.

Examples:
  .\Scripts\ValidatePackage.ps1 -EngineRoot D:\UE_5.5

  .\Scripts\ValidatePackage.ps1 `
      -EngineRoot D:\UE_5.5 `
      -Configuration Shipping

  .\Scripts\ValidatePackage.ps1 `
      -EngineRoot D:\UE_5.5 `
      -Configuration Development `
      -ArchiveRoot D:\Development\GrimrockPrototype\Saved\Packaging\Diagnostics
'@
}

function Resolve-GrimrockEngineRoot
{
    param([string]$RequestedRoot)

    if ([string]::IsNullOrWhiteSpace($RequestedRoot))
    {
        return $null
    }

    if (-not (Test-Path -LiteralPath $RequestedRoot -PathType Container))
    {
        Write-Host "[MISSING] Racine Unreal Engine introuvable : $RequestedRoot"
        return $null
    }

    return (Resolve-Path -LiteralPath $RequestedRoot).Path
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

if ($Help)
{
    Show-GrimrockPackageUsage
    exit 0
}

if (-not ($ValidConfigurations -contains $Configuration))
{
    Write-Host "[INVALID] Configuration '$Configuration'. Valeurs possibles : Development, Shipping."
    Write-Host ''
    Show-GrimrockPackageUsage
    exit 2
}

if ([string]::IsNullOrWhiteSpace($EngineRoot))
{
    Write-Host '[INFO] Aucune racine Unreal Engine n''a ete fournie.'
    Write-Host 'Utilisez -EngineRoot ou definissez la variable d''environnement UE_ROOT.'
    Write-Host ''
    Show-GrimrockPackageUsage
    exit 2
}

if (-not (Test-Path -LiteralPath $ProjectFile -PathType Leaf))
{
    Write-Host "[MISSING] Projet Unreal introuvable : $ProjectFile"
    exit 3
}

$ResolvedEngineRoot = Resolve-GrimrockEngineRoot -RequestedRoot $EngineRoot
if ($null -eq $ResolvedEngineRoot)
{
    Write-Host ''
    Show-GrimrockPackageUsage
    exit 3
}

$RunUAT = Join-Path $ResolvedEngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
$AutomationToolDll = Join-Path $ResolvedEngineRoot 'Engine\Binaries\DotNET\AutomationTool\AutomationTool.dll'
$AutomationToolCsproj = Join-Path $ResolvedEngineRoot 'Engine\Source\Programs\AutomationTool\AutomationTool.csproj'
$AutomationToolLauncherCsproj = Join-Path $ResolvedEngineRoot 'Engine\Source\Programs\AutomationToolLauncher\AutomationToolLauncher.csproj'
$BuildUAT = Join-Path $ResolvedEngineRoot 'Engine\Build\BatchFiles\BuildUAT.bat'
$InstalledBuildMarker = Join-Path $ResolvedEngineRoot 'Engine\Build\InstalledBuild.txt'

$HasRunUAT = Test-Path -LiteralPath $RunUAT -PathType Leaf
$HasPrecompiledAutomationTool = Test-Path -LiteralPath $AutomationToolDll -PathType Leaf
$HasAutomationToolProject = Test-Path -LiteralPath $AutomationToolCsproj -PathType Leaf
$HasAutomationToolLauncherProject = Test-Path -LiteralPath $AutomationToolLauncherCsproj -PathType Leaf
$HasBuildUAT = Test-Path -LiteralPath $BuildUAT -PathType Leaf
$HasSourceAutomationTool = $HasAutomationToolProject -and $HasAutomationToolLauncherProject -and $HasBuildUAT
$IsInstalledBuild = Test-Path -LiteralPath $InstalledBuildMarker -PathType Leaf

Write-Host 'GrimrockPrototype TD04.3 - Cook / Package Validation'
Write-Host "Repository    : $RepoRoot"
Write-Host "Project       : $ProjectFile"
Write-Host "Engine        : $ResolvedEngineRoot"
Write-Host "Target        : $GameTarget"
Write-Host "Platform      : $Platform"
Write-Host "Configuration : $Configuration"

Write-Host ''
Write-Host '=== Packaging prerequisite check ==='

if ($HasRunUAT)
{
    Write-Host "[OK] RunUAT.bat : $RunUAT"
}
else
{
    Write-Host "[MISSING] RunUAT.bat : $RunUAT"
}

if ($HasPrecompiledAutomationTool)
{
    Write-Host "[OK] AutomationTool precompiled : $AutomationToolDll"
}
elseif ($HasSourceAutomationTool)
{
    Write-Host '[OK] AutomationTool precompiled absent, but source rebuild prerequisites are present.'
    Write-Host "     AutomationTool.csproj         : $AutomationToolCsproj"
    Write-Host "     AutomationToolLauncher.csproj : $AutomationToolLauncherCsproj"
    Write-Host "     BuildUAT.bat                   : $BuildUAT"
}
else
{
    Write-Host "[MISSING] AutomationTool.dll : $AutomationToolDll"
    Write-Host ('          AutomationTool.csproj         : ' + $(if ($HasAutomationToolProject) { 'present' } else { 'missing' }))
    Write-Host ('          AutomationToolLauncher.csproj : ' + $(if ($HasAutomationToolLauncherProject) { 'present' } else { 'missing' }))
    Write-Host ('          BuildUAT.bat                   : ' + $(if ($HasBuildUAT) { 'present' } else { 'missing' }))
}

if (-not $HasRunUAT -or (-not $HasPrecompiledAutomationTool -and -not $HasSourceAutomationTool))
{
    Write-Host ''
    Write-Host '[FAIL] L''installation Unreal Engine ne contient pas les prerequis necessaires au packaging.'

    if ($IsInstalledBuild)
    {
        Write-Host 'Cette installation semble etre une build installee/precompilee.'
        Write-Host 'Action conseillee : lancer "Verify / Verifier" sur Unreal Engine 5.5.4 dans Epic Games Launcher.'
    }
    else
    {
        Write-Host 'Action conseillee : restaurer/recompiler les fichiers Engine manquants, puis verifier AutomationTool.'
    }

    Write-Host ''
    Write-Host 'Le packaging n''a pas ete lance et aucun dossier d''archive de session n''a ete cree.'
    exit 4
}

if ([string]::IsNullOrWhiteSpace($ArchiveRoot))
{
    $ArchiveRoot = Join-Path $RepoRoot 'Saved\Packaging\TD04'
}

$Timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$SessionName = "TD04-$Configuration-$Timestamp"
$SessionArchivePath = Join-Path $ArchiveRoot $SessionName
New-Item -ItemType Directory -Path $SessionArchivePath -Force | Out-Null

Write-Host "[OK] Packaging prerequisites validated."
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

# UI-RPG-PACK01: a successful cook does not imply dynamically loaded DataAssets are staged.
# Scan the actual packaged PAK contents, never the Editor Asset Registry or the source Content folder.
$UnrealPak = Join-Path $ResolvedEngineRoot 'Engine\\Binaries\\Win64\\UnrealPak.exe'
if (-not (Test-Path -LiteralPath $UnrealPak -PathType Leaf))
{
    throw "Verification des assets Shipping impossible : UnrealPak.exe introuvable : $UnrealPak"
}

Write-Host ''
Write-Host '=== UI-RPG packaged asset validation ==='
$PackagedAssetListing = New-Object System.Collections.Generic.List[string]
foreach ($PakFile in $PakFiles)
{
    $Listing = @(& $UnrealPak $PakFile.FullName '-List' 2>&1)
    if ($LASTEXITCODE -ne 0)
    {
        throw "UnrealPak -List a echoue pour : $($PakFile.FullName)"
    }
    $PackagedAssetListing.Add(($Listing -join "`n"))
}
$PackagedPaths = $PackagedAssetListing -join "`n"
$PresentationPattern = '(?i)[/\\]GrimrockPrototype[/\\]Core[/\\]DataAssets[/\\]UI[/\\]RPG[/\\]DA_RPGTalentPresentation\\.uasset'
$SkillsPattern = '(?i)[/\\]GrimrockPrototype[/\\]Core[/\\]DataAssets[/\\]RPG[/\\]Skills[/\\](DA_Skill_[A-Za-z0-9_]+\\.uasset)'
$PresentationFound = [regex]::IsMatch($PackagedPaths, $PresentationPattern)
$SkillAssets = @(
    [regex]::Matches($PackagedPaths, $SkillsPattern) |
        ForEach-Object { $_.Groups[1].Value } |
        Sort-Object -Unique
)
Write-Host ('Talent presentation : ' + $(if ($PresentationFound) { '[OK]' } else { '[MISSING]' }))
Write-Host "RPG Skill assets    : $($SkillAssets.Count) / 25"
if (-not $PresentationFound -or $SkillAssets.Count -ne 25)
{
    throw 'UI-RPG-PACK01 failed: Shipping PAK is missing DA_RPGTalentPresentation or one or more of the 25 canonical DA_Skill_* assets.'
}
Write-Host '[OK] UI-RPG Shipping DataAssets verified in packaged PAK.'

$ArchiveFiles = @(Get-ChildItem -LiteralPath $SessionArchivePath -Recurse -File)
$ArchiveBytes = 0L
foreach ($ArchiveFile in $ArchiveFiles)
{
    $ArchiveBytes += [long]$ArchiveFile.Length
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
