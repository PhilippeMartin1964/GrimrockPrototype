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

if (-not $HasRunUAT)
{
    Write-Host "[MISSING] RunUAT.bat : $RunUAT"
    Write-Host '[FAIL] Verifier la racine Unreal Engine et restaurer RunUAT.bat.'
    Write-Host 'Le packaging n''a pas ete lance et aucun dossier d''archive de session n''a ete cree.'
    exit 4
}

Write-Host "[OK] RunUAT.bat : $RunUAT"

if ($HasPrecompiledAutomationTool)
{
    Write-Host "[OK] AutomationTool precompiled : $AutomationToolDll"
}
else
{
    # A missing DLL or optional batch file alone does not prove that UAT
    # cannot start. Let the engine's own launcher handle its bootstrap.
    Write-Host "[INFO] AutomationTool.dll absent : $AutomationToolDll"
    Write-Host ('       AutomationTool.csproj         : ' + $(if ($HasAutomationToolProject) { 'present' } else { 'missing' }))
    Write-Host ('       AutomationToolLauncher.csproj : ' + $(if ($HasAutomationToolLauncherProject) { 'present' } else { 'missing' }))
    Write-Host ('       BuildUAT.bat                   : ' + $(if ($HasBuildUAT) { 'present' } else { 'missing' }))
    Write-Host ''
    Write-Host '=== AutomationTool bootstrap check (BuildCookRun -Help) ==='
    Write-Host "Executable : $RunUAT"
    Write-Host 'Arguments  : BuildCookRun -Help'

    & $RunUAT 'BuildCookRun' '-Help'
    $BootstrapExitCode = $LASTEXITCODE
    if ($BootstrapExitCode -ne 0)
    {
        Write-Host ''
        Write-Host "[FAIL] Le demarrage natif d'AutomationTool a echoue (code $BootstrapExitCode)."
        if ($IsInstalledBuild)
        {
            Write-Host 'Build installee/precompilee : pour une installation Launcher, utiliser Verify / Verifier sur UE 5.5.4.'
        }
        else
        {
            Write-Host 'Consulter la premiere erreur UAT et restaurer/recompiler les fichiers Engine necessaires.'
        }
        Write-Host 'Le packaging n''a pas ete lance et aucun dossier d''archive de session n''a ete cree.'
        exit 4
    }

    Write-Host '[OK] AutomationTool / BuildCookRun help returned exit code 0.'
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

# UI-RPG-PACK01: dynamically loaded RPG DataAssets must exist in the cooked
# Windows content consumed by the staging/IoStore steps. This validates the
# authoritative cook output without assuming a specific PAK/IoStore listing CLI.
$CookedRpgDataRoot = Join-Path $RepoRoot 'Saved\Cooked\Windows\GrimrockPrototype\Content\GrimrockPrototype\Core\DataAssets'
$CookedTalentPresentation = Join-Path $CookedRpgDataRoot 'UI\RPG\DA_RPGTalentPresentation.uasset'
$CookedSkillsRoot = Join-Path $CookedRpgDataRoot 'RPG\Skills'

Write-Host ''
Write-Host '=== UI-RPG cooked asset validation ==='

$PresentationFound = Test-Path -LiteralPath $CookedTalentPresentation -PathType Leaf
$SkillAssets = @()
if (Test-Path -LiteralPath $CookedSkillsRoot -PathType Container)
{
    $SkillAssets = @(
        Get-ChildItem -LiteralPath $CookedSkillsRoot -File -Filter 'DA_Skill_*.uasset' |
            Sort-Object Name
    )
}

Write-Host ('Talent presentation : ' + $(if ($PresentationFound) { '[OK]' } else { '[MISSING]' }))
Write-Host "RPG Skill assets    : $($SkillAssets.Count) / 25"

if (-not $PresentationFound -or $SkillAssets.Count -ne 25)
{
    throw 'UI-RPG-PACK01 failed: cooked Windows content is missing DA_RPGTalentPresentation or one or more of the 25 canonical DA_Skill_* assets.'
}

Write-Host '[OK] UI-RPG Shipping DataAssets verified in cooked Windows content.'

# ARMEMENT01.17.1.4.SHIPPING01: C++ dynamically loads the oil decal instance,
# so full cook must preserve both native master and instance, unlike PIE.
$CookedOilRoot = Join-Path $RepoRoot 'Saved\Cooked\Windows\GrimrockPrototype\Content\GrimrockPrototype\Art\Materials\Surfaces'
$CookedOilMaster = Join-Path $CookedOilRoot 'M_Surface_Oil.uasset'
$CookedOilInstance = Join-Path $CookedOilRoot 'MI_Surface_Oil.uasset'

Write-Host ''
Write-Host '=== Shipping oil decal cooked asset validation ==='
$OilMasterFound = Test-Path -LiteralPath $CookedOilMaster -PathType Leaf
$OilInstanceFound = Test-Path -LiteralPath $CookedOilInstance -PathType Leaf
Write-Host ('Oil master  : ' + $(if ($OilMasterFound) { '[OK]' } else { '[MISSING]' }))
Write-Host ('Oil instance: ' + $(if ($OilInstanceFound) { '[OK]' } else { '[MISSING]' }))
if (-not $OilMasterFound -or -not $OilInstanceFound)
{
    throw 'ARMEMENT01.17.1.4 Shipping failed: cooked M_Surface_Oil or MI_Surface_Oil missing.'
}
Write-Host '[OK] Both oil decal resources present in cooked Windows content.'


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
