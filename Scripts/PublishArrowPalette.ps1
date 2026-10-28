[CmdletBinding()]
param([string]$EngineRoot = 'D:\UE_5.5', [switch]$Apply)
# LOT02.13.2.2: one canonical palette entry; no Git mutation.
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$Repo = Split-Path -Parent $PSScriptRoot
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Project = Join-Path $Repo 'GrimrockPrototype.uproject'
$Palette = 'Content/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.uasset'
$Arrows = 'Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Arrows_01.uasset'
$ShortBow = 'Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_ShortBow_01.uasset'
$Recurve = 'Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_RecurveBow_01.uasset'
$Protected = @($Arrows,$ShortBow,$Recurve)
$LogDir = Join-Path $Repo 'Saved\ARMEMENT-LOT02'
if ((& git -C $Repo branch --show-current).Trim() -ne 'master' -or $LASTEXITCODE -ne 0) {
    throw 'Master required.'
}
if (-not (Test-Path -LiteralPath $Editor -PathType Leaf)) { throw "Missing $Editor" }
if (@(& git -C $Repo diff --cached --name-only).Count -gt 0) {
    throw 'Git staging must be empty.'
}
if (@(& git -C $Repo status --porcelain=v1 -- @($Palette + $Protected)).Count -gt 0) {
    throw 'Palette, arrows or bows are already locally modified.'
}
foreach ($RelativePath in @($Palette + $Protected)) {
    if (-not (Test-Path -LiteralPath (Join-Path $Repo $RelativePath) -PathType Leaf)) {
        throw "Missing required package: $RelativePath"
    }
}
$Hashes = @{}
foreach ($File in $Protected) {
    $Hashes[$File] = (Get-FileHash -LiteralPath (Join-Path $Repo $File) -Algorithm SHA256).Hash
}
$Before = @(& git -C $Repo status --porcelain=v1 --untracked-files=all)
& (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipAutomation
if ($LASTEXITCODE -ne 0 -or -not $?) { throw 'Editor build failed.' }
New-Item -ItemType Directory -Path $LogDir -Force | Out-Null
$ArgsUE = @($Project, '-run=GridArrowPaletteAuthoring',
    '-Unattended','-NoSplash','-NoP4','-NoSound','-NullRHI','-log')
if ($Apply) { $ArgsUE += '-Apply' }
$Log = Join-Path $LogDir 'LOT02_13_2_2_ArrowPalette.log'
& $Editor @ArgsUE *> $Log
$ExitCode = $LASTEXITCODE
$Messages = @(Select-String -LiteralPath $Log -Pattern '\[LOT02\.13\.2\.2\]' |
    ForEach-Object { $_.Line })
$Messages | ForEach-Object { Write-Host $_ }
if ($ExitCode -ne 0) { throw "Arrow palette commandlet failed ($ExitCode): $Log" }
$ExpectedPattern = if ($Apply) { '(SAVED|PRESENT_SAME)' } else { '(DRY_RUN|PRESENT_SAME)' }
if (@($Messages | Where-Object { $_ -match ("\[LOT02\.13\.2\.2\] " + $ExpectedPattern) }).Count -ne 1) {
    throw 'Missing unambiguous authoring result.'
}
if (-not $Apply) {
    Write-Host '[LOT02.13.2.2] Preflight OK. No packages written.'
    return
}
foreach ($File in $Protected) {
    if ((Get-FileHash -LiteralPath (Join-Path $Repo $File) -Algorithm SHA256).Hash -ne $Hashes[$File]) {
        throw "Protected package changed: $File"
    }
}
$After = @(& git -C $Repo status --porcelain=v1 --untracked-files=all)
$NewChanges = @($After | Where-Object { $_ -notin $Before })
if (@($NewChanges | Where-Object {
    $_.Length -lt 4 -or $_.Substring(3).Replace('\','/') -cne $Palette
}).Count -gt 0) {
    throw 'Unexpected file modifications; do not commit.'
}
& (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipBuild -AutomationFilter 'Grimrock.RPG.ARMEMENT.LOT02'
if ($LASTEXITCODE -ne 0 -or -not $?) { throw 'LOT02 automation failed; no commit made.' }
Write-Host '[LOT02.13.2.2] Arrow palette registered and LOT02 passed; visuals remain pending.'
Write-Host '[LOT02.13.2.2] No git add, commit or push.'
& git -C $Repo status --short
