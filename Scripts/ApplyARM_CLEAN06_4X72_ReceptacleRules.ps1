<#
 ARM-CLEAN06.4X.16.7.2 — guarded physical write of ONLY two
 native receptacle definition assets in the validation worktree.
 Backup/verify every Git LFS source and every post-save artifact.
 The user's development checkout and all levels/saves are untouched.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$WorktreePath,
    [Parameter(Mandatory=$true)][string]$EngineRoot,
    [Parameter(Mandatory=$true)][string]$StateRoot,
    [Parameter(Mandatory=$true)][string]$RunDir,
    [Parameter(Mandatory=$true)][string]$ExpectedHead,
    [string]$GitHubRepository='PhilippeMartin1964/GrimrockPrototype'
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$StagePath = Join-Path $RunDir 'ARMClean064X72.stage'
function Stage([string]$Name) { Set-Content -LiteralPath $StagePath -Value $Name -Encoding ASCII }
function Invoke-GitChecked([string[]]$GitArguments) {
    $Out = @(& git.exe -C $WorktreePath @GitArguments)
    if ($LASTEXITCODE -ne 0) { throw 'Protected git operation failed.' }
    return $Out
}
function Require-Clean {
    $Dirty = @(Invoke-GitChecked -GitArguments @('status','--porcelain','--untracked-files=all'))
    if ($Dirty.Count -ne 0) { Stage 'WORKTREE_DIRTY'; throw 'Worktree must stay clean.' }
}
function Check-Report($R,[string]$Mode) {
    if ($null -eq $R -or
        [string]$R.schema -cne 'ARM-CLEAN06.4X7/real-receptacle-acceptance/v1' -or
        [string]$R.mode -cne $Mode -or
        [int]$R.definitions -ne 44 -or [int]$R.levels -ne 4 -or
        [int]$R.receptacle_lists -ne 6 -or [int]$R.receptacle_rules -ne 2 -or
        [int]$R.lock_lists -ne 2 -or [int]$R.lock_pointers -ne 2 -or
        [int]$R.lock_explicit_ids -ne 0 -or [int]$R.overrides -ne 0 -or
        [int]$R.targets -ne 2 -or
        [int]$R.already_canonical -ne $(if($Mode -ceq 'Verify'){2}else{0}) -or
        [int]$R.eligible_legacy -ne $(if($Mode -ceq 'Verify'){0}else{2}) -or
        [int]$R.packages_saved -ne $(if($Mode -ceq 'Apply'){2}else{0}) -or
        $R.legacy_pointers_retained -ne $true -or
        $R.savegames_opened_or_written -ne $false) {
        Stage 'UE_REPORT_INVALID'
        throw 'UE report fails exact real-asset gate.'
    }
}
Stage 'SCRIPT_STARTED'
$Parent = 'c9eb8bcfe5b0d7c43f4af1b13787c53260045aaa'
$Subject = 'fix(ARM-CLEAN06.4X.16.7.2F): repair validation hook syntax with exact checks'
$Files = @(
    'Content/GrimrockPrototype/Core/DataAssets/DA_Receptacle_TorchHolder.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Guardian.uasset'
)
$BinaryCommitSubject = 'feat(ARM-CLEAN06.4X.16.7.3): persist two native receptacle ItemIds'
$OriginalHead = (@(Invoke-GitChecked -GitArguments @('rev-parse','HEAD')) | Select-Object -First 1).Trim()
$ActualParent = (@(Invoke-GitChecked -GitArguments @('rev-parse','HEAD^')) | Select-Object -First 1).Trim()
$ActualSubject = (@(Invoke-GitChecked -GitArguments @('log','-1','--format=%s')) | Select-Object -First 1).Trim()
if ($OriginalHead -cne $ExpectedHead -or $ActualParent -cne $Parent -or
    $ActualSubject -cne $Subject) {
    Stage 'COMMIT_IDENTITY_REJECTED'
    throw 'Migration requires exact code commit and validated parent.'
}
$CodePaths = @(
    'Scripts/ApplyARM_CLEAN06_4X72_ReceptacleRules.ps1',
    'Scripts/ValidateUE.ps1'
)
$Changed = @(Invoke-GitChecked -GitArguments @('diff-tree','--no-commit-id','--name-only','-r','HEAD'))
if ($Changed.Count -ne $CodePaths.Count -or
    @($Changed | Where-Object { $CodePaths -cnotcontains $_ }).Count -ne 0 -or
    @($CodePaths | Where-Object { $Changed -cnotcontains $_ }).Count -ne 0) {
    Stage 'CODE_CHANGESET_MISMATCH'
    throw 'Code commit files differ from allowlist.'
}
$Project = Join-Path $WorktreePath 'GrimrockPrototype.uproject'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Validator = Join-Path $WorktreePath 'Scripts\ValidateUE.ps1'
$PsExe = Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe'
foreach ($Required in @($Project,$Editor,$Validator,$PsExe)) {
    if (-not (Test-Path -LiteralPath $Required -PathType Leaf)) {
        Stage 'EXECUTABLE_MISSING'
        throw 'UE or validation executable not found.'
    }
}
Require-Clean
$PrivateFolder = Join-Path $RunDir 'ARM-CLEAN06.4X72'
New-Item -ItemType Directory -Path $PrivateFolder -Force -ErrorAction Stop | Out-Null
Stage 'VERIFY_GIT_LFS_SOURCES'
foreach ($Relative in $Files) {
    $Attribute = @(Invoke-GitChecked -GitArguments @('check-attr','filter','--',$Relative))
    if ($Attribute.Count -ne 1 -or [string]$Attribute[0] -notmatch ': filter: lfs$') {
        Stage 'LFS_ATTRIBUTE_MISSING'
        throw 'Native package is not LFS tracked.'
    }
}
$Include = $Files -join ','
$LfsPull = Start-Process -FilePath 'git.exe' -ArgumentList ('-C "'+$WorktreePath+'" lfs pull origin "--include='+$Include+'" "--exclude="') -WorkingDirectory $WorktreePath -NoNewWindow -Wait -PassThru -RedirectStandardOutput (Join-Path $PrivateFolder 'lfs.stdout-private.log') -RedirectStandardError (Join-Path $PrivateFolder 'lfs.stderr-private.log') -ErrorAction Stop
if ($LfsPull.ExitCode -ne 0) { Stage 'LFS_FETCH_FAILED'; throw 'Cannot hydrate exact two native packages.' }
$CheckoutArgs = '-C "'+$WorktreePath+'" lfs checkout'
foreach ($Relative in $Files) { $CheckoutArgs += ' "'+$Relative+'"' }
$Checkout = Start-Process -FilePath 'git.exe' -ArgumentList $CheckoutArgs -WorkingDirectory $WorktreePath -NoNewWindow -Wait -PassThru -RedirectStandardOutput (Join-Path $PrivateFolder 'checkout.stdout-private.log') -RedirectStandardError (Join-Path $PrivateFolder 'checkout.stderr-private.log') -ErrorAction Stop
if ($Checkout.ExitCode -ne 0) { Stage 'LFS_CHECKOUT_FAILED'; throw 'Hydrated LFS checkout rejected.' }
$Expected = @{}
foreach ($Relative in $Files) {
    $Pointer = @(Invoke-GitChecked -GitArguments @('show',($ExpectedHead+':'+$Relative)) | Where-Object { $_ })
    if ($Pointer.Count -ne 3 -or $Pointer[0] -cne 'version https://git-lfs.github.com/spec/v1' -or
        [string]$Pointer[1] -notmatch '^oid sha256:([0-9a-f]{64})$') {
        Stage 'LFS_POINTER_INVALID'
        throw 'Git-tracked LFS pointer not canonical.'
    }
    $ExpectedHash = $Matches[1]
    if ([string]$Pointer[2] -notmatch '^size ([0-9]{3,12})$') {
        Stage 'LFS_POINTER_INVALID'
        throw 'Git-tracked LFS size absent.'
    }
    $ExpectedSize = [int64]$Matches[1]
    $Absolute = Join-Path $WorktreePath $Relative.Replace('/','\')
    if (-not (Test-Path -LiteralPath $Absolute -PathType Leaf) -or
        (Get-Item -LiteralPath $Absolute).Length -ne $ExpectedSize -or
        (Get-FileHash -LiteralPath $Absolute -Algorithm SHA256).Hash.ToLowerInvariant() -cne $ExpectedHash) {
        Stage 'LFS_SOURCE_HASH_MISMATCH'
        throw 'Native package differs from immutable Git LFS source.'
    }
    $Stream = [System.IO.File]::OpenRead($Absolute)
    try {
        [byte[]]$Header = @(0,0,0,0)
        if ($Stream.Read($Header,0,4) -ne 4 -or
            $Header[0] -ne 0xC1 -or $Header[1] -ne 0x83 -or
            $Header[2] -ne 0x2A -or $Header[3] -ne 0x9E) {
            Stage 'INVALID_UE_PACKAGE'
            throw 'Hydrated binary has no Unreal asset header.'
        }
    }
    finally { $Stream.Dispose() }
    $Expected[$Relative] = @{ Hash = $ExpectedHash; Bytes = $ExpectedSize }
}
Require-Clean
$BackupRoot = Join-Path $StateRoot ('Recovery\ARM064X72-'+$ExpectedHead.Substring(0,12)+'-'+(Get-Date -Format 'yyyyMMddHHmmssfff'))
$BackupsReady = $false
try {
    Stage 'BACKUP_NATIVE_SOURCES'
    foreach ($Relative in $Files) {
        $Original = Join-Path $WorktreePath $Relative.Replace('/','\')
        $Backup = Join-Path $BackupRoot $Relative.Replace('/','\')
        New-Item -ItemType Directory -Path (Split-Path -Parent $Backup) -Force -ErrorAction Stop | Out-Null
        Copy-Item -LiteralPath $Original -Destination $Backup -ErrorAction Stop
        if ((Get-FileHash -LiteralPath $Backup -Algorithm SHA256).Hash.ToLowerInvariant() -cne $Expected[$Relative].Hash -or
            (Get-Item -LiteralPath $Backup).Length -ne $Expected[$Relative].Bytes) {
            Stage 'BACKUP_HASH_MISMATCH'
            throw 'Private original backup could not be verified.'
        }
    }
    $BackupsReady = $true
    function Run-UE([string]$Mode,[string]$ReportPath) {
        $LogPath = Join-Path $PrivateFolder ($Mode.ToLowerInvariant()+'-private.log')
        & $Editor $Project '-run=GridArmClean064X71ReceptacleRules' '-Unattended' '-NoSplash' '-NoP4' '-NullRHI' '-stdout' '-FullStdOutLogOutput' ('-'+$Mode) ("-Output="+$ReportPath) *> $LogPath
        if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $ReportPath -PathType Leaf)) {
            Stage ('UE_'+$Mode.ToUpperInvariant()+'_FAILED')
            throw 'UE commandlet failed or did not provide a report.'
        }
        $R = Get-Content -LiteralPath $ReportPath -Raw -Encoding UTF8 | ConvertFrom-Json
        Check-Report $R $Mode
    }
    Stage 'READ_ONLY_AUDIT'
    Run-UE 'Audit' (Join-Path $PrivateFolder 'audit-private.json')
    Require-Clean
    Stage 'APPLY_TWO_NATIVE_UASSETS'
    Run-UE 'Apply' (Join-Path $PrivateFolder 'applied-private.json')
    $ActualChanged = @(Invoke-GitChecked -GitArguments @('diff','--name-only'))
    if ($ActualChanged.Count -ne 2 -or
        @($ActualChanged | Where-Object { $Files -cnotcontains $_ }).Count -ne 0) {
        Stage 'UNEXPECTED_BINARY_DIFF'
        throw 'Unreal changed files outside the exact two native packages.'
    }
    $Status = @(Invoke-GitChecked -GitArguments @('status','--porcelain','--untracked-files=all'))
    if ($Status.Count -ne 2 -or
        @($Status | Where-Object {
            ([string]$_).Length -lt 4 -or
            ([string]$_).Substring(0,2) -cne ' M' -or
            $Files -cnotcontains ([string]$_).Substring(3).Replace('\','/')
        }).Count -ne 0) {
        Stage 'UNEXPECTED_WORKTREE_DIRTY'
        throw 'Unauthorized file appeared during Unreal write.'
    }
    Stage 'VERIFY_REOPEN_SECOND_EDITOR'
    Run-UE 'Verify' (Join-Path $PrivateFolder 'reopened-private.json')
    Stage 'POST_BINARY_AUTOMATION'
    $ValidateArgs = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "'+$Validator+'" -EngineRoot "'+$EngineRoot+'" -SkipBuild -AutomationFilter "Grimrock" -ReportRoot "'+(Join-Path $PrivateFolder 'post-automation')+'"'
    $Validation = Start-Process -FilePath $PsExe -ArgumentList $ValidateArgs -WorkingDirectory $WorktreePath -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput (Join-Path $PrivateFolder 'suite.stdout-private.log') -RedirectStandardError (Join-Path $PrivateFolder 'suite.stderr-private.log') -ErrorAction Stop
    if ($Validation.ExitCode -ne 0) {
        Stage 'POST_AUTOMATION_FAILED'
        throw 'Grimrock suite failed after binary conversion.'
    }
    $AfterAutomation = @(Invoke-GitChecked -GitArguments @('diff','--name-only'))
    if (($AfterAutomation -join '|') -cne ($ActualChanged -join '|')) {
        Stage 'POST_AUTOMATION_CHANGED_FILESET'
        throw 'Automation changed native file allowlist.'
    }
    Stage 'REMOTE_LEASE_CHECK'
    [void](Invoke-GitChecked -GitArguments @('fetch','--no-tags','origin','master'))
    $Remote = (@(Invoke-GitChecked -GitArguments @('rev-parse','refs/remotes/origin/master')) | Select-Object -First 1).Trim()
    if ($Remote -cne $ExpectedHead) {
        Stage 'REMOTE_MASTER_MOVED'
        throw 'Master advanced, refuse binary commit.'
    }
    Stage 'STAGE_TWO_BINARY_PACKAGES'
    [void](Invoke-GitChecked -GitArguments (@('add','--')+$Files))
    $Staged = @(Invoke-GitChecked -GitArguments @('diff','--cached','--name-only'))
    if ($Staged.Count -ne 2 -or
        @($Staged | Where-Object { $Files -cnotcontains $_ }).Count -ne 0) {
        Stage 'STAGED_FILESET_MISMATCH'
        throw 'Git staged unauthorized file.'
    }
    Stage 'COMMIT_NATIVE_BINARY_CHILD'
    & git -C $WorktreePath -c 'user.name=Grimrock UE Automation' -c 'user.email=grimrock-ue-automation@users.noreply.github.com' commit -m $BinaryCommitSubject | Out-Null
    if ($LASTEXITCODE -ne 0) { throw 'Git binary child commit failed.' }
    $NewHead = (@(Invoke-GitChecked -GitArguments @('rev-parse','HEAD')) | Select-Object -First 1).Trim()
    $NewParent = (@(Invoke-GitChecked -GitArguments @('rev-parse','HEAD^')) | Select-Object -First 1).Trim()
    $ChildFiles = @(Invoke-GitChecked -GitArguments @('diff-tree','--no-commit-id','--name-only','-r','HEAD'))
    if ($NewHead -ceq $ExpectedHead -or $NewParent -cne $ExpectedHead -or
        $ChildFiles.Count -ne 2 -or
        @($ChildFiles | Where-Object { $Files -cnotcontains $_ }).Count -ne 0) {
        Stage 'BINARY_COMMIT_MISMATCH'
        throw 'Binary child commit invalid.'
    }
    Require-Clean
    Stage 'PUSH_FAST_FORWARD'
    & git -C $WorktreePath push origin HEAD:refs/heads/master | Out-Null
    if ($LASTEXITCODE -ne 0) { throw 'Fast-forward push of binary child failed.' }
    $BackupsReady = $false
    Stage 'SUCCESS'
    Write-Host ('[ARM-CLEAN06.4X72] EXACT_TWO_PACKAGES_PUSHED '+$NewHead)
}
catch {
    if ($BackupsReady) {
        try {
            $Current = (@(Invoke-GitChecked -GitArguments @('rev-parse','HEAD')) | Select-Object -First 1).Trim()
            if ($Current -cne $ExpectedHead) { throw 'Cannot restore after commit; original backup remains private.' }
            $Dirty = @(Invoke-GitChecked -GitArguments @('diff','--name-only'))
            if (@($Dirty | Where-Object { $Files -cnotcontains $_ }).Count -ne 0) {
                throw 'Unrelated file modified; refuse restore.'
            }
            foreach ($Relative in $Files) {
                $Original = Join-Path $WorktreePath $Relative.Replace('/','\')
                $Backup = Join-Path $BackupRoot $Relative.Replace('/','\')
                if (-not (Test-Path -LiteralPath $Backup -PathType Leaf)) {
                    throw 'Private original backup missing.'
                }
                Copy-Item -LiteralPath $Backup -Destination $Original -Force -ErrorAction Stop
                if ((Get-FileHash -LiteralPath $Original -Algorithm SHA256).Hash.ToLowerInvariant() -cne
                    $Expected[$Relative].Hash) {
                    throw 'Private restore hash mismatch.'
                }
            }
            Require-Clean
        }
        catch { Write-Warning '[ARM-CLEAN06.4X72] Private rollback incomplete; backup preserved.' }
    }
    throw
}
