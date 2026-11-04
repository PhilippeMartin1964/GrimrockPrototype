# VALIDATION-AUTO01.8 — Agent Windows sans runner GitHub Actions.
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

function Get-PublicSetupCode {
    param([string]$Message)
    if ($Message -match '(?i)worktrees de ce depot') { return 'WORKTREE_REGISTRATION' }
    if ($Message -match '(?i)Worktree de validation modifie') { return 'WORKTREE_DIRTY' }
    if ($Message -match '(?i)Repertoire de validation non vide|Worktree d.origine differente') { return 'WORKTREE_LOCATION' }
    if ($Message -match '(?i)\blfs\b') { return 'LFS' }
    if ($Message -match '(?i)\bgh\b|authentifie') { return 'GITHUB_AUTH' }
    if ($Message -match '(?i)\bgit\b|checkout|fetch') { return 'GIT' }
    if ($Message -match '(?i)Harness UE introuvable') { return 'HARNESS_MISSING' }
    return 'OTHER'
}

function Get-PublicSafeDiagnostics {
    param([string]$RunLogPath, [string]$ReportRoot)
    # A public issue may contain ONLY whitelisted, structured records.
    # Never include raw log lines, absolute paths, arbitrary error text or secrets.
    $Safe = New-Object 'System.Collections.Generic.List[string]'
    $Logs = @($RunLogPath, (Join-Path (Split-Path -Parent $RunLogPath) 'Agent.stderr.log'))
    foreach ($Log in $Logs) {
        if (-not (Test-Path -LiteralPath $Log -PathType Leaf)) { continue }
        foreach ($RawLine in @(Get-Content -LiteralPath $Log -Tail 2500 -ErrorAction SilentlyContinue)) {
            $Line = [string]$RawLine
            $Location = [regex]::Match($Line,
                '(?i)(?<file>[A-Za-z_][A-Za-z0-9_.-]{0,95}\.(?:cpp|h|cs|inl))\((?<line>\d{1,7})(?:,\d{1,7})?\)\s*:\s*(?:fatal\s+)?error(?:\s+(?<code>C\d{4}|CS\d{4}|MSB\d{4}))?\b')
            if ($Location.Success) {
                $Code = $Location.Groups['code'].Value
                if ([string]::IsNullOrEmpty($Code)) { $Code = 'sans-code' }
                $Record = 'Compilation : ' + $Location.Groups['file'].Value + ':' +
                    $Location.Groups['line'].Value + ' / ' + $Code
                if (-not $Safe.Contains($Record)) { $Safe.Add($Record) }
                continue
            }
            if ($Line -match '(?i)\berror\s+(?<code>C\d{4}|CS\d{4}|MSB\d{4})\b') {
                $Record = 'Compilateur : ' + $Matches['code']
                if (-not $Safe.Contains($Record)) { $Safe.Add($Record) }
            }
            if ($Line -match '^\s*Failed\s*:\s*(\d{1,5})\s*$') {
                $Record = 'Tests Automation en echec : ' + $Matches[1]
                if (-not $Safe.Contains($Record)) { $Safe.Add($Record) }
            }
        }
    }
    if (Test-Path -LiteralPath $ReportRoot -PathType Container) {
        $Indexes = @(Get-ChildItem -LiteralPath $ReportRoot -Filter 'index.json' -File -Recurse -ErrorAction SilentlyContinue |
            Select-Object -Last 2)
        foreach ($Index in $Indexes) {
            try {
                $Report = Get-Content -LiteralPath $Index.FullName -Raw -ErrorAction Stop |
                    ConvertFrom-Json -ErrorAction Stop
                if ($null -ne $Report.failed -and [int]$Report.failed -gt 0) {
                    $Record = 'Rapport UE : ' + [int]$Report.failed + ' test(s) en echec'
                    if (-not $Safe.Contains($Record)) { $Safe.Add($Record) }
                }
                if ($Report.PSObject.Properties['tests']) {
                    foreach ($Test in @($Report.tests)) {
                        if ($null -eq $Test) { continue }
                        $State = [string]$Test.state
                        $TestPath = [string]$Test.fullTestPath
                        if ($State -match '^(?i:fail|failed|error)$' -and
                            $TestPath -cmatch '^Grimrock\.[A-Za-z0-9_.]{1,180}$') {
                            $Record = 'Test : ' + $TestPath
                            if (-not $Safe.Contains($Record)) { $Safe.Add($Record) }
                        }
                        if ($TestPath -ceq 'Grimrock.UI.RPG06.Skills.ProductionCatalog' -and
                            $State -match '^(?i:fail|failed|error)$' -and
                            $Test.PSObject.Properties['entries']) {
                            # Fixed allowlist: no raw error text is ever published.
                            $AllowedSkills = @(
                                'HeavyWeapons', 'LightWeapons', 'RangedWeapons', 'Throwing', 'Shield',
                                'Armor', 'Athletics', 'Acrobatics', 'Perception', 'Survival', 'Stealth',
                                'Lockpicking', 'Traps', 'Mechanics', 'Crafting', 'Alchemy', 'Arcana',
                                'Runes', 'Religion', 'Nature', 'Medicine', 'History', 'Persuasion',
                                'Intimidation', 'Deception'
                            )
                            foreach ($Entry in @($Test.entries)) {
                                if ($null -eq $Entry -or $null -eq $Entry.event) { continue }
                                if ([string]$Entry.event.type -notmatch '^(?i:error|fatal)$') { continue }
                                $Message = [string]$Entry.event.message
                                foreach ($SkillName in $AllowedSkills) {
                                    $SkillId = 'Skill_' + $SkillName
                                    if (-not $Message.Contains($SkillId)) { continue }
                                    $Category = 'UNKNOWN'
                                    if ($Message -match 'production asset loads') { $Category = 'ASSET_LOAD' }
                                    elseif ($Message -match 'matches canonical mechanical fields') { $Category = 'CANONICAL_FIELDS' }
                                    elseif ($Message -match 'serialized DisplayName') { $Category = 'DISPLAY_NAME' }
                                    elseif ($Message -match 'structurally valid') { $Category = 'STRUCTURE' }
                                    $Record = 'Skill : ' + $SkillId + ' / ' + $Category
                                    if (-not $Safe.Contains($Record)) { $Safe.Add($Record) }
                                }
                            }
                        }
                    }
                }
            } catch {
                if (-not $Safe.Contains('Rapport UE illisible')) { $Safe.Add('Rapport UE illisible') }
            }
        }
    }
    if ($Safe.Count -eq 0) {
        $Safe.Add('Pas de diagnostic public fiable ; seuls les journaux locaux sont exhaustifs.')
    }
    return @($Safe | Select-Object -First 20)
}

