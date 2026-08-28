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

$RepositoryRoot = Split-Path -Parent $PSScriptRoot
$CheckScript = Join-Path $PSScriptRoot "CheckCppFormat.ps1"
$FormatScript = Join-Path $PSScriptRoot "FormatCpp.ps1"
$ExpectedFile = "Source/GrimrockPrototype/Private/Tests/GridTD074ActivationComponentCharacterizationTests.cpp"
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
        throw "TD07.7 final format fix must run from '$BaseBranch'. Current branch: '$CurrentBranch'."
    }

    $TrackedStatus = @(git status --porcelain --untracked-files=no)
    if ($TrackedStatus.Count -gt 0)
    {
        throw "Tracked working tree is not clean. TD07.7 final format fix aborted."
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
        throw "Local HEAD ($Head) is not equal to $Remote/$BaseBranch ($RemoteHead)."
    }

    Write-Host "=== TD07.7 final pre-format check ==="
    $Before = Invoke-FormatCheck
    Write-Host $Before.Output

    if ($Before.ExitCode -ne 1)
    {
        throw "Expected pre-format CheckCppFormat exit code 1, got $($Before.ExitCode)."
    }

    $InvalidFiles = @(
        $Before.Output -split "\r?\n" |
            Where-Object { $_.StartsWith("[FORMAT] ") } |
            ForEach-Object { $_.Substring(9).Trim().Replace("\", "/") } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
            Sort-Object -Unique
    )

    if ($InvalidFiles.Count -ne 1)
    {
        throw "Expected exactly one remaining format violation, found $($InvalidFiles.Count)."
    }

    if ($InvalidFiles[0] -ne $ExpectedFile)
    {
        throw "Unexpected remaining format violation: $($InvalidFiles[0]). Expected: $ExpectedFile"
    }

    Write-Host ""
    Write-Host "Confirmed single remaining violation: $ExpectedFile"
    Write-Host "Running authoritative FormatCpp.ps1..."

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

    if ($ChangedFiles.Count -ne 1 -or $ChangedFiles[0] -ne $ExpectedFile)
    {
        Write-Host "Changed files:"
        $ChangedFiles | ForEach-Object { Write-Host "  $_" }
        Restore-FormattingChanges
        throw "Formatter changed a file outside the single characterized TD07.7 violation."
    }

    Write-Host ""
    Write-Host "=== TD07.7 final post-format check ==="
    $After = Invoke-FormatCheck
    Write-Host $After.Output

    if ($After.ExitCode -ne 0)
    {
        Restore-FormattingChanges
        throw "Global C++ format baseline is still not green."
    }

    git diff --check
    if ($LASTEXITCODE -ne 0)
    {
        Restore-FormattingChanges
        throw "git diff --check failed."
    }

    git add -- $ExpectedFile
    if ($LASTEXITCODE -ne 0)
    {
        Restore-FormattingChanges
        throw "git add failed."
    }

    git commit -m "Apply exact TD07.7 clang-format fix"
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
        throw "Commit $Commit was created locally but push failed."
    }

    Write-Host ""
    Write-Host "[OK] TD07.7 exact clang-format fix applied."
    Write-Host "Commit: $Commit"
    Write-Host "File  : $ExpectedFile"
}
finally
{
    Pop-Location
}
