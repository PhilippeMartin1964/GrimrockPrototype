# One-shot transaction in the dedicated validation worktree. Never changes the principal checkout.
[CmdletBinding()]
param(
    [string]$WorktreePath,
    [string]$RepositoryRoot,
    [string]$EngineRoot,
    [string]$StateRoot,
    [string]$RunDir,
    [string]$ExpectedHead,
    [string]$GitHubRepository,
    [switch]$SelfTest
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'ARM_CLEAN06_4X66_Rollback.ps1')
if ([string]::IsNullOrWhiteSpace($RunDir) -or -not (Test-Path -LiteralPath $RunDir -PathType Container)) {
    throw 'An existing private run directory is required.'
}
Assert-ArmX66OrdinaryPath $RunDir
$StagePath = Join-Path $RunDir 'ArmClean065G71.stage'
if ($SelfTest) { Set-Content -LiteralPath $StagePath -Value 'SELFTEST_INIT' -Encoding ASCII }
$GitApplication = (Get-Command git.exe -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source
$ExpectedParent = '1d7e42380d92e2dc97fcf17010c14bc41c49ac4c'
$ExpectedSubject = 'ARM-CLEAN06.5G.71: guarded native puzzle category transaction'
$BinarySubject = 'ARM-CLEAN06.5G.71B: materialize 26 native puzzle category tags'
$Packages = @(
    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/DA_Canonical_EmptyVial.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/DA_Canonical_Gem_Blue.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/DA_Canonical_GoblinKnife.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/DA_Canonical_Item_Antidote.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/DA_Canonical_Item_Arrows_01.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/DA_Canonical_Item_Bomb_Fire.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/DA_Canonical_Item_Bomb_Toxic.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/DA_Canonical_Item_DefensiveElixir_Fire.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/DA_Canonical_Item_DefensiveElixir_Ice.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/DA_Canonical_Item_DefensiveElixir_Lightning.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/DA_Canonical_Item_DefensiveElixir_Poison.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/DA_Canonical_Item_DisarmTools_01.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/DA_Canonical_Item_Flask_Acid.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/DA_Canonical_Item_Flask_CorrosiveCloud.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/DA_Canonical_Item_Flask_Oil.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/DA_Canonical_Item_Panacea.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/DA_Canonical_Item_Potion_Healing_01.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/DA_Canonical_Item_RatMeat.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/DA_Canonical_Item_RatTooth.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/DA_Canonical_Item_SurfaceCatalyst_01.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/DA_Canonical_Item_TestNote.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/DA_Canonical_Item_Torch.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/DA_Canonical_Key_Copper.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/DA_Canonical_Key_Iron.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/DA_Canonical_Shuriken.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/DA_Canonical_Stone.uasset'
)
$CodePaths = @(
    'Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridEditorPuzzleCategoryMigrationPlan.h',
    'Source/GrimrockPrototypeEditor/Private/Tests/GridEditorARMCLEAN065G69PuzzleCategoryTests.cpp',
    'Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridArmClean065G71PuzzleCategoryCommandlet.h',
    'Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridArmClean065G71PuzzleCategoryCommandlet.cpp',
    'Scripts/ApplyARM_CLEAN06_5G71_PuzzleCategories.ps1',
    'Scripts/UEValidationAgent.ps1',
    'docs/Architecture/ARM_CLEAN06_5G71_NATIVE_PUZZLE_CATEGORIES.md',
    'docs/Architecture/ARM_CLEAN06_CURRENT_HANDOFF.md'
)
function Set-Stage([string]$Value) {
    Set-Content -LiteralPath $StagePath -Value $Value -Encoding ASCII
}
function Invoke-PuzzleMigrationGit([string[]]$Arguments) {
    $Output = @(& $GitApplication -C $WorktreePath @Arguments)
    if ($LASTEXITCODE -ne 0) { throw 'Native puzzle migration git command failed.' }
    return $Output
}
function Normalize-PuzzleMigrationPath([string]$Path) {
    return [IO.Path]::GetFullPath($Path.Replace('/', '\')).TrimEnd('\')
}
function Assert-SameSet([string[]]$Actual,[string[]]$Expected) {
    if ($Actual.Count -ne $Expected.Count) { throw 'Unexpected exact file set.' }
    $Seen = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    foreach ($Entry in $Expected) {
        if (-not $Seen.Add($Entry)) { throw 'Duplicate expected entry.' }
    }
    foreach ($Entry in $Actual) {
        if (-not $Seen.Remove($Entry)) { throw 'Unexpected exact file set.' }
    }
    if ($Seen.Count -ne 0) { throw 'Missing exact file entry.' }
}
function Assert-Head {
    if ((Invoke-PuzzleMigrationGit @('rev-parse','HEAD')) -cne $ExpectedHead) { throw 'Unexpected HEAD.' }
}
function Assert-Remote {
    [void](Invoke-PuzzleMigrationGit @('fetch','--no-tags','origin','master'))
    if ((Invoke-PuzzleMigrationGit @('rev-parse','refs/remotes/origin/master')) -cne $ExpectedHead) {
        throw 'Remote master moved; transaction refused.'
    }
}
function Get-Dirty {
    $Rows = @(Invoke-PuzzleMigrationGit @('status','--porcelain=v1','--untracked-files=all'))
    foreach ($Row in $Rows) {
        if ($Row.Length -lt 4 -or $Row.Substring(0,2) -notin @(' M','M ','MM') -or
            $Row.Substring(3) -cnotin $Packages) { throw 'Unexpected dirty path.' }
    }
    return $Rows
}
function Assert-Pointer([string[]]$Lines,[string]$Hash,[long]$Bytes) {
    if ($Lines.Count -ne 3 -or $Lines[0] -cne 'version https://git-lfs.github.com/spec/v1' -or
        $Lines[1] -cne ('oid sha256:' + $Hash) -or $Lines[2] -cne ('size ' + $Bytes)) {
        throw 'Git LFS pointer does not match native bytes.'
    }
}
function Quote-Arg([string]$Value) {
    if ($Value -match '[\r\n"]') {
        throw 'Unsafe process argument.'
    }
    return '"' + $Value + '"'
}
function Invoke-Child([string]$Executable,[string[]]$Arguments,[string]$Name) {
    $ArgLine = ($Arguments | ForEach-Object { Quote-Arg $_ }) -join ' '
    $P = Start-Process -FilePath $Executable -ArgumentList $ArgLine -WorkingDirectory $WorktreePath -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput (Join-Path $EvidenceDir ($Name+'.stdout.log')) -RedirectStandardError (Join-Path $EvidenceDir ($Name+'.stderr.log')) -ErrorAction Stop
    return $P.ExitCode
}
function Invoke-Probe([string]$Mode,[int]$ExitExpected,[string]$Name,[string[]]$Extra = @()) {
    $Json = Join-Path $EvidenceDir ($Name+'.json')
    $User = Join-Path $EvidenceDir ($Name+'-User')
    $Log = Join-Path $EvidenceDir ($Name+'.log')
    $ProbeArguments = @($Project,'-run=GridArmClean065G71PuzzleCategory',('-'+$Mode),('-Output='+$Json),('-UserDir='+$User),('-abslog='+$Log),'-Unattended','-NoP4','-NoSplash','-NoSound','-NullRHI') + $Extra
    $Exit = Invoke-Child -Executable $Editor -Arguments $ProbeArguments -Name $Name
    if ($Exit -ne $ExitExpected -or -not (Test-Path -LiteralPath $Json -PathType Leaf)) {
        throw 'Unexpected native probe result.'
    }
    $Report = Get-Content -LiteralPath $Json -Raw | ConvertFrom-Json
    if ($Report.schema -cne 'ARM-CLEAN06.5G71/puzzle/v1' -or ($Report.tagged -notin @(0,26)) -or
        $Report.savegames_touched -ne $false -or [string]::IsNullOrWhiteSpace($Report.normalized_manifest)) {
        throw 'Native report rejected.'
    }
    return $Report
}
if ($SelfTest) {
    Set-Stage 'SELFTEST_GIT'
    $Repo = Split-Path -Parent $PSScriptRoot
    $WorktreePath = $Repo
    $LocalHead = @(Invoke-PuzzleMigrationGit -Arguments @('rev-parse','HEAD'))
    if ($LocalHead.Count -ne 1 -or $LocalHead[0] -cnotmatch '^[0-9a-f]{40}$') { throw 'Hand throw Git self-test failed.' }
    Set-Stage 'SELFTEST_SET'
    Assert-SameSet -Actual @('b','a') -Expected @('a','b')
    $Rejected = $false
    try { Assert-SameSet -Actual @('b','c') -Expected @('a','b') } catch { $Rejected = $true }
    if (-not $Rejected) { throw 'Mismatch self-test failed.' }
    Set-Stage 'SELFTEST_PATH'
    $Normalized = Normalize-PuzzleMigrationPath -Path $Repo
    if ((Normalize-PuzzleMigrationPath ($Repo.Replace('\','/')+'/')) -ine $Normalized) { throw 'Path normalization self-test failed.' }
    Set-Stage 'SELFTEST_POINTER'
    $DummyHash = 'a'*64
    Assert-Pointer -Lines @('version https://git-lfs.github.com/spec/v1',('oid sha256:'+$DummyHash),'size 42') -Hash $DummyHash -Bytes 42
    Set-Stage 'SELFTEST_QUOTE'
    if ((Quote-Arg 'a b') -cne '"a b"') { throw 'Argument self-test failed.' }
    Set-Stage 'SELFTEST_SUCCESS'
    Write-Host '[OK] ARM 5G71 transaction helper self-test (no native writes).'
    return
}
$Committed = $false
$Writing = $false
$Candidate = @{}
$Before = @{}
$Backup = @{}
try {
    if (-not (Test-Path -LiteralPath $StagePath -PathType Leaf) -or
        (Get-Content -LiteralPath $StagePath -TotalCount 1) -cne 'SELFTEST_SUCCESS') { throw 'Native self-test not acquired for this run.' }
    Set-Stage 'PREFLIGHT_INPUT'
    foreach ($Required in @($WorktreePath,$RepositoryRoot,$EngineRoot,$StateRoot,$ExpectedHead,$GitHubRepository)) {
        if ([string]::IsNullOrWhiteSpace($Required)) { throw 'Missing transaction argument.' }
    }
    if ((Get-Content -LiteralPath $StagePath -TotalCount 1) -cne 'PREFLIGHT_INPUT') { throw 'Stage write failed.' }
    if ($GitHubRepository -cne 'PhilippeMartin1964/GrimrockPrototype' -or $ExpectedHead -cnotmatch '^[0-9a-f]{40}$') { throw 'Unexpected repository or revision.' }
    Set-Stage 'PREFLIGHT_LOCATION'
    $WorktreePath = Normalize-PuzzleMigrationPath $WorktreePath
    $RepositoryRoot = Normalize-PuzzleMigrationPath $RepositoryRoot
    $RunDir = Normalize-PuzzleMigrationPath $RunDir
    $RunRoot = [IO.Path]::GetFullPath((Join-Path $StateRoot 'Runs')).TrimEnd('\') + '\'
    if ((Split-Path -Leaf $WorktreePath) -cne 'GrimrockPrototype_UEValidation' -or
        $WorktreePath -ieq $RepositoryRoot -or -not $RunDir.StartsWith($RunRoot,[StringComparison]::OrdinalIgnoreCase) -or
        -not (Split-Path -Leaf $RunDir).StartsWith($ExpectedHead.Substring(0,12)+'-')) { throw 'Unexpected transaction location.' }
    Set-Stage 'PREFLIGHT_ORDINARY_PATH'
    foreach ($P in @($WorktreePath,$RepositoryRoot,$RunDir)) { Assert-ArmX66OrdinaryPath $P }
    Set-Stage 'PREFLIGHT_WORKTREE'
    if ((Invoke-PuzzleMigrationGit @('rev-parse','--abbrev-ref','HEAD')) -cne 'HEAD') { throw 'Detached validation worktree required.' }
    $RegisteredRaw = @(& git.exe -C $RepositoryRoot worktree list --porcelain)
    if ($LASTEXITCODE -ne 0) { throw 'Worktree listing failed.' }
    $Registered = @($RegisteredRaw | Where-Object { $_ -like 'worktree *' } | ForEach-Object {
        Normalize-PuzzleMigrationPath ($_.Substring(9).Trim())
    })
    if (@($Registered | Where-Object { $_ -ieq $WorktreePath }).Count -ne 1 -or
        (Normalize-PuzzleMigrationPath (Invoke-PuzzleMigrationGit @('rev-parse','--show-toplevel'))) -ine $WorktreePath) { throw 'Worktree not registered.' }
    Set-Stage 'PREFLIGHT_HEAD'
    Assert-Head
    Set-Stage 'PREFLIGHT_REMOTE'
    Assert-Remote
    Set-Stage 'PREFLIGHT_ANCESTRY'
    if ((Invoke-PuzzleMigrationGit @('rev-parse','HEAD~1')) -cne $ExpectedParent -or
        (Invoke-PuzzleMigrationGit @('log','-1','--format=%s')) -cne $ExpectedSubject) { throw 'Wrong one-shot revision.' }
    Set-Stage 'PREFLIGHT_DIFF'
    Assert-SameSet @(Invoke-PuzzleMigrationGit @('diff','--name-only',$ExpectedParent,'HEAD')) $CodePaths
    Set-Stage 'PREFLIGHT_CLEAN'
    if (@(Get-Dirty).Count -ne 0) { throw 'Worktree must be clean.' }
    Set-Stage 'PREFLIGHT_VALIDATION'
    $StatusText = @(& gh.exe api ('repos/'+$GitHubRepository+'/commits/'+$ExpectedHead+'/status'))
    if ($LASTEXITCODE -ne 0) { throw 'Cannot read validation status.' }
    $Status = ($StatusText -join '') | ConvertFrom-Json
    foreach ($Context in @('UE5.5.4/SilentTask','UE5.5.4/LocalAuto')) {
        if (@($Status.statuses | Where-Object { $_.context -ceq $Context -and $_.state -ceq 'success' }).Count -ne 1) { throw 'Exact code validation required.' }
    }
    Set-Stage 'PREFLIGHT_SNAPSHOT'
    $EvidenceDir = Join-Path $RunDir 'ARM-CLEAN06.5G71'
    if (Test-Path -LiteralPath $EvidenceDir) { throw 'Private transaction already exists.' }
    [void](New-Item -ItemType Directory -Path $EvidenceDir)
    $Project = Join-Path $WorktreePath 'GrimrockPrototype.uproject'
    $Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
    $Validator = Join-Path $WorktreePath 'Scripts\ValidateUE.ps1'
    $Ps = Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe'
    Set-Stage 'PREFLIGHT_EXECUTABLES'
    foreach ($P in @($Project,$Editor,$Validator,$Ps)) { if (-not (Test-Path -LiteralPath $P -PathType Leaf)) { throw 'Required executable or file absent.' } }
    Set-Stage 'PREFLIGHT_LFS'
    foreach ($P in $Packages) {
        $Live = Join-Path $WorktreePath $P
        $Before[$P] = Get-ArmX66FileState $Live
        if ($Before[$P] -eq 'ABSENT') { throw 'Native source absent.' }
        Assert-Pointer @(Invoke-PuzzleMigrationGit @('show',($ExpectedHead+':'+$P))) $Before[$P] (Get-Item -LiteralPath $Live).Length
        $Backup[$P] = Join-Path $EvidenceDir ((Split-Path -Leaf $P)+'.before')
        Copy-Item -LiteralPath $Live -Destination $Backup[$P]
        if ((Get-ArmX66FileState $Backup[$P]) -cne $Before[$P]) { throw 'Backup mismatch.' }
    }
    $Before | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $EvidenceDir 'before.json') -Encoding UTF8
    Set-Stage 'AUDIT'
    $Audit = Invoke-Probe 'Audit' 0 'audit'
    $Prior = Invoke-Probe 'Verify' 4 'prior'
    if ($Audit.tagged -ne 0 -or $Prior.tagged -ne 0 -or
        $Audit.packages_saved -ne 0 -or $Prior.packages_saved -ne 0 -or
        $Audit.normalized_manifest -cne $Prior.normalized_manifest) { throw 'Source projection mismatch.' }
    foreach ($P in $Packages) { if ((Get-ArmX66FileState (Join-Path $WorktreePath $P)) -cne $Before[$P]) { throw 'Read-only probe changed native bytes.' } }
    Assert-Head
    Assert-Remote
    if (@(Get-Dirty).Count -ne 0) { throw 'Dirty before apply.' }
    Set-Stage 'APPLY'
    $Writing = $true
    try {
        $ApplyArguments = @(
            ('-ExpectedHead=' + $ExpectedHead)
            ('-BackupDir=' + $EvidenceDir)
        )
        $Applied = Invoke-Probe -Mode 'Apply' -ExitExpected 0 -Name 'apply' -Extra $ApplyArguments
    } finally {
        Assert-Head
        Get-Dirty | Out-Null
        foreach ($P in $Packages) { $Candidate[$P] = Get-ArmX66FileState (Join-Path $WorktreePath $P) }
        $Candidate | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $EvidenceDir 'candidate.json') -Encoding UTF8
    }
    if ($Applied.tagged -ne 26 -or $Applied.packages_saved -ne 26 -or $Applied.normalized_manifest -cne $Audit.normalized_manifest) { throw 'Apply projection mismatch.' }
    foreach ($P in $Packages) { if ($Candidate[$P] -eq 'ABSENT' -or $Candidate[$P] -ceq $Before[$P]) { throw 'Expected 26 changed native definitions.' } }
    Set-Stage 'REOPEN'
    $Reopened = Invoke-Probe 'Verify' 0 'reopened'
    if ($Reopened.tagged -ne 26 -or $Reopened.packages_saved -ne 0 -or $Reopened.normalized_manifest -cne $Audit.normalized_manifest) { throw 'Reopened projection mismatch.' }
    # Snapshot all 26 hashes after independent reopen, before Automation.
    foreach ($P in $Packages) {
        if ((Get-ArmX66FileState (Join-Path $WorktreePath $P)) -cne $Candidate[$P]) { throw 'Reopen changed native bytes.' }
    }
    Set-Stage 'AUTOMATION'
    $ReportRoot = Join-Path $EvidenceDir 'PostAutomation'
    $Exit = Invoke-Child $Ps @('-NoLogo','-NoProfile','-NonInteractive','-ExecutionPolicy','Bypass','-File',$Validator,'-EngineRoot',$EngineRoot,'-SkipBuild','-AutomationFilter','Grimrock','-ReportRoot',$ReportRoot) 'post-automation'
    if ($Exit -ne 0) { throw 'Post-write full automation failed.' }
    $Indices = @(Get-ChildItem -LiteralPath $ReportRoot -Recurse -File -Filter 'index.json')
    if ($Indices.Count -ne 1) { throw 'Ambiguous full automation report.' }
    $Result = Get-Content -LiteralPath $Indices[0].FullName -Raw | ConvertFrom-Json
    if ($Result.failed -ne 0 -or $Result.notRun -ne 0 -or ($Result.succeeded+$Result.succeededWithWarnings) -le 0) { throw 'Full automation incomplete.' }
    Set-Stage 'PUBLISH'
    Assert-Head
    Assert-Remote
    Assert-SameSet @((Get-Dirty) | ForEach-Object { $_.Substring(3) }) $Packages
    foreach ($P in $Packages) {
        if ((Get-ArmX66FileState (Join-Path $WorktreePath $P)) -cne $Candidate[$P]) { throw 'Candidate changed after verification.' }
    }
    [void](Invoke-PuzzleMigrationGit (@('add','--')+$Packages))
    Assert-SameSet @(Invoke-PuzzleMigrationGit @('diff','--cached','--name-only')) $Packages
    foreach ($P in $Packages) {
        Assert-Pointer @(Invoke-PuzzleMigrationGit @('show',(':'+$P))) $Candidate[$P] (Get-Item -LiteralPath (Join-Path $WorktreePath $P)).Length
    }
    Assert-Remote
    [void](Invoke-PuzzleMigrationGit @('commit','-m',$BinarySubject))
    $Committed = $true
    $New = [string](Invoke-PuzzleMigrationGit @('rev-parse','HEAD'))
    if ((Invoke-PuzzleMigrationGit @('rev-parse','HEAD^')) -cne $ExpectedHead -or @(Get-Dirty).Count -ne 0) { throw 'Unexpected binary child.' }
    [void](Invoke-PuzzleMigrationGit @('push','origin','HEAD:refs/heads/master'))
    [void](Invoke-PuzzleMigrationGit @('fetch','--no-tags','origin','master'))
    if ((Invoke-PuzzleMigrationGit @('rev-parse','refs/remotes/origin/master')) -cne $New) { throw 'Binary child not confirmed on remote.' }
    Set-Content -LiteralPath (Join-Path $EvidenceDir 'binary.sha') -Value $New -Encoding ASCII
    Set-Stage 'SUCCESS'
} catch {
    if ($Writing -and -not $Committed) {
        try {
            Assert-Head
            Get-Dirty | Out-Null
            foreach ($P in @(Invoke-PuzzleMigrationGit @('diff','--cached','--name-only'))) {
                if ($P -cnotin $Packages -or -not $Candidate.ContainsKey($P)) { throw 'Unknown staged candidate.' }
                Assert-Pointer @(Invoke-PuzzleMigrationGit @('show',(':'+$P))) $Candidate[$P] (Get-Item -LiteralPath (Join-Path $WorktreePath $P)).Length
                [void](Invoke-PuzzleMigrationGit @('restore','--staged','--',$P))
            }
            foreach ($P in $Packages) {
                $After = if ($Candidate.ContainsKey($P)) { $Candidate[$P] } else { '' }
                Restore-ArmX66NativeFile -Live (Join-Path $WorktreePath $P) -Before $Before[$P] -After $After -Backup $Backup[$P] -Archive (Join-Path $EvidenceDir ((Split-Path -Leaf $P)+'.rejected'))
            }
            if (@(Get-Dirty).Count -ne 0) { throw 'Rollback did not restore clean state.' }
        } catch {
            Set-Stage 'ROLLBACK_INCOMPLETE'
            throw
        }
    }
    throw
}
