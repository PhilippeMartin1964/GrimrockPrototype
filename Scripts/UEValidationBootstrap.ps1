# VALIDATION-AUTO01.4 — Bootstrap stable pour agent Windows auto-actualise.
[CmdletBinding()]
param(
    [string]$RepositoryRoot = (Split-Path -Parent $PSScriptRoot),
    [string]$EngineRoot = 'D:\UE_5.5',
    [string]$WorktreePath,
    [string]$StateRoot = (Join-Path $env:LOCALAPPDATA 'GrimrockPrototype\UEValidationAgent'),
    [string]$AutomationFilter = 'Grimrock',
    [switch]$UseRHI
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$TranscriptStarted = $false
try {
    if ([string]::IsNullOrWhiteSpace($StateRoot)) { throw 'StateRoot obligatoire.' }
    New-Item -ItemType Directory -Path $StateRoot -Force | Out-Null
    try {
        Start-Transcript -LiteralPath (Join-Path $StateRoot 'Bootstrap.latest.log') -Force -ErrorAction Stop | Out-Null
        $TranscriptStarted = $true
    } catch { Write-Warning 'Journal bootstrap indisponible.' }
    $RepositoryRoot = (Resolve-Path -LiteralPath $RepositoryRoot -ErrorAction Stop).Path
    if (-not (Test-Path -LiteralPath (Join-Path $RepositoryRoot '.git'))) { throw 'Checkout Git absent.' }
    if (-not (Get-Command git -ErrorAction SilentlyContinue)) { throw 'git absent.' }
    $Origin = @(& git -C $RepositoryRoot remote get-url origin)
    if ($LASTEXITCODE -ne 0 -or $Origin.Count -ne 1 -or
        $Origin[0] -notmatch '(?i)(github\.com[:/])PhilippeMartin1964/GrimrockPrototype(\.git)?$') {
        throw 'Remote origin inattendu.'
    }
    # Uniquement git fetch ; aucun pull, checkout ou ecriture dans le dossier de travail.
    & git -C $RepositoryRoot fetch --no-tags origin master | Out-Null
    if ($LASTEXITCODE -ne 0) { throw 'git fetch impossible.' }
    $Commits = @(& git -C $RepositoryRoot rev-parse refs/remotes/origin/master)
    if ($LASTEXITCODE -ne 0 -or $Commits.Count -ne 1) { throw 'Lecture SHA impossible.' }
    $Sha = [string]$Commits[0]
    if ($Sha -notmatch '^[0-9a-f]{40}$') { throw 'SHA Git invalide.' }
    $Versions = Join-Path $StateRoot 'AgentVersions'
    New-Item -ItemType Directory -Path $Versions -Force | Out-Null
    $ScriptPath = Join-Path $Versions ("UEValidationAgent-$Sha.ps1")
    $Temporary = $ScriptPath + '.tmp'
    try {
        $GitRevision = $Sha + ':Scripts/UEValidationAgent.ps1'
        & git -C $RepositoryRoot show $GitRevision |
            Set-Content -LiteralPath $Temporary -Encoding UTF8 -ErrorAction Stop
        if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $Temporary -PathType Leaf)) {
            throw 'Lecture agent depuis Git impossible.'
        }
        $Tokens = $null
        $Errors = $null
        [void][System.Management.Automation.Language.Parser]::ParseFile($Temporary, [ref]$Tokens, [ref]$Errors)
        if ($null -ne $Errors -and $Errors.Count -gt 0) { throw 'Syntaxe de l agent Git invalide.' }
        Move-Item -LiteralPath $Temporary -Destination $ScriptPath -Force
    } finally {
        Remove-Item -LiteralPath $Temporary -Force -ErrorAction SilentlyContinue
    }
    Write-Host ('[VALIDATION-AUTO01.4] Agent depuis master : ' + $Sha.Substring(0,12))
    $AgentArguments = @{
        RepositoryRoot = $RepositoryRoot
        EngineRoot = $EngineRoot
        WorktreePath = $WorktreePath
        StateRoot = $StateRoot
        AutomationFilter = $AutomationFilter
    }
    if ($UseRHI) { $AgentArguments.UseRHI = $true }
    & $ScriptPath @AgentArguments
}
finally {
    if ($TranscriptStarted) {
        try { Stop-Transcript | Out-Null } catch { Write-Warning 'Arret transcript impossible.' }
    }
}
