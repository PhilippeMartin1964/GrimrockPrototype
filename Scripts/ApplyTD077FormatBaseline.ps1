param(
    [Parameter(Mandatory = $false)]
    [string]$Remote = "origin",

    [Parameter(Mandatory = $false)]
    [string]$BaseBranch = "master",

    [Parameter(Mandatory = $false)]
    [string]$ClangFormatPath
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$ExpectedBaselineViolationCount = 126
$RepositoryRoot = Split-Path -Parent $PSScriptRoot
$CheckScript = Join-Path $PSScriptRoot "CheckCppFormat.ps1"
$FormatScript = Join-Path $PSScriptRoot "FormatCpp.ps1"
$CurrentExe = (Get-Process -Id $PID).Path

function Invoke-FormatCheck
{
    $Arguments = @(
        "-NoProfile",
        "-ExecutionPolicy", "Bypass",
        "-File", $CheckScript
    )

    if (-not [string]::IsNullOrWhiteSpace($ClangFormatPath))
    {
        $Arguments += @("-ClangFormatPath", $ClangFormatPath)
    }

    $PreviousErrorActionPreference = $ErrorActionPreference
    try
    {
        $ErrorActionPreference = "Continue"
        $Output = (& $CurrentExe @Arguments 2>&1 | Out-String).TrimEnd()
        $ExitCode = $LASTEXITCODE
    }
    finally
    {
        $ErrorActionPreference = $PreviousErrorActionPreference
    }

    return [pscustomobject]@{
        ExitCode = $ExitCode
        Output   = $Output
    }
}

function Restore-FormattingChanges
{
    Write-Host ""
    Write-Host "Restoring tracked formatting changes..."
    git restore --worktree -- .
    if ($LASTEXITCODE -ne 0)
    {
        throw "Failed to restore tracked formatting changes."
    }
}

Push-Location $RepositoryRoot
try
{
    $CurrentBranch = (git rev-parse --abbrev-ref HEAD).Trim()
    if ($LASTEXITCODE -ne 0)
    {
        throw "Could not resolve current branch."
    }

    if ($CurrentBranch -ne $BaseBranch)
    {
        throw "TD07.7 format normalization must run from '$BaseBranch'. Current branch: '$CurrentBranch'."
    }

    $TrackedStatus = @(git status --porcelain --untracked-files=no)
    if ($TrackedStatus.Count -gt 0)
    {
        throw "Tracked working tree is not clean. TD07.7 format normalization aborted."
    }

    git fetch $Remote $BaseBranch
    if ($LASTEXITCODE -ne 0)
    {
        throw "git fetch $Remote $BaseBranch failed."
    }

    $Head = (git rev-parse HEAD).Trim()
    $RemoteHead = (git rev-parse "$Remote/$BaseBranch").Trim()
    if ($Head -ne $RemoteHead)
    {
        throw "Local HEAD ($Head) is not equal to $Remote/$BaseBranch ($RemoteHead). Pull/push before TD07.7 formatting."
    }

    Write-Host "=== TD07.7 pre-format baseline ==="
    $Before = Invoke-FormatCheck
    Write-Host $Before.Output

    if ($Before.ExitCode -ne 1)
    {
        throw "Expected pre-format CheckCppFormat exit code 1, got $($Before.ExitCode)."
    }

    $ExpectedFiles = @(
        $Before.Output -split "\r?\n" |
            Where-Object { $_ -like "[FORMAT]*" } |
            ForEach-Object { $_.Substring(9).Trim().Replace("\", "/") } |
            Sort-Object -Unique
    )

    if ($ExpectedFiles.Count -ne $ExpectedBaselineViolationCount)
    {
        throw "TD07.7 baseline changed. Expected $ExpectedBaselineViolationCount format violations, found $($ExpectedFiles.Count)."
    }

    if ("Source/GrimrockPrototype/Private/Runtime/GridActivationComponent.cpp" -notin $ExpectedFiles)
    {
        throw "TD07.7 expected GridActivationComponent.cpp in the characterized format baseline."
    }

    Write-Host ""
    Write-Host "=== TD07.7 formatting $($ExpectedFiles.Count) characterized files ==="

    $FormatArguments = @()
    if (-not [string]::IsNullOrWhiteSpace($ClangFormatPath))
    {
        $FormatArguments += @("-ClangFormatPath", $ClangFormatPath)
    }

    & $FormatScript @FormatArguments
    if ($LASTEXITCODE -ne 0)
    {
        Restore-FormattingChanges
        throw "FormatCpp.ps1 failed."
    }

    $ChangedFiles = @(
        git diff --name-only |
            ForEach-Object { $_.Trim().Replace("\", "/") } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
            Sort-Object -Unique
    )

    $UnexpectedChanged = @($ChangedFiles | Where-Object { $_ -notin $ExpectedFiles })
    $ExpectedButUnchanged = @($ExpectedFiles | Where-Object { $_ -notin $ChangedFiles })

    if ($UnexpectedChanged.Count -gt 0 -or $ExpectedButUnchanged.Count -gt 0)
    {
        Write-Host ""
        Write-Host "Unexpected changed files:"
        $UnexpectedChanged | ForEach-Object { Write-Host "  $_" }
        Write-Host "Expected files not changed:"
        $ExpectedButUnchanged | ForEach-Object { Write-Host "  $_" }
        Restore-FormattingChanges
        throw "Formatted diff does not match the characterized TD07.7 baseline."
    }

    if ($ChangedFiles.Count -ne $ExpectedBaselineViolationCount)
    {
        Restore-FormattingChanges
        throw "Expected exactly $ExpectedBaselineViolationCount changed files after formatting, found $($ChangedFiles.Count)."
    }

    Write-Host ""
    Write-Host "=== TD07.7 post-format verification ==="
    $After = Invoke-FormatCheck
    Write-Host $After.Output
    if ($After.ExitCode -ne 0)
    {
        Restore-FormattingChanges
        throw "Global C++ format baseline is still not green after FormatCpp.ps1."
    }

    git diff --check
    if ($LASTEXITCODE -ne 0)
    {
        Restore-FormattingChanges
        throw "git diff --check failed after TD07.7 formatting."
    }

    Write-Host ""
    Write-Host "Formatting diff validated: $($ChangedFiles.Count) characterized files only."

    git add -- $ChangedFiles
    if ($LASTEXITCODE -ne 0)
    {
        Restore-FormattingChanges
        throw "git add failed."
    }

    git commit -m "Normalize TD07.7 first-party C++ formatting"
    if ($LASTEXITCODE -ne 0)
    {
        git reset
        Restore-FormattingChanges
        throw "git commit failed."
    }

    $Commit = (git rev-parse HEAD).Trim()

    git push $Remote $BaseBranch
    if ($LASTEXITCODE -ne 0)
    {
        throw "Formatting commit $Commit was created locally but push failed."
    }

    Write-Host ""
    Write-Host "[OK] TD07.7 first-party C++ formatting normalized."
    Write-Host "Commit: $Commit"
    Write-Host "Files : $($ChangedFiles.Count)"
}
finally
{
    Pop-Location
}
