[CmdletBinding()]
param([string]$EngineRoot = 'D:\UE_5.5', [switch]$Apply)
# ARMEMENT-LOT02.12: batch authoring via the canonical C++ commandlet.
# No Git staging, commit, push, asset removal or legacy migration.
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$Repo = Split-Path -Parent $PSScriptRoot
$Manifest = Join-Path $Repo 'Config\Armement\WeaponProductionManifest.csv'
$Project = Join-Path $Repo 'GrimrockPrototype.uproject'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Out = Join-Path $Repo 'Saved\ARMEMENT-LOT02'
$Validator = Join-Path $PSScriptRoot 'ValidateUE.ps1'
$Intake = Join-Path $PSScriptRoot 'AuditStarterWeaponArt.ps1'
$Ids = @('ARM038','ARM048','ARM054','ARM057','ARM058',
         'ARM061','ARM062','ARM067','ARM069','ARM070','ARM081')
$Shared = @(
    'Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.uasset'
)
if ((& git -C $Repo branch --show-current).Trim() -ne 'master' -or $LASTEXITCODE -ne 0) {
    throw 'Master is required.'
}
if (-not (Test-Path -LiteralPath $Editor -PathType Leaf)) {
    throw "Unreal commandlet missing: $Editor"
}
$Staged = @(& git -C $Repo diff --cached --name-only)
if ($LASTEXITCODE -ne 0 -or $Staged.Count -ne 0) { throw 'Git index must be empty.' }
foreach ($Rel in $Shared) {
    if (@(& git -C $Repo status --porcelain=v1 -- $Rel).Count -gt 0) {
        throw "Shared catalogue already modified: $Rel"
    }
}
$ManifestRows = @(Import-Csv -LiteralPath $Manifest)
$Targets = @($ManifestRows | Where-Object { $_.ArchetypeId -in $Ids })
if ($Targets.Count -ne 11 -or @($Targets | Where-Object {
    $_.ProductionState -ne 'Planned'
}).Count -ne 0) { throw 'Exactly 11 Planned rows required.' }
& $Intake -NoThrow | Out-Host
$FileRows = @(Import-Csv -LiteralPath (Join-Path $Out 'LOT02_11_ArtIntake.csv'))
if ($FileRows.Count -ne 11 -or @($FileRows | Where-Object {
    $_.Found -ne '6' -or $_.Status -ne 'FILES_PRESENT_UE_UNVERIFIED'
}).Count -ne 0) { throw 'Incomplete art intake; abort.' }

# Compile once; skip any repeated rebuild and do one final automation suite.
& $Validator -EngineRoot $EngineRoot -SkipAutomation
if ($LASTEXITCODE -ne 0 -or -not $?) { throw 'Editor build failed.' }
New-Item -ItemType Directory -Force -Path $Out | Out-Null

function Invoke-Production([string]$Action, [string]$Id, [string]$Stage, [bool]$Write) {
    $Stem = 'LOT02_12_' + $Action + '_' + $(if ($Id) { $Id } else { 'All' })
    if ($Stage) { $Stem += '_' + $Stage }
    $Report = Join-Path $Out ($Stem + '.csv')
    $Log = Join-Path $Out ($Stem + '.log')
    $ArgsUE = @($Project, '-run=GridWeaponPipeline', "-Action=$Action",
        "-Manifest=$Manifest", "-Report=$Report",
        '-Unattended','-NoSplash','-NoP4','-NoSound','-NullRHI','-log')
    if ($Id) { $ArgsUE += "-Archetype=$Id" }
    if ($Stage) { $ArgsUE += "-Stage=$Stage" }
    if ($Write) { $ArgsUE += '-Apply' }
    & $Editor @ArgsUE *> $Log
    $Code = $LASTEXITCODE
    $Messages = @(Select-String -LiteralPath $Log -Pattern '\[INDUS02\]' |
        ForEach-Object { $_.Line })
    if ($Code -ne 0 -or -not (Test-Path -LiteralPath $Report -PathType Leaf)) {
        $Messages | ForEach-Object { Write-Host $_ }
        throw "Commandlet failure: $Action $Id $Stage (exit $Code). Log: $Log"
    }
    $Rows = @(Import-Csv -LiteralPath $Report)
    $Expected = if ($Id) { 1 } else { $ManifestRows.Count }
    if ($Rows.Count -ne $Expected) { throw "Unexpected report count: $Stem" }
    if ($Write) {
        $Pattern = '\[INDUS02\] (SAVED|PRESENT_SAME) ' + $Stage +
                   ' for ' + $Id + '(;|$)'
        if (@($Messages | Where-Object { $_ -match $Pattern }).Count -ne 1) {
            $Messages | ForEach-Object { Write-Host $_ }
            throw "No positive stage confirmation: $Stem"
        }
    }
    Write-Host ("[LOT02.12] {0} {1} {2}: {3}" -f $Action,$Id,$Stage,
        (($Rows | Where-Object { -not $Id -or $_.ArchetypeId -eq $Id } |
          ForEach-Object { $_.Status }) -join ','))
    return $Rows
}