function Publish-FailureIssue {
    param([string]$Sha, [int]$ValidationExitCode, [string]$RunLogPath, [string]$ReportRoot)
    $ShortSha = $Sha.Substring(0, 12)
    $Title = "[UEValidation] Echec du commit $ShortSha"
    $SafeDetails = @(Get-PublicSafeDiagnostics -RunLogPath $RunLogPath -ReportRoot $ReportRoot)
    $SafeMarkdown = ($SafeDetails | ForEach-Object { '- ' + $_ }) -join [Environment]::NewLine
    $Body = @"
## Validation automatique UE 5.5.4

- Commit : $Sha
- Suite : $AutomationFilter
- Etat : ECHEC
- Code de sortie UE : $ValidationExitCode
- Les journaux complets restent prives sur la machine Windows.

### Diagnostics publics structures (liste blanche)
$SafeMarkdown

Aucune ligne brute des journaux, aucun chemin absolu ni message libre ne sont publies.
"@
    $BodyPath = Join-Path $StateRoot "failure-$ShortSha.md"
    Set-Content -LiteralPath $BodyPath -Value $Body -Encoding UTF8
    $Issue = & gh issue create --repo $GitHubRepository --title $Title --body-file $BodyPath 2>&1
    if ($LASTEXITCODE -ne 0) { Write-Warning "Issue GitHub non creee : $($Issue -join ' ')" }
    else { Write-Host "Issue de diagnostic : $($Issue -join ' ')" }
}

