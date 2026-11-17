<#
ARM-CLEAN06.4X.37: one-shot, OFFLINE native Unreal Editor transaction.
No scheduled validation task calls this script. Default = read-only preflight.
-Apply requires an explicit operator invocation and creates 24 verified
SHA-256 backups outside the repository before any UE SavePackage.
On failure it restores ONLY the two allowlisted catalogues.
This script intentionally does not git commit or push binary changes.
#>
# Important: a successful Windows validation in the isolated worktree
# does NOT install the commandlet DLL in the user's primary checkout.
# The mandatory local UBT rebuild below makes the class discoverable.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$RepoRoot,
    [Parameter(Mandatory=$true)][string]$EngineRoot,
    [Parameter(Mandatory=$true)][string]$StateRoot,
    [Parameter(Mandatory=$true)][string]$ExpectedHead,
    [switch]$Apply
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if ($ExpectedHead -cnotmatch '^[a-f0-9]{40}$') { throw 'ExpectedHead must be exact SHA40.' }
$RepoRoot = [IO.Path]::GetFullPath($RepoRoot).TrimEnd('\','/')
$StateRoot = [IO.Path]::GetFullPath($StateRoot).TrimEnd('\','/')
$EngineRoot = [IO.Path]::GetFullPath($EngineRoot).TrimEnd('\','/')
if ($StateRoot.StartsWith(($RepoRoot+'\'), [StringComparison]::OrdinalIgnoreCase) -or
    $RepoRoot.StartsWith(($StateRoot+'\'), [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Protected StateRoot must be separate from the repository.'
}
$Project = Join-Path $RepoRoot 'GrimrockPrototype.uproject'
$EditorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$BuildBatch = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
if (-not (Test-Path -LiteralPath $Project -PathType Leaf) -or
    -not (Test-Path -LiteralPath $EditorCmd -PathType Leaf) -or
    -not (Test-Path -LiteralPath $BuildBatch -PathType Leaf)) {
    throw 'Exact project or installed UE5.5.4 editor/build executables missing.'
}
$ActualHead = @(& git -C $RepoRoot rev-parse HEAD)
if ($LASTEXITCODE -ne 0 -or $ActualHead.Count -ne 1 -or
    ([string]$ActualHead[0]).Trim() -cne $ExpectedHead) {
    throw 'HEAD mismatch; do not touch native assets.'
}
function Assert-Clean {
    $Branch = @(& git -C $RepoRoot branch --show-current)
    if ($LASTEXITCODE -ne 0 -or $Branch.Count -ne 1 -or
        ([string]$Branch[0]).Trim() -cne 'master') {
        throw 'Only the checked-out master branch may materialize binary assets.'
    }
    $Remote = @(& git -C $RepoRoot rev-parse refs/remotes/origin/master)
    if ($LASTEXITCODE -ne 0 -or $Remote.Count -ne 1 -or
        ([string]$Remote[0]).Trim() -cne $ExpectedHead) {
        throw 'Remote master has diverged or is not fetched at the pinned SHA.'
    }
    $Status = @(& git -C $RepoRoot status --porcelain --untracked-files=all)
    if ($LASTEXITCODE -ne 0 -or $Status.Count -ne 0) {
        throw 'Worktree must be clean; no user changes may be overwritten.'
    }
}
function Sha([string]$Path) {
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}
function Unreal([string]$Mode, [string[]]$Extra, [switch]$AllowProfileMismatch) {
    $Args = @($Project, '-run=GridArmClean064X37NativeArmWorldProfiles',
        ('-'+$Mode), '-NullRHI', '-Unattended', '-NoSplash', '-NoSound', '-nop4')
    $Args += $Extra
    & $EditorCmd @Args
    $script:LastNativeExitCode = [int]$LASTEXITCODE
    # Verify code 4 denotes real profile differences ONLY: BuildPlan
    # failures already returned code 3, and all other exits are fatal.
    # Never grant this exception to Preflight or Apply.
    if ($script:LastNativeExitCode -ne 0 -and
        -not ($AllowProfileMismatch -and $Mode -ceq 'Verify' -and
            $script:LastNativeExitCode -eq 4)) {
        throw ('UE '+$Mode+' exited with '+$script:LastNativeExitCode)
    }
}
# The scheduled Windows validator compiles in its OWN detached worktree.
# It never refreshes the user's primary project's native Editor module.
# Commandlets introduced in X.37 therefore require this mandatory local
# build BEFORE trying -run=..., even when the remote CI checks are green.
# A UBT failure stops safely before backup/snapshot and SavePackage.
Assert-Clean
Write-Host '[ARM-CLEAN06.4X.37] Building the LOCAL GrimrockPrototypeEditor before commandlet preflight.'
$BuildArgs = @('GrimrockPrototypeEditor', 'Win64', 'Development',
    "-Project=$Project", '-WaitMutex', '-NoHotReloadFromIDE')
& $BuildBatch @BuildArgs
if ($LASTEXITCODE -ne 0) {
    throw ('UE local Editor build failed with code ' + $LASTEXITCODE +
        '. No preflight, backups, or native migration performed.')
}
Assert-Clean
Unreal 'Preflight' @()
Assert-Clean
if (-not $Apply) {
    Write-Host 'X.37: read-only UE preflight succeeded; no package written.'
    return
}

# X.38: real data may already be in exact parity. A second independent
# Unreal process performs read-only Verify BEFORE any snapshot or Apply.
# Exit 0 = zero mismatch -> no write, no snapshot and no LFS commit.
# Exit 4 = differences exist -> continue with the safeguarded 24 backups.
# Any other exit code aborts before an attempt to modify packages.
Unreal 'Verify' @() -AllowProfileMismatch
if ($script:LastNativeExitCode -eq 0) {
    Assert-Clean
    Write-Host '[ARM-CLEAN06.4X.38] VERIFIED NO-OP: all 22 native ARM profiles already match their 8 visual + 2 physics properties.'
    Write-Host 'Two real catalogues are unchanged; no SavePackage, rollback, LFS commit or migration needed for these 220 fields.'
    Write-Host '06.4 remains PARTIAL; other X.19 legacy references, runtime readers and SaveGames are NOT converted.'
    return
}
Write-Host '[ARM-CLEAN06.4X.38] Verified differences exist; protected native Apply and 24 SHA256 backups are required.'

$Suffixes = @(
 'ArcaneOrb_01','ArcaneWand_01','AssassinDagger_01','Dagger_01',
 'Dart_01','ElementalStaff_01','FireWand_01','GreatAxe_01',
 'HolyAegis_01','HolyScepter_01','Javelin_01','LongSword_01',
 'Mace_01','MainGauche_01','RecurveBow_01','RoundShield_01',
 'ShortBow_01','ShortSpear_01','Stiletto_01','StraightSword_01',
 'ThrowingKnife_01','WarPick_01'
)
if ($Suffixes.Count -ne 22) { throw 'Unexpected source count.' }
$SourceFiles = @($Suffixes | ForEach-Object {
    'Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_'+$_+'.uasset'
})
$Targets = @(
 'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_CanonicalWeaponVisualCatalogue.uasset',
 'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_CanonicalWeaponCatalogue.uasset'
)
$All = @($SourceFiles) + @($Targets)
if ($All.Count -ne 24 -or @($All | Select-Object -Unique).Count -ne 24) {
    throw 'Unexpected file allowlist.'
}
$Lfs = @(& git -C $RepoRoot lfs ls-files -n)
if ($LASTEXITCODE -ne 0) { throw 'Git LFS required.' }
foreach ($P in $Targets) {
    if ($Lfs -notcontains $P) { throw "Target is not tracked in Git LFS: $P" }
}
$Snapshot = Join-Path $StateRoot ('ARM_CLEAN06_4X37\'+$ExpectedHead)
if (Test-Path -LiteralPath $Snapshot) {
    throw 'Private snapshot already exists; refuse destructive overwrite.'
}
New-Item -ItemType Directory -Path $Snapshot -Force | Out-Null
$Items = New-Object System.Collections.ArrayList
foreach ($Rel in $All) {
    $SourceFile = Join-Path $RepoRoot $Rel
    if (-not (Test-Path -LiteralPath $SourceFile -PathType Leaf)) {
        throw "Protected source missing: $Rel"
    }
    $BackupFile = Join-Path $Snapshot $Rel
    New-Item -ItemType Directory -Path (Split-Path -Parent $BackupFile) -Force | Out-Null
    $Before = Sha $SourceFile
    Copy-Item -LiteralPath $SourceFile -Destination $BackupFile -ErrorAction Stop
    if ((Sha $BackupFile) -cne $Before -or (Sha $SourceFile) -cne $Before) {
        throw 'Backup hash mismatch; no UE Apply may run.'
    }
    [void]$Items.Add([ordered]@{ path=$Rel; sha256=$Before })
}
$Manifest = Join-Path $Snapshot 'approved-backup-manifest.json'
$Receipt = [ordered]@{
    schema='ARM-CLEAN06.4X.37/native-backup/v1'
    head=$ExpectedHead
    source_count=22
    output_count=2
    verified=$true
    files=@($Items.ToArray())
}
($Receipt | ConvertTo-Json -Depth 5) |
    Set-Content -LiteralPath $Manifest -Encoding UTF8
if (-not (Test-Path -LiteralPath $Manifest)) { throw 'Backup receipt missing.' }
function Restore-Targets {
    foreach ($Rel in $Targets) {
        $BackupFile = Join-Path $Snapshot $Rel
        $LiveFile = Join-Path $RepoRoot $Rel
        Copy-Item -LiteralPath $BackupFile -Destination $LiveFile -Force
        $Previous = @($Items | Where-Object { $_.path -ceq $Rel })[0]
        if ((Sha $LiveFile) -cne $Previous.sha256) {
            throw "Rollback checksum failure: $Rel"
        }
    }
}
try {
    Assert-Clean
    Unreal 'Apply' @('-BackupManifest='+$Manifest, '-ExpectedHead='+$ExpectedHead)
    Unreal 'Verify' @()  # a second, independent Unreal Editor process
    foreach ($Rel in $SourceFiles) {
        $Previous = @($Items | Where-Object { $_.path -ceq $Rel })[0]
        if ((Sha (Join-Path $RepoRoot $Rel)) -cne $Previous.sha256) {
            throw "Historical source changed unexpectedly: $Rel"
        }
    }
    $Changed = @(& git -C $RepoRoot status --porcelain --untracked-files=all)
    if ($LASTEXITCODE -ne 0) { throw 'Git status unavailable after native write.' }
    foreach ($Line in $Changed) {
        $Rel = $Line.Substring(3).Replace('\','/')
        if ($Targets -notcontains $Rel) {
            throw "Unexpected package or user file changed: $Rel"
        }
    }
    Write-Host ('X.37 native data parities verified; snapshot: '+$Snapshot)
    Write-Host 'Only two allowlisted LFS catalogues may now be staged in a SEPARATE atomic binary commit.'
    Write-Host 'No automatic git add, commit, push, migration of level/palette or SaveGame occurred.'
}
catch {
    Write-Warning ('X.37 failed; restoring BOTH native target catalogues: '+$_.Exception.Message)
    Restore-Targets
    throw
}
