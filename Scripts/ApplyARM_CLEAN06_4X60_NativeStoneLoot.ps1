<#
 ARM-CLEAN06.4X.60: protected single-package native UE5.5.4 transaction.
 Default is read-only. -Apply requires exact master and private SHA256 backup.
 -Publish requires fresh UE Verify, identical ordered manifest, full Grimrock
 tests, and staged LFS receipt. Never changes SaveGames or other packages.
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
$Relative = 'Content/GrimrockPrototype/Monsters/GoblinThrower/Data/DA_MON_GoblinThrower.uasset'
$Live = Join-Path $RepoRoot ($Relative.Replace('/','\'))
$Project = Join-Path $RepoRoot 'GrimrockPrototype.uproject'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Build = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$Validator = Join-Path $RepoRoot 'Scripts\ValidateUE.ps1'
foreach ($File in @($Live,$Project,$Editor,$Build,$Validator)) {
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
function Sha([string]$Path) {
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}
function CheckNativeHeader([string]$Path) {
    $Stream = [IO.File]::OpenRead($Path)
    try {
        [byte[]]$Header = @(0,0,0,0)
        if ($Stream.Read($Header,0,4) -ne 4 -or
            $Header[0] -ne 0xC1 -or $Header[1] -ne 0x83 -or
            $Header[2] -ne 0x2A -or $Header[3] -ne 0x9E) { throw ('Invalid native asset: '+$Path) }
    } finally { $Stream.Dispose() }
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
function CheckReport($Report,[string]$Mode,[int]$Old,[int]$Saved) {
    if ($null -eq $Report -or
        [string]$Report.schema -cne 'ARM-CLEAN06.4X60/stone-one/v1' -or
        [string]$Report.mode -cne $Mode -or
        [int]$Report.targets -ne 1 -or [int]$Report.loot_rows -ne 5 -or
        [int]$Report.legacy_pointers -ne $Old -or [int]$Report.packages_saved -ne $Saved -or
        $Report.only_goblin_package_allowed -ne $true -or $Report.savegames_touched -ne $false -or
        [string]::IsNullOrEmpty([string]$Report.normalized_monsters) -or
        @($Report.source_evidence).Count -ne 5) {
        throw ('Strict native Stone report refused: '+$Mode)
    }
    $Ids = @('GoblinKnife','Stone','EmptyVial','Item_RatMeat','Item_RatTooth')
    for ($Index=0; $Index -lt 5; ++$Index) {
        if ([string]$Report.source_evidence[$Index].item_id -cne $Ids[$Index] -or
            [string]::IsNullOrEmpty([string]$Report.source_evidence[$Index].authored_properties)) {
            throw 'Source evidence row invalid.'
        }
    }
}
function Manifest($Report) {
    return (ConvertTo-Json -InputObject ([pscustomobject][ordered]@{
        monsters = [string]$Report.normalized_monsters
        evidence = @($Report.source_evidence)
    }) -Depth 8 -Compress)
}
function RunUE([string]$Mode,[string]$Output,[string[]]$Extra,[switch]$AllowDifference) {
    $EditorArguments = @($Project,'-run=GridArmClean064X60NativeStoneLoot',
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
    if ($Current.Count -ne 1 -or ([string]$Current[0]).Replace('\','/') -cne (' M '+$Relative)) {
        throw 'Expected one and ONLY one unstaged Goblin loot package.'
    }
}
CheckClean
[void](Git @('fetch','--no-tags','origin','master'))
$Remote = ([string]@(Git @('rev-parse','refs/remotes/origin/master'))[0]).Trim()
if ($Remote -cne $ExpectedHead) { throw 'origin/master differs from ExpectedHead.' }
if ($Apply) { CheckWindowsStatuses }
if ((Head) -cne $ExpectedHead) {
    [void](Git @('merge','--ff-only','refs/remotes/origin/master'))
}
if ((Head) -cne $ExpectedHead) { throw 'HEAD differs from pinned SHA.' }
CheckClean
if (@(Git @('lfs','ls-files','-n')) -cnotcontains $Relative) { throw 'Goblin loot not registered in Git LFS.' }
CheckNativeHeader $Live
$Before = Sha $Live
$BuildArgs = @('GrimrockPrototypeEditor','Win64','Development',
    ('-Project='+$Project),'-WaitMutex','-NoHotReloadFromIDE')
& $Build @BuildArgs
if ($LASTEXITCODE -ne 0) { throw 'UE Editor build failed; no native file modified.' }
CheckClean
if ((Head) -cne $ExpectedHead -or (Sha $Live) -cne $Before) { throw 'HEAD or Goblin loot changed during UE build.' }
$RunId = $ExpectedHead.Substring(0,12)+'-'+(Get-Date -Format 'yyyyMMddHHmmssfff')
$Run = Join-Path $StateRoot ('ARM_CLEAN06_4X60\'+$RunId)
if (Test-Path -LiteralPath $Run) { throw 'Run directory already exists.' }
New-Item -ItemType Directory -Path $Run -Force | Out-Null
$Pre = RunUE 'Preflight' (Join-Path $Run 'preflight.json') @()
$Old = [int]$Pre.Data.legacy_pointers
if ($Old -ne 0 -and $Old -ne 1) { throw 'Only 0 or one old pointers are legal.' }
CheckReport $Pre.Data 'Preflight' $Old 0
$BeforeManifest = Manifest $Pre.Data
CheckClean
if ((Sha $Live) -cne $Before) { throw 'Read-only preflight changed Goblin loot bytes.' }
if (-not $Apply) {
    Write-Host ('[ARM-CLEAN06.4X60] READ ONLY: old='+$Old+'; no assets changed; HEAD='+$ExpectedHead)
    return
}
$Prior = RunUE 'Verify' (Join-Path $Run 'prior-verify.json') @() -AllowDifference
CheckReport $Prior.Data 'Verify' $Old 0
if ((Manifest $Prior.Data) -cne $BeforeManifest -or (Sha $Live) -cne $Before) {
    throw 'Prior Verify disagrees with original Goblin loot manifest or bytes.'
}
if ($Old -eq 0 -and $Prior.Exit -eq 0) {
    Write-Host '[ARM-CLEAN06.4X60] VERIFIED NO-OP: Goblin loot already pointerless.'
    return
}
if ($Old -ne 1 -or $Prior.Exit -ne 4) { throw 'Verify disagrees with one old Goblin loot pointers.' }
$Backup = Join-Path $Run ($Relative.Replace('/','\'))
New-Item -ItemType Directory -Path (Split-Path -Parent $Backup) -Force | Out-Null
Copy-Item -LiteralPath $Live -Destination $Backup -ErrorAction Stop
if ((Sha $Backup) -cne $Before -or (Sha $Live) -cne $Before) { throw 'SHA256 snapshot mismatch.' }
$Committed = $false
try {
    CheckClean
    if ((Head) -cne $ExpectedHead) { throw 'HEAD changed before Apply.' }
    $Result = RunUE 'Apply' (Join-Path $Run 'apply.json') @(('-ExpectedHead='+$ExpectedHead),('-Backup='+$Backup))
    CheckReport $Result.Data 'Apply' 0 1
    if ((Manifest $Result.Data) -cne $BeforeManifest) { throw 'Apply changed unrelated Goblin loot fields.' }
    $After = Sha $Live
    if ($After -ceq $Before) { throw 'Expected actual native Goblin loot change.' }
    CheckNativeHeader $Live
    $Verify = RunUE 'Verify' (Join-Path $Run 'reopened-verify.json') @()
    CheckReport $Verify.Data 'Verify' 0 0
    if ((Manifest $Verify.Data) -cne $BeforeManifest -or (Sha $Live) -cne $After) {
        throw 'Reopened native Goblin loot differs from original preserved manifest or saved bytes.'
    }
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
    CheckOneDirty
    if ((Sha $Live) -cne $After) { throw 'Automation changed the verified native asset.' }
    [void](Git @('fetch','--no-tags','origin','master'))
    $AfterRemote = ([string]@(Git @('rev-parse','refs/remotes/origin/master'))[0]).Trim()
    if ((Head) -cne $ExpectedHead -or $AfterRemote -cne $ExpectedHead) {
        throw 'Master advanced during native verification; refuse publication.'
    }
    if (-not $Publish) {
        Write-Host ('[ARM-CLEAN06.4X60] ONE VERIFIED LOCAL UASSET; backup: '+$Run)
        Write-Host 'No publication requested; allowlisted Goblin loot remains locally modified.'
        return
    }
    [void](Git @('add','--',$Relative))
    $Staged = @(Git @('diff','--cached','--name-only'))
    if ($Staged.Count -ne 1 -or [string]$Staged[0] -cne $Relative) {
        throw 'Staged set must contain only the Goblin loot.'
    }
    $LfsPointer = @(Git @('show',(':'+$Relative)))
    if ($LfsPointer -cnotcontains 'version https://git-lfs.github.com/spec/v1' -or
        $LfsPointer -cnotcontains ('oid sha256:'+$After) -or
        $LfsPointer -cnotcontains ('size '+(Get-Item -LiteralPath $Live).Length)) {
        throw 'Staged LFS pointer does not match verified native bytes.'
    }
    [void](Git @('-c','user.name=Grimrock UE Migration',
        '-c','user.email=grimrock-ue-migration@users.noreply.github.com',
        'commit','-m','ARM-CLEAN06.4X60B: persist pointerless Stone loot in GoblinThrower'))
    $Committed = $true
    $NewHead = Head
    if (([string]@(Git @('rev-parse','HEAD^'))[0]).Trim() -cne $ExpectedHead) {
        throw 'Local native binary commit parent mismatch; do not rewrite.'
    }
    [void](Git @('push','origin','HEAD:refs/heads/master'))
    Write-Host ('[ARM-CLEAN06.4X60] ONE NATIVE GOBLIN PUSHED '+$NewHead)
    Write-Host ('[ARM-CLEAN06.4X60] PRIVATE SNAPSHOT '+$Run)
    Write-Host ('[ARM-CLEAN06.4X60] SOURCE EVIDENCE '+(Join-Path $Run 'preflight.json'))
    Write-Host 'Await SilentTask and LocalAuto on binary SHA, then Goblin loot PIE/editor checks.'
}
catch {
    if (-not $Committed -and (Head) -ceq $ExpectedHead) {
        Write-Warning ('Native transaction failed; restoring allowlisted Goblin loot: '+$_.Exception.Message)
        $Dirty = @(Git @('status','--porcelain','--untracked-files=all'))
        foreach ($Line in $Dirty) {
            $Text = [string]$Line
            if ($Text.Length -lt 4 -or $Text.Substring(3).Replace('\','/') -cne $Relative) {
                throw 'Unexpected user file modification: automatic rollback refused.'
            }
        }
        # Original index was clean. Undo only an allowed stage operation,
        # never reset any unrelated user file or committed history.
        [void](Git @('restore','--staged','--',$Relative))
        Copy-Item -LiteralPath $Backup -Destination $Live -Force
        if ((Sha $Live) -cne $Before -or @(Git @('status','--porcelain','--untracked-files=all')).Count -ne 0) {
            throw 'CRITICAL: backup restoration or clean-worktree verification failed.'
        }
        Write-Host '[ARM-CLEAN06.4X60] ROLLBACK VERIFIED: source SHA256 restored; worktree clean.'
    }
    else { Write-Warning 'HEAD changed or native commit exists; never reset or force-push committed history.' }
    throw
}
