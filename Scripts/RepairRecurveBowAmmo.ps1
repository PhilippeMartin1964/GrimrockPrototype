[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\UE_5.5',
    [switch]$Apply
)
# LOT02.13.1: single package migration via existing C++ Marksman ammo commandlet.
# No automatic git staging, commit or push. Old short bow and arrow are read-only.
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$Repo = Split-Path -Parent $PSScriptRoot
$Project = Join-Path $Repo 'GrimrockPrototype.uproject'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Output = Join-Path $Repo 'Saved\ARMEMENT-LOT02'
$Bow = 'Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_RecurveBow_01.uasset'
$ShortBow = 'Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_ShortBow_01.uasset'
$Arrow = 'Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Arrows_01.uasset'
if ((& git -C $Repo branch --show-current).Trim() -ne 'master' -or $LASTEXITCODE -ne 0) {
    throw 'Master only.'
}
if (-not (Test-Path -LiteralPath $Editor -PathType Leaf)) { throw "Missing $Editor" }
if (@(& git -C $Repo diff --cached --name-only).Count -ne 0) { throw 'Git index must be empty.' }
if (@(& git -C $Repo status --porcelain=v1 -- @($Bow,$ShortBow,$Arrow)).Count -ne 0) {
    throw 'Recurve bow, historical short bow or arrows have pre-existing local changes.'
}
foreach ($File in @($Bow,$ShortBow,$Arrow)) {
    if (-not (Test-Path -LiteralPath (Join-Path $Repo $File) -PathType Leaf)) {
        throw "Missing existing package: $File"
    }
}
$BeforeOther = @(& git -C $Repo status --porcelain=v1 --untracked-files=all)
$ShortBowHash = (Get-FileHash -LiteralPath (Join-Path $Repo $ShortBow) -Algorithm SHA256).Hash
$ArrowHash = (Get-FileHash -LiteralPath (Join-Path $Repo $Arrow) -Algorithm SHA256).Hash
& (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipAutomation
if ($LASTEXITCODE -ne 0 -or -not $?) { throw 'UE5.5.4 build failed.' }
New-Item -ItemType Directory -Path $Output -Force | Out-Null
$ArgsEditor = @($Project, '-run=GridMarksmanAmmunitionAuthoring',
    '-Archetype=ARM048','-Unattended','-NoSplash','-NoP4','-NoSound','-NullRHI','-log')
if ($Apply) { $ArgsEditor += '-Apply' }
$Log = Join-Path $Output 'LOT02_13_1_RecurveBowAmmo.log'
& $Editor @ArgsEditor *> $Log
$Code = $LASTEXITCODE
$Messages = @(Select-String -LiteralPath $Log -Pattern '\[LOT02\.13\.1\]' |
    ForEach-Object { $_.Line })
$Messages | ForEach-Object { Write-Host $_ }
if ($Code -ne 0) { throw "ARM048 ammo link failed, exit $Code. Log: $Log" }
$Expected = if ($Apply) { '(SAVED|PRESENT_SAME)' } else { '(DRY_RUN|PRESENT_SAME)' }
if (@($Messages | Where-Object { $_ -match ("\[LOT02\.13\.1\] " + $Expected + ' ARM048|\[LOT02\.13\.1\] DRY_RUN no package written') }).Count -ne 1) {
    throw 'Missing explicit migration result in commandlet diagnostics.'
}
if (-not $Apply) {
    Write-Host '[LOT02.13.1] Read-only preflight succeeded.'
    return
}
if ((Get-FileHash -LiteralPath (Join-Path $Repo $ShortBow) -Algorithm SHA256).Hash -ne $ShortBowHash -or
    (Get-FileHash -LiteralPath (Join-Path $Repo $Arrow) -Algorithm SHA256).Hash -ne $ArrowHash) {
    throw 'Historical bow or arrow unexpectedly modified.'
}
$After = @(& git -C $Repo status --porcelain=v1 --untracked-files=all)
$Changes = @($After | Where-Object { $_ -notin $BeforeOther })
if (@($Changes | Where-Object { $_.Length -lt 4 -or $_.Substring(3).Replace('\','/') -ne $Bow }).Count -gt 0) {
    $Changes | ForEach-Object { Write-Host $_ }
    throw 'Unexpected local changes; do not stage or publish.'
}
Write-Host '[LOT02.13.1] Validating LOT02 and Industrial without staging.'
& (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipBuild -AutomationFilter 'Grimrock.RPG.ARMEMENT.LOT02'
if ($LASTEXITCODE -ne 0 -or -not $?) { throw 'LOT02 regression failed; no Git change.' }
& (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipBuild -AutomationFilter 'Grimrock.RPG.ARMEMENT.Industrial'
if ($LASTEXITCODE -ne 0 -or -not $?) { throw 'Industrial regression failed; no Git change.' }
$Matrix = @(Import-Csv -LiteralPath (Join-Path $Output 'LOT02_13_ProductionFunctionalMatrix.csv'))
$Arm048 = @($Matrix | Where-Object { $_.ArchetypeId -eq 'ARM048' })
if ($Arm048.Count -ne 1 -or $Arm048[0].Ammo -ne 'ARROWS_LINKED_PIE_REQUIRED' -or
    $Arm048[0].Status -ne 'AUTOMATED_PASS_PIE_PENDING') {
    throw 'ARM048 remains blocked by the functional matrix.'
}
Write-Host '[LOT02.13.1] ARM048 arrow link accepted; PIE still required. No Git staging or commit.'
& git -C $Repo status --short
