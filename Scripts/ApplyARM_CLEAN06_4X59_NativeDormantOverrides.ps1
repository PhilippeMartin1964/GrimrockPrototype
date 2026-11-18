<#
 ARM-CLEAN06.4X.59: protected single-level dormant-override-only migration.
 Default read-only; Apply is never automatic. Preflight, backups, reopened
 manifest, full Grimrock, SHA256, LFS and strict master checks are mandatory.
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
$Names = @('DA_GridLevel_00','DA_GridLevel_01','DA_GridLevel_New_Level_03','DA_GridLevel_Pillars_of_Light')
$AllRelative = @($Names | ForEach-Object { 'Content/GrimrockPrototype/Core/DataAssets/GrimrockLevels/'+$_+'.uasset' })
$Relative = @($AllRelative[0])
$Live = @($AllRelative | ForEach-Object { Join-Path $RepoRoot ($_.Replace('/','\')) })
$Project = Join-Path $RepoRoot 'GrimrockPrototype.uproject'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Build = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$Validator = Join-Path $RepoRoot 'Scripts\ValidateUE.ps1'
foreach ($File in (@($Live)+@($Project,$Editor,$Build,$Validator))) {
    if (-not (Test-Path -LiteralPath $File -PathType Leaf)) { throw ('Missing file: '+$File) }
}
if (@(Get-Process -Name 'UnrealEditor','UnrealEditor-Cmd' -ErrorAction SilentlyContinue).Count -ne 0) {
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
function CheckWindowsStatuses {
    $Raw = @(& gh.exe api --method GET ('repos/PhilippeMartin1964/GrimrockPrototype/commits/'+$ExpectedHead+'/status'))
    if ($LASTEXITCODE -ne 0) { throw 'Could not verify exact GitHub statuses.' }
    $Checks = ($Raw -join [Environment]::NewLine) | ConvertFrom-Json
    foreach ($Context in @('UE5.5.4/SilentTask','UE5.5.4/LocalAuto')) {
        $OK = @($Checks.statuses | Where-Object { $_.context -ceq $Context -and $_.state -ceq 'success' })
        if ($OK.Count -ne 1) { throw ('Required Windows validation not acquired: '+$Context) }
    }
}
function Sha([string]$Path) {
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}
function CheckHashes([string[]]$Expected) {
    if ($Expected.Count -ne 4) { throw 'Four SHA256 values required.' }
    for ($Index=0; $Index -lt 4; ++$Index) {
        if ((Sha $Live[$Index]) -cne $Expected[$Index]) { throw ('Unexpected native byte change: '+$AllRelative[$Index]) }
    }
}
function NativeHeader([string]$Path) {
    $Stream = [IO.File]::OpenRead($Path)
    try {
        [byte[]]$Header = @(0,0,0,0)
        if ($Stream.Read($Header,0,4) -ne 4 -or
            $Header[0] -ne 0xC1 -or $Header[1] -ne 0x83 -or
            $Header[2] -ne 0x2A -or $Header[3] -ne 0x9E) {
            throw ('Invalid or non-hydrated UE package: '+$Path)
        }
    } finally { $Stream.Dispose() }
}
function CheckReport($Report,[string]$Mode,[int]$Old,[int]$Saved) {
    if ($null -eq $Report -or
        [string]$Report.schema -cne 'ARM-CLEAN06.4X59/dormant-one/v1' -or
        [string]$Report.mode -cne $Mode -or
        [int]$Report.targets -ne 1 -or [int]$Report.dormant_owners -ne 3 -or
        [int]$Report.legacy_pointers -ne $Old -or [int]$Report.packages_saved -ne $Saved -or
        $Report.only_level_00_allowed -ne $true -or $Report.savegames_touched -ne $false -or
        @($Report.levels).Count -ne 4) {
        throw ('Strict native single-level dormant report refused: '+$Mode)
    }
    $Counts = @(3,0,0,0)
    for ($Index=0; $Index -lt 4; ++$Index) {
        $ExpectedPath = '/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/'+$Names[$Index]+'.'+$Names[$Index]
        if ([string]$Report.levels[$Index].level -cne $ExpectedPath -or
            [int]$Report.levels[$Index].dormant_owners -ne $Counts[$Index] -or
            [string]::IsNullOrEmpty([string]$Report.levels[$Index].normalized_authoring)) {
            throw ('Unexpected level manifest at index '+$Index)
        }
    }
}
function CheckSameManifest($Reference,$Candidate) {
    for ($Index=0; $Index -lt 4; ++$Index) {
        if ([string]$Reference.levels[$Index].normalized_authoring -cne
            [string]$Candidate.levels[$Index].normalized_authoring) {
            throw ('Reflected level authoring changed beyond approved dormant lists: '+$Names[$Index])
        }
    }
}
function RunUE([string]$Mode,[string]$Output,[string[]]$Extra,[switch]$AllowDifference) {
    $EditorArguments = @($Project,'-run=GridArmClean064X59NativeDormantOverrides',
        ('-'+$Mode),('-Output='+$Output),
        '-Unattended','-NoSplash','-NoP4','-NoSound','-NullRHI') + @($Extra)
    & $Editor @EditorArguments | Out-Host
    $Code = [int]$LASTEXITCODE
    if ($Code -ne 0 -and
        -not ($AllowDifference -and $Mode -ceq 'Verify' -and $Code -eq 4)) {
        throw ('Native '+$Mode+' failed: exit '+$Code)
    }
    if (-not (Test-Path -LiteralPath $Output -PathType Leaf)) { throw ('Missing private report: '+$Output) }
    return [pscustomobject]@{
        Exit=$Code
        Data=(Get-Content -LiteralPath $Output -Raw -Encoding UTF8 | ConvertFrom-Json)
    }
}
function CheckOneDirty {
    $Current = @(Git @('status','--porcelain','--untracked-files=all'))
    if ($Current.Count -ne 1) { throw ('Expected only level 00 changed, got '+$Current.Count) }
    foreach ($File in $Relative) {
        if (@($Current | Where-Object { ([string]$_).Replace('\','/') -ceq (' M '+$File) }).Count -ne 1) {
            throw ('Unexpected dirty set; expected unstaged target '+$File)
        }
    }
}
CheckClean
[void](Git @('fetch','--no-tags','origin','master'))
$Remote = ([string]@(Git @('rev-parse','refs/remotes/origin/master'))[0]).Trim()
if ($Remote -cne $ExpectedHead) { throw 'origin/master differs from ExpectedHead.' }
if ($Apply) { CheckWindowsStatuses }
if ((Head) -cne $ExpectedHead) { [void](Git @('merge','--ff-only','refs/remotes/origin/master')) }
if ((Head) -cne $ExpectedHead) { throw 'HEAD differs from pinned SHA.' }
CheckClean
$Lfs = @(Git @('lfs','ls-files','-n'))
for ($Index=0; $Index -lt 4; ++$Index) {
    if ($Lfs -cnotcontains $AllRelative[$Index]) { throw ('Not tracked by LFS: '+$AllRelative[$Index]) }
    NativeHeader $Live[$Index]
}
$Before = @($Live | ForEach-Object { Sha $_ })
$BuildArgs = @('GrimrockPrototypeEditor','Win64','Development',
    ('-Project='+$Project),'-WaitMutex','-NoHotReloadFromIDE')
& $Build @BuildArgs
if ($LASTEXITCODE -ne 0) { throw 'UE Editor build failed; no native modification authorized.' }
CheckClean
if ((Head) -cne $ExpectedHead) { throw 'HEAD changed during build.' }
CheckHashes $Before

$RunId = $ExpectedHead.Substring(0,12)+'-'+(Get-Date -Format 'yyyyMMddHHmmssfff')
$Run = Join-Path $StateRoot ('ARM_CLEAN06_4X59\'+$RunId)
if (Test-Path -LiteralPath $Run) { throw 'Run directory already exists.' }
New-Item -ItemType Directory -Path $Run -Force | Out-Null
$Pre = RunUE 'Preflight' (Join-Path $Run 'preflight.json') @()
$Old = [int]$Pre.Data.legacy_pointers
if ($Old -ne 0 -and $Old -ne 3) { throw 'Only 0 or 3 dormant pointers are legal.' }
CheckReport $Pre.Data 'Preflight' $Old 0
CheckClean
CheckHashes $Before
if (-not $Apply) {
    Write-Host ('[ARM-CLEAN06.4X59] READ ONLY: old='+$Old+'; no assets changed; HEAD='+$ExpectedHead)
    return
}
$Prior = RunUE 'Verify' (Join-Path $Run 'prior-verify.json') @() -AllowDifference
CheckReport $Prior.Data 'Verify' $Old 0
CheckSameManifest $Pre.Data $Prior.Data
CheckHashes $Before
if ($Old -eq 0 -and $Prior.Exit -eq 0) {
    Write-Host '[ARM-CLEAN06.4X59] VERIFIED NO-OP: all three dormant lists already empty.'
    return
}
if ($Old -ne 3 -or $Prior.Exit -ne 4) { throw 'Verify disagrees with original three dormant pointers.' }
$Backups = @()
for ($Index=0; $Index -lt 1; ++$Index) {
    $Backup = Join-Path $Run ($Relative[$Index].Replace('/','\'))
    New-Item -ItemType Directory -Path (Split-Path -Parent $Backup) -Force | Out-Null
    Copy-Item -LiteralPath $Live[$Index] -Destination $Backup -ErrorAction Stop
    if ((Sha $Backup) -cne $Before[$Index]) { throw ('Snapshot SHA256 mismatch: '+$Relative[$Index]) }
    $Backups += $Backup
}
$Committed = $false
try {
    CheckClean
    CheckHashes $Before
    if ((Head) -cne $ExpectedHead) { throw 'HEAD changed before Apply.' }
    $Receipt = @(('-ExpectedHead='+$ExpectedHead))
    for ($Index=0; $Index -lt 1; ++$Index) { $Receipt += ('-Backup'+$Index+'='+$Backups[$Index]) }
    $Result = RunUE 'Apply' (Join-Path $Run 'apply.json') $Receipt
    CheckReport $Result.Data 'Apply' 0 1
    CheckSameManifest $Pre.Data $Result.Data
    $After = @($Live | ForEach-Object { Sha $_ })
    for ($Index=0; $Index -lt 4; ++$Index) {
        if (($Index -lt 1 -and $After[$Index] -ceq $Before[$Index]) -or
            ($Index -ge 1 -and $After[$Index] -cne $Before[$Index])) {
            throw ('Unexpected native write scope: '+$AllRelative[$Index])
        }
        NativeHeader $Live[$Index]
    }
    $Verify = RunUE 'Verify' (Join-Path $Run 'reopened-verify.json') @()
    CheckReport $Verify.Data 'Verify' 0 0
    CheckSameManifest $Pre.Data $Verify.Data
    CheckHashes $After
    CheckOneDirty
    $Reports = Join-Path $Run 'full-grimrock-suite'
    & $Validator -EngineRoot $EngineRoot -SkipBuild -AutomationFilter 'Grimrock' -ReportRoot $Reports
    if ($LASTEXITCODE -ne 0) { throw 'Full UE Grimrock Automation failed.' }
    $Indices = @(Get-ChildItem -LiteralPath $Reports -Filter 'index.json' -File -Recurse)
    if ($Indices.Count -ne 1) { throw 'Expected one full Automation report.' }
    $Automation = Get-Content -LiteralPath $Indices[0].FullName -Raw | ConvertFrom-Json
    if ([int]$Automation.failed -ne 0 -or [int]$Automation.notRun -ne 0 -or
        ([int]$Automation.succeeded + [int]$Automation.succeededWithWarnings) -le 0) {
        throw 'Incomplete or failed full Automation report.'
    }
    CheckHashes $After
    CheckOneDirty
    [void](Git @('fetch','--no-tags','origin','master'))
    $AfterRemote = ([string]@(Git @('rev-parse','refs/remotes/origin/master'))[0]).Trim()
    if ((Head) -cne $ExpectedHead -or $AfterRemote -cne $ExpectedHead) {
        throw 'Master advanced during native verification; refuse publication.'
    }
    if (-not $Publish) {
        Write-Host ('[ARM-CLEAN06.4X59] ONE VERIFIED LOCAL UASSET; backup: '+$Run)
        Write-Host 'No publication requested; one allowlisted level remains locally modified.'
        return
    }
    [void](Git (@('add','--') + $Relative))
    $Staged = @(Git @('diff','--cached','--name-only'))
    if ($Staged.Count -ne 1) { throw 'Staged set must contain only level 00.' }
    for ($Index=0; $Index -lt 1; ++$Index) {
        if ($Staged -cnotcontains $Relative[$Index]) { throw 'Unexpected staged set.' }
        $Pointer = @(Git @('show',(':'+$Relative[$Index])))
        if ($Pointer -cnotcontains 'version https://git-lfs.github.com/spec/v1' -or
            $Pointer -cnotcontains ('oid sha256:'+$After[$Index]) -or
            $Pointer -cnotcontains ('size '+(Get-Item -LiteralPath $Live[$Index]).Length)) {
            throw ('Staged LFS pointer differs from validated bytes: '+$Relative[$Index])
        }
    }
    [void](Git @('-c','user.name=Grimrock UE Migration',
        '-c','user.email=grimrock-ue-migration@users.noreply.github.com',
        'commit','-m','ARM-CLEAN06.4X59B: remove three dormant acceptance references from level 00'))
    $Committed = $true
    $NewHead = Head
    if (([string]@(Git @('rev-parse','HEAD^'))[0]).Trim() -cne $ExpectedHead) {
        throw 'Binary commit parent mismatch; do not rewrite.'
    }
    [void](Git @('push','origin','HEAD:refs/heads/master'))
    Write-Host ('[ARM-CLEAN06.4X59] ONE NATIVE LEVEL PUSHED '+$NewHead)
    Write-Host ('[ARM-CLEAN06.4X59] PRIVATE SNAPSHOT '+$Run)
    Write-Host 'Await SilentTask and LocalAuto on binary SHA, then dormant override PIE/editor checks.'
}
catch {
    if (-not $Committed -and (Head) -ceq $ExpectedHead) {
        Write-Warning ('Native transaction failed; restoring allowlisted level 00: '+$_.Exception.Message)
        $Dirty = @(Git @('status','--porcelain','--untracked-files=all'))
        foreach ($Line in $Dirty) {
            $Text = [string]$Line
            if ($Text.Length -lt 4 -or $Relative -cnotcontains $Text.Substring(3).Replace('\','/')) {
                throw 'Unexpected user file modification: automatic rollback refused.'
            }
        }
        [void](Git (@('restore','--staged','--') + $Relative))
        for ($Index=0; $Index -lt 1; ++$Index) {
            Copy-Item -LiteralPath $Backups[$Index] -Destination $Live[$Index] -Force
        }
        CheckHashes $Before
        CheckClean
        Write-Host '[ARM-CLEAN06.4X59] ROLLBACK VERIFIED: source SHA256 restored; worktree clean.'
    }
    else { Write-Warning 'HEAD changed or binary commit exists; never reset or force-push committed history.' }
    throw
}
