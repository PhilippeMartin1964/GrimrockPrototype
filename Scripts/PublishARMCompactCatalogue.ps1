# ARM-CLEAN03.3 — one-shot, isolated Windows catalogue migration.
# Called ONLY by VALIDATION-AUTO01 after a successful full Grimrock suite.
# It changes one Unreal-authored LFS .uasset in the detached validation worktree.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$WorktreePath,
    [Parameter(Mandatory=$true)][string]$EngineRoot,
    [Parameter(Mandatory=$true)][string]$ExpectedHead,
    [Parameter(Mandatory=$true)][string]$RunDir
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$Target = 'Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.uasset'
$Trigger = 'feat(ARM-CLEAN03.3): enable one-shot guarded catalogue migration'
$StageFile = Join-Path $RunDir 'ARMCompactMigration.stage'
function Set-Stage([string]$Name) {
    Set-Content -LiteralPath $StageFile -Value $Name -Encoding ASCII
}
function GitLines([string[]]$Arguments) {
    $Output = @(& git -C $WorktreePath @Arguments)
    if ($LASTEXITCODE -ne 0) {
        throw ('ARM-CLEAN03: Git command failed: ' + $Arguments[0])
    }
    return @($Output)
}
function GitOne([string[]]$Arguments) {
    $Output = @(GitLines -Arguments $Arguments)
    if ($Output.Count -ne 1) { throw 'ARM-CLEAN03: expected exactly one Git result.' }
    return [string]$Output[0].Trim()
}
function GitChanges {
    return @(GitLines -Arguments @('status','--porcelain','--untracked-files=all'))
}

try {
Set-Stage 'PREFLIGHT'
if ($ExpectedHead -notmatch '^[0-9a-f]{40}$') { throw 'ARM-CLEAN03: ExpectedHead is invalid.' }
if (-not (Test-Path -LiteralPath (Join-Path $WorktreePath '.git'))) {
    throw 'ARM-CLEAN03: detached validation worktree is missing.'
}
if ((GitOne -Arguments @('rev-parse','HEAD')) -cne $ExpectedHead) {
    throw 'ARM-CLEAN03: worktree HEAD differs from approved SHA.'
}
$BranchLines = @(GitLines -Arguments @('branch','--show-current'))
if (@($BranchLines | Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) }).Count -ne 0) {
    throw 'ARM-CLEAN03: migration is allowed only in detached validation worktree.'
}
if ((GitOne -Arguments @('log','-1','--format=%s')) -cne $Trigger) {
    throw 'ARM-CLEAN03: one-shot trigger is absent.'
}
$Origin = GitOne -Arguments @('remote','get-url','origin')
if ($Origin -notmatch '(?i)github\.com[:/]PhilippeMartin1964/GrimrockPrototype(\.git)?$') {
    throw 'ARM-CLEAN03: unexpected Git origin.'
}
if (@(GitChanges).Count -ne 0) { throw 'ARM-CLEAN03: detached worktree is not clean.' }
$Attributes = GitOne -Arguments @('check-attr','filter','--',$Target)
if ($Attributes -notmatch ': filter: lfs$') { throw 'ARM-CLEAN03: catalogue is not Git LFS tracked.' }
$AssetFile = Join-Path $WorktreePath $Target
if (-not (Test-Path -LiteralPath $AssetFile -PathType Leaf)) {
    throw 'ARM-CLEAN03: production asset is missing.'
}
$OriginalBytes = (Get-Item -LiteralPath $AssetFile).Length
if ($OriginalBytes -lt 1024) { throw 'ARM-CLEAN03: LFS content is not materialized.' }

$ProjectFile = Join-Path $WorktreePath 'GrimrockPrototype.uproject'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Validate = Join-Path $WorktreePath 'Scripts\ValidateUE.ps1'
$PowerShell = Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe'
foreach ($File in @($ProjectFile, $Editor, $Validate, $PowerShell)) {
    if (-not (Test-Path -LiteralPath $File -PathType Leaf)) {
        throw 'ARM-CLEAN03: required local Windows/UE component is missing.'
    }
}
foreach ($Value in @($ProjectFile,$EngineRoot,$Validate,$RunDir)) {
    if ($Value.Contains('"')) { throw 'ARM-CLEAN03: unsupported path quotation.' }
}

Set-Stage 'COMMANDLET'
$Standard = Join-Path $RunDir 'ARMCompactCommandlet.stdout.log'
$Errors = Join-Path $RunDir 'ARMCompactCommandlet.stderr.log'
$Arguments = '"' + $ProjectFile + '" -run=GridWeaponCatalogueCompact -Apply -Unattended -NoSplash -NoP4 -NullRHI'
$Process = Start-Process -FilePath $Editor -ArgumentList $Arguments -WorkingDirectory $WorktreePath -WindowStyle Hidden -PassThru -Wait -RedirectStandardOutput $Standard -RedirectStandardError $Errors -ErrorAction Stop
if ($Process.ExitCode -ne 0) {
    Set-Stage 'COMMANDLET_FAILED'
    throw 'ARM-CLEAN03: native compact commandlet failed.'
}

