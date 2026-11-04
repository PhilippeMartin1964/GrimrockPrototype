# VALIDATION-AUTO01 — Agent Windows sans runner GitHub Actions.
# Ne touche jamais au checkout de developpement; le worktree dedie est la seule cible build.
[CmdletBinding()]
param(
    [string]$RepositoryRoot = (Split-Path -Parent $PSScriptRoot),
    [string]$EngineRoot = 'D:\UE_5.5',
    [string]$WorktreePath,
    [string]$StateRoot = (Join-Path $env:LOCALAPPDATA 'GrimrockPrototype\UEValidationAgent'),
    [string]$AutomationFilter = 'Grimrock',
    [string]$GitHubRepository = 'PhilippeMartin1964/GrimrockPrototype',
    [switch]$UseRHI
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Invoke-Checked {
    param([string]$Executable, [string[]]$Arguments)
    $Output = @(& $Executable @Arguments)
    $Code = $LASTEXITCODE
    if ($Code -ne 0) {
        throw "$Executable $($Arguments -join ' ') a echoue (code $Code): $($Output -join ' ')"
    }
    return $Output
}

function Save-JsonAtomic {
    param([string]$Path, [object]$Value)
    $Temp = "$Path.tmp"
    $Value | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $Temp -Encoding UTF8
    Move-Item -LiteralPath $Temp -Destination $Path -Force
}

function Publish-CommitStatus {
    param([string]$Sha, [string]$State, [string]$Description)
    $Description = $Description.Substring(0, [Math]::Min(135, $Description.Length))
    & gh api -X POST "repos/$GitHubRepository/statuses/$Sha" -f "state=$State" -f 'context=UE5.5.4/LocalAuto' -f "description=$Description" --silent
    if ($LASTEXITCODE -ne 0) { throw "Publication du statut GitHub impossible (code $LASTEXITCODE)." }
}

function Get-SafeDiagnosticLines {
    param([string]$LogPath)
    if (-not (Test-Path -LiteralPath $LogPath -PathType Leaf)) { return @() }
    $Tail = @(Get-Content -LiteralPath $LogPath -Tail 1500 -ErrorAction SilentlyContinue)
    $Matches = @($Tail | Where-Object {
        $_ -match '(?i)(error\s+[A-Z]+\d+|fatal error|LogAutomationController: Error:|\*\*\*\* TEST COMPLETE\. EXIT CODE:|exception|\[ERROR\])'
    } | Select-Object -Last 18)
    $Clean = foreach ($Line in $Matches) {
        $Value = [string]$Line
        $Value = $Value -replace '(?i)[A-Z]:\\Users\\[^\\]+', '<PROFIL_UTILISATEUR>'
        $Value = $Value -replace '(?i)(github_pat_|ghp_|gho_)[A-Za-z0-9_]+', '<SECRET>'
        $Value = $Value -replace '(?i)(token|password|authorization)\s*[:=]\s*\S+', '$1=<SECRET>'
        if ($Value.Length -gt 240) { $Value = $Value.Substring(0, 240) }
        $Value
    }
    return @($Clean)
}

function Publish-FailureIssue {
    param([string]$Sha, [string]$ErrorText, [string]$RunLogPath, [string]$ReportPath)
    $ShortSha = $Sha.Substring(0, 12)
    $Title = "[UEValidation] Echec du commit $ShortSha"
    $SafeErrors = @(Get-SafeDiagnosticLines -LogPath $RunLogPath)
    $Details = if ($SafeErrors.Count -gt 0) { ($SafeErrors | ForEach-Object { "- $_" }) -join "`n" } else { '- Aucun diagnostic filtre. Voir les journaux locaux.' }
    $Sanitized = $ErrorText -replace '(?i)[A-Z]:\\Users\\[^\\]+', '<PROFIL_UTILISATEUR>'
    $Sanitized = $Sanitized -replace '(?i)(github_pat_|ghp_|gho_)[A-Za-z0-9_]+', '<SECRET>'
    if ($Sanitized.Length -gt 500) { $Sanitized = $Sanitized.Substring(0, 500) }
    $Body = @"
## Validation automatique UE 5.5.4

- Commit : $Sha
- Suite : ``$AutomationFilter``
- Etat : **ECHEC**
- Diagnostic general : $Sanitized
- Les journaux complets restent sur la machine Windows et ne sont pas publies dans ce depot public.

### Extraits de diagnostics (filtres et tronques)
$Details

Ce ticket est emis automatiquement par ``Scripts/UEValidationAgent.ps1``. Aucun fichier utilisateur n'a ete modifie par l'agent.
"@
    $BodyPath = Join-Path $StateRoot "failure-$ShortSha.md"
    Set-Content -LiteralPath $BodyPath -Value $Body -Encoding UTF8
    $Issue = & gh issue create --repo $GitHubRepository --title $Title --body-file $BodyPath 2>&1
    if ($LASTEXITCODE -ne 0) { Write-Warning "Issue GitHub non creee : $($Issue -join ' ')" }
    else { Write-Host "Issue de diagnostic : $($Issue -join ' ')" }
}

$Mutex = [System.Threading.Mutex]::new($false, 'Local\GrimrockPrototypeUEValidationAgent')
$HasMutex = $false
try {
    try { $HasMutex = $Mutex.WaitOne(0) }
    catch [System.Threading.AbandonedMutexException] { $HasMutex = $true }
    if (-not $HasMutex) {
        Write-Host '[VALIDATION-AUTO01] Une validation est deja en cours.'
        return
    }

    $RepositoryRoot = (Resolve-Path -LiteralPath $RepositoryRoot).Path
    if ([string]::IsNullOrWhiteSpace($WorktreePath)) {
        $WorktreePath = Join-Path (Split-Path -Parent $RepositoryRoot) 'GrimrockPrototype_UEValidation'
    }
    $WorktreePath = [IO.Path]::GetFullPath($WorktreePath)
    if ($WorktreePath.TrimEnd('\') -ieq $RepositoryRoot.TrimEnd('\')) {
        throw 'Le worktree de validation ne doit pas correspondre au checkout utilisateur.'
    }
    if (-not (Test-Path -LiteralPath (Join-Path $RepositoryRoot '.git'))) {
        throw "Ce repertoire n'est pas le checkout Git de GrimrockPrototype : $RepositoryRoot"
    }
    if (-not (Test-Path -LiteralPath (Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'))) {
        throw "UE 5.5.4 introuvable : $EngineRoot"
    }
    foreach ($Command in @('git', 'gh')) {
        if (-not (Get-Command $Command -ErrorAction SilentlyContinue)) { throw "Commande obligatoire absente : $Command" }
    }
    Invoke-Checked -Executable 'git' -Arguments @('lfs', 'version') | Out-Null
    & gh auth status -h github.com *> $null
    if ($LASTEXITCODE -ne 0) { throw 'GitHub CLI n est pas authentifie. Executer gh auth login (une seule fois).' }

    # Refuse de valider accidentellement un autre depot.
    $Remote = (Invoke-Checked -Executable 'git' -Arguments @('-C', $RepositoryRoot, 'remote', 'get-url', 'origin') | Select-Object -First 1).ToString().Trim()
    if ($Remote -notmatch '(?i)(github\.com[:/])PhilippeMartin1964/GrimrockPrototype(\.git)?$') {
        throw "Remote origin inattendu : $Remote"
    }

    New-Item -ItemType Directory -Force -Path $StateRoot | Out-Null
    $StatePath = Join-Path $StateRoot 'state.json'
    $Previous = $null
    if (Test-Path -LiteralPath $StatePath) { $Previous = Get-Content -LiteralPath $StatePath -Raw | ConvertFrom-Json }

    # FETCH ne change jamais les fichiers suivis / non suivis du checkout utilisateur.
    Invoke-Checked -Executable 'git' -Arguments @('-C', $RepositoryRoot, 'fetch', '--no-tags', 'origin', 'master') | Out-Null
    $Sha = (Invoke-Checked -Executable 'git' -Arguments @('-C', $RepositoryRoot, 'rev-parse', 'refs/remotes/origin/master') | Select-Object -First 1).ToString().Trim()
    if ($Sha -notmatch '^[0-9a-f]{40}$') { throw "SHA Git invalide : $Sha" }
    if ($Previous -and $Previous.sha -eq $Sha) {
        Write-Host "[VALIDATION-AUTO01] Commit deja traite : $Sha ($($Previous.result))."
        return
    }

    $Time = Get-Date -Format 'yyyyMMdd-HHmmss'
    $RunDir = Join-Path $StateRoot ("Runs\$($Sha.Substring(0,12))-$Time")
    New-Item -ItemType Directory -Force -Path $RunDir | Out-Null
    $RunLog = Join-Path $RunDir 'Agent.log'
    $RunError = $null
    $Result = 'failure'
    $ExitCode = -1
    $ReportRoot = Join-Path $RunDir 'Automation'

    try {
        Publish-CommitStatus -Sha $Sha -State 'pending' -Description "Validation Windows en cours : $AutomationFilter"

        if (Test-Path -LiteralPath $WorktreePath) {
            if (-not (Test-Path -LiteralPath (Join-Path $WorktreePath '.git'))) {
                throw "Repertoire de validation non vide / non gere : $WorktreePath. Aucune suppression automatique."
            }
            $ActualRoot = (Invoke-Checked -Executable 'git' -Arguments @('-C', $WorktreePath, 'rev-parse', '--show-toplevel') | Select-Object -First 1).ToString().Trim()
            if ([IO.Path]::GetFullPath($ActualRoot).TrimEnd('\') -ine $WorktreePath.TrimEnd('\')) {
                throw "Worktree d'origine differente : $ActualRoot"
            }
            $Dirty = @(Invoke-Checked -Executable 'git' -Arguments @('-C', $WorktreePath, 'status', '--porcelain', '--untracked-files=normal'))
            if ($Dirty.Count -gt 0) { throw 'Worktree de validation modifie. Refus de supprimer ou ecraser des fichiers.' }
            Invoke-Checked -Executable 'git' -Arguments @('-C', $WorktreePath, 'checkout', '--detach', $Sha) | Out-Null
        }
        else {
            # Evite un telechargement massif en double : Git LFS est restaure ensuite.
            $env:GIT_LFS_SKIP_SMUDGE = '1'
            try {
                Invoke-Checked -Executable 'git' -Arguments @('-C', $RepositoryRoot, 'worktree', 'add', '--detach', $WorktreePath, $Sha) | Out-Null
            }
            finally { Remove-Item Env:\GIT_LFS_SKIP_SMUDGE -ErrorAction SilentlyContinue }
        }
        Invoke-Checked -Executable 'git' -Arguments @('-C', $WorktreePath, 'lfs', 'pull') | Out-Null
        $ValidatePath = Join-Path $WorktreePath 'Scripts\ValidateUE.ps1'
        if (-not (Test-Path -LiteralPath $ValidatePath -PathType Leaf)) { throw "Harness UE introuvable : $ValidatePath" }
        Write-Host "[VALIDATION-AUTO01] Validation $Sha : $AutomationFilter"
        # $RunLog est conserve localement. Ce script ne pousse aucun asset / resultat build.
        $Args = @{ EngineRoot = $EngineRoot; AutomationFilter = $AutomationFilter; ReportRoot = $ReportRoot }
        if ($UseRHI) { $Args.UseRHI = $true }
        & $ValidatePath @Args *> $RunLog
        $ExitCode = $LASTEXITCODE
        if ($ExitCode -ne 0) { throw "ValidateUE.ps1 a retourne le code $ExitCode." }
        Publish-CommitStatus -Sha $Sha -State 'success' -Description "Build + Automation OK ($AutomationFilter)"
        $Result = 'success'
        Write-Host "[VALIDATION-AUTO01] SUCCES $Sha. Journaux : $RunDir"
    }
    catch {
        $RunError = $_.Exception.Message
        Write-Warning "[VALIDATION-AUTO01] ECHEC $Sha : $RunError"
        try { Publish-CommitStatus -Sha $Sha -State 'failure' -Description "Echec UE ($AutomationFilter). Voir issue automatique." }
        catch { Write-Warning "Impossible de publier le statut d'echec : $($_.Exception.Message)" }
        try { Publish-FailureIssue -Sha $Sha -ErrorText $RunError -RunLogPath $RunLog -ReportPath $ReportRoot }
        catch { Write-Warning "Impossible de creer l'issue : $($_.Exception.Message)" }
    }
    finally {
        Save-JsonAtomic -Path $StatePath -Value @{
            sha = $Sha; result = $Result; exit_code = $ExitCode; automation_filter = $AutomationFilter
            timestamp = (Get-Date).ToString('o'); run_directory = $RunDir; error = $RunError
        }
    }
    if ($Result -ne 'success') { throw "Validation automatique $Sha en echec. $RunError" }
}
finally {
    if ($HasMutex) { $Mutex.ReleaseMutex() }
    $Mutex.Dispose()
}