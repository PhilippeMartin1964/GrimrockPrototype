param(
    [Parameter(Mandatory = $false)]
    [string]$EngineRoot = $env:UE_ROOT,

    [Parameter(Mandatory = $false)]
    [string]$Remote = "origin",

    [Parameter(Mandatory = $false)]
    [string]$BaseBranch = "master"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$RepositoryRoot = Split-Path -Parent $PSScriptRoot
$CurrentExe = (Get-Process -Id $PID).Path

$DependencyScript = Join-Path $PSScriptRoot "CheckProjectDependencies.ps1"
$FormatScript = Join-Path $PSScriptRoot "CheckCppFormat.ps1"
$ValidateUEScript = Join-Path $PSScriptRoot "ValidateUE.ps1"
$ValidatePackageScript = Join-Path $PSScriptRoot "ValidatePackage.ps1"

function Assert-FileExists
{
    param(
        [string]$Path,
        [string]$Description
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf))
    {
        throw "$Description introuvable : $Path"
    }
}

function Invoke-TD078PowerShellStep
{
    param(
        [string]$Label,
        [string]$ScriptPath,
        [string[]]$Arguments
    )

    Write-Host ""
    Write-Host "=== $Label ==="

    & $CurrentExe -NoProfile -ExecutionPolicy Bypass -File $ScriptPath @Arguments
    $ExitCode = $LASTEXITCODE
    if ($ExitCode -ne 0)
    {
        throw "$Label failed with exit code $ExitCode."
    }

    Write-Host "[OK] $Label"
}

foreach ($RequiredScript in @($DependencyScript, $FormatScript, $ValidateUEScript, $ValidatePackageScript))
{
    Assert-FileExists -Path $RequiredScript -Description "TD07.8 required validation script"
}

Push-Location $RepositoryRoot
try
{
    Write-Host "GrimrockPrototype TD07.8 - Future-Proofing Stop Condition"
    Write-Host "Repository : $RepositoryRoot"
    Write-Host "Engine     : $EngineRoot"
    Write-Host ""

    $CurrentBranch = (git rev-parse --abbrev-ref HEAD).Trim()
    if ($LASTEXITCODE -ne 0)
    {
        throw "Could not resolve current Git branch."
    }

    if ($CurrentBranch -ne $BaseBranch)
    {
        throw "TD07.8 must run from '$BaseBranch'. Current branch: '$CurrentBranch'."
    }

    $TrackedStatus = @(git status --porcelain --untracked-files=no)
    if ($TrackedStatus.Count -gt 0)
    {
        throw "Tracked working tree is not clean. TD07.8 aborted."
    }

    git fetch $Remote --prune
    if ($LASTEXITCODE -ne 0)
    {
        throw "git fetch $Remote --prune failed."
    }

    $Head = (git rev-parse HEAD).Trim()
    $RemoteHead = (git rev-parse "$Remote/$BaseBranch").Trim()
    if ($Head -ne $RemoteHead)
    {
        throw "Local HEAD ($Head) is not equal to $Remote/$BaseBranch ($RemoteHead)."
    }

    $RemoteRefs = @(
        git for-each-ref "--format=%(refname:short)" "refs/remotes/$Remote" |
            ForEach-Object { $_.Trim() } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    )

    $UnexpectedRemoteBranches = @(
        $RemoteRefs | Where-Object {
            $_ -ne $Remote -and
            $_ -ne "$Remote/HEAD" -and
            $_ -ne "$Remote/$BaseBranch"
        }
    )

    if ($UnexpectedRemoteBranches.Count -gt 0)
    {
        Write-Host "Unexpected remote branches:"
        $UnexpectedRemoteBranches | ForEach-Object { Write-Host "  $_" }
        throw "TD07.8 requires master to remain the sole active remote branch."
    }

    foreach ($RemovedOneShot in @(
        "Scripts/AuditTD077Hygiene.ps1",
        "Scripts/ApplyTD077FormatBaseline.ps1",
        "Scripts/FinalizeTD077Format.ps1"
    ))
    {
        if (Test-Path -LiteralPath (Join-Path $RepositoryRoot $RemovedOneShot))
        {
            throw "Obsolete TD07.7 one-shot script still exists: $RemovedOneShot"
        }
    }

    Write-Host "[OK] Git branch / cleanup contract validated."

    Invoke-TD078PowerShellStep -Label "TD07.1 project dependency contract" -ScriptPath $DependencyScript -Arguments @(
        "-EngineRoot", $EngineRoot
    )

    Invoke-TD078PowerShellStep -Label "TD07.7 global C++ format baseline" -ScriptPath $FormatScript -Arguments @()

    Invoke-TD078PowerShellStep -Label "Editor build + TD07.3 strict current schema" -ScriptPath $ValidateUEScript -Arguments @(
        "-EngineRoot", $EngineRoot,
        "-AutomationFilter", "Grimrock.TechnicalDebt.TD07_3_8.StrictCurrentSchema"
    )

    foreach ($Filter in @(
        "Grimrock.TechnicalDebt.TD07_2",
        "Grimrock.TechnicalDebt.TD07_4.Characterization",
        "Grimrock.TechnicalDebt.TD07_5.Recovery.ReceptacleCommands",
        "Grimrock.TechnicalDebt.TD01_3.EventCommandContract.RuntimeHardening",
        "Grimrock.Quests.MON21_4.Characterization"
    ))
    {
        Invoke-TD078PowerShellStep -Label "Automation $Filter" -ScriptPath $ValidateUEScript -Arguments @(
            "-EngineRoot", $EngineRoot,
            "-SkipBuild",
            "-AutomationFilter", $Filter
        )
    }

    Invoke-TD078PowerShellStep -Label "Win64 Shipping package" -ScriptPath $ValidatePackageScript -Arguments @(
        "-EngineRoot", $EngineRoot,
        "-Configuration", "Shipping"
    )

    Write-Host ""
    Write-Host "=== TD07.8 summary ==="
    Write-Host "Branch             : $BaseBranch only"
    Write-Host "Dependencies       : validated"
    Write-Host "C++ format         : validated"
    Write-Host "Strict schema      : validated"
    Write-Host "UE compatibility   : validated"
    Write-Host "Activation         : validated"
    Write-Host "Receptacle recovery: validated"
    Write-Host "Event->Command     : validated"
    Write-Host "MON21.4 assumptions: validated"
    Write-Host "Shipping           : validated"
    Write-Host ""
    Write-Host "[OK] TD07.8 future-proofing stop condition validated."
}
finally
{
    Pop-Location
}