$Actual = @(GitChanges)
if ($Actual.Count -ne 1 -or $Actual[0] -cne (' M ' + $Target)) {
    Set-Stage 'FILESET_UNEXPECTED'
    throw 'ARM-CLEAN03: commandlet changed files outside the exact one-asset allowlist.'
}
$NewBytes = (Get-Item -LiteralPath $AssetFile).Length
if ($NewBytes -lt 1024 -or $NewBytes -ge $OriginalBytes) {
    Set-Stage 'SIZE_INVALID'
    throw 'ARM-CLEAN03: compact asset must be materialized and smaller than the original.'
}
Set-Stage 'ASSET_SAVED'

# Validate the SAVED compact .uasset, not a transient in-memory candidate.
# The asset is still uncommitted in the detached worktree.
$AfterRoot = Join-Path $RunDir 'ARMCompactPostAutomation'
$AfterOut = Join-Path $RunDir 'ARMCompactPostAutomation.stdout.log'
$AfterErr = Join-Path $RunDir 'ARMCompactPostAutomation.stderr.log'
$AfterArgs = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' +
    $Validate + '" -EngineRoot "' + $EngineRoot +
    '" -SkipBuild -AutomationFilter "Grimrock" -ReportRoot "' + $AfterRoot + '"'
Set-Stage 'POST_AUTOMATION'
$After = Start-Process -FilePath $PowerShell -ArgumentList $AfterArgs -WorkingDirectory $WorktreePath -WindowStyle Hidden -PassThru -Wait -RedirectStandardOutput $AfterOut -RedirectStandardError $AfterErr -ErrorAction Stop
if ($After.ExitCode -ne 0) {
    Set-Stage 'POST_AUTOMATION_FAILED'
    throw 'ARM-CLEAN03: full Grimrock post-migration automation failed.'
}
$PostChanges = @(GitChanges)
if ($PostChanges.Count -ne 1 -or $PostChanges[0] -cne (' M ' + $Target)) {
    Set-Stage 'FILESET_UNEXPECTED'
    throw 'ARM-CLEAN03: post-migration tests modified unexpected files.'
}
Set-Stage 'PARITY_AND_AUTOMATION_OK'

# Refuse stale fast-forward bases. Never force-push; never touch the user's
# regular worktree, UBT global settings, or any other data asset.
[void](GitLines -Arguments @('fetch','--no-tags','origin','master'))
if ((GitOne -Arguments @('rev-parse','refs/remotes/origin/master')) -cne $ExpectedHead) {
    Set-Stage 'REMOTE_CHANGED'
    throw 'ARM-CLEAN03: origin/master diverged; no asset commit was created.'
}
[void](GitLines -Arguments @('add','--',$Target))
$Staged = @(GitLines -Arguments @('diff','--cached','--name-only','--diff-filter=ACMR'))
if ($Staged.Count -ne 1 -or $Staged[0] -cne $Target) {
    Set-Stage 'FILESET_UNEXPECTED'
    throw 'ARM-CLEAN03: staged change is not the unique catalogue asset.'
}
[void](GitLines -Arguments @('-c','user.name=Grimrock UE Automation',
                  '-c','user.email=grimrock-ue-automation@users.noreply.github.com',
                  'commit','-m','assets(ARM-CLEAN03): migrate weapon catalogue to compact lossless rules'))
Set-Stage 'COMMIT_CREATED'
$PublishedSha = GitOne -Arguments @('rev-parse','HEAD')
if ($PublishedSha -cne $ExpectedHead) {
    & git -C $WorktreePath push origin 'HEAD:refs/heads/master' | Out-Null
    if ($LASTEXITCODE -ne 0) {
        Set-Stage 'PUSH_FAILED'
        throw 'ARM-CLEAN03: asset commit push failed; no force push attempted.'
    }
}
Set-Stage 'SUCCESS'
Write-Host "[ARM-CLEAN03] COMPACT_PUBLISHED $PublishedSha size-before=$OriginalBytes size-after=$NewBytes"
exit 0
}
catch {
    # Only our detached worktree was clean at entry. If a failed commandlet
    # left exactly the expected single binary modified, restore that binary
    # from the validated HEAD so the next scheduled run is not blocked.
    try {
        if ($ExpectedHead -match '^[0-9a-f]{40}$' -and
            (Test-Path -LiteralPath (Join-Path $WorktreePath '.git'))) {
            $DirtyAfterFailure = @(& git -C $WorktreePath status --porcelain --untracked-files=all)
            if ($LASTEXITCODE -eq 0 -and $DirtyAfterFailure.Count -eq 1 -and
                $DirtyAfterFailure[0] -cne $null -and
                $DirtyAfterFailure[0] -ceq (' M ' + $Target)) {
                & git -C $WorktreePath restore "--source=$ExpectedHead" --worktree -- $Target | Out-Null
            }
        }
    }
    catch {
        # Never conceal the original failure; any unexpected diff is left
        # untouched for diagnosis, rather than deleting user data.
    }
    throw
}
