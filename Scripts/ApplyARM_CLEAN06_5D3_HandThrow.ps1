# One-shot transaction in the dedicated validation worktree. Never changes the principal checkout.
param(
    [Parameter(Mandatory=$true)][string]$WorktreePath,
    [Parameter(Mandatory=$true)][string]$RepositoryRoot,
    [Parameter(Mandatory=$true)][string]$EngineRoot,
    [Parameter(Mandatory=$true)][string]$StateRoot,
    [Parameter(Mandatory=$true)][string]$RunDir,
    [Parameter(Mandatory=$true)][string]$ExpectedHead,
    [Parameter(Mandatory=$true)][string]$GitHubRepository
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'ARM_CLEAN06_4X66_Rollback.ps1')
$StagePath = Join-Path $RunDir 'ArmClean065D3.stage'
$ExpectedParent = '0be0a8ddd471d74a2a70e05fff47df0a5782b7f0'
$ExpectedSubject = 'fix(ARM-CLEAN06.5D.3): preserve explicit physical throw permission for passive gear'
$FailedParent = 'e7e4f11d7c9dd6ea622e03c71dafd038f5e4f11a'
$BinarySubject = 'ARM-CLEAN06.5D.3B: persist 19 ordinary ARM flight policies'
$Packages = @(
    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_CanonicalWeaponCatalogue.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_CanonicalWeaponVisualCatalogue.uasset'
)
$CodePaths = @(
    'Source/GrimrockPrototype/Public/Runtime/Items/GridCanonicalItemCatalogues.h',
    'Source/GrimrockPrototype/Private/Runtime/Items/GridCanonicalItemCatalogues.cpp',
    'Source/GrimrockPrototype/Private/Runtime/GridCanonicalThrowBallisticsReadModel.h',
    'Source/GrimrockPrototype/Private/Runtime/GridPhysicalThrowStrengthReadModel.h',
    'Source/GrimrockPrototype/Private/Runtime/GridCanonicalProjectileVisualReadModel.h',
    'Source/GrimrockPrototype/Private/Runtime/GridLevelRuntimeActorWorldItems.cpp',
    'Source/GrimrockPrototype/Private/Runtime/GridThrownItemActor.cpp',
    'docs/Architecture/ARM_CLEAN06_CURRENT_HANDOFF.md',
    'Source/GrimrockPrototype/Private/Tests/GridARMCLEAN063AVCanonicalSpecializedThrowTests.cpp',
    'Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridEditorArmHandThrowPlan.h',
    'Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridArmClean065D3HandThrowCommandlet.h',
    'Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridArmClean065D3HandThrowCommandlet.cpp',
    'Source/GrimrockPrototypeEditor/Private/Tests/GridEditorARMCLEAN065D3HandThrowTests.cpp',
    'docs/Architecture/ARM_CLEAN06_5D3_ARM_HAND_THROW.md',
    'Scripts/ApplyARM_CLEAN06_5D3_HandThrow.ps1',
    'Scripts/UEValidationAgent.ps1'
)
function Set-Stage([string]$Value) {
    Set-Content -LiteralPath $StagePath -Value $Value -Encoding ASCII
}
function Invoke-HandThrowGit([string[]]$Arguments) {
    $Output = @(& git.exe -C $WorktreePath @Arguments)
    if ($LASTEXITCODE -ne 0) { throw 'Native hand-throw git command failed.' }
    return $Output
}
function Assert-SameSet([string[]]$Actual,[string[]]$Expected) {
    if ($Actual.Count -ne $Expected.Count -or
        (@(Compare-Object -ReferenceObject $Expected -DifferenceObject $Actual -CaseSensitive).Count -ne 0)) {
        throw 'Unexpected exact file set.'
    }
}
function Assert-Head {
    if ((Invoke-HandThrowGit @('rev-parse','HEAD')) -cne $ExpectedHead) { throw 'Unexpected HEAD.' }
}
function Assert-Remote {
    [void](Invoke-HandThrowGit @('fetch','--no-tags','origin','master'))
    if ((Invoke-HandThrowGit @('rev-parse','refs/remotes/origin/master')) -cne $ExpectedHead) {
        throw 'Remote master moved; transaction refused.'
    }
}
function Get-Dirty {
    $Rows = @(Invoke-HandThrowGit @('status','--porcelain=v1','--untracked-files=all'))
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
    if ($Value.Contains('"') -or $Value.Contains([char]10) -or $Value.Contains([char]13)) {
        throw 'Unsafe process argument.'
    }
    return '"' + $Value + '"'
}
function Invoke-Child([string]$Executable,[string[]]$Arguments,[string]$Name) {
    $ArgLine = ($Arguments | ForEach-Object { Quote-Arg $_ }) -join ' '
    $P = Start-Process -FilePath $Executable -ArgumentList $ArgLine -WorkingDirectory $WorktreePath -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput (Join-Path $Private ($Name+'.stdout.log')) -RedirectStandardError (Join-Path $Private ($Name+'.stderr.log')) -ErrorAction Stop
    return $P.ExitCode
}
function Invoke-Probe([string]$Mode,[int]$ExitExpected,[string]$Name,[string[]]$Extra = @()) {
    $Json = Join-Path $Private ($Name+'.json')
    $User = Join-Path $Private ($Name+'-User')
    $Log = Join-Path $Private ($Name+'.log')
    $Args = @($Project,'-run=GridArmClean065D3HandThrow',('-'+$Mode),('-Output='+$Json),('-UserDir='+$User),('-abslog='+$Log),'-Unattended','-NoP4','-NoSplash','-NoSound','-NullRHI') + $Extra
    $Exit = Invoke-Child $Editor $Args $Name
    if ($Exit -ne $ExitExpected -or -not (Test-Path -LiteralPath $Json -PathType Leaf)) {
        throw 'Unexpected native probe result.'
    }
    $Report = Get-Content -LiteralPath $Json -Raw | ConvertFrom-Json
    if ($Report.schema -cne 'ARM-CLEAN06.5D3/arm-hand-throw/v1' -or $Report.ordinary -ne 19 -or
        $Report.specialized -ne 3 -or $Report.physically_throwable -le 0 -or
        $Report.savegames_touched -ne $false -or [string]::IsNullOrWhiteSpace($Report.normalized_manifest)) {
        throw 'Native report rejected.'
    }
    return $Report
}
$Committed = $false
$Writing = $false
$Candidate = @{}
$Before = @{}
$Backup = @{}
try {
    Set-Stage 'PREFLIGHT'
    if ($GitHubRepository -cne 'PhilippeMartin1964/GrimrockPrototype' -or $ExpectedHead -cnotmatch '^[0-9a-f]{40}$') { throw 'Unexpected repository or revision.' }
    $WorktreePath = [IO.Path]::GetFullPath($WorktreePath)
    $RepositoryRoot = [IO.Path]::GetFullPath($RepositoryRoot)
    $RunDir = [IO.Path]::GetFullPath($RunDir)
    $RunRoot = [IO.Path]::GetFullPath((Join-Path $StateRoot 'Runs')).TrimEnd('\') + '\'
    if ((Split-Path -Leaf $WorktreePath) -cne 'GrimrockPrototype_UEValidation' -or
        $WorktreePath -ieq $RepositoryRoot -or -not $RunDir.StartsWith($RunRoot,[StringComparison]::OrdinalIgnoreCase) -or
        -not (Split-Path -Leaf $RunDir).StartsWith($ExpectedHead.Substring(0,12)+'-')) { throw 'Unexpected transaction location.' }
    foreach ($P in @($WorktreePath,$RepositoryRoot,$RunDir)) { Assert-ArmX66OrdinaryPath $P }
    if ((Invoke-HandThrowGit @('rev-parse','--abbrev-ref','HEAD')) -cne 'HEAD') { throw 'Detached validation worktree required.' }
    $Registered = @(& git.exe -C $RepositoryRoot worktree list --porcelain)
    if ($LASTEXITCODE -ne 0 -or ('worktree '+$WorktreePath.Replace('\','/')) -cnotin $Registered) { throw 'Worktree not registered.' }
    Assert-Head
    Assert-Remote
    if ((Invoke-HandThrowGit @('rev-parse','HEAD^')) -cne $FailedParent -or (Invoke-HandThrowGit @('rev-parse','HEAD^^')) -cne $ExpectedParent -or (Invoke-HandThrowGit @('log','-1','--format=%s')) -cne $ExpectedSubject) { throw 'Wrong one-shot revision.' }
    Assert-SameSet @(Invoke-HandThrowGit @('diff','--name-only',$ExpectedParent,'HEAD')) $CodePaths
    if (@(Get-Dirty).Count -ne 0) { throw 'Worktree must be clean.' }
    $StatusText = @(& gh.exe api ('repos/'+$GitHubRepository+'/commits/'+$ExpectedHead+'/status'))
    if ($LASTEXITCODE -ne 0) { throw 'Cannot read validation status.' }
    $Status = ($StatusText -join '') | ConvertFrom-Json
    foreach ($Context in @('UE5.5.4/SilentTask','UE5.5.4/LocalAuto')) {
        if (@($Status.statuses | Where-Object { $_.context -ceq $Context -and $_.state -ceq 'success' }).Count -ne 1) { throw 'Exact code validation required.' }
    }
    $Private = Join-Path $RunDir 'ARM-CLEAN06.5D3'
    if (Test-Path -LiteralPath $Private) { throw 'Private transaction already exists.' }
    [void](New-Item -ItemType Directory -Path $Private)
    $Project = Join-Path $WorktreePath 'GrimrockPrototype.uproject'
    $Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
    $Validator = Join-Path $WorktreePath 'Scripts\ValidateUE.ps1'
    $Ps = Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe'
    foreach ($P in @($Project,$Editor,$Validator,$Ps)) { if (-not (Test-Path -LiteralPath $P -PathType Leaf)) { throw 'Required executable or file absent.' } }
    foreach ($P in $Packages) {
        $Live = Join-Path $WorktreePath $P
        $Before[$P] = Get-ArmX66FileState $Live
        if ($Before[$P] -eq 'ABSENT') { throw 'Native source absent.' }
        Assert-Pointer @(Invoke-HandThrowGit @('show',($ExpectedHead+':'+$P))) $Before[$P] (Get-Item -LiteralPath $Live).Length
        $Backup[$P] = Join-Path $Private ((Split-Path -Leaf $P)+'.before')
        Copy-Item -LiteralPath $Live -Destination $Backup[$P]
        if ((Get-ArmX66FileState $Backup[$P]) -cne $Before[$P]) { throw 'Backup mismatch.' }
    }
    $Before | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $Private 'before.json') -Encoding UTF8
    Set-Stage 'AUDIT'
    $Audit = Invoke-Probe 'Audit' 0 'audit'
    $Prior = Invoke-Probe 'Verify' 4 'prior'
    if ($Audit.authored -ne 0 -or $Prior.authored -ne 0 -or $Audit.packages_saved -ne 0 -or $Prior.packages_saved -ne 0 -or
        $Audit.normalized_manifest -cne $Prior.normalized_manifest) { throw 'Source projection mismatch.' }
    foreach ($P in $Packages) { if ((Get-ArmX66FileState (Join-Path $WorktreePath $P)) -cne $Before[$P]) { throw 'Read-only probe changed native bytes.' } }
    Assert-Head
    Assert-Remote
    if (@(Get-Dirty).Count -ne 0) { throw 'Dirty before apply.' }
    Set-Stage 'APPLY'
    $Writing = $true
    try {
        $Applied = Invoke-Probe 'Apply' 0 'apply' @(
            ('-ExpectedHead='+$ExpectedHead),('-Backup0='+$Backup[$Packages[0]]),('-Backup1='+$Backup[$Packages[1]])
    } finally {
        Assert-Head
        [void]@(Get-Dirty)
        foreach ($P in $Packages) { $Candidate[$P] = Get-ArmX66FileState (Join-Path $WorktreePath $P) }
        $Candidate | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $Private 'candidate.json') -Encoding UTF8
    }
    if ($Applied.authored -ne 19 -or $Applied.packages_saved -ne 2 -or $Applied.normalized_manifest -cne $Audit.normalized_manifest) { throw 'Apply projection mismatch.' }
    foreach ($P in $Packages) { if ($Candidate[$P] -eq 'ABSENT' -or $Candidate[$P] -ceq $Before[$P]) { throw 'Expected two changed binary packages.' } }
    Set-Stage 'REOPEN'
    $Reopened = Invoke-Probe 'Verify' 0 'reopened'
    if ($Reopened.authored -ne 19 -or $Reopened.packages_saved -ne 0 -or $Reopened.normalized_manifest -cne $Audit.normalized_manifest) { throw 'Reopened projection mismatch.' }
    Set-Stage 'AUTOMATION'
    $ReportRoot = Join-Path $Private 'PostAutomation'
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
    [void](Invoke-HandThrowGit (@('add','--')+$Packages))
    Assert-SameSet @(Invoke-HandThrowGit @('diff','--cached','--name-only')) $Packages
    foreach ($P in $Packages) {
        Assert-Pointer @(Invoke-HandThrowGit @('show',(':'+$P))) $Candidate[$P] (Get-Item -LiteralPath (Join-Path $WorktreePath $P)).Length
    }
    Assert-Remote
    [void](Invoke-HandThrowGit @('commit','-m',$BinarySubject))
    $Committed = $true
    $New = [string](Invoke-HandThrowGit @('rev-parse','HEAD'))
    if ((Invoke-HandThrowGit @('rev-parse','HEAD^')) -cne $ExpectedHead -or @(Get-Dirty).Count -ne 0) { throw 'Unexpected binary child.' }
    [void](Invoke-HandThrowGit @('push','origin','HEAD:refs/heads/master'))
    [void](Invoke-HandThrowGit @('fetch','--no-tags','origin','master'))
    if ((Invoke-HandThrowGit @('rev-parse','refs/remotes/origin/master')) -cne $New) { throw 'Binary child not confirmed on remote.' }
    Set-Content -LiteralPath (Join-Path $Private 'binary.sha') -Value $New -Encoding ASCII
    Set-Stage 'SUCCESS'
} catch {
    if ($Writing -and -not $Committed) {
        try {
            Assert-Head
            [void]@(Get-Dirty)
            foreach ($P in @(Invoke-HandThrowGit @('diff','--cached','--name-only'))) {
                if ($P -cnotin $Packages -or -not $Candidate.ContainsKey($P)) { throw 'Unknown staged candidate.' }
                Assert-Pointer @(Invoke-HandThrowGit @('show',(':'+$P))) $Candidate[$P] (Get-Item -LiteralPath (Join-Path $WorktreePath $P)).Length
                [void](Invoke-HandThrowGit @('restore','--staged','--',$P))
            }
            foreach ($P in $Packages) {
                $After = if ($Candidate.ContainsKey($P)) { $Candidate[$P] } else { '' }
                Restore-ArmX66NativeFile -Live (Join-Path $WorktreePath $P) -Before $Before[$P] -After $After -Backup $Backup[$P] -Archive (Join-Path $Private ((Split-Path -Leaf $P)+'.rejected'))
            }
            if (@(Get-Dirty).Count -ne 0) { throw 'Rollback did not restore clean state.' }
        } catch {
            Set-Stage 'ROLLBACK_INCOMPLETE'
            throw
        }
    }
    throw
}
