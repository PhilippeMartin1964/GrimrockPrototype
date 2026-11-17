<#
 ARM-CLEAN06.4X.54: protected native four-package UE5.5.4 transaction.
 No automatic invocation. Default is read-only. -Apply requires a clean
 master at the exact SHA, four private SHA256 backups and byte-equal receipts.
 -Publish requires -Apply, fresh UE Verify and full Grimrock tests, then
 one allowlisted binary-only fast-forward commit. Never changes SaveGames.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$RepoRoot,
    [Parameter(Mandatory=$true)][string]$EngineRoot,
    [Parameter(Mandatory=$true)][string]$StateRoot,
    [Parameter(Mandatory=$true)][string]$ExpectedHead,
    [switch]$Apply,
    [switch]$Publish
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if ($ExpectedHead -cnotmatch '^[a-f0-9]{40}$') { throw 'ExpectedHead must be exact SHA40.' }
if ($Publish -and -not $Apply) { throw '-Publish requires -Apply.' }
$RepoRoot = [IO.Path]::GetFullPath($RepoRoot).TrimEnd('\','/')
$EngineRoot = [IO.Path]::GetFullPath($EngineRoot).TrimEnd('\','/')
$StateRoot = [IO.Path]::GetFullPath($StateRoot).TrimEnd('\','/')
if ($StateRoot.Equals($RepoRoot,[StringComparison]::OrdinalIgnoreCase) -or
    $StateRoot.StartsWith(($RepoRoot+'\'),[StringComparison]::OrdinalIgnoreCase) -or
    $RepoRoot.StartsWith(($StateRoot+'\'),[StringComparison]::OrdinalIgnoreCase)) {
    throw 'StateRoot must be independent of the project and not its parent.'
}
$Relative = @(
    'Content/GrimrockPrototype/Core/DataAssets/DA_Receptacle_TorchHolder.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Guardian.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/DA_Lock_CopperWall.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/DA_Lock_IronWall.uasset'
)
$Project = Join-Path $RepoRoot 'GrimrockPrototype.uproject'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Build = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$Validator = Join-Path $RepoRoot 'Scripts\ValidateUE.ps1'
foreach ($File in @($Project,$Editor,$Build,$Validator)) {
    if (-not (Test-Path -LiteralPath $File -PathType Leaf)) {
        throw ('Missing project or UE executable: '+$File)
    }
}
$Live = @($Relative | ForEach-Object { Join-Path $RepoRoot ($_.Replace('/','\')) })
foreach ($File in $Live) {
    if (-not (Test-Path -LiteralPath $File -PathType Leaf)) {
        throw ('Missing allowlisted native package: '+$File)
    }
}
$OpenEditors = @(Get-Process -Name 'UnrealEditor','UnrealEditor-Cmd' -ErrorAction SilentlyContinue)
if ($OpenEditors.Count -ne 0) {
    throw 'Close all Unreal Editor processes before native work.'
}
function Git([string[]]$GitArguments) {
    $Out = @(& git.exe -C $RepoRoot @GitArguments)
    if ($LASTEXITCODE -ne 0) { throw ('Git failed: '+($GitArguments -join ' ')) }
    return $Out
}
function Head { return ([string]@(Git @('rev-parse','HEAD'))[0]).Trim() }
function CheckClean {
    if (([string]@(Git @('branch','--show-current'))[0]).Trim() -cne 'master') {
        throw 'Only master is allowed.'
    }
    if (@(Git @('status','--porcelain','--untracked-files=all')).Count -ne 0) {
        throw 'Non-clean user worktree; refusing native modification.'
    }
}
function Sha([string]$Path) {
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}
function NativeHeader([string]$Path) {
    $Stream = [IO.File]::OpenRead($Path)
    try {
        [byte[]]$Header = @(0,0,0,0)
        if ($Stream.Read($Header,0,4) -ne 4 -or
            $Header[0] -ne 0xC1 -or $Header[1] -ne 0x83 -or
            $Header[2] -ne 0x2A -or $Header[3] -ne 0x9E) {
            throw ('Non-hydrated or invalid UE .uasset: '+$Path)
        }
    } finally { $Stream.Dispose() }
}
function CheckReport($Report,[string]$Mode,[int]$Old,[int]$Saved) {
    if ($null -eq $Report -or
        [string]$Report.schema -cne 'ARM-CLEAN06.4X54/shared-four/v1' -or
        [string]$Report.mode -cne $Mode -or
        [int]$Report.targets -ne 4 -or
        [int]$Report.legacy_pointers -ne $Old -or
        [int]$Report.packages_saved -ne $Saved -or
        $Report.only_four_packages_allowed -ne $true -or
        $Report.savegames_touched -ne $false) {
        throw ('Strict native four-package report refused: '+$Mode)
    }
}
function RunUE([string]$Mode,[string]$Output,[string[]]$Extra,[switch]$AllowDifference) {
    $EditorArguments = @($Project,'-run=GridArmClean064X54NativeSharedRules',
        ('-'+$Mode),('-Output='+$Output),
        '-Unattended','-NoSplash','-NoP4','-NoSound','-NullRHI') + @($Extra)
    & $Editor @EditorArguments | Out-Host
    $Code = [int]$LASTEXITCODE
    if ($Code -ne 0 -and
        -not ($AllowDifference -and $Mode -ceq 'Verify' -and $Code -eq 4)) {
        throw ('Native '+$Mode+' failed: exit '+$Code)
    }
    if (-not (Test-Path -LiteralPath $Output -PathType Leaf)) {
        throw ('Missing private UE report: '+$Output)
    }
    return [pscustomobject]@{
        Exit=$Code
        Data=(Get-Content -LiteralPath $Output -Raw -Encoding UTF8 | ConvertFrom-Json)
    }
}
function CheckFourDirty {
    $Current = @(Git @('status','--porcelain','--untracked-files=all'))
    if ($Current.Count -ne 4) {
        throw ('Expected four and ONLY four changed assets, got '+$Current.Count)
    }
    foreach ($Line in $Current) {
        $Text = [string]$Line
        if ($Text.Length -lt 4 -or $Text.Substring(0,2) -cne ' M' -or
            $Relative -cnotcontains $Text.Substring(3).Replace('\','/')) {
            throw ('Unexpected modified file in native transaction: '+$Text)
        }
    }
}
CheckClean
[void](Git @('fetch','--no-tags','origin','master'))
$Remote = ([string]@(Git @('rev-parse','refs/remotes/origin/master'))[0]).Trim()
if ($Remote -cne $ExpectedHead) { throw 'origin/master differs from ExpectedHead.' }
if ((Head) -cne $ExpectedHead) {
    [void](Git @('merge','--ff-only','refs/remotes/origin/master'))
}
if ((Head) -cne $ExpectedHead) { throw 'HEAD differs from pinned SHA.' }
CheckClean
$Lfs = @(Git @('lfs','ls-files','-n'))
for ($Index=0; $Index -lt 4; ++$Index) {
    if ($Lfs -cnotcontains $Relative[$Index]) {
        throw ('Target not registered in Git LFS: '+$Relative[$Index])
    }
    NativeHeader $Live[$Index]
}
$Before = @($Live | ForEach-Object { Sha $_ })
$BuildArgs = @('GrimrockPrototypeEditor','Win64','Development',
    ('-Project='+$Project),'-WaitMutex','-NoHotReloadFromIDE')
& $Build @BuildArgs
if ($LASTEXITCODE -ne 0) { throw 'UE Editor build failed; no native file modified.' }
CheckClean
if ((Head) -cne $ExpectedHead) { throw 'HEAD changed during build.' }
for ($Index=0; $Index -lt 4; ++$Index) {
    if ((Sha $Live[$Index]) -cne $Before[$Index]) {
        throw 'Native target changed during UE build.'
    }
}
$RunId = $ExpectedHead.Substring(0,12)+'-'+(Get-Date -Format 'yyyyMMddHHmmssfff')
$Run = Join-Path $StateRoot ('ARM_CLEAN06_4X54\'+$RunId)
if (Test-Path -LiteralPath $Run) { throw 'Run directory already exists.' }
New-Item -ItemType Directory -Path $Run -Force -ErrorAction Stop | Out-Null
$Pre = RunUE 'Preflight' (Join-Path $Run 'preflight.json') @()
$Old = [int]$Pre.Data.legacy_pointers
if ($Old -ne 0 -and $Old -ne 4) { throw 'Only 0 or 4 old pointers are legal.' }
CheckReport $Pre.Data 'Preflight' $Old 0
CheckClean
if (-not $Apply) {
    Write-Host ('[ARM-CLEAN06.4X54] READ ONLY: old='+$Old+'; no assets changed; HEAD='+$ExpectedHead)
    return
}
$Prior = RunUE 'Verify' (Join-Path $Run 'prior-verify.json') @() -AllowDifference
CheckReport $Prior.Data 'Verify' $Old 0
if ($Old -eq 0 -and $Prior.Exit -eq 0) {
    Write-Host '[ARM-CLEAN06.4X54] VERIFIED NO-OP: four packages already pointerless.'
    return
}
if ($Old -ne 4 -or $Prior.Exit -ne 4) {
    throw 'Verify disagrees with four original historical pointers.'
}
$Backups = @()
for ($Index=0; $Index -lt 4; ++$Index) {
    $Backup = Join-Path $Run $Relative[$Index].Replace('/','\')
    New-Item -ItemType Directory -Path (Split-Path -Parent $Backup) -Force | Out-Null
    Copy-Item -LiteralPath $Live[$Index] -Destination $Backup -ErrorAction Stop
    if ((Sha $Backup) -cne $Before[$Index] -or
        (Sha $Live[$Index]) -cne $Before[$Index]) {
        throw ('SHA256 snapshot mismatch at '+$Index)
    }
    $Backups += $Backup
}
$Committed = $false
try {
    CheckClean
    $Receipt = @('-ExpectedHead='+$ExpectedHead)
    for ($Index=0; $Index -lt 4; ++$Index) {
        $Receipt += ('-Backup'+$Index+'='+$Backups[$Index])
    }
    $Result = RunUE 'Apply' (Join-Path $Run 'apply.json') $Receipt
    CheckReport $Result.Data 'Apply' 0 4
    for ($Index=0; $Index -lt 4; ++$Index) {
        if ((Sha $Live[$Index]) -ceq $Before[$Index]) {
            throw ('Expected actual native change at index '+$Index)
        }
    }
    $Verify = RunUE 'Verify' (Join-Path $Run 'reopened-verify.json') @()
    CheckReport $Verify.Data 'Verify' 0 0
    CheckFourDirty
    $Reports = Join-Path $Run 'full-grimrock-suite'
    & $Validator -EngineRoot $EngineRoot -SkipBuild -AutomationFilter 'Grimrock' -ReportRoot $Reports
    if ($LASTEXITCODE -ne 0) {
        throw 'Full UE Grimrock Automation failed; restoring four backups.'
    }
    CheckFourDirty
    [void](Git @('fetch','--no-tags','origin','master'))
    $AfterRemote = ([string]@(Git @('rev-parse','refs/remotes/origin/master'))[0]).Trim()
    if ((Head) -cne $ExpectedHead -or $AfterRemote -cne $ExpectedHead) {
        throw 'Master advanced during native verification; refuse publication.'
    }
    if (-not $Publish) {
        Write-Host ('[ARM-CLEAN06.4X54] FOUR VERIFIED LOCAL UASSETS; backup: '+$Run)
        Write-Host 'No publication requested; four allowlisted native assets remain locally modified.'
        return
    }
    [void](Git (@('add','--') + $Relative))
    $Staged = @(Git @('diff','--cached','--name-only'))
    if ($Staged.Count -ne 4) { throw 'Staged set does not contain exactly four assets.' }
    foreach ($File in $Staged) {
        if ($Relative -cnotcontains [string]$File) { throw ('Unexpected staged file: '+$File) }
    }
    [void](Git @('-c','user.name=Grimrock UE Migration',
        '-c','user.email=grimrock-ue-migration@users.noreply.github.com',
        'commit','-m','ARM-CLEAN06.4X54B: persist four native pointerless puzzle rules'))
    $Committed = $true
    $NewHead = Head
    $Parent = ([string]@(Git @('rev-parse','HEAD^'))[0]).Trim()
    if ($Parent -cne $ExpectedHead) {
        throw 'Local native binary commit parent mismatch; do not rewrite.'
    }
    [void](Git @('push','origin','HEAD:refs/heads/master'))
    Write-Host ('[ARM-CLEAN06.4X54] FOUR NATIVE ASSETS PUSHED '+$NewHead)
    Write-Host ('[ARM-CLEAN06.4X54] PRIVATE SNAPSHOTS '+$Run)
    Write-Host 'Await SilentTask and LocalAuto on binary SHA, then PIE puzzle checks.'
}
catch {
    if (-not $Committed) {
        Write-Warning ('Native transaction failed; restoring four allowlisted assets: '+$_.Exception.Message)
        $Dirty = @(Git @('status','--porcelain','--untracked-files=all'))
        foreach ($Line in $Dirty) {
            $Text = [string]$Line
            if ($Text.Length -lt 4 -or
                $Relative -cnotcontains $Text.Substring(3).Replace('\','/')) {
                throw 'Unexpected user file modification: automatic rollback refused.'
            }
        }
        for ($Index=0; $Index -lt 4; ++$Index) {
            Copy-Item -LiteralPath $Backups[$Index] -Destination $Live[$Index] -Force
            if ((Sha $Live[$Index]) -cne $Before[$Index]) {
                throw ('CRITICAL: rollback SHA256 mismatch for '+$Relative[$Index])
            }
        }
        if (@(Git @('status','--porcelain','--untracked-files=all')).Count -ne 0) {
            throw 'CRITICAL: bytes restored but worktree is not clean.'
        }
        Write-Host '[ARM-CLEAN06.4X54] ROLLBACK VERIFIED: four source SHA256 restored; worktree clean.'
    }
    else {
        Write-Warning 'Native commit was created; never reset or force-push committed history.'
    }
    throw
}
