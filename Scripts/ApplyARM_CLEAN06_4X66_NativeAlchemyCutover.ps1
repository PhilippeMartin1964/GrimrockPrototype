<#
 ARM-CLEAN06.4X.61: protected ten-alchemy registry + detailed-action catalogue native transaction.
 Default read-only; Apply requires an explicit save directory. Preflight, backups, reopened
 manifest, full Grimrock, SHA256, LFS and strict master checks are mandatory.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$RepoRoot,
    [Parameter(Mandatory=$true)][string]$EngineRoot,
    [Parameter(Mandatory=$true)][string]$StateRoot,
    [Parameter(Mandatory=$true)][string]$ExpectedHead,
    [string]$SaveDirectory,
    [switch]$Apply,
    [switch]$Publish
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if ($ExpectedHead -cnotmatch '^[a-f0-9]{40}$') { throw 'ExpectedHead must be exact SHA40.' }
if ($Apply -and [string]::IsNullOrWhiteSpace($SaveDirectory)) { throw 'Apply requires an explicit SaveDirectory.' }
if ($Publish -ne $Apply) { throw 'Native Apply and Publish must be requested together; default is read-only.' }
$RepoRoot = [IO.Path]::GetFullPath($RepoRoot).TrimEnd('\','/')
$EngineRoot = [IO.Path]::GetFullPath($EngineRoot).TrimEnd('\','/')
$StateRoot = [IO.Path]::GetFullPath($StateRoot).TrimEnd('\','/')
if ($StateRoot.Equals($RepoRoot,[StringComparison]::OrdinalIgnoreCase) -or
    $StateRoot.StartsWith(($RepoRoot+'\'),[StringComparison]::OrdinalIgnoreCase) -or
    $RepoRoot.StartsWith(($StateRoot+'\'),[StringComparison]::OrdinalIgnoreCase)) {
    throw 'StateRoot must be independent of the project and not its parent.'
}
$ExistingCount = 2
$NewNames = @('DA_Canonical_Item_Antidote','DA_Canonical_Item_Bomb_Fire','DA_Canonical_Item_Bomb_Toxic',
    'DA_Canonical_Item_DefensiveElixir_Fire','DA_Canonical_Item_DefensiveElixir_Ice',
    'DA_Canonical_Item_DefensiveElixir_Lightning','DA_Canonical_Item_DefensiveElixir_Poison',
    'DA_Canonical_Item_Flask_Acid','DA_Canonical_Item_Flask_CorrosiveCloud','DA_Canonical_Item_Panacea')
$NewRelative = @($NewNames | ForEach-Object { 'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/'+$_+'.uasset' })
$Relative = @(
    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ActionCatalogue.uasset'
) + $NewRelative
$Live = @($Relative | ForEach-Object { Join-Path $RepoRoot ($_.Replace('/','\')) })
$Project = Join-Path $RepoRoot 'GrimrockPrototype.uproject'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Build = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$Validator = Join-Path $RepoRoot 'Scripts\ValidateUE.ps1'
foreach ($File in (@($Live[0],$Live[1])+@($Project,$Editor,$Build,$Validator))) {
    if (-not (Test-Path -LiteralPath $File -PathType Leaf)) { throw ('Missing file: '+$File) }
}
if (@(Get-Process -Name 'UnrealEditor','UnrealEditor-Cmd','GrimrockPrototype','GrimrockPrototype-Win64-Shipping' -ErrorAction SilentlyContinue).Count -ne 0) {
    throw 'Close all Unreal Editor and game processes before native work.'
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
function StateHash([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path)) { return 'ABSENT' }
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw ('Expected file or absence: '+$Path) }
    return (Sha $Path)
}
function CheckHashes([string[]]$Expected) {
    if ($Expected.Count -ne 12) { throw 'Twelve file-state receipts required.' }
    for ($Index=0; $Index -lt 12; ++$Index) {
        if ((StateHash $Live[$Index]) -cne $Expected[$Index]) {
            throw ('Unexpected native byte/absence change: '+$Relative[$Index])
        }
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
    $ExpectedCount = if ($Old -eq 10) { 38 } elseif ($Old -eq 0) { 48 } else { throw 'Invalid old count.' }
    if ($null -eq $Report -or
        [string]$Report.schema -cne 'ARM-CLEAN06.4X66/ten-alchemy-twelve-packages/v1' -or
        [string]$Report.mode -cne $Mode -or
        [int]$Report.targets -ne 12 -or [int]$Report.registry_count -ne $ExpectedCount -or
        [int]$Report.unregistered_count -ne $Old -or [int]$Report.detailed_actions -ne $(if ($Old -eq 10) { 4 } else { 14 }) -or
        [int]$Report.packages_saved -ne $Saved -or
        $Report.savegames_touched -ne $false -or
        [string]::IsNullOrEmpty([string]$Report.normalized_manifest) -or
        @($Report.new_definitions).Count -ne 10) {
        throw ('Strict twelve-package report refused: '+$Mode)
    }
    for ($Index=0; $Index -lt 10; ++$Index) {
        $ExpectedPath = '/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/'+$NewNames[$Index]+'.'+$NewNames[$Index]
        if ([string]$Report.new_definitions[$Index] -cne $ExpectedPath) {
            throw ('Unexpected new package at index '+$Index)
        }
    }
}
function CheckSameManifest($Reference,$Candidate) {
    if ([string]$Reference.normalized_manifest -cne [string]$Candidate.normalized_manifest) {
        throw 'Root, action catalogue or source manifest changed beyond approved alchemy cutover.'
    }
}
function RunUE([string]$Mode,[string]$Output,[string[]]$Extra,[switch]$AllowDifference) {
    $EditorArguments = @($Project,'-run=GridArmClean064X66NativeAlchemyCutover',
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
function CheckTwelveDirty {
    $Current = @(Git @('status','--porcelain','--untracked-files=all'))
    if ($Current.Count -ne 12) { throw ('Expected exactly twelve native changes, got '+$Current.Count) }
    for ($Index=0; $Index -lt 12; ++$Index) {
        $Prefix = if ($Index -lt $ExistingCount) { ' M ' } else { '?? ' }
        $Expected = $Prefix + $Relative[$Index]
        if (@($Current | Where-Object { ([string]$_).Replace('\','/') -ceq $Expected }).Count -ne 1) {
            throw ('Unexpected dirty set; expected '+$Expected)
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
$Before = @($Live | ForEach-Object { StateHash $_ })
for ($Index=0; $Index -lt 12; ++$Index) {
    if ($Before[$Index] -ceq 'ABSENT') {
        if ($Index -lt $ExistingCount -or $Lfs -ccontains $Relative[$Index]) {
            throw ('Unexpected missing tracked package: '+$Relative[$Index])
        }
        $Attr = @(Git @('check-attr','filter','--',$Relative[$Index]))
        if ($Attr.Count -ne 1 -or -not ([string]$Attr[0]).EndsWith(': filter: lfs')) {
            throw ('New native package is not governed by LFS: '+$Relative[$Index])
        }
    }
    else {
        if ($Lfs -cnotcontains $Relative[$Index]) { throw ('Not tracked by LFS: '+$Relative[$Index]) }
        NativeHeader $Live[$Index]
    }
}
$AbsentCount = @($Before | Where-Object { $_ -ceq 'ABSENT' }).Count
if ($AbsentCount -ne 0 -and $AbsentCount -ne 10) { throw 'Partial native definition set refused.' }
$BuildArgs = @('GrimrockPrototypeEditor','Win64','Development',
    ('-Project='+$Project),'-WaitMutex','-NoHotReloadFromIDE')
& $Build @BuildArgs
if ($LASTEXITCODE -ne 0) { throw 'UE Editor build failed; no native modification authorized.' }
CheckClean
if ((Head) -cne $ExpectedHead) { throw 'HEAD changed during build.' }
CheckHashes $Before

$RunId = $ExpectedHead.Substring(0,12)+'-'+(Get-Date -Format 'yyyyMMddHHmmssfff')
$Run = Join-Path $StateRoot ('ARM_CLEAN06_4X66\'+$RunId)
if (Test-Path -LiteralPath $Run) { throw 'Run directory already exists.' }
New-Item -ItemType Directory -Path $Run -Force | Out-Null
$Pre = RunUE 'Preflight' (Join-Path $Run 'preflight.json') @()
$Old = [int]$Pre.Data.unregistered_count
if (($Old -eq 10 -and $AbsentCount -ne 10) -or ($Old -eq 0 -and $AbsentCount -ne 0) -or ($Old -ne 10 -and $Old -ne 0)) { throw 'Root/catalogue/physical file state disagrees.' }
CheckReport $Pre.Data 'Preflight' $Old 0
CheckClean
CheckHashes $Before
if (-not $Apply) {
    Write-Host ('[ARM-CLEAN06.4X66] READ ONLY: old='+$Old+'; no assets changed; HEAD='+$ExpectedHead)
    return
}
$Prior = RunUE 'Verify' (Join-Path $Run 'prior-verify.json') @() -AllowDifference
CheckReport $Prior.Data 'Verify' $Old 0
CheckSameManifest $Pre.Data $Prior.Data
CheckHashes $Before
if ($Old -eq 0 -and $Prior.Exit -eq 0) {
    Write-Host '[ARM-CLEAN06.4X66] VERIFIED NO-OP: 48 definitions and all 14 detailed actions already canonical; no save installation attempted.'
    return
}
if ($Old -ne 10 -or $Prior.Exit -ne 4 -or $AbsentCount -ne 10) { throw 'Verify disagrees with original ten unregistered identities.' }
$Backups = @()
for ($Index=0; $Index -lt $ExistingCount; ++$Index) {
    $Backup = Join-Path $Run ($Relative[$Index].Replace('/','\'))
    New-Item -ItemType Directory -Path (Split-Path -Parent $Backup) -Force | Out-Null
    Copy-Item -LiteralPath $Live[$Index] -Destination $Backup -ErrorAction Stop
    if ((Sha $Backup) -cne $Before[$Index]) { throw ('Snapshot SHA256 mismatch: '+$Relative[$Index]) }
    $Backups += $Backup
}

# The preceding conversion validates copies in a separate private snapshot.
# Installation is permitted only after the twelve native packages and the full
# suite have passed, and only for the explicitly selected save directory.
$SaveDirectory = [IO.Path]::GetFullPath($SaveDirectory).TrimEnd('\','/')
$CopyTool = Join-Path $RepoRoot 'Scripts\PrepareARM_CLEAN06_4X64_SaveCopies.ps1'
if (-not (Test-Path -LiteralPath $CopyTool -PathType Leaf)) { throw 'Missing validated save copy tool.' }
$SaveReceiptPath = @(& $CopyTool -RepoRoot $RepoRoot -EngineRoot $EngineRoot -StateRoot $StateRoot -SaveDirectory $SaveDirectory -ExpectedHead $ExpectedHead -PassThru)
if ($SaveReceiptPath.Count -ne 1) { throw 'Expected exactly one private save receipt.' }
$SaveReceipt = Get-Content -LiteralPath $SaveReceiptPath[0] -Raw -Encoding UTF8 | ConvertFrom-Json
if ($SaveReceipt.schema -cne 'ARM-CLEAN06.4X64/save-copy-receipt/v1' -or
    $SaveReceipt.head -cne $ExpectedHead -or $SaveReceipt.source_directory -ine $SaveDirectory -or
    -not $SaveReceipt.live_saves_unchanged -or $SaveReceipt.installed -or
    @($SaveReceipt.files).Count -ne [int]$SaveReceipt.save_count) { throw 'Invalid save receipt.' }
$SaveRows = @($SaveReceipt.files)
$SaveSnapshot = [string]$SaveReceipt.snapshot
$Installed = New-Object 'System.Collections.Generic.List[object]'
function CheckSaveSet([switch]$Converted) {
    $Entries = @(Get-ChildItem -LiteralPath $SaveDirectory -Force)
    if (@($Entries | Where-Object { $_.PSIsContainer -or ($_.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0 }).Count -ne 0) {
        throw 'Save directory contains a new directory or reparse point.'
    }
    $Actual = @($Entries | Where-Object { $_.Extension -ieq '.sav' })
    if ($Actual.Count -ne $SaveRows.Count) { throw 'Live save set changed.' }
    foreach ($Row in $SaveRows) {
        if ($Row.name -notmatch '^[A-Za-z0-9_-]+\.sav$' -or
            @($Actual | Where-Object { $_.Name -ceq $Row.name }).Count -ne 1) { throw 'Save filename mismatch.' }
        $Expected = if ($Converted) { [string]$Row.converted_sha256 } else { [string]$Row.original_sha256 }
        if ((Sha (Join-Path $SaveDirectory $Row.name)) -cne $Expected -or
            (Sha (Join-Path (Join-Path $SaveSnapshot 'Original') $Row.name)) -cne $Row.original_sha256 -or
            (Sha (Join-Path (Join-Path $SaveSnapshot 'Converted') $Row.name)) -cne $Row.converted_sha256) {
            throw ('Private or live save hash mismatch: '+$Row.name)
        }
    }
}
function ReplaceSave([string]$Source,[string]$Target,[string]$ExpectedBefore,[string]$ExpectedAfter) {
    if ((Sha $Target) -cne $ExpectedBefore -or (Sha $Source) -cne $ExpectedAfter) { throw 'Save compare-and-replace refused.' }
    $Temp = Join-Path $SaveDirectory ('.armx66-'+[guid]::NewGuid().ToString('N')+'.tmp')
    try {
        Copy-Item -LiteralPath $Source -Destination $Temp
        if ((Sha $Temp) -cne $ExpectedAfter -or (Sha $Target) -cne $ExpectedBefore) { throw 'Save replacement race.' }
        [IO.File]::Replace($Temp,$Target,$null)
    } finally {
        if (Test-Path -LiteralPath $Temp -PathType Leaf) { Remove-Item -LiteralPath $Temp -Force }
    }
}
CheckSaveSet

$Committed = $false
try {
    CheckClean
    CheckHashes $Before
    CheckSaveSet
    if ((Head) -cne $ExpectedHead) { throw 'HEAD changed before Apply.' }
    $Receipt = @(('-ExpectedHead='+$ExpectedHead))
    for ($Index=0; $Index -lt $ExistingCount; ++$Index) { $Receipt += ('-Backup'+$Index+'='+$Backups[$Index]) }
    $Result = RunUE 'Apply' (Join-Path $Run 'apply.json') $Receipt
    CheckReport $Result.Data 'Apply' 0 12
    CheckSameManifest $Pre.Data $Result.Data
    $After = @($Live | ForEach-Object { StateHash $_ })
    for ($Index=0; $Index -lt 12; ++$Index) {
        if ($After[$Index] -ceq 'ABSENT' -or $After[$Index] -ceq $Before[$Index]) {
            throw ('Expected actual native write: '+$Relative[$Index])
        }
        NativeHeader $Live[$Index]
    }
    $Verify = RunUE 'Verify' (Join-Path $Run 'reopened-verify.json') @()
    CheckReport $Verify.Data 'Verify' 0 0
    CheckSameManifest $Pre.Data $Verify.Data
    CheckHashes $After
    CheckTwelveDirty
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
    CheckTwelveDirty
    [void](Git @('fetch','--no-tags','origin','master'))
    $AfterRemote = ([string]@(Git @('rev-parse','refs/remotes/origin/master'))[0]).Trim()
    if ((Head) -cne $ExpectedHead -or $AfterRemote -cne $ExpectedHead) {
        throw 'Master advanced during native verification; refuse publication.'
    }
    if (@(Get-Process -Name 'UnrealEditor','UnrealEditor-Cmd','GrimrockPrototype','GrimrockPrototype-Win64-Shipping' -ErrorAction SilentlyContinue).Count -ne 0) { throw 'Game/editor reopened during verification.' }
    CheckSaveSet
    foreach ($Row in $SaveRows) {
        if ($Row.original_sha256 -ceq $Row.converted_sha256) { continue }
        $Source = Join-Path (Join-Path $SaveSnapshot 'Converted') $Row.name
        $Target = Join-Path $SaveDirectory $Row.name
        $Installed.Add($Row)
        ReplaceSave $Source $Target $Row.original_sha256 $Row.converted_sha256
    }
    CheckSaveSet -Converted
    Write-Host ('[ARM-CLEAN06.4X66] VERIFIED SAVE INSTALLATION '+$Installed.Count+' changed; originals: '+$SaveSnapshot)
    if (-not $Publish) {
        Write-Host ('[ARM-CLEAN06.4X66] TWELVE VERIFIED LOCAL UASSETS; backup: '+$Run)
        Write-Host 'No publication requested; twelve allowlisted packages remain locally modified. Converted saves are installed; do not revert assets alone.'
        return
    }
    [void](Git (@('add','--') + $Relative))
    $Staged = @(Git @('diff','--cached','--name-only'))
    if ($Staged.Count -ne 12) { throw 'Staged set must contain only the twelve approved assets.' }
    for ($Index=0; $Index -lt 12; ++$Index) {
        if ($Staged -cnotcontains $Relative[$Index]) { throw 'Unexpected staged set.' }
        $Pointer = @(Git @('show',(':'+$Relative[$Index])))
        if ($Pointer -cnotcontains 'version https://git-lfs.github.com/spec/v1' -or
            $Pointer -cnotcontains ('oid sha256:'+$After[$Index]) -or
            $Pointer -cnotcontains ('size '+(Get-Item -LiteralPath $Live[$Index]).Length)) {
            throw ('Staged LFS pointer differs from validated bytes: '+$Relative[$Index])
        }
    }
    CheckSaveSet -Converted
    [void](Git @('-c','user.name=Grimrock UE Migration',
        '-c','user.email=grimrock-ue-migration@users.noreply.github.com',
        'commit','-m','ARM-CLEAN06.4X66B: register ten alchemy definitions and unique detailed actions'))
    $Committed = $true
    $NewHead = Head
    if (([string]@(Git @('rev-parse','HEAD^'))[0]).Trim() -cne $ExpectedHead) {
        throw 'Binary commit parent mismatch; do not rewrite.'
    }
    CheckSaveSet -Converted
    [void](Git @('push','origin','HEAD:refs/heads/master'))
    Write-Host ('[ARM-CLEAN06.4X66] TWELVE NATIVE PACKAGES PUSHED '+$NewHead)
    Write-Host ('[ARM-CLEAN06.4X66] PRIVATE SNAPSHOTS '+$Run)
    Write-Host ('[ARM-CLEAN06.4X66] PRIVATE SAVE RECEIPT '+$SaveReceiptPath[0])
    Write-Host 'Await SilentTask and LocalAuto on binary SHA, then alchemy PIE/editor checks.'
}
catch {
    if (-not $Committed -and (Head) -ceq $ExpectedHead) {
        foreach ($Row in $Installed) {
            $Target = Join-Path $SaveDirectory $Row.name
            if ((Sha $Target) -ceq $Row.original_sha256) { continue }
            ReplaceSave (Join-Path (Join-Path $SaveSnapshot 'Original') $Row.name) $Target $Row.converted_sha256 $Row.original_sha256
        }
        CheckSaveSet
        Write-Host '[ARM-CLEAN06.4X66] ORIGINAL SAVE HASHES VERIFIED.'
        Write-Warning ('Native transaction failed; restoring the approved two source packages and removing ten newly created files: '+$_.Exception.Message)
        $Dirty = @(Git @('status','--porcelain','--untracked-files=all'))
        foreach ($Line in $Dirty) {
            $Text = [string]$Line
            if ($Text.Length -lt 4 -or $Relative -cnotcontains $Text.Substring(3).Replace('\','/')) {
                throw 'Unexpected user file modification: automatic rollback refused.'
            }
        }
        $StagedNow = @(Git @('diff','--cached','--name-only'))
        foreach ($File in $StagedNow) {
            if ($Relative -cnotcontains [string]$File) { throw 'Unexpected staged user file: rollback refused.' }
        }
        if ($StagedNow.Count -gt 0) { [void](Git (@('restore','--staged','--') + $StagedNow)) }
        for ($Index=0; $Index -lt $ExistingCount; ++$Index) {
            Copy-Item -LiteralPath $Backups[$Index] -Destination $Live[$Index] -Force
        }
        for ($Index=$ExistingCount; $Index -lt 12; ++$Index) {
            if ($Before[$Index] -cne 'ABSENT') { throw 'Refusing deletion of pre-existing asset.' }
            if (Test-Path -LiteralPath $Live[$Index] -PathType Leaf) {
                Remove-Item -LiteralPath $Live[$Index] -Force
            }
        }
        CheckHashes $Before
        CheckClean
        Write-Host '[ARM-CLEAN06.4X66] ROLLBACK VERIFIED: source SHA256 restored; worktree clean.'
    }
    else { Write-Warning 'HEAD changed or binary commit exists; never reset or force-push committed history.' }
    throw
}
