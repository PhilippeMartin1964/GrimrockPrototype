param(
    [string]$EngineRoot = $env:UE_ROOT
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$RepoRoot = Split-Path -Parent $PSScriptRoot
$ProjectFile = Join-Path $RepoRoot 'GrimrockPrototype.uproject'

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

function Find-PluginDescriptor
{
    param(
        [string]$PluginName,
        [string]$ResolvedEngineRoot
    )

    $ProjectPluginPath = Join-Path $RepoRoot ("Plugins\{0}\{0}.uplugin" -f $PluginName)
    if (Test-Path -LiteralPath $ProjectPluginPath -PathType Leaf)
    {
        return (Resolve-Path -LiteralPath $ProjectPluginPath).Path
    }

    $EnginePluginsRoot = Join-Path $ResolvedEngineRoot 'Engine\Plugins'
    if (-not (Test-Path -LiteralPath $EnginePluginsRoot -PathType Container))
    {
        throw "Dossier Engine Plugins introuvable : $EnginePluginsRoot"
    }

    $Descriptor = Get-ChildItem -LiteralPath $EnginePluginsRoot -Recurse -File -Filter "$PluginName.uplugin" |
        Select-Object -First 1

    if ($null -ne $Descriptor)
    {
        return $Descriptor.FullName
    }

    return $null
}

if (-not (Test-Path -LiteralPath $ProjectFile -PathType Leaf))
{
    throw "Projet Unreal introuvable : $ProjectFile"
}

$ResolvedEngineRoot = Resolve-GrimrockEngineRoot -RequestedRoot $EngineRoot
$Project = Get-Content -LiteralPath $ProjectFile -Raw | ConvertFrom-Json

Write-Host 'GrimrockPrototype TD07.1 - Project Dependency Check'
Write-Host "Repository : $RepoRoot"
Write-Host "Project    : $ProjectFile"
Write-Host "Engine     : $ResolvedEngineRoot"
Write-Host ''

$PluginReferences = @($Project.Plugins)
$EnabledPlugins = @($PluginReferences | Where-Object { $_.Enabled -eq $true })
$DisabledOptionalPlugins = @($PluginReferences | Where-Object { $_.Enabled -eq $false -and $_.Optional -eq $true })

foreach ($Plugin in $EnabledPlugins)
{
    $Descriptor = Find-PluginDescriptor -PluginName ([string]$Plugin.Name) -ResolvedEngineRoot $ResolvedEngineRoot
    if ([string]::IsNullOrWhiteSpace($Descriptor))
    {
        throw "Plugin active introuvable : $($Plugin.Name)"
    }

    Write-Host "[OK] Enabled plugin $($Plugin.Name) -> $Descriptor"
}

foreach ($Plugin in $DisabledOptionalPlugins)
{
    $LocalDescriptor = Join-Path $RepoRoot ("Plugins\{0}\{0}.uplugin" -f [string]$Plugin.Name)
    if (Test-Path -LiteralPath $LocalDescriptor -PathType Leaf)
    {
        Write-Host "[INFO] Optional disabled plugin $($Plugin.Name) is installed locally and is not required by the project."
    }
    else
    {
        Write-Host "[OK] Optional disabled plugin $($Plugin.Name) is absent, as allowed by the project contract."
    }
}

Write-Host ''
Write-Host "Enabled plugins validated : $($EnabledPlugins.Count)"
Write-Host "Optional disabled plugins : $($DisabledOptionalPlugins.Count)"
Write-Host '[OK] Project dependency contract validated.'
exit 0
