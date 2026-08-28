param(
    [Parameter(Mandatory = $false)]
    [string]$Remote = "origin",

    [Parameter(Mandatory = $false)]
    [string]$BaseBranch = "master"
)

$ErrorActionPreference = "Stop"

$RepositoryRoot = Split-Path -Parent $PSScriptRoot
$AuditScript = Join-Path $PSScriptRoot "AuditTD075RemoteBranches.ps1"

$ExpectedBranches = @(
    "Test-Receptacle",
    "codex/cc6-4-class-visuals-final-temp",
    "codex/cc6-4-class-visuals-squash-temp",
    "codex/cc6-4-final-repair-base",
    "codex/character-creation-cc0-tests",
    "codex/character-creation-cc1-rpg-model",
    "codex/character-creation-cc2-creation-api",
    "codex/character-creation-cc3-startup-widget",
    "codex/rpg-damage-countermeasures",
    "codex/tmp-cc6-3-squash-base",
    "codex/tmp-cc6-4-inventory-class-icon-fix",
    "codex/tmp-cc6-5-visual-polish",
    "codex/tmp-full-body-inventory-docs",
    "feature/readables-per-instance-content",
    "td06-1-party-inventory-rebaseline",
    "td06-5-api-materialize",
    "td06-5-materialize"
)

Push-Location $RepositoryRoot
try {
    $CurrentBranch = (git rev-parse --abbrev-ref HEAD).Trim()
    if ($LASTEXITCODE -ne 0) {
        throw "Could not resolve current branch."
    }
    if ($CurrentBranch -ne $BaseBranch) {
        throw "TD07.5 cleanup must run from '$BaseBranch'. Current branch: '$CurrentBranch'."
    }

    git fetch --prune $Remote
    if ($LASTEXITCODE -ne 0) {
        throw "git fetch --prune $Remote failed."
    }

    $BaseRef = "$Remote/$BaseBranch"
    git rev-parse --verify $BaseRef | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "Base ref '$BaseRef' does not exist."
    }

    $ActualRemoteBranches = @(
        git for-each-ref --format="%(refname:short)" "refs/remotes/$Remote/" |
            Where-Object {
                $_ -ne $Remote -and
                $_ -ne "$Remote/HEAD" -and
                $_ -ne $BaseRef
            } |
            ForEach-Object {
                if ($_.StartsWith("$Remote/")) {
                    $_.Substring($Remote.Length + 1)
                }
                else {
                    $_
                }
            } |
            Sort-Object
    )

    $ExpectedSorted = @($ExpectedBranches | Sort-Object)
    $Unexpected = @($ActualRemoteBranches | Where-Object { $_ -notin $ExpectedSorted })
    $Missing = @($ExpectedSorted | Where-Object { $_ -notin $ActualRemoteBranches })

    if ($Unexpected.Count -gt 0 -or $Missing.Count -gt 0) {
        Write-Host "Unexpected remote branches:"
        $Unexpected | ForEach-Object { Write-Host "  $_" }
        Write-Host "Expected branches already missing:"
        $Missing | ForEach-Object { Write-Host "  $_" }
        throw "Remote branch set differs from the TD07.5 audited allow-list. Cleanup aborted."
    }

    Write-Host "TD07.5 deleting $($ExpectedBranches.Count) audited historical branches from '$Remote'..."
    foreach ($Branch in $ExpectedBranches) {
        Write-Host "  deleting $Remote/$Branch"
        git push $Remote --delete $Branch
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to delete '$Remote/$Branch'."
        }
    }

    git fetch --prune $Remote
    if ($LASTEXITCODE -ne 0) {
        throw "Post-cleanup git fetch --prune $Remote failed."
    }

    Write-Host ""
    Write-Host "[OK] TD07.5 historical remote branches deleted."
    Write-Host ""

    & $AuditScript -Remote $Remote -BaseBranch $BaseBranch
    if ($LASTEXITCODE -ne 0) {
        throw "Post-cleanup TD07.5 audit failed."
    }
}
finally {
    Pop-Location
}
