<#
 ARM-CLEAN06.4X.46 — one approved native Unreal Engine 5.5.4 transaction.
 Explicit operator run ONLY; never called by CI or an automation hook.
 Default is read-only; -Apply requires a SHA256 private backup outside repo.
 -Publish also requires -Apply, the reopened native Verify and full Grimrock
 Automation success before ONE binary-only fast-forward commit/push.
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
if ($ExpectedHead -cnotmatch '^[a-f0-9]{40}$') { throw 'ExpectedHead must be SHA40.' }
if ($Publish -and -not $Apply) { throw '-Publish requires explicit -Apply.' }
$RepoRoot = [IO.Path]::GetFullPath($RepoRoot).TrimEnd('\','/')
$EngineRoot = [IO.Path]::GetFullPath($EngineRoot).TrimEnd('\','/')
$StateRoot = [IO.Path]::GetFullPath($StateRoot).TrimEnd('\','/')
if ($StateRoot.Equals($RepoRoot,[StringComparison]::OrdinalIgnoreCase) -or
    $StateRoot.StartsWith(($RepoRoot+'\'),[StringComparison]::OrdinalIgnoreCase) -or
    $RepoRoot.StartsWith(($StateRoot+'\'),[StringComparison]::OrdinalIgnoreCase)) {
    throw 'StateRoot and RepoRoot must be separate directories.'
}
$Relative = 'Content/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_00.uasset'
$Target = Join-Path $RepoRoot $Relative.Replace('/','\')
$Project = Join-Path $RepoRoot 'GrimrockPrototype.uproject'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Build = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$Validator = Join-Path $RepoRoot 'Scripts\ValidateUE.ps1'
foreach ($F in @($Project,$Target,$Editor,$Build,$Validator)) {
    if (-not (Test-Path -LiteralPath $F -PathType Leaf)) {
        throw ('Required file or UE executable missing: '+$F)
    }
}
# Never replace files under a running user Editor or a competing commandlet.
$OpenEditors = @(Get-Process -Name 'UnrealEditor','UnrealEditor-Cmd' -ErrorAction SilentlyContinue)
if ($OpenEditors.Count -ne 0) {
    throw 'Close Unreal Editor/UnrealEditor-Cmd before native conversion or git fast-forward.'
}
function Git([string[]]$GitArguments) {
    $Out = @(& git.exe -C $RepoRoot @GitArguments)
    if ($LASTEXITCODE -ne 0) { throw ('Protected Git operation failed: '+($GitArguments -join ' ')) }
    return $Out
}
function Head() { return ([string]@(Git @('rev-parse','HEAD'))[0]).Trim() }
function CheckBranchAndClean {
    $B = ([string]@(Git @('branch','--show-current'))[0]).Trim()
    if ($B -cne 'master') { throw 'Native conversion requires master only.' }
    $Dirty = @(Git @('status','--porcelain','--untracked-files=all'))
    if ($Dirty.Count -ne 0) { throw 'User worktree not clean; no native overwrite.' }
}
function Sha([string]$F) {
    return (Get-FileHash -LiteralPath $F -Algorithm SHA256).Hash.ToLowerInvariant()
}
function GetNativeHeader {
    $Stream = [IO.File]::OpenRead($Target)
    try {
        [byte[]]$Header = @(0,0,0,0)
        if ($Stream.Read($Header,0,4) -ne 4 -or
            $Header[0] -ne 0xC1 -or $Header[1] -ne 0x83 -or
            $Header[2] -ne 0x2A -or $Header[3] -ne 0x9E) {
            throw 'Source is not a hydrated Unreal .uasset file.'
        }
    } finally { $Stream.Dispose() }
}
function CheckReport($Report,[string]$Mode,[int]$ExpectedOld,[int]$ExpectedSaved) {
    if ($null -eq $Report -or
        [string]$Report.schema -cne 'ARM-CLEAN06.4X46/native-loose44/v1' -or
        [string]$Report.mode -cne $Mode -or
        [int]$Report.physical_items -ne 44 -or
        [int]$Report.legacy_pointers -ne $ExpectedOld -or
        [int]$Report.packages_saved -ne $ExpectedSaved -or
        [string]$Report.target -cne 'DA_GridLevel_00' -or
        $Report.other_world_objects_modified -ne $false -or
        $Report.palette_or_other_levels_modified -ne $false -or
        $Report.savegames_touched -ne $false) {
        throw 'Native Unreal report failed strict one-package contract.'
    }
}
function RunUE([string]$Mode,[string]$ReportPath,[string[]]$Extra,[switch]$AllowDifference) {
    $Arguments = @($Project, '-run=GridArmClean064X46NativeLoosePointers',
        ('-'+$Mode), ('-Output='+$ReportPath),
        '-Unattended','-NoSplash','-NoP4','-NoSound','-NullRHI')
    $Arguments += $Extra
    # Avoid streaming Editor stdout into the function return value: only
    # the typed result object below is returned to CheckReport.
    & $Editor @Arguments | Out-Host
    $Exit = [int]$LASTEXITCODE
    if ($Exit -ne 0 -and
        -not ($AllowDifference -and $Mode -ceq 'Verify' -and $Exit -eq 4)) {
        throw ('Native '+$Mode+' failed: exit '+$Exit)
    }
    if (-not (Test-Path -LiteralPath $ReportPath -PathType Leaf)) {
        throw ('Native '+$Mode+' did not produce a private JSON report.')
    }
    return [pscustomobject]@{
        Exit = $Exit
        Data = (Get-Content -LiteralPath $ReportPath -Raw -Encoding UTF8 | ConvertFrom-Json)
    }
}

# A clean checkout may be advanced ONLY by a safe fast-forward to the exact
# user-provided remote SHA. Never reset, checkout -f, rebase or force-push.
CheckBranchAndClean
[void](Git @('fetch','--no-tags','origin','master'))
$Remote = ([string]@(Git @('rev-parse','refs/remotes/origin/master'))[0]).Trim()
if ($Remote -cne $ExpectedHead) {
    throw 'origin/master does not match ExpectedHead. Refuse migration.'
}
if ((Head) -cne $ExpectedHead) {
    [void](Git @('merge','--ff-only','refs/remotes/origin/master'))
}
if ((Head) -cne $ExpectedHead) { throw 'Local HEAD is not pinned to ExpectedHead.' }
CheckBranchAndClean
$LfsFiles = @(Git @('lfs','ls-files','-n'))
if ($LfsFiles -cnotcontains $Relative) { throw 'Exact native target is not registered with Git LFS.' }
GetNativeHeader
$BeforeHash = Sha $Target

# CI compiles only in its private worktree, not the user's main checkout.
# Always compile this local commandlet BEFORE Unreal -run invocation.
$BuildArguments = @('GrimrockPrototypeEditor','Win64','Development',
    ('-Project='+$Project),'-WaitMutex','-NoHotReloadFromIDE')
& $Build @BuildArguments
if ($LASTEXITCODE -ne 0) { throw 'Local UE Editor compilation failed. No asset modified.' }
CheckBranchAndClean
if ((Head) -cne $ExpectedHead -or (Sha $Target) -cne $BeforeHash) {
    throw 'Source changed while compiling; no native conversion.'
}

$RunId = $ExpectedHead.Substring(0,12)+'-'+(Get-Date -Format 'yyyyMMddHHmmssfff')
$Run = Join-Path $StateRoot ('ARM_CLEAN06_4X46\'+$RunId)
if (Test-Path -LiteralPath $Run) { throw 'Private run already exists.' }
New-Item -ItemType Directory -Path $Run -Force -ErrorAction Stop | Out-Null
$Pre = RunUE 'Preflight' (Join-Path $Run 'preflight.json') @()
if ($Pre.Exit -ne 0) { throw 'Native read-only preflight failed.' }
$Old = [int]$Pre.Data.legacy_pointers
if ($Old -ne 44 -and $Old -ne 0) { throw 'Not an all-or-nothing 44-pointer state.' }
CheckReport $Pre.Data 'Preflight' $Old 0
CheckBranchAndClean
if (-not $Apply) {
    Write-Host ('[ARM-CLEAN06.4X46] READ ONLY: old pointers='+$Old+', no assets written. HEAD='+$ExpectedHead)
    return
}
$PriorVerify = RunUE 'Verify' (Join-Path $Run 'before-verify.json') @() -AllowDifference
CheckReport $PriorVerify.Data 'Verify' $Old 0
if ($Old -eq 0 -and $PriorVerify.Exit -eq 0) {
    Write-Host '[ARM-CLEAN06.4X46] VERIFIED NO-OP: 44 entries are already pointerless.'
    return
}
if ($Old -ne 44 -or $PriorVerify.Exit -ne 4) {
    throw 'Read-only Verify disagrees with expected 44 legacy pointers.'
}

# Back up the ONE original native file privately, verify twice by SHA-256,
# then let the commandlet require byte equality independently before SavePackage.
$Backup = Join-Path $Run $Relative.Replace('/','\')
New-Item -ItemType Directory -Path (Split-Path -Parent $Backup) -Force -ErrorAction Stop | Out-Null
Copy-Item -LiteralPath $Target -Destination $Backup -ErrorAction Stop
if ((Sha $Backup) -cne $BeforeHash -or (Sha $Target) -cne $BeforeHash) {
    throw 'Private backup hash mismatch. Apply cancelled.'
}
$Saved = $false
$Committed = $false
try {
    CheckBranchAndClean
    $Result = RunUE 'Apply' (Join-Path $Run 'applied.json') @(
        ('-ExpectedHead='+$ExpectedHead), ('-BackupFile='+$Backup))
    CheckReport $Result.Data 'Apply' 0 1
    $Saved = $true
    if ((Sha $Target) -ceq $BeforeHash) {
        throw 'Apply did not change the protected native package.'
    }
    # Independent Unreal process; never trust in-memory save values alone.
    $Verify = RunUE 'Verify' (Join-Path $Run 'reopened-verify.json') @()
    CheckReport $Verify.Data 'Verify' 0 0
    $Changed = @(Git @('status','--porcelain','--untracked-files=all'))
    if ($Changed.Count -ne 1 -or ([string]$Changed[0]).Substring(0,2) -cne ' M' -or
        ([string]$Changed[0]).Substring(3).Replace('\','/') -cne $Relative) {
        throw 'Unexpected file changes during native SavePackage.'
    }

    $PrivateTests = Join-Path $Run 'full-suite'
    Write-Host '[ARM-CLEAN06.4X46] Running COMPLETE Grimrock Automation on converted native file.'
    & $Validator -EngineRoot $EngineRoot -SkipBuild -AutomationFilter 'Grimrock' -ReportRoot $PrivateTests
    if ($LASTEXITCODE -ne 0) { throw 'Full Grimrock Automation failed; restoring backup.' }
    $AfterTests = @(Git @('status','--porcelain','--untracked-files=all'))
    if ($AfterTests.Count -ne 1 -or ([string]$AfterTests[0]).Substring(0,2) -cne ' M' -or
        ([string]$AfterTests[0]).Substring(3).Replace('\','/') -cne $Relative) {
        throw 'Post-migration test modified unexpected native or user files.'
    }
    [void](Git @('fetch','--no-tags','origin','master'))
    $AfterRemote = ([string]@(Git @('rev-parse','refs/remotes/origin/master'))[0]).Trim()
    if ((Head) -cne $ExpectedHead -or $AfterRemote -cne $ExpectedHead) {
        throw 'Master advanced during native verification; refuse publication.'
    }
    if (-not $Publish) {
        Write-Host ('[ARM-CLEAN06.4X46] VERIFIED LOCAL .uasset change; private backup: '+$Backup)
        Write-Host 'No commit/push requested. Only the exact one native package is dirty.'
        return
    }
    [void](Git (@('add','--')+@($Relative)))
    $StagedFiles = @(Git @('diff','--cached','--name-only'))
    if ($StagedFiles.Count -ne 1 -or $StagedFiles[0] -cne $Relative) {
        throw 'Only one native .uasset is allowed in the binary commit.'
    }
    $CommitMessage = 'ARM-CLEAN06.4X46B: persist native pointerless 44 loose item references'
    [void](Git @('-c','user.name=Grimrock UE Migration',
        '-c','user.email=grimrock-ue-migration@users.noreply.github.com',
        'commit','-m',$CommitMessage))
    $Committed = $true
    $NewHead = Head
    $NewParent = ([string]@(Git @('rev-parse','HEAD^'))[0]).Trim()
    if ($NewParent -cne $ExpectedHead) {
        throw 'Binary commit parent mismatch; keep private backup and inspect local commit.'
    }
    [void](Git @('push','origin','HEAD:refs/heads/master'))
    Write-Host ('[ARM-CLEAN06.4X46] EXACT ONE NATIVE PACKAGE PUSHED '+$NewHead)
    Write-Host ('[ARM-CLEAN06.4X46] PRIVATE RECOVERY SNAPSHOT '+$Backup)
    Write-Host 'Await SilentTask and LocalAuto green on the binary SHA; then validate PIE again.'
}
catch {
    if (-not $Committed) {
        Write-Warning ('Native transaction failed; restoring ONLY allowlisted target: '+$_.Exception.Message)
        if (Test-Path -LiteralPath $Backup -PathType Leaf) {
            # Never overwrite anything besides the original exact target.
            $Dirty = @(Git @('status','--porcelain','--untracked-files=all'))
            if (@($Dirty | Where-Object {
                ([string]$_).Length -lt 4 -or
                ([string]$_).Substring(3).Replace('\','/') -cne $Relative
            }).Count -ne 0) {
                throw 'Unexpected file dirtied: automatic restore refused, private snapshot retained.'
            }
            Copy-Item -LiteralPath $Backup -Destination $Target -Force -ErrorAction Stop
            if ((Sha $Target) -cne $BeforeHash) {
                throw 'CRITICAL: native rollback hash differs from approved original!'
            }
            # X.47: after a validation failure the operator must be able
            # to see unambiguously that the original package was restored.
            # Do not report rollback success if Git still sees any dirty file.
            $RollbackStatus = @(Git @('status','--porcelain','--untracked-files=all'))
            if ($RollbackStatus.Count -ne 0) {
                throw 'CRITICAL: native file bytes restored, but the worktree is not clean.'
            }
            Write-Host ('[ARM-CLEAN06.4X46] ROLLBACK VERIFIED: native target SHA256=' +
                $BeforeHash + '; worktree clean; no binary commit or push.')
        }
    }
    else {
        Write-Warning 'Local binary commit exists; no automatic rollback/rewrite of committed history.'
    }
    throw
}
