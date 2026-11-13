# ARM-CLEAN06.2AN - one-shot native UE canonical staging in detached CI worktree.
# Invoked ONLY after successful complete Windows build and Grimrock Automation.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$WorktreePath,
    [Parameter(Mandatory=$true)][string]$EngineRoot,
    [Parameter(Mandatory=$true)][string]$ExpectedHead,
    [Parameter(Mandatory=$true)][string]$RunDir
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$Trigger = 'fix(ARM-CLEAN06.2AN.1): invoke native Git safely during canonical asset staging'
$StageFile = Join-Path $RunDir 'ARMClean062AN.stage'
$Prefix = 'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage'
function Stage([string]$Value) {
    Set-Content -LiteralPath $StageFile -Value $Value -Encoding ASCII
}
function Invoke-GitChecked([string[]]$Arguments) {
    $Output = @(& git -C $WorktreePath @Arguments)
    if ($LASTEXITCODE -ne 0) { throw "Git failed: $($Arguments[0])" }
    return @($Output)
}
function GitOne([string[]]$Arguments) {
    $Output = @(Invoke-GitChecked -Arguments $Arguments)
    if ($Output.Count -ne 1) { throw 'Expected one Git record.' }
    return ([string]$Output[0]).Trim()
}
function Changes {
    return @(Invoke-GitChecked -Arguments @('status','--porcelain','--untracked-files=all'))
}
try {
    Stage 'PREFLIGHT'
    Stage 'PREFLIGHT_GIT'
    if ($ExpectedHead -notmatch '^[a-f0-9]{40}$' -or
        -not (Test-Path -LiteralPath (Join-Path $WorktreePath '.git'))) {
        throw 'Invalid SHA or worktree.'
    }
    if ((GitOne -Arguments @('rev-parse','HEAD')) -cne $ExpectedHead -or
        (GitOne -Arguments @('log','-1','--format=%s')) -cne $Trigger) {
        throw 'Worktree or one-shot trigger mismatch.'
    }
    if (@(Invoke-GitChecked -Arguments @('branch','--show-current') | Where-Object { $_ }).Count -ne 0) {
        throw 'Only a detached validation worktree is allowed.'
    }
    if ((GitOne -Arguments @('remote','get-url','origin')) -notmatch
        '(?i)github\.com[:/]PhilippeMartin1964/GrimrockPrototype(\.git)?$') {
        throw 'Unexpected repository origin.'
    }
    if (@(Changes).Count -ne 0) { throw 'Validation worktree not clean.' }
    Stage 'PREFLIGHT_MANIFEST'
    $Rows = @(Import-Csv -LiteralPath (Join-Path $WorktreePath 'Config/Armement/WeaponProductionManifest.csv') -Encoding UTF8)
    if ($Rows.Count -ne 22 -or @($Rows | Where-Object { $_.ProductionState -cne 'Active' }).Count -ne 0) {
        throw 'Manifest is not the approved 22-row source.'
    }
    $Items = @($Rows | ForEach-Object { [string]$_.ItemDefinitionId })
    if ($Items.Count -ne 22 -or @($Items | Select-Object -Unique).Count -ne 22 -or
        @($Items | Where-Object { $_ -cnotmatch '^Weapon_[A-Za-z0-9_]+$' }).Count -ne 0) {
        throw 'Unapproved item identities.'
    }
    $Offensive = @($Rows | Where-Object { $_.AttackId })
    if ($Offensive.Count -ne 19 -or
        @($Offensive | Where-Object { $_.MigrationPresentationId -cnotmatch '^PRES_ARM[0-9]{3}$' }).Count -ne 0) {
        throw 'Manifest action presentation contract mismatch.'
    }
    Stage 'PREFLIGHT_PATHS'
    $Expected = @(
        "$Prefix/DA_CanonicalWeaponCatalogue.uasset",
        "$Prefix/DA_CanonicalWeaponVisualCatalogue.uasset",
        "$Prefix/DA_ActionCatalogue.uasset",
        "$Prefix/DA_ActionPresentationCatalogue.uasset",
        "$Prefix/DA_ItemSystem.uasset",
        "$Prefix/Definitions/DA_Canonical_Item_Arrows_01.uasset"
    )
    foreach ($Id in $Items) {
        $Expected += "$Prefix/Definitions/DA_Canonical_$Id.uasset"
    }
    $Expected = @($Expected | Sort-Object -Unique)
    if ($Expected.Count -ne 28) { throw 'Expected 28 unique new packages.' }
    foreach ($Path in $Expected) {
        if ((GitOne -Arguments @('check-attr','filter','--',$Path)) -notmatch ': filter: lfs$' -or
            (Test-Path -LiteralPath (Join-Path $WorktreePath $Path))) {
            throw 'Destination occupied or lacks Git LFS rules.'
        }
    }
    $StageFolder = Join-Path $WorktreePath $Prefix
    if ((Test-Path -LiteralPath $StageFolder) -and
        @(Get-ChildItem -LiteralPath $StageFolder -Force -Recurse -ErrorAction Stop).Count -gt 0) {
        throw 'Canonical stage folder contains preexisting files.'
    }
    Stage 'PREFLIGHT_ENV'
    $Project = Join-Path $WorktreePath 'GrimrockPrototype.uproject'
    $Editor = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
    $Validate = Join-Path $WorktreePath 'Scripts/ValidateUE.ps1'
    $PsExe = Join-Path $env:SystemRoot 'System32/WindowsPowerShell/v1.0/powershell.exe'
    foreach ($Path in @($Project,$Editor,$Validate,$PsExe)) {
        if (-not (Test-Path -LiteralPath $Path -PathType Leaf) -or $Path.Contains('"')) {
            throw 'Required native executable missing or invalid path.'
        }
    }
    Stage 'COMMANDLET'
    $Out = Join-Path $RunDir 'CanonicalStage.stdout.log'
    $Err = Join-Path $RunDir 'CanonicalStage.stderr.log'
    $Arguments = '"' + $Project + '" -run=GridArmClean06CanonicalStage -Apply -Unattended -NoSplash -NoP4 -NoSound -NullRHI'
    $Proc = Start-Process -FilePath $Editor -ArgumentList $Arguments -WorkingDirectory $WorktreePath -WindowStyle Hidden -PassThru -Wait -RedirectStandardOutput $Out -RedirectStandardError $Err -ErrorAction Stop
    if ($Proc.ExitCode -ne 0) {
        Stage 'COMMANDLET_FAILED'
        throw 'Unreal native asset stage failed.'
    }
    Stage 'VERIFY_FILESET'
    $Actual = @(Changes)
    if ($Actual.Count -ne 28) { throw 'Only 28 new assets are authorized.' }
    $ActualPaths = @()
    foreach ($Line in $Actual) {
        if (-not $Line.StartsWith('?? ')) { throw 'Unexpected tracked modification.' }
        $AssetPath = $Line.Substring(3)
        if (-not ($Expected -ccontains $AssetPath)) { throw 'Unexpected new file.' }
        $ActualPaths += $AssetPath
    }
    if ((@($ActualPaths | Sort-Object) -join '|') -cne ($Expected -join '|')) {
        throw 'Asset fileset does not match the 28 expected paths.'
    }
    foreach ($Path in $Expected) {
        $Size = (Get-Item -LiteralPath (Join-Path $WorktreePath $Path)).Length
        if ($Size -lt 128 -or $Size -gt 33554432) { throw 'Binary asset size outside accepted bounds.' }
    }
    Stage 'POST_AUTOMATION'
    # A new Editor process must reload every .uasset and compare all UPROPERTY
    # fields to the 22+ammo source candidate; no transient-only success accepted.
    $AfterRoot = Join-Path $RunDir 'CanonicalStagePostAutomation'
    $AfterOut = Join-Path $RunDir 'CanonicalStagePost.stdout.log'
    $AfterErr = Join-Path $RunDir 'CanonicalStagePost.stderr.log'
    $AfterArgs = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' +
        $Validate + '" -EngineRoot "' + $EngineRoot +
        '" -SkipBuild -AutomationFilter "Grimrock" -ReportRoot "' + $AfterRoot + '"'
    $After = Start-Process -FilePath $PsExe -ArgumentList $AfterArgs -WorkingDirectory $WorktreePath -WindowStyle Hidden -PassThru -Wait -RedirectStandardOutput $AfterOut -RedirectStandardError $AfterErr -ErrorAction Stop
    if ($After.ExitCode -ne 0) {
        Stage 'POST_AUTOMATION_FAILED'
        throw 'Post-stage real-binary UE Automation failed.'
    }
    $Again = @(Changes)
    if (($Again -join '|') -cne ($Actual -join '|')) { throw 'Post-stage UE modified unexpected assets.' }
    Stage 'PARITY_OK'
    [void](Invoke-GitChecked -Arguments @('fetch','--no-tags','origin','master'))
    if ((GitOne -Arguments @('rev-parse','refs/remotes/origin/master')) -cne $ExpectedHead) {
        Stage 'REMOTE_CHANGED'
        throw 'Remote master advanced during staging.'
    }
    [void](Invoke-GitChecked -Arguments (@('add','--') + $Expected))
    $Staged = @(Invoke-GitChecked -Arguments @('diff','--cached','--name-only','--diff-filter=A') | Sort-Object)
    if ($Staged.Count -ne 28 -or ($Staged -join '|') -cne ($Expected -join '|')) {
        Stage 'FILESET_UNEXPECTED'
        throw 'Git staged fileset differs from approved 28 new binaries.'
    }
    # Prove Git staged LFS *pointers* rather than raw binary Git blobs.
    foreach ($Path in $Expected) {
        $Pointer = @(& git -C $WorktreePath show ":$Path")
        if ($LASTEXITCODE -ne 0 -or
            (@($Pointer) -join "`n") -notmatch '^version https://git-lfs.github.com/spec/v1') {
            Stage 'LFS_FAILED'
            throw 'Git staged a non-LFS asset.'
        }
    }
    [void](Invoke-GitChecked -Arguments @('-c','user.name=Grimrock UE Automation',
        '-c','user.email=grimrock-ue-automation@users.noreply.github.com',
        'commit','-m','assets(ARM-CLEAN06.2AO): stage 28 native canonical DataAssets with verified parity'))
    Stage 'COMMIT_CREATED'
    $PublishedSha = GitOne -Arguments @('rev-parse','HEAD')
    & git -C $WorktreePath push origin 'HEAD:refs/heads/master' | Out-Null
    if ($LASTEXITCODE -ne 0) {
        Stage 'PUSH_FAILED'
        throw 'Asset push failed, no force push performed.'
    }
    Stage 'SUCCESS'
    Write-Host "[ARM-CLEAN06.2AN] PUBLISHED $PublishedSha; 28 Unreal-authored LFS assets."
    exit 0
}
catch {
    # No destructive generic cleanup. If AND ONLY IF changes are our exact
    # untracked new CanonicalStage .uassets, first make SHA-verified private
    # recovery copies, then clear those copies from the detached worktree.
    # Unexpected changes and post-commit states are left untouched.
    try {
        $Dirty = @(& git -C $WorktreePath status --porcelain --untracked-files=all)
        $CanRecover = $LASTEXITCODE -eq 0 -and $Dirty.Count -gt 0 -and $Dirty.Count -le 28
        $RecoverPaths = @()
        if ($CanRecover) {
            foreach ($Entry in $Dirty) {
                if (-not $Entry.StartsWith('?? ')) { $CanRecover = $false; break }
                $RecoveryPath = $Entry.Substring(3)
                if (-not ($Expected -ccontains $RecoveryPath)) { $CanRecover = $false; break }
                $RecoverPaths += $RecoveryPath
            }
        }
        if ($CanRecover) {
            $RecoveryFolder = Join-Path $RunDir 'CanonicalStageRecovery'
            foreach ($Path in $RecoverPaths) {
                $Source = Join-Path $WorktreePath $Path
                $Backup = Join-Path $RecoveryFolder $Path
                New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Backup) | Out-Null
                Copy-Item -LiteralPath $Source -Destination $Backup -ErrorAction Stop
                if ((Get-FileHash -LiteralPath $Source -Algorithm SHA256).Hash -cne
                    (Get-FileHash -LiteralPath $Backup -Algorithm SHA256).Hash) {
                    throw 'Recovery SHA mismatch; original remains.'
                }
            }
            foreach ($Path in $RecoverPaths) {
                Remove-Item -LiteralPath (Join-Path $WorktreePath $Path) -Force -ErrorAction Stop
            }
        }
    }
    catch { Write-Warning 'Canonical stage recovery incomplete; isolated worktree kept for diagnosis.' }
    throw
}