$Mutex = [System.Threading.Mutex]::new($false, 'Local\GrimrockPrototypeUEValidationAgent')
$HasMutex = $false
$TranscriptStarted = $false
try {
    try { $HasMutex = $Mutex.WaitOne(0) }
    catch [System.Threading.AbandonedMutexException] { $HasMutex = $true }
    if (-not $HasMutex) {
        Write-Host '[VALIDATION-AUTO01] Une validation est deja en cours.'
        return
    }

    # Transcript de l'agent des le preflight, meme si git/gh/UE echoue avant les tests.
    New-Item -ItemType Directory -Force -Path $StateRoot | Out-Null
    try {
        Start-Transcript -Path (Join-Path $StateRoot 'Agent.latest.log') -Force -ErrorAction Stop | Out-Null
        $TranscriptStarted = $true
    }
    catch { Write-Warning "Transcript indisponible : $($_.Exception.Message)" }

    $RepositoryRoot = (Resolve-Path -LiteralPath $RepositoryRoot).Path
    if ([string]::IsNullOrWhiteSpace($WorktreePath)) {
        $WorktreePath = Join-Path (Split-Path -Parent $RepositoryRoot) 'GrimrockPrototype_UEValidation'
    }
    $WorktreePath = [IO.Path]::GetFullPath($WorktreePath)
    $UserRoot = $RepositoryRoot.TrimEnd('\')
    $ValidationRoot = $WorktreePath.TrimEnd('\')
    if ($ValidationRoot -ieq $UserRoot -or $ValidationRoot.StartsWith($UserRoot + '\', [System.StringComparison]::OrdinalIgnoreCase)) {
        throw 'Le worktree de validation doit etre hors du checkout utilisateur.'
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
    & gh auth status -h github.com | Out-Null
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
    if ($Previous -and $Previous.sha -eq $Sha -and $Previous.automation_filter -eq $AutomationFilter -and
        $Previous.result -in @('success', 'failure')) {
        Write-Host "[VALIDATION-AUTO01] Commit deja traite : $Sha ($($Previous.result))."
        return
    }

    $Time = Get-Date -Format 'yyyyMMdd-HHmmss'
    $RunDir = Join-Path $StateRoot ("Runs\$($Sha.Substring(0,12))-$Time")
    New-Item -ItemType Directory -Force -Path $RunDir | Out-Null
    $RunLog = Join-Path $RunDir 'Agent.log'
    $RunError = $null
    $Result = 'infrastructure_error'
    $Phase = 'setup'
    $ExitCode = -1
    $ReportRoot = Join-Path $RunDir 'Automation'

    try {
        Publish-CommitStatus -Sha $Sha -State 'pending' -Description "Validation Windows en cours : $AutomationFilter"

        if (Test-Path -LiteralPath $WorktreePath) {
            if (-not (Test-Path -LiteralPath (Join-Path $WorktreePath '.git'))) {
                throw "Repertoire de validation non vide / non gere : $WorktreePath. Aucune suppression automatique."
            }
            # Verifier l'appartenance au depot de depart, pas seulement le chemin.
            $RegisteredWorktrees = @(Invoke-Checked -Executable 'git' -Arguments @('-C', $RepositoryRoot, 'worktree', 'list', '--porcelain') |
                Where-Object { $_ -like 'worktree *' } | ForEach-Object {
                    $GitPath = $_.Substring(9).Trim()
                    # Sous Windows, git peut ecrire D:/... alors que GetFullPath donne D:\...
                    [IO.Path]::GetFullPath($GitPath.Replace('/', '\')).TrimEnd('\')
                })
            if (@($RegisteredWorktrees | Where-Object { $_ -ieq $ValidationRoot }).Count -ne 1) {
                throw 'Le dossier existe mais ne fait pas partie des worktrees de ce depot. Aucune modification.'
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
        # ThirdParty/Lua54 est un sous-module Git (pas Git LFS).
        # Un nouveau worktree est cree avec les sous-modules non initialises :
        # sans cette etape, Lua54.cpp ne trouve pas onelua.c (MSVC C1083)
        # et GridLuaVm.cpp ne trouve pas lua.h (MSVC C1083).
        # Limiter l'initialisation au sous-module officiel declare dans .gitmodules.
        Invoke-Checked -Executable 'git' -Arguments @('-C', $WorktreePath, 'submodule', 'update', '--init', '--', 'ThirdParty/Lua54') | Out-Null
        foreach ($LuaRequired in @('onelua.c', 'lua.h', 'lauxlib.h', 'lualib.h')) {
            $LuaPath = Join-Path $WorktreePath ('ThirdParty\Lua54\' + $LuaRequired)
            if (-not (Test-Path -LiteralPath $LuaPath -PathType Leaf)) {
                throw "Sous-module Lua54 incomplet : fichier absent $LuaRequired"
            }
        }
        # Les assets volumineux .uasset/.umap sont geres separement par Git LFS.
        Invoke-Checked -Executable 'git' -Arguments @('-C', $WorktreePath, 'lfs', 'pull') | Out-Null
        $ValidatePath = Join-Path $WorktreePath 'Scripts\ValidateUE.ps1'
        if (-not (Test-Path -LiteralPath $ValidatePath -PathType Leaf)) { throw "Harness UE introuvable : $ValidatePath" }
        Write-Host "[VALIDATION-AUTO01] Validation $Sha : $AutomationFilter"
        # Processus PowerShell distinct pour eviter que exit / stderr natif du harness
        # ne perturbent le code de sortie ou le processus de surveillance Windows PS 5.1.
        $Phase = 'testing'
        $PsExe = Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe'
        if (-not (Test-Path -LiteralPath $PsExe -PathType Leaf)) { throw "PowerShell.exe introuvable : $PsExe" }
        foreach ($Value in @($ValidatePath, $EngineRoot, $AutomationFilter, $ReportRoot)) {
            if ($Value.Contains('"')) { throw 'Guillemet interdit dans les arguments du harness.' }
        }
        $ValidateArguments = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' + $ValidatePath +
            '" -EngineRoot "' + $EngineRoot + '" -AutomationFilter "' + $AutomationFilter + '" -ReportRoot "' + $ReportRoot + '"'
        if ($UseRHI) { $ValidateArguments += ' -UseRHI' }
        $RunStderrLog = Join-Path $RunDir 'Agent.stderr.log'
        $Process = Start-Process -FilePath $PsExe -ArgumentList $ValidateArguments -WorkingDirectory $WorktreePath `
            -RedirectStandardOutput $RunLog -RedirectStandardError $RunStderrLog -WindowStyle Hidden -Wait -PassThru -ErrorAction Stop
        $ExitCode = $Process.ExitCode
        $Result = 'failure'
        if ($ExitCode -ne 0) { throw "ValidateUE.ps1 a retourne le code $ExitCode. Voir $RunLog et $RunStderrLog." }
        $Phase = 'publication'
        Publish-CommitStatus -Sha $Sha -State 'success' -Description "Build + Automation OK ($AutomationFilter)"
        $Result = 'success'
        Write-Host "[VALIDATION-AUTO01] SUCCES $Sha. Journaux : $RunDir"
    }
    catch {
        $RunError = $_.Exception.Message
        if ($Phase -eq 'publication') { $Result = 'infrastructure_error' }
        Write-Warning "[VALIDATION-AUTO01] ECHEC ($Phase) $Sha : $RunError"
        $PublicCause = if ($Phase -eq 'setup') { Get-PublicSetupCode -Message $RunError } else { 'TESTS' }
        try { Publish-CommitStatus -Sha $Sha -State 'failure' -Description "Echec $Phase/$PublicCause ($AutomationFilter)." }
        catch { Write-Warning "Impossible de publier le statut d'echec : $($_.Exception.Message)" }
        # Les incidents de recuperation Git/LFS restent reessayables au cycle suivant.
        # Une erreur de compilation/test est definitive pour ce SHA, sans issues multiples.
        if ($Result -eq 'failure') {
            # One-shot, explicitly allowlisted Skill_Traps repair. This helper
            # publishes only the single canonical .uasset if the sole failed
            # test is Skills.ProductionCatalog and master has not moved.
            $RepairHelper = Join-Path $WorktreePath 'Scripts\RepairCanonicalSkillTraps.ps1'
            if ($Phase -eq 'testing' -and $ExitCode -ne 0 -and (Test-Path -LiteralPath $RepairHelper -PathType Leaf)) {
                try {
                    $RepairOut = Join-Path $RunDir 'SkillTrapsRepair.stdout.log'
                    $RepairErr = Join-Path $RunDir 'SkillTrapsRepair.stderr.log'
                    $RepairArguments = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' + $RepairHelper +
                        '" -WorktreePath "' + $WorktreePath + '" -EngineRoot "' + $EngineRoot +
                        '" -ReportRoot "' + $ReportRoot + '" -ExpectedHead "' + $Sha + '"'
                    $RepairProcess = Start-Process -FilePath $PsExe -ArgumentList $RepairArguments -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput $RepairOut -RedirectStandardError $RepairErr -PassThru -Wait -ErrorAction Stop
                    if ($RepairProcess.ExitCode -eq 0) {
                        Write-Host '[VALIDATION-AUTO01.7] Repair published: next commit will receive normal full UE validation.'
                    }
                    elseif ($RepairProcess.ExitCode -ne 2) {
                        Write-Warning "Targeted asset repair not completed (code $($RepairProcess.ExitCode)); see private logs."
                    }
                }
                catch { Write-Warning 'Targeted asset repair failed; private logs only.' }
            }
            # ARMEMENT01.17.1.4 : authoring des materiaux natifs Unreal, controle par le resultat Automation.
            $OilHelper = Join-Path $WorktreePath 'Scripts\AuthorOilSurfaceMaterial.ps1'
            if ($Phase -eq 'testing' -and $ExitCode -ne 0 -and (Test-Path -LiteralPath $OilHelper -PathType Leaf)) {
                try {
                    $OilOut = Join-Path $RunDir 'OilMaterialAuthoring.stdout.log'
                    $OilErr = Join-Path $RunDir 'OilMaterialAuthoring.stderr.log'
                    $OilArguments = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' + $OilHelper +
                        '" -WorktreePath "' + $WorktreePath + '" -EngineRoot "' + $EngineRoot +
                        '" -ReportRoot "' + $ReportRoot + '" -ExpectedHead "' + $Sha + '"'
                    $OilProcess = Start-Process -FilePath $PsExe -ArgumentList $OilArguments -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput $OilOut -RedirectStandardError $OilErr -PassThru -Wait -ErrorAction Stop
                    if ($OilProcess.ExitCode -eq 0) {
                        Write-Host '[ARMEMENT01.17.1.4] Materiaux natifs publies : validation du nouveau commit au prochain cycle.'
                    }
                    elseif ($OilProcess.ExitCode -ne 2) {
                        Write-Warning "Authoring d'huile non termine (code $($OilProcess.ExitCode)). Journaux locaux uniquement."
                    }
                }
                catch { Write-Warning "Authoring automatique d'huile indisponible ; journaux locaux uniquement." }
            }
            try { Publish-FailureIssue -Sha $Sha -ValidationExitCode $ExitCode -RunLogPath $RunLog -ReportRoot $ReportRoot }
            catch { Write-Warning "Impossible de creer l'issue : $($_.Exception.Message)" }
        }
    }
    finally {
        Save-JsonAtomic -Path $StatePath -Value @{
            sha = $Sha; result = $Result; phase = $Phase; exit_code = $ExitCode; automation_filter = $AutomationFilter
            timestamp = (Get-Date).ToString('o'); run_directory = $RunDir; error = $RunError
        }
    }
    if ($Result -ne 'success') { throw "Validation automatique $Sha en echec. $RunError" }
}
finally {
    if ($TranscriptStarted) {
        try { Stop-Transcript | Out-Null } catch { Write-Warning "Arret du transcript impossible : $($_.Exception.Message)" }
    }
    if ($HasMutex) { $Mutex.ReleaseMutex() }
    $Mutex.Dispose()
}