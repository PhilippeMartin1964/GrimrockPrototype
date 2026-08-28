param(
    [Parameter(Mandatory = $false)]
    [string]$Remote = "origin",

    [Parameter(Mandatory = $false)]
    [string]$BaseBranch = "master"
)

$ErrorActionPreference = "Stop"

$RepositoryRoot = Split-Path -Parent $PSScriptRoot
$DiagnosticsDir = Join-Path $RepositoryRoot "Saved\Diagnostics\TD07"
$ReportPath = Join-Path $DiagnosticsDir "TD07_5_RemoteBranchAudit.txt"

Push-Location $RepositoryRoot
try {
    New-Item -ItemType Directory -Force -Path $DiagnosticsDir | Out-Null

    git fetch --prune $Remote
    if ($LASTEXITCODE -ne 0) {
        throw "git fetch --prune $Remote failed."
    }

    $BaseRef = "$Remote/$BaseBranch"
    git rev-parse --verify $BaseRef | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "Base ref '$BaseRef' does not exist."
    }

    $Branches = @(
        git for-each-ref --format="%(refname:short)" "refs/remotes/$Remote/" |
            Where-Object {
                $_ -ne $Remote -and
                $_ -ne "$Remote/HEAD" -and
                $_ -ne $BaseRef
            } |
            Sort-Object
    )

    $Rows = foreach ($Branch in $Branches) {
        $Counts = (git rev-list --left-right --count "$BaseRef...$Branch").Trim() -split "\s+"
        if ($Counts.Count -ne 2) {
            throw "Could not parse ahead/behind counts for '$Branch'."
        }

        $Behind = [int]$Counts[0]
        $Ahead = [int]$Counts[1]
        $Tip = (git log -1 --format="%h %s" $Branch).Trim()

        [pscustomobject]@{
            Branch = $Branch
            Ahead  = $Ahead
            Behind = $Behind
            Tip    = $Tip
        }
    }

    $BehindOnly = @($Rows | Where-Object { $_.Ahead -eq 0 })
    $Diverged = @($Rows | Where-Object { $_.Ahead -gt 0 -and $_.Behind -gt 0 })
    $AheadOnly = @($Rows | Where-Object { $_.Ahead -gt 0 -and $_.Behind -eq 0 })

    $Lines = [System.Collections.Generic.List[string]]::new()
    $Lines.Add("GrimrockPrototype TD07.5 - Remote Branch Audit")
    $Lines.Add("Base: $BaseRef")
    $Lines.Add("Remote branches excluding base: $($Rows.Count)")
    $Lines.Add("Behind-only: $($BehindOnly.Count)")
    $Lines.Add("Diverged: $($Diverged.Count)")
    $Lines.Add("Ahead-only: $($AheadOnly.Count)")
    $Lines.Add("")
    $Lines.Add("Branch | Ahead | Behind | Tip")

    foreach ($Row in $Rows) {
        $Lines.Add("$($Row.Branch) | $($Row.Ahead) | $($Row.Behind) | $($Row.Tip)")
    }

    $Lines | Set-Content -Path $ReportPath -Encoding UTF8
    $Lines | ForEach-Object { Write-Host $_ }

    Write-Host ""
    Write-Host "Report: $ReportPath"
}
finally {
    Pop-Location
}
