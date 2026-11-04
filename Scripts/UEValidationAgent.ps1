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
    # Suivi de la création des deux seuls matériaux Unreal autorisés.
    # Ne publie qu'une phase issue d'une liste blanche fixe, jamais un journal brut.
    $OilRunDir = Split-Path -Parent $RunLogPath
    $OilStagePath = Join-Path $OilRunDir 'OilMaterial.stage'
    if (Test-Path -LiteralPath $OilStagePath -PathType Leaf) {
        $OilStage = [string](Get-Content -LiteralPath $OilStagePath -TotalCount 1 -ErrorAction SilentlyContinue)
        $KnownOilStages = @('PREFLIGHT', 'REPORT_ACCEPTED', 'COMMANDLET_STARTED',
            'COMMANDLET_COMPLETED', 'COMMANDLET_FAILED', 'FILESET_STATUS',
            'FILESET_OUTSIDE_ALLOWLIST', 'FILESET_MISSING_MASTER',
            'FILESET_CREATION', 'ASSETS_VERIFIED', 'COMMIT_CREATED', 'SUCCESS')
        if ($OilStage -in $KnownOilStages) {
            $Safe.Add('Materiau huile : phase ' + $OilStage)
        }
        else {
            $Safe.Add('Materiau huile : phase inconnue')
        }
        # Le message Git brut reste prive : seul un code borne est publie.
        if ($OilStage -eq 'COMMIT_CREATED') {
            $GitPushLog = Join-Path $OilRunDir 'OilMaterialAuthoring.stderr.log'
            $FailureClass = 'PUSH_UNKNOWN'
            if (Test-Path -LiteralPath $GitPushLog -PathType Leaf) {
                $TailText = (@(Get-Content -LiteralPath $GitPushLog -Tail 140 -ErrorAction SilentlyContinue) -join [Environment]::NewLine)
                if ($TailText -match '(?i)bandwidth|quota|storage limit') { $FailureClass = 'LFS_QUOTA' }
                elseif ($TailText -match '(?i)non-fast-forward|fetch first|rejected|stale info') { $FailureClass = 'REF_DIVERGENCE' }
                elseif ($TailText -match '(?i)403|401|permission denied|authentication failed|access denied') { $FailureClass = 'AUTHORIZATION' }
                elseif ($TailText -match '(?i)git-lfs|git lfs|lfs|batch response') { $FailureClass = 'LFS_TRANSFER' }
                elseif ($TailText -match '(?i)timed out|timeout|connection|could not resolve|network|EOF') { $FailureClass = 'NETWORK' }
            }
            $Safe.Add('Materiau huile : push ' + $FailureClass)
        }
        if ($OilStage -eq 'COMMANDLET_FAILED') {
            $AllowedGraphStages = @(
                'UV_POSITION','Z_POSITION','NOISE_POSITION','NOISE_AMPLITUDE',
                'WARP_AMPLITUDE','WARP_RADIUS','RADIUS_BASE','UV_MASK',
                'CENTER_MASK','RADIUS_MASK','MASK_ALPHA','OPACITY_ALPHA',
                'BASECOLOR_OUTPUT','ROUGHNESS_OUTPUT','OPACITY_OUTPUT'
            )
            $OilStdOut = Join-Path $OilRunDir 'OilMaterialCommandlet.stdout.log'
            if (Test-Path -LiteralPath $OilStdOut -PathType Leaf) {
                $OilLog = (@(Get-Content -LiteralPath $OilStdOut -Tail 1000 -ErrorAction SilentlyContinue) -join [Environment]::NewLine)
                foreach ($GraphStage in $AllowedGraphStages) {
                    if ($OilLog.Contains('[OIL_DECAL_GRAPH:' + $GraphStage + ']')) {
                        $Safe.Add('Décalque huile : connexion ' + $GraphStage)
                        break
                    }
                }
            }
        }
        $OilCommandletLog = Join-Path $OilRunDir 'OilMaterialCommandlet.stdout.log'
        if (Test-Path -LiteralPath $OilCommandletLog -PathType Leaf) {
            $Relevant = @(
                Get-Content -LiteralPath $OilCommandletLog -Tail 500 -ErrorAction SilentlyContinue |
                Where-Object { $_ -match '\[ARMEMENT01\.17\.1\.4\]' -and $_ -match '(?i)error|erreur|impossible|incompatible|Échec' }
            )
            if ($Relevant.Count -gt 0) {
                $Safe.Add('Materiau huile : le commandlet signale une erreur dans ses journaux locaux')
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
            if ($Dirty.Count -gt 0) {
                # Récupération spécifique : un commandlet précédent peut avoir
                # réécrit M_Surface_Oil avant que son commit n'ait été accepté.
                # Ne JAMAIS restaurer un fichier inattendu ou non sauvegardé.
                $AllowedOilAssets = @(
                    'Content/GrimrockPrototype/Art/Materials/Surfaces/M_Surface_Oil.uasset',
                    'Content/GrimrockPrototype/Art/Materials/Surfaces/MI_Surface_Oil.uasset'
                )
                $DirtyOilPaths = @()
                foreach ($Entry in $Dirty) {
                    if (-not $Entry.StartsWith(' M ') -or $Entry.Length -le 3) {
                        throw 'Worktree de validation modifie (index ou statut non autorise) : protection active.'
                    }
                    $Path = $Entry.Substring(3)
                    if ($Path -notin $AllowedOilAssets) {
                        throw 'Worktree de validation modifie (fichier hors liste blanche) : protection active.'
                    }
                    $DirtyOilPaths += $Path
                }
                if ($DirtyOilPaths.Count -lt 1 -or $DirtyOilPaths.Count -gt 2) {
                    throw 'Worktree de validation modifie : nombre de fichiers inattendu.'
                }
                $OldHead = @(Invoke-Checked -Executable 'git' -Arguments @('-C', $WorktreePath, 'rev-parse', 'HEAD'))[0].Trim()
                if ($OldHead -notmatch '^[0-9a-f]{40}$') {
                    throw 'Revision du worktree invalide : sauvegarde non tentee.'
                }
                $BackupRoot = Join-Path $StateRoot ('Recovery\Oil-' + $OldHead.Substring(0,12) + '-' + (Get-Date -Format 'yyyyMMddHHmmssfff'))
                New-Item -ItemType Directory -Path $BackupRoot -Force | Out-Null
                foreach ($OilPath in $DirtyOilPaths) {
                    $Source = Join-Path $WorktreePath $OilPath
                    $Destination = Join-Path $BackupRoot ([IO.Path]::GetFileName($OilPath))
                    if (-not (Test-Path -LiteralPath $Source -PathType Leaf)) {
                        throw 'Asset d huile absent : restauration refusee.'
                    }
                    Copy-Item -LiteralPath $Source -Destination $Destination -ErrorAction Stop
                    $SourceHash = (Get-FileHash -LiteralPath $Source -Algorithm SHA256).Hash
                    $BackupHash = (Get-FileHash -LiteralPath $Destination -Algorithm SHA256).Hash
                    if ($SourceHash -ne $BackupHash) {
                        throw 'Sauvegarde huile non integre : restauration refusee.'
                    }
                }
                # Restore est limite AUX DEUX assets autorises, dans le seul
                # worktree temporaire. Les copies exactes demeurent privées.
                Invoke-Checked -Executable 'git' -Arguments (@('-C', $WorktreePath,
                    'restore', '--source=HEAD', '--worktree', '--') + $DirtyOilPaths) | Out-Null
                $AfterRestore = @(Invoke-Checked -Executable 'git' -Arguments @('-C', $WorktreePath,
                    'status', '--porcelain', '--untracked-files=normal'))
                if ($AfterRestore.Count -gt 0) {
                    throw 'Worktree toujours modifie apres sauvegarde des assets huile ; abandon.'
                }
                Write-Host '[VALIDATION-AUTO01] Assets d huile preserves dans Recovery, worktree restaure pour la prochaine validation.'
            }
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

            # ARMEMENT01.17.2A: only when the unique failing Automation test
            # explicitly requires the native starter catalyst DataAsset.
            $CatalystHelper = Join-Path $WorktreePath 'Scripts\AuthorStartingSurfaceCatalyst.ps1'
            if ($Phase -eq 'testing' -and $ExitCode -ne 0 -and (Test-Path -LiteralPath $CatalystHelper -PathType Leaf)) {
                try {
                    $CatalystOut = Join-Path $RunDir 'CatalystAuthoring.stdout.log'
                    $CatalystErr = Join-Path $RunDir 'CatalystAuthoring.stderr.log'
                    $CatalystArguments = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' + $CatalystHelper +
                        '" -WorktreePath "' + $WorktreePath + '" -EngineRoot "' + $EngineRoot +
                        '" -ReportRoot "' + $ReportRoot + '" -ExpectedHead "' + $Sha + '"'
                    $CatalystProcess = Start-Process -FilePath $PsExe -ArgumentList $CatalystArguments -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput $CatalystOut -RedirectStandardError $CatalystErr -PassThru -Wait -ErrorAction Stop
                    if ($CatalystProcess.ExitCode -eq 0) {
                        Write-Host '[ARMEMENT01.17.2] Catalyst native asset published: next commit receives full validation.'
                    }
                    elseif ($CatalystProcess.ExitCode -ne 2) {
                        Write-Warning 'Catalyst native asset authoring incomplete; private logs only.'
                    }
                }
                catch { Write-Warning 'Catalyst authoring unavailable; private logs only.' }
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