$Plan = @(Invoke-Production 'Plan' '' '' $false)
$Relevant = @($Plan | Where-Object { $_.ArchetypeId -in $Ids })
$Relevant | Format-Table ArchetypeId,ProductionState,Art,Status,Detail -AutoSize
if ($Relevant.Count -ne 11 -or @($Relevant | Where-Object {
    $_.ProductionState -ne 'Planned' -or $_.Art -ne 'READY' -or
    $_.Status -in @('CONFLICT','BLOCKED','ART_MISSING')
}).Count -gt 0) { throw 'Preflight failed; no authoring allowed.' }
if (-not $Apply) {
    Write-Host '[LOT02.12] All 11 ready. Dry run only; rerun with -Apply to author.'
    return
}

$Before = @(& git -C $Repo status --porcelain=v1 --untracked-files=all)
$HashRpgBefore = (Get-FileHash -LiteralPath (Join-Path $Repo $Shared[0]) -Algorithm SHA256).Hash
foreach ($Id in $Ids) {
    Write-Host "=== [LOT02.12] $Id ==="
    foreach ($Stage in @('Visual','Item','Palette')) {
        $Result = @(Invoke-Production 'Build' $Id $Stage $true)
        if ($Result.Count -ne 1 -or $Result[0].Art -ne 'READY' -or
            $Result[0].Status -eq 'CONFLICT') {
            throw "Aborted $Id $Stage; no rollback performed."
        }
    }
}
$Final = @(Invoke-Production 'Audit' '' '' $false)
$New = @($Final | Where-Object { $_.ArchetypeId -in $Ids })
$New | Format-Table ArchetypeId,Art,Visual,Item,Palette,Status -AutoSize
if ($New.Count -ne 11 -or @($New | Where-Object {
    $_.Status -ne 'COMPLETE' -or $_.Art -ne 'READY' -or
    $_.Visual -ne 'PRESENT_SAME' -or
    $_.Item -ne 'PRESENT_SAME' -or
    $_.Palette -ne 'PRESENT_SAME'
}).Count -gt 0) { throw 'Not all eleven definitions are COMPLETE.' }
$HashRpgAfter = (Get-FileHash -LiteralPath (Join-Path $Repo $Shared[0]) -Algorithm SHA256).Hash
if ($HashRpgAfter -ne $HashRpgBefore) { throw 'RPG profile catalogue was modified.' }

& $Validator -EngineRoot $EngineRoot -SkipBuild -AutomationFilter 'Grimrock.RPG.ARMEMENT.Industrial'
if ($LASTEXITCODE -ne 0 -or -not $?) { throw 'Industrial regression failed; no Git action taken.' }

$After = @(& git -C $Repo status --porcelain=v1 --untracked-files=all)
$Changes = @($After | Where-Object { $_ -notin $Before })
$Allowed = @($Shared[1],$Shared[2])
foreach ($Target in $Targets) {
    $Obj = ($Target.ItemPath -split '\.')[0]
    $Allowed += 'Content/' + $Obj.Substring('/Game/'.Length) + '.uasset'
}
$Unexpected = @($Changes | Where-Object {
    $_.Length -lt 4 -or $_.Substring(3).Replace('\','/') -notin $Allowed
})
if ($Unexpected.Count -gt 0) {
    $Unexpected | ForEach-Object { Write-Host $_ }
    throw 'Unexpected project changes; no Git action taken.'
}
Write-Host '[LOT02.12] 11/11 COMPLETE + Industrial suite PASS.'
Write-Host '[LOT02.12] Not activated in kits; no stage/commit/push; PIE still required.'
& git -C $Repo status --short --untracked-files=all
