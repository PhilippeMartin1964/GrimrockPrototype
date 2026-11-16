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
    param([string]$Sha, [string]$State, [string]$Description, [string]$Context = 'UE5.5.4/LocalAuto')
    $Description = $Description.Substring(0, [Math]::Min(135, $Description.Length))
    & gh api -X POST "repos/$GitHubRepository/statuses/$Sha" -f "state=$State" -f "context=$Context" -f "description=$Description" --silent
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
            # C1083 is a file-open failure, not necessarily a source-code error.
            # Publish ONLY fixed categories; never the file path or raw message.
            if ($Line -match '(?i)\b(?:fatal\s+)?error\s+C1083\b') {
                $FileKind = 'UNSPECIFIED_FILE'
                if ($Line -match '(?i)cannot\s+open\s+compiler\s+generated\s+file') {
                    $FileKind = 'COMPILER_OUTPUT'
                }
                elseif ($Line -match '(?i)cannot\s+open\s+precompiled\s+header\s+file') {
                    $FileKind = 'PCH_FILE'
                }
                elseif ($Line -match '(?i)cannot\s+open\s+include\s+file') {
                    $FileKind = if ($Line -match '(?i)\.generated\.h\b') { 'GENERATED_HEADER' } else { 'INCLUDE_FILE' }
                }
                elseif ($Line -match '(?i)cannot\s+open\s+source\s+file') {
                    $FileKind = 'SOURCE_FILE'
                }
                # C1083 can say only "Cannot open file" without source/include.
                # Classify its extension, NEVER disclose the path or basename.
                if ($FileKind -eq 'UNSPECIFIED_FILE') {
                    if ($Line -match '(?i)\.generated\.h\b') {
                        $FileKind = 'GENERATED_HEADER'
                    }
                    elseif ($Line -match '(?i)\.(?:cpp|cxx|cc|c)\b') {
                        $FileKind = 'SOURCE_FILE'
                    }
                    elseif ($Line -match '(?i)\.(?:hpp|h|inl)\b') {
                        $FileKind = 'INCLUDE_FILE'
                    }
                    elseif ($Line -match '(?i)\.(?:obj|pch|pdb|idb|ipch|tlog|ilk|exp)\b') {
                        $FileKind = 'COMPILER_OUTPUT'
                    }
                    elseif ($Line -match '(?i)\.(?:rsp|response)\b') {
                        $FileKind = 'RESPONSE_FILE'
                    }
                }
                $Reason = 'UNKNOWN_REASON'
                if ($Line -match '(?i)no\s+such\s+file\s+or\s+directory|file\s+not\s+found') {
                    $Reason = 'NOT_FOUND'
                }
                elseif ($Line -match '(?i)permission\s+denied|access\s+is\s+denied|sharing\s+violation') {
                    $Reason = 'ACCESS_DENIED'
                }
                elseif ($Line -match '(?i)no\s+space\s+left|disk\s+full|not\s+enough\s+space') {
                    $Reason = 'NO_SPACE'
                }
                $Record = 'Compilateur : C1083 / ' + $FileKind + ' / ' + $Reason
                if (-not $Safe.Contains($Record)) { $Safe.Add($Record) }
                # A basename with a C++ extension is safe and distinguishes a
                # missing tracked source from a generated include. No path,
                # directory, line contents or arbitrary quoted value leaves PC.
                $ProjectFileMatches = [regex]::Matches($Line,
                    '(?i)\b(?<name>[A-Za-z_][A-Za-z0-9_.-]{0,95}\.(?:cpp|cxx|cc|h|hpp|inl))\b')
                if ($ProjectFileMatches.Count -gt 0) {
                    $BaseName = $ProjectFileMatches[$ProjectFileMatches.Count - 1].Groups['name'].Value
                    $FileRecord = 'Compilateur : C1083 / Projet=' + $BaseName
                    if (-not $Safe.Contains($FileRecord)) { $Safe.Add($FileRecord) }
                }
                continue
            }
            # Linker/UHT may fail without producing file(line) : error Cxxxx.
            # Only known tool error codes and static categories may leave Windows.
            if ($Line -match '(?i)\b(?:fatal\s+)?error\s+(?<code>C\d{4}|CS\d{4}|MSB\d{4}|LNK\d{4})\b') {
                $Record = 'Compilateur : ' + $Matches['code'].ToUpperInvariant()
                if (-not $Safe.Contains($Record)) { $Safe.Add($Record) }
            }
            if ($Line -match '(?i)Windows\s+(?<sdk>10\.0\.\d{4,6}\.\d{1,5})\s+SDK') {
                $Record = 'Environnement : UBT_SDK=' + $Matches['sdk']
                if (-not $Safe.Contains($Record)) { $Safe.Add($Record) }
            }
            if ($Line -match '(?i)\bUnrealHeaderTool\b' -and $Line -match '(?i)\berror\b') {
                if (-not $Safe.Contains('Compilation : UHT_ERROR')) { $Safe.Add('Compilation : UHT_ERROR') }
            }
            if ($Line -match '(?i)EXCEPTION_ACCESS_VIOLATION|Unhandled Exception: EXCEPTION|Unhandled Exception: System.OutOfMemoryException') {
                if (-not $Safe.Contains('Processus Unreal : CRASH_OR_UNHANDLED_EXCEPTION')) {
                    $Safe.Add('Processus Unreal : CRASH_OR_UNHANDLED_EXCEPTION')
                }
            }
            if ($Line -match '^\s*Failed\s*:\s*(\d{1,5})\s*$') {
                $Record = 'Tests Automation en echec : ' + $Matches[1]
                if (-not $Safe.Contains($Record)) { $Safe.Add($Record) }
            }
        }
    }
    # C1083 on assert.h/stddef.h may be missing Windows SDK UCRT headers
    # rather than a project include. Check the documented installed toolchain
    # and SDK directories with booleans only; NEVER disclose actual paths.
    if (@($Safe | Where-Object { $_ -like 'Compilateur : C1083*' }).Count -gt 0) {
        $VsRoot = Join-Path $env:ProgramFiles 'Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.44.35207'
        $VcClPresent = Test-Path -LiteralPath (Join-Path $VsRoot 'bin\Hostx64\x64\cl.exe') -PathType Leaf
        $VcIncludePresent = Test-Path -LiteralPath (Join-Path $VsRoot 'include\type_traits') -PathType Leaf
        $KitRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\Include'
        $KnownUcrt = Join-Path $KitRoot '10.0.28000.0\ucrt'
        $KnownAssert = Test-Path -LiteralPath (Join-Path $KnownUcrt 'assert.h') -PathType Leaf
        $KnownStddef = Test-Path -LiteralPath (Join-Path $KnownUcrt 'stddef.h') -PathType Leaf
        $AnyUcrt = $false
        if (Test-Path -LiteralPath $KitRoot -PathType Container) {
            foreach ($SdkDir in @(Get-ChildItem -LiteralPath $KitRoot -Directory -ErrorAction SilentlyContinue)) {
                if ((Test-Path -LiteralPath (Join-Path $SdkDir.FullName 'ucrt\assert.h') -PathType Leaf) -and
                    (Test-Path -LiteralPath (Join-Path $SdkDir.FullName 'ucrt\stddef.h') -PathType Leaf)) {
                    $AnyUcrt = $true
                    break
                }
            }
        }
        $Safe.Add('Environnement : MSVC14.44_CL=' + $(if ($VcClPresent) { 'PRESENT' } else { 'ABSENT' }))
        $Safe.Add('Environnement : MSVC14.44_CPP=' + $(if ($VcIncludePresent) { 'PRESENT' } else { 'ABSENT' }))
        $Safe.Add('Environnement : SDK28000_UCRT_ASSERT=' + $(if ($KnownAssert) { 'PRESENT' } else { 'ABSENT' }))
        $Safe.Add('Environnement : SDK28000_UCRT_STDDEF=' + $(if ($KnownStddef) { 'PRESENT' } else { 'ABSENT' }))
        $Safe.Add('Environnement : ANY_SDK_UCRT_PAIR=' + $(if ($AnyUcrt) { 'PRESENT' } else { 'ABSENT' }))
    }
    # Project-local SDK choice is safe to report as a version number only.
    $LocalSdkXml = Join-Path $WorktreePath 'Saved\UnrealBuildTool\BuildConfiguration.xml'
    if (Test-Path -LiteralPath $LocalSdkXml -PathType Leaf) {
        $SdkText = Get-Content -LiteralPath $LocalSdkXml -Raw -ErrorAction SilentlyContinue
        if ($SdkText -match '<WindowsSdkVersion>(?<sdk>10\.0\.\d{4,6}\.\d{1,5})</WindowsSdkVersion>') {
            $Safe.Add('Environnement : LOCAL_SDK=' + $Matches['sdk'])
        }
    }
    # Stage emitted by the exact same ValidateUE.ps1 invocation. Only the
    # fixed allowlist is published; never expose the private stage file path.
    $ValidationStage = Join-Path $ReportRoot 'Validation.stage'
    if (Test-Path -LiteralPath $ValidationStage -PathType Leaf) {
        $Stage = ([string](Get-Content -LiteralPath $ValidationStage -TotalCount 1 -ErrorAction SilentlyContinue)).Trim()
        $KnownStages = @('PREFLIGHT','BUILD_STARTED','BUILD_FAILED','BUILD_OK',
            'AUTOMATION_STARTED','AUTOMATION_PROCESS_EXITED',
            'AUTOMATION_REPORT_MISSING','AUTOMATION_REPORT_READING',
            'AUTOMATION_REPORT_OK','AUTOMATION_NO_TESTS',
            'AUTOMATION_TESTS_FAILED','AUTOMATION_EXIT_NONZERO','SUCCESS')
        if ($Stage -cin $KnownStages) {
            $Safe.Add('Validation : phase ' + $Stage)
        }
        else {
            $Safe.Add('Validation : phase UNKNOWN')
        }
    }
    else {
        $Safe.Add('Validation : phase NOT_RECORDED')
    }
    # An exact integer exit code is safe and distinguishes a missing report
    # from a native Editor-Cmd termination. Reject any free-form contents.
    $EditorExitCodePath = Join-Path $ReportRoot 'Automation.exitcode'
    if (Test-Path -LiteralPath $EditorExitCodePath -PathType Leaf) {
        $EditorExit = ([string](Get-Content -LiteralPath $EditorExitCodePath -TotalCount 1 -ErrorAction SilentlyContinue)).Trim()
        if ($EditorExit -match '^-?\d{1,10}$') {
            $Safe.Add('UnrealEditor-Cmd : sortie ' + $EditorExit)
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
                        # ARM-CLEAN06.1: only a fixed, non-sensitive error
                        # classification may leave the private Automation report.
                        if ($TestPath -ceq 'Grimrock.RPG.ARM.CLEAN06_1.SingleResolver.AmmoAfterRegister' -and
                            $State -match '^(?i:fail|failed|error)$' -and
                            $Test.PSObject.Properties['entries']) {
                            foreach ($Entry in @($Test.entries)) {
                                if ($null -eq $Entry -or $null -eq $Entry.event) { continue }
                                if ([string]$Entry.event.type -notmatch '^(?i:error|fatal|warning)$') { continue }
                                $Message = [string]$Entry.event.message
                                $Category = 'OTHER'
                                if ($Message -match 'Register fixture') { $Category = 'REGISTRY' }
                                elseif ($Message -match 'Canonical subsystem remains rooted') { $Category = 'SUBSYSTEM_LIFETIME' }
                                elseif ($Message -match 'Authored ammo kind after registration') { $Category = 'AMMO_MUTATION' }
                                elseif ($Message -match 'Ensure condition failed|ensure condition|EnsureCondition') { $Category = 'UNREAL_ENSURE' }
                                $Record = 'ARM-CLEAN06.1 AmmoAfterRegister : ' + $Category
                                if (-not $Safe.Contains($Record)) { $Safe.Add($Record) }
                            }
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
    # UE can terminate without index.json (crash, abrupt Quit, worker abort).
    # Read private editor logs but export ONLY whitelisted test paths and
    # fixed state/categories, never raw text or local file paths.
    if (Test-Path -LiteralPath $ReportRoot -PathType Container) {
        $AutomationLogs = @(
            Get-ChildItem -LiteralPath $ReportRoot -Filter 'Automation.log' -File -Recurse -ErrorAction SilentlyContinue |
                Sort-Object LastWriteTimeUtc | Select-Object -Last 1
        )
        if ($AutomationLogs.Count -eq 0) {
            $AutomationLogs = @(
                Get-ChildItem -LiteralPath $ReportRoot -Filter 'Automation.console.log' -File -Recurse -ErrorAction SilentlyContinue |
                    Sort-Object LastWriteTimeUtc | Select-Object -Last 1
            )
        }
        if ($AutomationLogs.Count -gt 0) {
            $Safe.Add('Automation : journal présent')
            $LastStarted = ''
            $LastFinished = ''
            $LastFinishState = ''
            $TestCompleteCode = ''
            $bFatal = $false
            $bUnrealEnsure = $false
            $bAutomationCommandSeen = $false
            foreach ($RawLine in @(Get-Content -LiteralPath $AutomationLogs[-1].FullName -Tail 12000 -ErrorAction SilentlyContinue)) {
                $Line = [string]$RawLine
                if ($Line -match '(?i)Automation RunTest') { $bAutomationCommandSeen = $true }
                if ($Line -match '(?i)Fatal error:|Assertion failed:|Critical error:|EXCEPTION_ACCESS_VIOLATION|Unhandled Exception') {
                    $bFatal = $true
                }
                if ($Line -match '(?i)Ensure condition failed:') { $bUnrealEnsure = $true }
                $Started = [regex]::Match($Line, 'Test Started\.\s*Name=\{[^}]{1,128}\}\s*Path=\{(?<test>Grimrock\.[A-Za-z0-9_.]{1,180})\}')
                if ($Started.Success) { $LastStarted = $Started.Groups['test'].Value }
                $Finished = [regex]::Match($Line,
                    'Test Completed\.\s*Result=\{(?<state>Success|Failed|Fail|Error|Passed|Aborted)\}.*?Path=\{(?<test>Grimrock\.[A-Za-z0-9_.]{1,180})\}')
                if ($Finished.Success) {
                    $LastFinished = $Finished.Groups['test'].Value
                    $LastFinishState = $Finished.Groups['state'].Value.ToUpperInvariant()
                }
                $Completed = [regex]::Match($Line, '\*\*\*\* TEST COMPLETE\. EXIT CODE:\s*(?<code>\d{1,4})')
                if ($Completed.Success) { $TestCompleteCode = $Completed.Groups['code'].Value }
            }
            if ($bAutomationCommandSeen) { $Safe.Add('Automation : commande détectée') }
            if ($LastStarted) { $Safe.Add('Automation : dernier test démarré ' + $LastStarted) }
            if ($LastFinished) { $Safe.Add('Automation : dernier test terminé ' + $LastFinished + ' / ' + $LastFinishState) }
            if ($TestCompleteCode) { $Safe.Add('Automation : fin de suite code ' + $TestCompleteCode) }
            if ($bFatal) { $Safe.Add('Unreal : FATAL_OR_ASSERTION') }
            if ($bUnrealEnsure) { $Safe.Add('Unreal : ENSURE_CONDITION_FAILED') }
        }
        else {
            $Safe.Add('Automation : journal absent')
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
    # ARMEMENT01.17.2A: allowlisted catalyst phase only, never private
    # commandlet stdout/stderr, user paths or unrestricted exception messages.
    $CatalystRunDir = Split-Path -Parent $RunLogPath
    $CatalystStagePath = Join-Path $CatalystRunDir 'CatalystAuthoring.stage'
    if (Test-Path -LiteralPath $CatalystStagePath -PathType Leaf) {
        $CatalystStage = [string](Get-Content -LiteralPath $CatalystStagePath -TotalCount 1 -ErrorAction SilentlyContinue)
        $KnownCatalystStages = @('PREFLIGHT','WORKTREE','CLEAN','HEAD','REMOTE',
            'TARGET_ABSENT','LFS','REPORT_FILE','REPORT_PARSE','REPORT_FILTER',
            'REPORT_ACCEPTED','COMMANDLET_STARTED','COMMANDLET_FAILED',
            'COMMANDLET_COMPLETED','FILESET','ASSET_VERIFIED','COMMIT_CREATED','SUCCESS')
        if ($CatalystStage -in $KnownCatalystStages) {
            $Safe.Add('Catalyseur : phase ' + $CatalystStage)
        }
        else {
            $Safe.Add('Catalyseur : phase inconnue')
        }
    }
    if ($Safe.Count -eq 0) {
        $Safe.Add('Pas de diagnostic public fiable ; seuls les journaux locaux sont exhaustifs.')
    }
    return @($Safe | Select-Object -First 20)
}

# EFFECTS-ORG01.1: publish only vetted project package references from
# the read-only Asset Registry CSV. Local logs and licensed vendor paths stay private.
function Publish-EffectsInventoryIssue {
    param([string]$Sha, [string]$WorktreePath, [datetime]$NotBeforeUtc)

    # Exactly this inventory milestone, never unrelated builds.
    $Subject = @(& git -C $WorktreePath log -1 '--format=%s')
    if ($LASTEXITCODE -ne 0 -or $Subject.Count -ne 1 -or
        $Subject[0] -notmatch '^EFFECTS-ORG01[.]1:') { return }

    $CsvPath = Join-Path $WorktreePath 'Saved\Audits\EFFECTS-ORG01\EffectsAssetRegistry.csv'
    if (-not (Test-Path -LiteralPath $CsvPath -PathType Leaf)) { return }
    $File = Get-Item -LiteralPath $CsvPath
    if ($File.Length -gt 1048576 -or $File.LastWriteTimeUtc -lt $NotBeforeUtc) {
        Write-Warning '[EFFECTS-ORG01.1] Rapport manquant, obsolete ou trop volumineux.'
        return
    }

    $Rows = @(Import-Csv -LiteralPath $CsvPath -Encoding UTF8)
    if ($Rows.Count -lt 10 -or $Rows.Count -gt 200) {
        Write-Warning '[EFFECTS-ORG01.1] Cardinalite inattendue.'
        return
    }

    # Only this fixed package namespace, no vendor, absolute local path or free text.
    $Pattern = '^/Game/GrimrockPrototype/[A-Za-z0-9_/-]+$'
    $Body = New-Object 'System.Collections.Generic.List[string]'
    $Body.Add('# EFFECTS-ORG01.1 - Asset Registry')
    $Body.Add('')
    $Body.Add("- Commit : $Sha")
    $Body.Add('- Resultat du test UE 5.5.4 en lecture seule.')
    $Body.Add('- Chemins autorises exclusivement : /Game/GrimrockPrototype/.')
    $Body.Add('- Les chemins Windows, packs tiers, journaux et valeurs inconnues ne sont pas publies.')
    $Body.Add('- Zero referent dans AssetRegistry ne prouve pas qu un asset est inutilise.')
    $Body.Add('')
    $Body.Add('| Package | Referents internes | Dependances internes |')
    $Body.Add('|---|---|---|')
    $Count = 0

    foreach ($Row in $Rows) {
        $Package = [string]$Row.Package
        if ($Package.Length -gt 240 -or $Package -cnotmatch $Pattern) { continue }
        $Refs = New-Object 'System.Collections.Generic.List[string]'
        $Deps = New-Object 'System.Collections.Generic.List[string]'
        foreach ($Name in @(([string]$Row.Referencers) -split ';\s*')) {
            if ($Name.Length -le 240 -and $Name -cmatch $Pattern -and $Refs.Count -lt 12) {
                $Refs.Add($Name)
            }
        }
        foreach ($Name in @(([string]$Row.Dependencies) -split ';\s*')) {
            if ($Name.Length -le 240 -and $Name -cmatch $Pattern -and $Deps.Count -lt 12) {
                $Deps.Add($Name)
            }
        }
        $Body.Add("| $Package | $($Refs -join ', ') | $($Deps -join ', ') |")
        ++$Count
    }
    if ($Count -lt 10) {
        Write-Warning '[EFFECTS-ORG01.1] Aucun inventaire publiable.'
        return
    }
    $Body.Add('')
    $Body.Add("Packages publics controles : $Count")
    $Body.Add('Le graphe complet des assets reste dans le rapport local Windows.')

    $Path = Join-Path $StateRoot ("effects-org01-" + $Sha.Substring(0,12) + ".md")
    $Body | Set-Content -LiteralPath $Path -Encoding UTF8
    $Title = "[EFFECTS-ORG01.1] Inventaire AssetRegistry " + $Sha.Substring(0,12)
    & gh issue create --repo $GitHubRepository --title $Title --body-file $Path | Out-Null
    if ($LASTEXITCODE -ne 0) {
        Write-Warning '[EFFECTS-ORG01.1] Publication issue impossible ; rapport reste local.'
    }
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
    # VALIDATION-AUTO01.11: the already-scheduled agent migrates its OWN task.
    # Fetch the helper from this exact remote SHA; never require a local git pull.
    # Run before the cached-commit fast path so a failed migration can recover.
    $SilentReceipt = Join-Path $StateRoot 'SilentTaskVerified.sha'
    try {
        $SilentHelper = Join-Path $StateRoot 'UEValidationSilentTask.ps1'
        $SilentTemp = $SilentHelper + '.tmp'
        try {
            $VersionedHelper = $Sha + ':Scripts/UEValidationSilentTask.ps1'
            Invoke-Checked -Executable 'git' -Arguments @('-C', $RepositoryRoot, 'show', $VersionedHelper) |
                Set-Content -LiteralPath $SilentTemp -Encoding UTF8 -ErrorAction Stop
            $HelperTokens = $null
            $HelperErrors = $null
            [void][System.Management.Automation.Language.Parser]::ParseFile(
                $SilentTemp, [ref]$HelperTokens, [ref]$HelperErrors)
            if ($null -ne $HelperErrors -and $HelperErrors.Count -gt 0) {
                throw 'SilentTask helper syntax invalid.'
            }
            Move-Item -LiteralPath $SilentTemp -Destination $SilentHelper -Force -ErrorAction Stop
        }
        finally {
            Remove-Item -LiteralPath $SilentTemp -Force -ErrorAction SilentlyContinue
        }
        . $SilentHelper
        $SilentResult = Set-UEValidationSilentTask -RepositoryRoot $RepositoryRoot -StateRoot $StateRoot
        $ReportedSha = if (Test-Path -LiteralPath $SilentReceipt -PathType Leaf) {
            ([string](Get-Content -LiteralPath $SilentReceipt -TotalCount 1)).Trim()
        } else { '' }
        if ($SilentResult -eq 'MIGRATED' -or $ReportedSha -cne $Sha) {
            Publish-CommitStatus -Sha $Sha -State 'success' -Context 'UE5.5.4/SilentTask' `
                -Description 'Action wscript silencieuse installee et verifiee'
            Set-Content -LiteralPath $SilentReceipt -Value $Sha -Encoding ASCII
        }
        Write-Host "[VALIDATION-AUTO01.11] Silent task: $SilentResult"
    }
    catch {
        Write-Warning ('[VALIDATION-AUTO01.11] Migration non confirmee: ' + $_.Exception.Message)
        try {
            Publish-CommitStatus -Sha $Sha -State 'failure' -Context 'UE5.5.4/SilentTask' `
                -Description 'Migration du lanceur silencieux non confirmee'
        }
        catch { Write-Warning '[VALIDATION-AUTO01.11] Publication du statut impossible.' }
    }

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
    $FailureStatusPublished = $false
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
            $Dirty = @(Invoke-Checked -Executable 'git' -Arguments @('-C', $WorktreePath, 'status', '--porcelain', '--untracked-files=all'))

            # ARM-CLEAN06.4U: one-shot recovery at the actual worktree
            # pre-check, before the older Catalyst/Oil recovery paths below.
            # Exact 5 outputs of the previously failed -Verify run only;
            # each byte is backed up outside the Git worktree first.
            $U4PriorHeads = @(
                'a414ee67c933c0c4754db649ca79aa9b51c67f6d',
                'b24a7d72cf57c79b3db59960d45a537b0b347273',
                '11f33948adbee45e77b8cee1315defb523d1eb7f'
            )
            $U4Parent = 'fcf13df12f31e5da4018fc10d2b621d84212b33e'
            $U4Subject = @(& git -C $RepositoryRoot show -s --format=%s $Sha)
            $U4ParentActual = @(& git -C $RepositoryRoot rev-parse ($Sha + '^'))
            $U4PreviousWorktreeHead = @(& git -C $WorktreePath rev-parse HEAD)
            $U4Prev = if ($U4PreviousWorktreeHead.Count -eq 1) {
                ([string]$U4PreviousWorktreeHead[0]).Trim()
            } else { '' }
            if ($LASTEXITCODE -eq 0 -and $U4Subject.Count -eq 1 -and
                $U4ParentActual.Count -eq 1 -and $U4PreviousWorktreeHead.Count -eq 1 -and
                ([string]$U4Subject[0]).Trim() -ceq
                    'feat(ARM-CLEAN06.4U): migrate authored nonarm QuickItems into real action catalogue' -and
                ([string]$U4ParentActual[0]).Trim() -ceq $U4Parent -and
                $U4Prev -cin $U4PriorHeads -and
                $Dirty.Count -eq 5) {
                $U4Exact = [ordered]@{
                    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ActionCatalogue.uasset' = ' M'
                    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.uasset' = ' M'
                    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/DA_Canonical_Item_Flask_Oil.uasset' = '??'
                    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/DA_Canonical_Item_Potion_Healing_01.uasset' = '??'
                    'Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/DA_Canonical_Item_SurfaceCatalyst_01.uasset' = '??'
                }
                $U4Allowed = $true
                $U4Seen = @{}
                foreach ($U4Entry in $Dirty) {
                    $U4Entry = [string]$U4Entry
                    if ($U4Entry.Length -lt 4) { $U4Allowed = $false; break }
                    $U4Rel = $U4Entry.Substring(3).Replace('\','/')
                    $U4Status = $U4Entry.Substring(0,2)
                    if (-not $U4Exact.Contains($U4Rel) -or
                        $U4Exact[$U4Rel] -cne $U4Status -or
                        $U4Seen.ContainsKey($U4Rel)) {
                        $U4Allowed = $false
                        break
                    }
                    $U4Seen[$U4Rel] = $true
                }
                if ($U4Allowed -and $U4Seen.Count -eq 5) {
                    $U4Backup = Join-Path $StateRoot ('Recovery\ARM4U-NativeReopen-' +
                        $U4Prev.Substring(0,12) + '-' + (Get-Date -Format 'yyyyMMddHHmmssfff'))
                    $U4Copies = @()
                    foreach ($U4Relative in $U4Exact.Keys) {
                        $U4Real = Join-Path $WorktreePath ($U4Relative.Replace('/','\'))
                        $U4RelativeBackup = Join-Path $U4Backup ($U4Relative.Replace('/','\'))
                        if (-not (Test-Path -LiteralPath $U4Real -PathType Leaf) -or
                            (((Get-Item -LiteralPath $U4Real).Attributes -band
                                [IO.FileAttributes]::ReparsePoint) -ne 0)) {
                            throw 'ARM-CLEAN06.4U: known generated asset missing or reparse point; no recovery.'
                        }
                        New-Item -ItemType Directory -Path (Split-Path -Parent $U4RelativeBackup) -Force -ErrorAction Stop | Out-Null
                        Copy-Item -LiteralPath $U4Real -Destination $U4RelativeBackup -ErrorAction Stop
                        if ((Get-FileHash -LiteralPath $U4Real -Algorithm SHA256).Hash -cne
                            (Get-FileHash -LiteralPath $U4RelativeBackup -Algorithm SHA256).Hash) {
                            throw 'ARM-CLEAN06.4U: private recovery SHA256 verification failed.'
                        }
                        $U4Copies += $U4Relative
                    }
                    if ($U4Copies.Count -ne 5) {
                        throw 'ARM-CLEAN06.4U: private recovery incomplete.'
                    }
                    $U4CheckAgain = @(Invoke-Checked -Executable 'git' -Arguments @(
                        '-C', $WorktreePath, 'status', '--porcelain', '--untracked-files=all'))
                    if ((@($U4CheckAgain) -join '|') -cne (@($Dirty) -join '|')) {
                        throw 'ARM-CLEAN06.4U: concurrent validation worktree changes; no recovery.'
                    }
                    foreach ($U4Relative in $U4Exact.Keys) {
                        if ($U4Exact[$U4Relative] -ceq '??') {
                            Remove-Item -LiteralPath (Join-Path $WorktreePath ($U4Relative.Replace('/','\'))) -Force -ErrorAction Stop
                        }
                        else {
                            Invoke-Checked -Executable 'git' -Arguments @(
                                '-C', $WorktreePath, 'restore', '--source=HEAD',
                                '--worktree', '--', $U4Relative) | Out-Null
                        }
                    }
                    $Dirty = @(Invoke-Checked -Executable 'git' -Arguments @(
                        '-C', $WorktreePath, 'status', '--porcelain', '--untracked-files=all'))
                    if ($Dirty.Count -ne 0) {
                        throw 'ARM-CLEAN06.4U: residual validation worktree changes after targeted recovery.'
                    }
                    Write-Host '[ARM-CLEAN06.4U] Exact five native assets backed up by SHA256; validation worktree restored.'
                }
            }

            # ARMEMENT01.17.2C: a failed native commandlet may leave a partial
            # asset batch. Recover ONLY its five known new binaries and the one
            # dedicated catalyst item, in the isolated validation worktree.
            # Create SHA256-verified private backups BEFORE restoring/deleting.
            $CatalystItem = 'Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_SurfaceCatalyst_01.uasset'
            $CatalystNew = @(
                'Content/GrimrockPrototype/Art/Materials/Catalyst/M_SurfaceCatalyst_Flask.uasset',
                'Content/GrimrockPrototype/Art/Materials/Catalyst/M_SurfaceCatalyst_Ember.uasset',
                'Content/GrimrockPrototype/Art/Materials/Catalyst/M_SurfaceCatalyst_RunePulse.uasset',
                'Content/GrimrockPrototype/Art/Meshes/Catalyst/SM_SurfaceCatalyst_01.uasset',
                'Content/GrimrockPrototype/Art/Textures/Catalyst/T_UI_Item_SurfaceCatalyst_01.uasset'
            )
            if ($Dirty.Count -gt 0) {
                $OnlyCatalystArtifacts = $true
                foreach ($Entry in $Dirty) {
                    if ($Entry.Length -lt 4) { $OnlyCatalystArtifacts = $false; break }
                    $Path = $Entry.Substring(3)
                    if (-not (($Entry.StartsWith('?? ') -and $Path -cin $CatalystNew) -or
                              ($Entry.StartsWith(' M ') -and $Path -ceq $CatalystItem))) {
                        $OnlyCatalystArtifacts = $false; break
                    }
                }
                if ($OnlyCatalystArtifacts) {
                    $OldHead = @(Invoke-Checked -Executable 'git' -Arguments @('-C', $WorktreePath, 'rev-parse', 'HEAD'))[0].Trim()
                    if ($OldHead -notmatch '^[a-f0-9]{40}$') { throw 'Catalyst recovery: invalid old head.' }
                    $BackupRoot = Join-Path $StateRoot ('Recovery\Catalyst-' + $OldHead.Substring(0,12) +
                        '-' + (Get-Date -Format 'yyyyMMddHHmmssfff'))
                    New-Item -ItemType Directory -Path $BackupRoot -Force | Out-Null
                    foreach ($Entry in $Dirty) {
                        $Path = $Entry.Substring(3)
                        $Original = Join-Path $WorktreePath $Path
                        $Backup = Join-Path $BackupRoot ([IO.Path]::GetFileName($Path))
                        if (-not (Test-Path -LiteralPath $Original -PathType Leaf)) {
                            throw 'Catalyst recovery: expected file missing.'
                        }
                        Copy-Item -LiteralPath $Original -Destination $Backup -ErrorAction Stop
                        if ((Get-FileHash -LiteralPath $Original -Algorithm SHA256).Hash -ne
                            (Get-FileHash -LiteralPath $Backup -Algorithm SHA256).Hash) {
                            throw 'Catalyst recovery: backup integrity failed.'
                        }
                    }
                    foreach ($Entry in $Dirty) {
                        $Path = $Entry.Substring(3)
                        if ($Entry.StartsWith('?? ')) {
                            Remove-Item -LiteralPath (Join-Path $WorktreePath $Path) -Force -ErrorAction Stop
                        }
                        else {
                            Invoke-Checked -Executable 'git' -Arguments @(
                                '-C', $WorktreePath, 'restore', '--source=HEAD', '--worktree', '--', $CatalystItem) | Out-Null
                        }
                    }
                    $Dirty = @(Invoke-Checked -Executable 'git' -Arguments @(
                        '-C', $WorktreePath, 'status', '--porcelain', '--untracked-files=normal'))
                    if ($Dirty.Count -gt 0) { throw 'Catalyst recovery: unexpected residual modifications.' }
                    Write-Host '[VALIDATION-AUTO01] Catalyst native partial assets preserved in Recovery and worktree restored.'
                }
            }
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
        # SDK preflight runs only in the detached validation worktree.
        $SdkSelector = Join-Path $WorktreePath 'Scripts\UEValidationSelectSDK.ps1'
        if (-not (Test-Path -LiteralPath $SdkSelector -PathType Leaf)) {
            throw 'SDK_SELECTION: helper missing from checked-out master.'
        }
        $SdkTokens = $null
        $SdkParseErrors = $null
        [void][System.Management.Automation.Language.Parser]::ParseFile(
            $SdkSelector, [ref]$SdkTokens, [ref]$SdkParseErrors)
        if ($SdkParseErrors -and $SdkParseErrors.Count -gt 0) {
            throw 'SDK_SELECTION: helper has invalid PowerShell syntax.'
        }
        & $SdkSelector -WorktreePath $WorktreePath
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
        $EffectsInventoryStartUtc = [DateTime]::UtcNow.AddSeconds(-5)
        $Process = Start-Process -FilePath $PsExe -ArgumentList $ValidateArguments -WorkingDirectory $WorktreePath `
            -RedirectStandardOutput $RunLog -RedirectStandardError $RunStderrLog -WindowStyle Hidden -Wait -PassThru -ErrorAction Stop
        $ExitCode = $Process.ExitCode
        $Result = 'failure'

        # ARM-CLEAN06.2A: one-shot read-only post-audio source audit, after
        # successful UBT build, including when an unrelated Automation test fails.
        $SourceParitySubject = @(& git -C $WorktreePath log -1 --format=%s)
        $SourceParityTrigger = 'chore(ARM-CLEAN06.2A): audit post-audio assets before canonical migration'
        if ($LASTEXITCODE -eq 0 -and $SourceParitySubject.Count -eq 1 -and
            [string]$SourceParitySubject[0] -ceq $SourceParityTrigger) {
            $ParityContext = 'UE5.5.4/ARM-CLEAN06.2A'
            $ParityPhase = 'PREFLIGHT'
            try {
                Publish-CommitStatus -Sha $Sha -State 'pending' -Context $ParityContext -Description 'Audit source ARM 06.2A read-only en cours'
                $ValidationStageFile = Join-Path $ReportRoot 'Validation.stage'
                $BuiltStage = if (Test-Path -LiteralPath $ValidationStageFile -PathType Leaf) {
                    ([string](Get-Content -LiteralPath $ValidationStageFile -TotalCount 1 -ErrorAction SilentlyContinue)).Trim()
                } else { '' }
                if ($BuiltStage -notin @('SUCCESS','AUTOMATION_REPORT_OK',
                    'AUTOMATION_TESTS_FAILED','AUTOMATION_EXIT_NONZERO')) {
                    throw 'ARM-CLEAN06.2A: UE build not confirmed, refusing audit.'
                }
                $ParityScript = Join-Path $WorktreePath 'Scripts\PreflightARM_CLEAN06_2.ps1'
                if (-not (Test-Path -LiteralPath $ParityScript -PathType Leaf)) {
                    throw 'ARM-CLEAN06.2A: dedicated read-only helper missing.'
                }
                $ParityTokens = $null
                $ParityParseErrors = $null
                [void][System.Management.Automation.Language.Parser]::ParseFile(
                    $ParityScript, [ref]$ParityTokens, [ref]$ParityParseErrors)
                if ($ParityParseErrors -and $ParityParseErrors.Count -gt 0) {
                    throw 'ARM-CLEAN06.2A: PowerShell parser rejected script.'
                }
                foreach ($Value in @($ParityScript,$WorktreePath,$EngineRoot,$RunDir)) {
                    if ($Value.Contains('"')) { throw 'ARM-CLEAN06.2A: unsupported path quote.' }
                }
                $ParityPhase = 'START'
                $ParityArgs = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' +
                    $ParityScript + '" -WorktreePath "' + $WorktreePath +
                    '" -EngineRoot "' + $EngineRoot + '" -ExpectedHead "' + $Sha +
                    '" -RunDir "' + $RunDir + '" -GitHubRepository "' + $GitHubRepository + '"'
                $ParityStdout = Join-Path $RunDir 'ARM062A.stdout.log'
                $ParityStderr = Join-Path $RunDir 'ARM062A.stderr.log'
                $ParityProcess = Start-Process -FilePath $PsExe -ArgumentList $ParityArgs -WorkingDirectory $WorktreePath -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput $ParityStdout -RedirectStandardError $ParityStderr -ErrorAction Stop
                if ($ParityProcess.ExitCode -ne 0) {
                    throw 'ARM-CLEAN06.2A: source parity script returned nonzero.'
                }
                Publish-CommitStatus -Sha $Sha -State 'success' -Context $ParityContext -Description 'Inventaire UE 56 assets, 1640 profils, 7 audio, aucune ecriture'
            }
            catch {
                $StagePath = Join-Path $RunDir 'ARMClean062A.stage'
                $ParityStage = if (Test-Path -LiteralPath $StagePath -PathType Leaf) {
                    ([string](Get-Content -LiteralPath $StagePath -TotalCount 1 -ErrorAction SilentlyContinue)).Trim()
                } else { $ParityPhase }
                if ($ParityStage -notin @('PREFLIGHT','BASELINE','AUDIO_DELTA','CURRENT_EXPORT',
                    'DIFF','PROFILE_PARITY_FAILED','CLEAN_CHECK','ISSUE','SUCCESS','START')) {
                    $ParityStage = 'UNKNOWN'
                }
                try {
                    Publish-CommitStatus -Sha $Sha -State 'failure' -Context $ParityContext -Description ('Audit ARM 06.2A bloque : ' + $ParityStage)
                }
                catch { Write-Warning '[ARM-CLEAN06.2A] Status unavailable.' }
                Write-Warning ('[ARM-CLEAN06.2A] Read-only audit blocked: ' + $ParityStage)
            }
        }
        if ($ExitCode -ne 0) { throw "ValidateUE.ps1 a retourne le code $ExitCode. Voir $RunLog et $RunStderrLog." }
        $Phase = 'publication'
        Publish-CommitStatus -Sha $Sha -State 'success' -Description "Build + Automation OK ($AutomationFilter)"
        
        # 06.2AK: one-shot publication of existing UE Automation counters.
        # No new C++ test, commandlet or source asset scan. Only 22 manifest ARM.
        $ReadinessSubject = 'chore(ARM-CLEAN06.2AK): publish canonical action blockers from UE report'
        $ActualSubject = @(& git -C $WorktreePath log -1 --format=%s)
        if ($LASTEXITCODE -eq 0 -and $ActualSubject.Count -eq 1 -and
            [string]$ActualSubject[0] -ceq $ReadinessSubject) {
            $ReadinessContext = 'UE5.5.4/ARM-CLEAN06.2AK'
            try {
                $Pattern = 'ARM 06\.2Q: (?<standard>\d+) standard attacks; (?<custom>\d+) customized attacks preserved;\s*(?<extras>\d+) other locally authored actions preserved; (?<bound>\d+) direct bindings\s+with real PresentationId; (?<noid>\d+) without PresentationId;\s*(?<noprof>\d+) without local presentation; (?<collision>\d+) duplicate presentation keys'
                $Evidence = $null
                foreach ($ReportFile in @(Get-ChildItem -LiteralPath $ReportRoot -File -Recurse -ErrorAction Stop |
                    Where-Object { $_.Name -in @('index.json','Automation.log') })) {
                    $Content = Get-Content -LiteralPath $ReportFile.FullName -Raw -Encoding UTF8
                    if ($Content -match $Pattern) { $Evidence = $Matches; break }
                }
                if (-not $Evidence) { throw '06.2Q Automation evidence not found.' }
                $Standard = [int]$Evidence['standard']; $Custom = [int]$Evidence['custom']
                $Extra = [int]$Evidence['extras']; $Bound = [int]$Evidence['bound']
                $NoKey = [int]$Evidence['noid']; $NoProfile = [int]$Evidence['noprof']
                $Collision = [int]$Evidence['collision']
                if ($Standard + $Custom -ne 19 -or $Bound -gt $Standard -or
                    $Extra -lt 0 -or $NoKey -gt $Standard -or
                    $NoProfile -gt $Standard -or $Collision -gt $Standard) {
                    throw '06.2Q report counters violate known 22-ARM invariants.'
                }
                $Description = "standard=$Standard custom=$Custom extra=$Extra bound=$Bound noKey=$NoKey noFX=$NoProfile collisions=$Collision"
                Publish-CommitStatus -Sha $Sha -State 'success' -Context $ReadinessContext -Description $Description
            }
            catch {
                Publish-CommitStatus -Sha $Sha -State 'failure' -Context $ReadinessContext -Description 'Compteurs 06.2Q absents/incoherents; aucun asset modifie'
            }
        }
        try {
            Publish-EffectsInventoryIssue -Sha $Sha -WorktreePath $WorktreePath -NotBeforeUtc $EffectsInventoryStartUtc
        }
        catch {
            Write-Warning '[EFFECTS-ORG01.1] La publication du rapport est non bloquante.'
        }

        # ARM-CLEAN06.0: one-shot READ-ONLY asset snapshot for this exact commit.
        # No asset save, no extra commit, no change to the development worktree.
        $BaselineTrigger = 'chore(ARM-CLEAN06.0): capture read-only UE asset migration baseline'
        $BaselineSubject = @(& git -C $WorktreePath log -1 --format=%s)
        if ($LASTEXITCODE -eq 0 -and $BaselineSubject.Count -eq 1 -and
            [string]$BaselineSubject[0] -ceq $BaselineTrigger) {
            $BaselineStatus = 'UE5.5.4/ARM-CLEAN06.0'
            $BaselinePhase = 'START'
            try {
                Publish-CommitStatus -Sha $Sha -State 'pending' -Context $BaselineStatus -Description 'Inventaire UE en lecture seule en cours'
                $BaselineScript = Join-Path $WorktreePath 'Scripts\CaptureARM_CLEAN06_Baseline.ps1'
                if (-not (Test-Path -LiteralPath $BaselineScript -PathType Leaf)) {
                    throw 'ARM-CLEAN06.0: baseline script not found.'
                }
                $BaselinePhase = 'PARSE'
                $BaselineTokens = $null
                $BaselineErrors = $null
                [void][System.Management.Automation.Language.Parser]::ParseFile(
                    $BaselineScript, [ref]$BaselineTokens, [ref]$BaselineErrors)
                if ($BaselineErrors -and $BaselineErrors.Count -gt 0) {
                    throw 'ARM-CLEAN06.0: baseline PowerShell syntax invalid.'
                }
                $BaselinePhase = 'PROCESS_LAUNCH'
                $BaselineArgs = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' +
                    $BaselineScript + '" -WorktreePath "' + $WorktreePath +
                    '" -EngineRoot "' + $EngineRoot + '" -ExpectedHead "' + $Sha +
                    '" -RunDir "' + $RunDir + '" -DevelopmentRoot "' + $RepositoryRoot +
                    '" -GitHubRepository "' + $GitHubRepository + '"'
                $BaselineOut = Join-Path $RunDir 'ARM060.stdout.log'
                $BaselineErr = Join-Path $RunDir 'ARM060.stderr.log'
                $BaselinePhase = 'PROCESS_RUNNING'
                $BaselineProcess = Start-Process -FilePath $PsExe -ArgumentList $BaselineArgs -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput $BaselineOut -RedirectStandardError $BaselineErr -PassThru -Wait -ErrorAction Stop
                if ($BaselineProcess.ExitCode -ne 0) {
                    throw 'ARM-CLEAN06.0: baseline capture returned nonzero.'
                }
                Publish-CommitStatus -Sha $Sha -State 'success' -Context $BaselineStatus -Description 'Baseline UE: 56 assets, 1640 profils; issue GitHub publiee'
                Write-Host '[ARM-CLEAN06.0] Read-only baseline completed.'
            }
            catch {
                $BaselineStageFile = Join-Path $RunDir 'ARMClean060.stage'
                $BaselineStage = if (Test-Path -LiteralPath $BaselineStageFile -PathType Leaf) {
                    ([string](Get-Content -LiteralPath $BaselineStageFile -TotalCount 1)).Trim()
                } else { $BaselinePhase }
                if ($BaselineStage -notin @('PREFLIGHT','CHECK_HEAD','HEAD_QUERY_FAILED','HEAD_MISMATCH','CHECK_WORKTREE','CHECK_ENGINE','START','PARSE',
                    'PROCESS_LAUNCH','PROCESS_RUNNING','COMMANDLET','COMMANDLET_FAILED',
                    'VERIFY','READ_JSON','CHECK_COUNTS','BINARY_HASH','SAVE_HASH',
                    'WRITE_RECEIPT','CHECK_CLEAN','ISSUE_BUILD','ISSUE_WRITE',
                    'WORKTREE_CHANGED','PUBLICATION','SUCCESS')) {
                    $BaselineStage = 'UNKNOWN'
                }
                try {
                    Publish-CommitStatus -Sha $Sha -State 'failure' -Context $BaselineStatus -Description ('Baseline non produite : ' + $BaselineStage)
                }
                catch { Write-Warning '[ARM-CLEAN06.0] Status publication failed.' }
                Write-Warning ('[ARM-CLEAN06.0] Extraction incomplete: ' + $BaselineStage)
            }
        }

        # ARM-CLEAN06.4A — explicit one-shot read-only recapture of the real
        # UE baseline, its AssetRegistry graph, 56 hashes and canonical root.
        # Runs only after successful UBT + complete Grimrock Automation.
        # Strictly no SavePackage, no migration, and no commit from this path.
        $Audit4ATrigger = 'chore(ARM-CLEAN06.4A): recapture read-only UE assets and references'
        $Audit4ASubject = @(& git -C $WorktreePath log -1 --format=%s)
        if ($LASTEXITCODE -eq 0 -and $Audit4ASubject.Count -eq 1 -and
            [string]$Audit4ASubject[0] -ceq $Audit4ATrigger) {
            $Audit4AContext = 'UE5.5.4/ARM-CLEAN06.4A'
            $Audit4APhase = 'START'
            try {
                Publish-CommitStatus -Sha $Sha -State 'pending' -Context $Audit4AContext -Description 'Audit UE en lecture seule: 56 assets, referents et source canonique'
                $Audit4AScript = Join-Path $WorktreePath 'Scripts\CaptureARM_CLEAN06_Baseline.ps1'
                if (-not (Test-Path -LiteralPath $Audit4AScript -PathType Leaf)) {
                    throw 'ARM-CLEAN06.4A: read-only capture script absent.'
                }
                $Audit4APhase = 'PARSE'
                $Audit4ATokens = $null
                $Audit4AErrors = $null
                [void][System.Management.Automation.Language.Parser]::ParseFile(
                    $Audit4AScript, [ref]$Audit4ATokens, [ref]$Audit4AErrors)
                if ($Audit4AErrors -and $Audit4AErrors.Count -gt 0) {
                    throw 'ARM-CLEAN06.4A: read-only capture script syntax invalid.'
                }
                $Audit4APhase = 'PROCESS_LAUNCH'
                foreach ($Audit4AValue in @($Audit4AScript,$WorktreePath,$EngineRoot,$RunDir,$RepositoryRoot,$GitHubRepository)) {
                    if ($Audit4AValue.Contains('"')) { throw 'ARM-CLEAN06.4A: unsupported path quote.' }
                }
                $Audit4AArgs = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' +
                    $Audit4AScript + '" -WorktreePath "' + $WorktreePath +
                    '" -EngineRoot "' + $EngineRoot + '" -ExpectedHead "' + $Sha +
                    '" -RunDir "' + $RunDir + '" -DevelopmentRoot "' + $RepositoryRoot +
                    '" -GitHubRepository "' + $GitHubRepository + '" -AuditLabel "ARM-CLEAN06.4A"'
                $Audit4AOut = Join-Path $RunDir 'ARM064A.stdout.log'
                $Audit4AErr = Join-Path $RunDir 'ARM064A.stderr.log'
                $Audit4APhase = 'PROCESS_RUNNING'
                $Audit4AProcess = Start-Process -FilePath $PsExe -ArgumentList $Audit4AArgs -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput $Audit4AOut -RedirectStandardError $Audit4AErr -PassThru -Wait -ErrorAction Stop
                if ($Audit4AProcess.ExitCode -ne 0) {
                    throw 'ARM-CLEAN06.4A: read-only recapture returned nonzero.'
                }
                Publish-CommitStatus -Sha $Sha -State 'success' -Context $Audit4AContext -Description 'Audit UE 56 assets, 1640 profils, 1 root, references et hash en prive'
                Write-Host '[ARM-CLEAN06.4A] Read-only recapture complete; no asset written.'
            }
            catch {
                $Audit4AStageFile = Join-Path $RunDir 'ARMClean064A.stage'
                $Audit4AStage = if (Test-Path -LiteralPath $Audit4AStageFile -PathType Leaf) {
                    ([string](Get-Content -LiteralPath $Audit4AStageFile -TotalCount 1)).Trim()
                } else { $Audit4APhase }
                if ($Audit4AStage -notin @('PREFLIGHT','CHECK_HEAD','HEAD_QUERY_FAILED','HEAD_MISMATCH',
                    'CHECK_WORKTREE','CHECK_ENGINE','START','PARSE','PROCESS_LAUNCH',
                    'PROCESS_RUNNING','COMMANDLET','COMMANDLET_FAILED','VERIFY',
                    'READ_JSON','CHECK_COUNTS','BINARY_HASH','SAVE_HASH',
                    'WRITE_RECEIPT','CANONICAL_BINARY','CHECK_CLEAN','ISSUE_BUILD',
                    'ISSUE_WRITE','WORKTREE_CHANGED','PUBLICATION','SUCCESS')) {
                    $Audit4AStage = 'UNKNOWN'
                }
                try {
                    Publish-CommitStatus -Sha $Sha -State 'failure' -Context $Audit4AContext -Description ('Audit UE non produit : ' + $Audit4AStage)
                }
                catch { Write-Warning '[ARM-CLEAN06.4A] Status publication failed.' }
                Write-Warning ('[ARM-CLEAN06.4A] Read-only capture incomplete: ' + $Audit4AStage)
            }
        }

        # ARM-CLEAN06.4B: exact opt-in, after successful full Grimrock suite.
        # A success status means comparison finished, never migration ready.
        $Compare4BTrigger = 'chore(ARM-CLEAN06.4B): compare UE baselines in guarded dry-run'
        $Compare4BSubject = @(& git -C $WorktreePath log -1 --format=%s)
        if ($LASTEXITCODE -eq 0 -and $Compare4BSubject.Count -eq 1 -and
            [string]$Compare4BSubject[0] -ceq $Compare4BTrigger) {
            $Compare4BContext = 'UE5.5.4/ARM-CLEAN06.4B'
            $Compare4BPhase = 'START'
            try {
                Publish-CommitStatus -Sha $Sha -State 'pending' -Context $Compare4BContext -Description 'Comparaison UE 06.0/06.4A read-only en cours'
                $Compare4BScript = Join-Path $WorktreePath 'Scripts\CompareARM_CLEAN06_4B_Baselines.ps1'
                if (-not (Test-Path -LiteralPath $Compare4BScript -PathType Leaf)) {
                    throw 'ARM-CLEAN06.4B: comparison script missing.'
                }
                $Compare4BPhase = 'PARSE'
                $Compare4BTokens = $null
                $Compare4BErrors = $null
                [void][System.Management.Automation.Language.Parser]::ParseFile(
                    $Compare4BScript, [ref]$Compare4BTokens, [ref]$Compare4BErrors)
                if ($Compare4BErrors -and $Compare4BErrors.Count -gt 0) {
                    throw 'ARM-CLEAN06.4B: invalid comparison PowerShell syntax.'
                }
                foreach ($Compare4BValue in @($Compare4BScript,$WorktreePath,$StateRoot,$RunDir,$GitHubRepository)) {
                    if ($Compare4BValue.Contains('"')) { throw 'ARM-CLEAN06.4B: unsupported path quote.' }
                }
                $Compare4BArgs = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' +
                    $Compare4BScript + '" -WorktreePath "' + $WorktreePath +
                    '" -StateRoot "' + $StateRoot + '" -RunDir "' + $RunDir +
                    '" -ExpectedHead "' + $Sha + '" -GitHubRepository "' + $GitHubRepository + '"'
                $Compare4BOut = Join-Path $RunDir 'ARM064B.stdout.log'
                $Compare4BErr = Join-Path $RunDir 'ARM064B.stderr.log'
                $Compare4BPhase = 'PROCESS_RUNNING'
                $Compare4BProcess = Start-Process -FilePath $PsExe -ArgumentList $Compare4BArgs -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput $Compare4BOut -RedirectStandardError $Compare4BErr -PassThru -Wait -ErrorAction Stop
                if ($Compare4BProcess.ExitCode -ne 0) {
                    throw 'ARM-CLEAN06.4B: comparison process returned nonzero.'
                }
                Publish-CommitStatus -Sha $Sha -State 'success' -Context $Compare4BContext -Description 'Comparaison complete; rapport prive, migration NON autorisee'
                Write-Host '[ARM-CLEAN06.4B] Private comparison complete; no binary changed.'
            }
            catch {
                $Compare4BStagePath = Join-Path $RunDir 'ARMClean064B.stage'
                $Compare4BStage = if (Test-Path -LiteralPath $Compare4BStagePath -PathType Leaf) {
                    ([string](Get-Content -LiteralPath $Compare4BStagePath -TotalCount 1)).Trim()
                } else { $Compare4BPhase }
                if ($Compare4BStage -notin @('PREFLIGHT','HEAD_MISMATCH','WORKTREE_CHANGED',
                    'READ_BASELINE','READ_CURRENT','COMPARE','COMPARE_MAPS',
                    'COMPARE_ASSETS','COMPARE_ASSET_CLASS','COMPARE_ASSET_PROPERTIES',
                    'COMPARE_ASSET_EDGES','COMPARE_ASSET_HASHES',
                    'COMPARE_FIELDS_MAP','COMPARE_FIELDS_VALUES',
                    'COMPARE_PROFILE_MAP','COMPARE_PROFILE_VALUES',
                    'COMPARE_PROFILES','WRITE_PRIVATE',
                    'CHECK_CLEAN','ISSUE_BUILD','ISSUE_WRITE',
                    'START','PARSE','PROCESS_RUNNING','SUCCESS')) {
                    $Compare4BStage = 'UNKNOWN'
                }
                try {
                    Publish-CommitStatus -Sha $Sha -State 'failure' -Context $Compare4BContext -Description ('Comparaison 06.4B bloquee : ' + $Compare4BStage)
                }
                catch { Write-Warning '[ARM-CLEAN06.4B] Status publication failed.' }
                Write-Warning ('[ARM-CLEAN06.4B] Comparison not completed: ' + $Compare4BStage)
            }
        }

        # ARM-CLEAN06.4C: exact one-shot private qualification of the 06.4B
        # proof. Only public CATEGORY COUNTS, never names or old/new values.
        # SUCCESS = classification generated; never a binary migration grant.
        $Qualify4CTrigger = 'chore(ARM-CLEAN06.4C): classify private UE parity deltas without writes'
        $Qualify4CSubject = @(& git -C $WorktreePath log -1 --format=%s)
        if ($LASTEXITCODE -eq 0 -and $Qualify4CSubject.Count -eq 1 -and
            [string]$Qualify4CSubject[0] -ceq $Qualify4CTrigger) {
            $Qualify4CContext = 'UE5.5.4/ARM-CLEAN06.4C'
            $Qualify4CPhase = 'START'
            try {
                Publish-CommitStatus -Sha $Sha -State 'pending' -Context $Qualify4CContext -Description 'Qualification privee des 8 proprietes et 4 dependances'
                $Qualify4CScript = Join-Path $WorktreePath 'Scripts\QualifyARM_CLEAN06_4C_Deltas.ps1'
                if (-not (Test-Path -LiteralPath $Qualify4CScript -PathType Leaf)) {
                    throw 'ARM-CLEAN06.4C: qualification script missing.'
                }
                $Qualify4CPhase = 'PARSE'
                $Qualify4CTokens = $null
                $Qualify4CErrors = $null
                [void][System.Management.Automation.Language.Parser]::ParseFile(
                    $Qualify4CScript, [ref]$Qualify4CTokens, [ref]$Qualify4CErrors)
                if ($Qualify4CErrors -and $Qualify4CErrors.Count -gt 0) {
                    throw 'ARM-CLEAN06.4C: invalid qualification PowerShell syntax.'
                }
                foreach ($Qualify4CValue in @($Qualify4CScript,$WorktreePath,$StateRoot,$RunDir,$GitHubRepository)) {
                    if ($Qualify4CValue.Contains('"')) { throw 'ARM-CLEAN06.4C: unsupported path quote.' }
                }
                $Qualify4CArgs = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' +
                    $Qualify4CScript + '" -WorktreePath "' + $WorktreePath +
                    '" -StateRoot "' + $StateRoot + '" -RunDir "' + $RunDir +
                    '" -ExpectedHead "' + $Sha + '" -GitHubRepository "' + $GitHubRepository + '"'
                $Qualify4COut = Join-Path $RunDir 'ARM064C.stdout.log'
                $Qualify4CErr = Join-Path $RunDir 'ARM064C.stderr.log'
                $Qualify4CPhase = 'PROCESS_RUNNING'
                $Qualify4CProcess = Start-Process -FilePath $PsExe -ArgumentList $Qualify4CArgs -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput $Qualify4COut -RedirectStandardError $Qualify4CErr -PassThru -Wait -ErrorAction Stop
                if ($Qualify4CProcess.ExitCode -ne 0) {
                    throw 'ARM-CLEAN06.4C: qualification process returned nonzero.'
                }
                Publish-CommitStatus -Sha $Sha -State 'success' -Context $Qualify4CContext -Description 'Qualification issue + rapport prive; conversion NON autorisee'
                Write-Host '[ARM-CLEAN06.4C] Private delta qualification finished, migration gate closed.'
            }
            catch {
                $Qualify4CStagePath = Join-Path $RunDir 'ARMClean064C.stage'
                $Qualify4CStage = if (Test-Path -LiteralPath $Qualify4CStagePath -PathType Leaf) {
                    ([string](Get-Content -LiteralPath $Qualify4CStagePath -TotalCount 1)).Trim()
                } else { $Qualify4CPhase }
                if ($Qualify4CStage -notin @('PREFLIGHT','HEAD_MISMATCH','WORKTREE_CHANGED',
                    'READ_COMPARISON','VERIFY_TOTALS','CLASSIFY','WRITE_PRIVATE',
                    'CHECK_CLEAN','ISSUE_BUILD','ISSUE_WRITE','SUCCESS',
                    'START','PARSE','PROCESS_RUNNING')) {
                    $Qualify4CStage = 'UNKNOWN'
                }
                try {
                    Publish-CommitStatus -Sha $Sha -State 'failure' -Context $Qualify4CContext -Description ('Qualification 06.4C bloquee : ' + $Qualify4CStage)
                }
                catch { Write-Warning '[ARM-CLEAN06.4C] Status publication failed.' }
                Write-Warning ('[ARM-CLEAN06.4C] Read-only qualification incomplete: ' + $Qualify4CStage)
            }
        }

        # ARM-CLEAN06.4D: real UE Editor read-only re-scan of all 56 assets,
        # then compare source UPROPERTY / edges / binary SHA to private 06.4A.
        # The printed mapping is a CANDIDATE, no migration permission.
        $DryRun4DTrigger = 'chore(ARM-CLEAN06.4D): verify live UE mapping candidates without binary writes'
        $DryRun4DSubject = @(& git -C $WorktreePath log -1 --format=%s)
        if ($LASTEXITCODE -eq 0 -and $DryRun4DSubject.Count -eq 1 -and
            [string]$DryRun4DSubject[0] -ceq $DryRun4DTrigger) {
            $DryRun4DContext = 'UE5.5.4/ARM-CLEAN06.4D'
            $DryRun4DPhase = 'START'
            try {
                Publish-CommitStatus -Sha $Sha -State 'pending' -Context $DryRun4DContext -Description 'Dry-run Unreal Editor : 56 assets, 1640 profils, mapping bloque'
                $DryRun4DScript = Join-Path $WorktreePath 'Scripts\PlanARM_CLEAN06_4D_EditorDryRun.ps1'
                if (-not (Test-Path -LiteralPath $DryRun4DScript -PathType Leaf)) {
                    throw 'ARM-CLEAN06.4D: live dry-run script missing.'
                }
                $DryRun4DPhase = 'PARSE'
                $DryRun4DTokens = $null
                $DryRun4DErrors = $null
                [void][System.Management.Automation.Language.Parser]::ParseFile(
                    $DryRun4DScript, [ref]$DryRun4DTokens, [ref]$DryRun4DErrors)
                if ($DryRun4DErrors -and $DryRun4DErrors.Count -gt 0) {
                    throw 'ARM-CLEAN06.4D: invalid PowerShell script.'
                }
                foreach ($DryRun4DValue in @($DryRun4DScript,$WorktreePath,$EngineRoot,$StateRoot,$RunDir,$GitHubRepository)) {
                    if ($DryRun4DValue.Contains('"')) { throw 'ARM-CLEAN06.4D: unsupported path quote.' }
                }
                $DryRun4DArgs = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' +
                    $DryRun4DScript + '" -WorktreePath "' + $WorktreePath +
                    '" -EngineRoot "' + $EngineRoot + '" -StateRoot "' + $StateRoot +
                    '" -RunDir "' + $RunDir + '" -ExpectedHead "' + $Sha +
                    '" -GitHubRepository "' + $GitHubRepository + '"'
                $DryRun4DOut = Join-Path $RunDir 'ARM064D.stdout.log'
                $DryRun4DErr = Join-Path $RunDir 'ARM064D.stderr.log'
                $DryRun4DPhase = 'PROCESS_RUNNING'
                $DryRun4DProcess = Start-Process -FilePath $PsExe -ArgumentList $DryRun4DArgs -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput $DryRun4DOut -RedirectStandardError $DryRun4DErr -PassThru -Wait -ErrorAction Stop
                if ($DryRun4DProcess.ExitCode -ne 0) {
                    throw 'ARM-CLEAN06.4D: live dry-run returned nonzero.'
                }
                Publish-CommitStatus -Sha $Sha -State 'success' -Context $DryRun4DContext -Description 'Live UE dry-run OK; mapping candidat, ecritures toujours interdites'
                Write-Host '[ARM-CLEAN06.4D] Real UE verification complete, migration gate remains closed.'
            }
            catch {
                $DryRun4DStagePath = Join-Path $RunDir 'ARMClean064D.stage'
                $DryRun4DStage = if (Test-Path -LiteralPath $DryRun4DStagePath -PathType Leaf) {
                    ([string](Get-Content -LiteralPath $DryRun4DStagePath -TotalCount 1)).Trim()
                } else { $DryRun4DPhase }
                if ($DryRun4DStage -notin @('PREFLIGHT','HEAD_MISMATCH','WORKTREE_CHANGED',
                    'READ_PRIVATE','UE_CAPTURE','UE_CAPTURE_FAILED','CHECK_LIVE_ALL',
                    'CHECK_LIVE_PROFILES','VERIFY_CHANGED_FOUR','WRITE_PRIVATE',
                    'CHECK_CLEAN','ISSUE_BUILD','ISSUE_WRITE','SUCCESS',
                    'START','PARSE','PROCESS_RUNNING')) {
                    $DryRun4DStage = 'UNKNOWN'
                }
                try {
                    Publish-CommitStatus -Sha $Sha -State 'failure' -Context $DryRun4DContext -Description ('Dry-run 06.4D bloque : ' + $DryRun4DStage)
                }
                catch { Write-Warning '[ARM-CLEAN06.4D] Status publication failed.' }
                Write-Warning ('[ARM-CLEAN06.4D] Live UE dry-run incomplete: ' + $DryRun4DStage)
            }
        }

        # ARM-CLEAN06.4E — explicit one-shot real authored presentation parity.
        # Existing source assets and canonical catalogue are loaded by UE,
        # but never saved. Public issue contains counts only.
        $Parity4ETrigger = 'chore(ARM-CLEAN06.4E): verify real authored presentation parity without writes'
        $Parity4ESubject = @(& git -C $WorktreePath log -1 --format=%s)
        if ($LASTEXITCODE -eq 0 -and $Parity4ESubject.Count -eq 1 -and
            [string]$Parity4ESubject[0] -ceq $Parity4ETrigger) {
            $Parity4EContext = 'UE5.5.4/ARM-CLEAN06.4E'
            $Parity4EPhase = 'START'
            try {
                Publish-CommitStatus -Sha $Sha -State 'pending' -Context $Parity4EContext -Description 'Parite presentations UE chargees; sorties privees uniquement'
                $Parity4EScript = Join-Path $WorktreePath 'Scripts\CheckARM_CLEAN06_4E_PresentationParity.ps1'
                if (-not (Test-Path -LiteralPath $Parity4EScript -PathType Leaf)) {
                    throw 'ARM-CLEAN06.4E: parity script absent.'
                }
                $Parity4EPhase = 'PARSE'
                $Parity4ETokens = $null
                $Parity4EErrors = $null
                [void][System.Management.Automation.Language.Parser]::ParseFile(
                    $Parity4EScript, [ref]$Parity4ETokens, [ref]$Parity4EErrors)
                if ($Parity4EErrors -and $Parity4EErrors.Count -gt 0) {
                    throw 'ARM-CLEAN06.4E: invalid PowerShell syntax.'
                }
                foreach ($Parity4EValue in @($Parity4EScript,$WorktreePath,$EngineRoot,$StateRoot,$RunDir,$GitHubRepository)) {
                    if ($Parity4EValue.Contains('"')) { throw 'ARM-CLEAN06.4E: unsupported path quote.' }
                }
                $Parity4EArgs = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' +
                    $Parity4EScript + '" -WorktreePath "' + $WorktreePath +
                    '" -EngineRoot "' + $EngineRoot + '" -StateRoot "' + $StateRoot +
                    '" -RunDir "' + $RunDir + '" -ExpectedHead "' + $Sha +
                    '" -GitHubRepository "' + $GitHubRepository + '"'
                $Parity4EOut = Join-Path $RunDir 'ARM064E.stdout.log'
                $Parity4EErr = Join-Path $RunDir 'ARM064E.stderr.log'
                $Parity4EPhase = 'PROCESS_RUNNING'
                $Parity4EProcess = Start-Process -FilePath $PsExe -ArgumentList $Parity4EArgs -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput $Parity4EOut -RedirectStandardError $Parity4EErr -PassThru -Wait -ErrorAction Stop
                if ($Parity4EProcess.ExitCode -ne 0) {
                    throw 'ARM-CLEAN06.4E: real UE parity returned nonzero.'
                }
                Publish-CommitStatus -Sha $Sha -State 'success' -Context $Parity4EContext -Description 'Audit presentation termine; migration non autorisee'
                Write-Host '[ARM-CLEAN06.4E] Real presentation parity report published; cutover gate closed.'
            }
            catch {
                $Parity4EStagePath = Join-Path $RunDir 'ARMClean064E.stage'
                $Parity4EStage = if (Test-Path -LiteralPath $Parity4EStagePath -PathType Leaf) {
                    ([string](Get-Content -LiteralPath $Parity4EStagePath -TotalCount 1)).Trim()
                } else { $Parity4EPhase }
                if ($Parity4EStage -notin @('PREFLIGHT','HEAD_MISMATCH','WORKTREE_CHANGED',
                    'READ_4D','UNREAL_PRESENTATION_PARITY','UNREAL_PARITY_FAILED',
                    'VERIFY_RESULT','WRITE_PRIVATE_GATE','CHECK_CLEAN',
                    'ISSUE_BUILD','ISSUE_WRITE','SUCCESS',
                    'START','PARSE','PROCESS_RUNNING')) {
                    $Parity4EStage = 'UNKNOWN'
                }
                try {
                    Publish-CommitStatus -Sha $Sha -State 'failure' -Context $Parity4EContext -Description ('Audit presentation 06.4E bloque : ' + $Parity4EStage)
                }
                catch { Write-Warning '[ARM-CLEAN06.4E] Status publication failed.' }
                Write-Warning ('[ARM-CLEAN06.4E] Real UE audit not completed: ' + $Parity4EStage)
            }
        }

        # ARM-CLEAN06.4F: one-shot private three-way UE source provenance.
        # Baseline 06.0 vs the post-user-audio 06.2A snapshot vs 06.4A,
        # guarded by the 06.4E actual canonical presentation parity.
        $Provenance4FTrigger = 'chore(ARM-CLEAN06.4F): attest source provenance from post-audio UE capture'
        $Provenance4FSubject = @(& git -C $WorktreePath log -1 --format=%s)
        if ($LASTEXITCODE -eq 0 -and $Provenance4FSubject.Count -eq 1 -and
            [string]$Provenance4FSubject[0] -ceq $Provenance4FTrigger) {
            $Provenance4FContext = 'UE5.5.4/ARM-CLEAN06.4F'
            $Provenance4FPhase = 'START'
            try {
                Publish-CommitStatus -Sha $Sha -State 'pending' -Context $Provenance4FContext -Description 'Provenance 06.0 / 06.2A / 06.4A + parite canonique 06.4E'
                $Provenance4FScript = Join-Path $WorktreePath 'Scripts\CompareARM_CLEAN06_4F_ThreeWayProvenance.ps1'
                if (-not (Test-Path -LiteralPath $Provenance4FScript -PathType Leaf)) {
                    throw 'ARM-CLEAN06.4F: source script missing.'
                }
                $Provenance4FPhase = 'PARSE'
                $Provenance4FTokens = $null
                $Provenance4FErrors = $null
                [void][System.Management.Automation.Language.Parser]::ParseFile(
                    $Provenance4FScript, [ref]$Provenance4FTokens, [ref]$Provenance4FErrors)
                if ($Provenance4FErrors -and $Provenance4FErrors.Count -gt 0) {
                    throw 'ARM-CLEAN06.4F: invalid PowerShell syntax.'
                }
                foreach ($Provenance4FValue in @($Provenance4FScript,$WorktreePath,$StateRoot,$RunDir,$GitHubRepository)) {
                    if ($Provenance4FValue.Contains('"')) { throw 'ARM-CLEAN06.4F: unsupported path quote.' }
                }
                $Provenance4FArgs = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' +
                    $Provenance4FScript + '" -WorktreePath "' + $WorktreePath +
                    '" -StateRoot "' + $StateRoot + '" -RunDir "' + $RunDir +
                    '" -ExpectedHead "' + $Sha + '" -GitHubRepository "' + $GitHubRepository + '"'
                $Provenance4FOut = Join-Path $RunDir 'ARM064F.stdout.log'
                $Provenance4FErr = Join-Path $RunDir 'ARM064F.stderr.log'
                $Provenance4FPhase = 'PROCESS_RUNNING'
                $Provenance4FProcess = Start-Process -FilePath $PsExe -ArgumentList $Provenance4FArgs -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput $Provenance4FOut -RedirectStandardError $Provenance4FErr -PassThru -Wait -ErrorAction Stop
                if ($Provenance4FProcess.ExitCode -ne 0) {
                    throw 'ARM-CLEAN06.4F: three-way source check returned nonzero.'
                }
                Publish-CommitStatus -Sha $Sha -State 'success' -Context $Provenance4FContext -Description 'Rapport de provenance UE produit; zero ecriture, gate fermee'
                Write-Host '[ARM-CLEAN06.4F] Three-way source history verified; no migration authorized.'
            }
            catch {
                $Provenance4FStagePath = Join-Path $RunDir 'ARMClean064F.stage'
                $Provenance4FStage = if (Test-Path -LiteralPath $Provenance4FStagePath -PathType Leaf) {
                    ([string](Get-Content -LiteralPath $Provenance4FStagePath -TotalCount 1)).Trim()
                } else { $Provenance4FPhase }
                if ($Provenance4FStage -notin @('PREFLIGHT','HEAD_MISMATCH','WORKTREE_CHANGED',
                    'READ_PROOFS','COMPARE','WRITE_PRIVATE','CHECK_CLEAN',
                    'ISSUE_BUILD','ISSUE_WRITE','SUCCESS','START','PARSE',
                    'PROCESS_RUNNING')) {
                    $Provenance4FStage = 'UNKNOWN'
                }
                try {
                    Publish-CommitStatus -Sha $Sha -State 'failure' -Context $Provenance4FContext -Description ('Provenance 06.4F bloquee : ' + $Provenance4FStage)
                }
                catch { Write-Warning '[ARM-CLEAN06.4F] Status publication failed.' }
                Write-Warning ('[ARM-CLEAN06.4F] Private source comparison incomplete: ' + $Provenance4FStage)
            }
        }

        # ARM-CLEAN06.4G — private 56+28 .uasset backup and restore drill.
        # Runs after full Build+Grimrock success, only on the exact stage
        # commit subject; no original asset or SaveGame writes.
        $Backup4GTrigger = 'chore(ARM-CLEAN06.4G): stage immutable UE binary backups and rollback drill'
        $Backup4GSubject = @(& git -C $WorktreePath log -1 --format=%s)
        if ($LASTEXITCODE -eq 0 -and $Backup4GSubject.Count -eq 1 -and
            [string]$Backup4GSubject[0] -ceq $Backup4GTrigger) {
            $Backup4GContext = 'UE5.5.4/ARM-CLEAN06.4G'
            $Backup4GPhase = 'START'
            try {
                Publish-CommitStatus -Sha $Sha -State 'pending' -Context $Backup4GContext -Description 'Photo privee des 84 binaires; verification des copies et restauration bac a sable'
                $Backup4GScript = Join-Path $WorktreePath 'Scripts\PrepareARM_CLEAN06_4G_BinarySnapshot.ps1'
                if (-not (Test-Path -LiteralPath $Backup4GScript -PathType Leaf)) {
                    throw 'ARM-CLEAN06.4G: snapshot script not found.'
                }
                $Backup4GPhase = 'PARSE'
                $Backup4GTokens = $null
                $Backup4GErrors = $null
                [void][System.Management.Automation.Language.Parser]::ParseFile(
                    $Backup4GScript, [ref]$Backup4GTokens, [ref]$Backup4GErrors)
                if ($Backup4GErrors -and $Backup4GErrors.Count -gt 0) {
                    throw 'ARM-CLEAN06.4G: invalid PowerShell syntax.'
                }
                foreach ($Backup4GValue in @($Backup4GScript,$WorktreePath,$EngineRoot,$StateRoot,$RunDir,$GitHubRepository)) {
                    if ($Backup4GValue.Contains('"')) { throw 'ARM-CLEAN06.4G: unsupported path quote.' }
                }
                $Backup4GArgs = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' +
                    $Backup4GScript + '" -WorktreePath "' + $WorktreePath +
                    '" -EngineRoot "' + $EngineRoot + '" -StateRoot "' + $StateRoot +
                    '" -RunDir "' + $RunDir + '" -ExpectedHead "' + $Sha +
                    '" -GitHubRepository "' + $GitHubRepository + '"'
                $Backup4GOut = Join-Path $RunDir 'ARM064G.stdout.log'
                $Backup4GErr = Join-Path $RunDir 'ARM064G.stderr.log'
                $Backup4GPhase = 'PROCESS_RUNNING'
                $Backup4GProcess = Start-Process -FilePath $PsExe -ArgumentList $Backup4GArgs -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput $Backup4GOut -RedirectStandardError $Backup4GErr -PassThru -Wait -ErrorAction Stop
                if ($Backup4GProcess.ExitCode -ne 0) {
                    throw 'ARM-CLEAN06.4G: guarded backup/restore check returned nonzero.'
                }
                Publish-CommitStatus -Sha $Sha -State 'success' -Context $Backup4GContext -Description '84 copies privees verifiees; restauration testee sans ecriture projet'
                Write-Host '[ARM-CLEAN06.4G] Private binary backup verified; no migration authorized.'
            }
            catch {
                $Backup4GStagePath = Join-Path $RunDir 'ARMClean064G.stage'
                $Backup4GStage = if (Test-Path -LiteralPath $Backup4GStagePath -PathType Leaf) {
                    ([string](Get-Content -LiteralPath $Backup4GStagePath -TotalCount 1)).Trim()
                } else { $Backup4GPhase }
                if ($Backup4GStage -notin @('PREFLIGHT','HEAD_MISMATCH','WORKTREE_CHANGED',
                    'PROVENANCE','BUILD_ALLOWLIST','CHECK_TRACKED','BUILD_LEGACY',
                    'CHECK_CANONICAL_DISCOVERY','CHECK_CANONICAL_NAMES',
                    'CHECK_FINAL_SCOPE','CHECK_OUTPUT_DIR','LIVE_UNREAL','LIVE_UNREAL_FAILED',
                    'COMPARE_LIVE','VERIFY_SOURCE_HASHES','COPY_PRIVATE_SNAPSHOT',
                    'SANDBOX_RESTORE_DRILL','WRITE_RECEIPTS','FINAL_CLEAN',
                    'ISSUE_BUILD','ISSUE_WRITE','SUCCESS','START','PARSE','PROCESS_RUNNING')) {
                    $Backup4GStage = 'UNKNOWN'
                }
                try {
                    Publish-CommitStatus -Sha $Sha -State 'failure' -Context $Backup4GContext -Description ('Sauvegarde 06.4G bloquee : ' + $Backup4GStage)
                }
                catch { Write-Warning '[ARM-CLEAN06.4G] Status publication failed.' }
                Write-Warning ('[ARM-CLEAN06.4G] Binary snapshot incomplete: ' + $Backup4GStage)
            }
        }

        # ARM-CLEAN06.4H — live UE level/palette/receptacle ItemId inventory
        # after full Build + Grimrock. Only private per-object findings;
        # public GitHub issue contains fixed aggregate counters.
        $Identity4HTrigger = 'chore(ARM-CLEAN06.4H): audit real world item identities and save schema read-only'
        $Identity4HSubject = @(& git -C $WorktreePath log -1 --format=%s)
        if ($LASTEXITCODE -eq 0 -and $Identity4HSubject.Count -eq 1 -and
            [string]$Identity4HSubject[0] -ceq $Identity4HTrigger) {
            $Identity4HContext = 'UE5.5.4/ARM-CLEAN06.4H'
            $Identity4HPhase = 'START'
            try {
                Publish-CommitStatus -Sha $Sha -State 'pending' -Context $Identity4HContext -Description 'Audit live UE: niveaux, palette, receptacles et schema SaveGame'
                $Identity4HScript = Join-Path $WorktreePath 'Scripts\AuditARM_CLEAN06_4H_WorldReferences.ps1'
                if (-not (Test-Path -LiteralPath $Identity4HScript -PathType Leaf)) {
                    throw 'ARM-CLEAN06.4H: identity audit script missing.'
                }
                $Identity4HPhase = 'PARSE'
                $Identity4HTokens = $null
                $Identity4HErrors = $null
                [void][System.Management.Automation.Language.Parser]::ParseFile(
                    $Identity4HScript, [ref]$Identity4HTokens, [ref]$Identity4HErrors)
                if ($Identity4HErrors -and $Identity4HErrors.Count -gt 0) {
                    throw 'ARM-CLEAN06.4H: invalid audit script syntax.'
                }
                foreach ($Identity4HValue in @($Identity4HScript,$WorktreePath,$EngineRoot,$StateRoot,$RunDir,$GitHubRepository)) {
                    if ($Identity4HValue.Contains('"')) { throw 'ARM-CLEAN06.4H: unsupported path quote.' }
                }
                $Identity4HArgs = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' +
                    $Identity4HScript + '" -WorktreePath "' + $WorktreePath +
                    '" -EngineRoot "' + $EngineRoot + '" -StateRoot "' + $StateRoot +
                    '" -RunDir "' + $RunDir + '" -ExpectedHead "' + $Sha +
                    '" -GitHubRepository "' + $GitHubRepository + '"'
                $Identity4HOut = Join-Path $RunDir 'ARM064H.stdout.log'
                $Identity4HErr = Join-Path $RunDir 'ARM064H.stderr.log'
                $Identity4HPhase = 'PROCESS_RUNNING'
                $Identity4HProcess = Start-Process -FilePath $PsExe -ArgumentList $Identity4HArgs -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput $Identity4HOut -RedirectStandardError $Identity4HErr -PassThru -Wait -ErrorAction Stop
                if ($Identity4HProcess.ExitCode -ne 0) {
                    throw 'ARM-CLEAN06.4H: live UE item reference audit returned nonzero.'
                }
                Publish-CommitStatus -Sha $Sha -State 'success' -Context $Identity4HContext -Description 'Inventaire UE produit; migration non autorisee, SaveGames non ouverts'
                Write-Host '[ARM-CLEAN06.4H] World reference inventory complete; migration gate remains closed.'
            }
            catch {
                $Identity4HStagePath = Join-Path $RunDir 'ARMClean064H.stage'
                $Identity4HStage = if (Test-Path -LiteralPath $Identity4HStagePath -PathType Leaf) {
                    ([string](Get-Content -LiteralPath $Identity4HStagePath -TotalCount 1)).Trim()
                } else { $Identity4HPhase }
                if ($Identity4HStage -notin @('PREFLIGHT','HEAD_MISMATCH','WORKTREE_CHANGED',
                    'READ_SNAPSHOT','REHASH_SNAPSHOT','UNREAL_AUDIT',
                    'UNREAL_AUDIT_FAILED','VERIFY_RESULT','WRITE_GATE',
                    'CHECK_CLEAN','ISSUE_BUILD','ISSUE_WRITE','SUCCESS',
                    'START','PARSE','PROCESS_RUNNING')) {
                    $Identity4HStage = 'UNKNOWN'
                }
                try {
                    Publish-CommitStatus -Sha $Sha -State 'failure' -Context $Identity4HContext -Description ('Inventaire 06.4H bloque : ' + $Identity4HStage)
                }
                catch { Write-Warning '[ARM-CLEAN06.4H] Status publication failed.' }
                Write-Warning ('[ARM-CLEAN06.4H] World references could not be audited: ' + $Identity4HStage)
            }
        }

        # ARM-CLEAN06.4I: real UE Editor reflection over the exact
        # 95 world references from the 06.4H private audit; group 40 missing
        # occurrences by distinct ItemId, without writes or private disclosure.
        $Distinct4ITrigger = 'chore(ARM-CLEAN06.4I): count distinct unresolved ItemIds and preserve source fields'
        $Distinct4ISubject = @(& git -C $WorktreePath log -1 --format=%s)
        if ($LASTEXITCODE -eq 0 -and $Distinct4ISubject.Count -eq 1 -and
            [string]$Distinct4ISubject[0] -ceq $Distinct4ITrigger) {
            $Distinct4IContext = 'UE5.5.4/ARM-CLEAN06.4I'
            $Distinct4IPhase = 'START'
            try {
                Publish-CommitStatus -Sha $Sha -State 'pending' -Context $Distinct4IContext -Description 'Read-only distinct ItemId UE: 40 references, 84 protected binaries'
                $Distinct4IScript = Join-Path $WorktreePath 'Scripts\AuditARM_CLEAN06_4I_DistinctItemIds.ps1'
                if (-not (Test-Path -LiteralPath $Distinct4IScript -PathType Leaf)) {
                    throw 'ARM-CLEAN06.4I: identity script missing.'
                }
                $Distinct4IPhase = 'PARSE'
                $Distinct4ITokens = $null
                $Distinct4IErrors = $null
                [void][System.Management.Automation.Language.Parser]::ParseFile(
                    $Distinct4IScript, [ref]$Distinct4ITokens, [ref]$Distinct4IErrors)
                if ($Distinct4IErrors -and $Distinct4IErrors.Count -gt 0) {
                    throw 'ARM-CLEAN06.4I: malformed PowerShell.'
                }
                foreach ($Distinct4IValue in @($Distinct4IScript,$WorktreePath,$EngineRoot,$StateRoot,$RunDir,$GitHubRepository)) {
                    if ($Distinct4IValue.Contains('"')) { throw 'ARM-CLEAN06.4I: unsupported path quote.' }
                }
                $Distinct4IArgs = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' +
                    $Distinct4IScript + '" -WorktreePath "' + $WorktreePath +
                    '" -EngineRoot "' + $EngineRoot + '" -StateRoot "' + $StateRoot +
                    '" -RunDir "' + $RunDir + '" -ExpectedHead "' + $Sha +
                    '" -GitHubRepository "' + $GitHubRepository + '"'
                $Distinct4IOut = Join-Path $RunDir 'ARM064I.stdout.log'
                $Distinct4IErr = Join-Path $RunDir 'ARM064I.stderr.log'
                $Distinct4IPhase = 'PROCESS_RUNNING'
                $Distinct4IProcess = Start-Process -FilePath $PsExe -ArgumentList $Distinct4IArgs -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput $Distinct4IOut -RedirectStandardError $Distinct4IErr -PassThru -Wait -ErrorAction Stop
                if ($Distinct4IProcess.ExitCode -ne 0) {
                    throw 'ARM-CLEAN06.4I: UE identity grouping failed.'
                }
                Publish-CommitStatus -Sha $Sha -State 'success' -Context $Distinct4IContext -Description 'Distinct IDs enumerated; source UPROPERTY private; migration blocked'
                Write-Host '[ARM-CLEAN06.4I] Distinct identity audit complete, no asset conversions.'
            }
            catch {
                $Distinct4IStagePath = Join-Path $RunDir 'ARMClean064I.stage'
                $Distinct4IStage = if (Test-Path -LiteralPath $Distinct4IStagePath -PathType Leaf) {
                    ([string](Get-Content -LiteralPath $Distinct4IStagePath -TotalCount 1)).Trim()
                } else { $Distinct4IPhase }
                if ($Distinct4IStage -notin @('PREFLIGHT','HEAD_MISMATCH','WORKTREE_CHANGED',
                    'READ_4H','FIND_4H','CHECK_4H_GATE_FILE','VERIFY_4H_RECEIPT',
                    'CHECK_4H_ROWS','CHECK_4H_CATEGORIES','VERIFY_BACKUP',
                    'UE_IDENTITY_AUDIT','UE_IDENTITY_FAILED',
                    'VERIFY_RESULT','WRITE_GATE','CHECK_CLEAN','ISSUE_BUILD',
                    'ISSUE_WRITE','SUCCESS','START','PARSE','PROCESS_RUNNING')) {
                    $Distinct4IStage = 'UNKNOWN'
                }
                try {
                    Publish-CommitStatus -Sha $Sha -State 'failure' -Context $Distinct4IContext -Description ('Identites 06.4I bloquees : ' + $Distinct4IStage)
                }
                catch { Write-Warning '[ARM-CLEAN06.4I] Status publication failed.' }
                Write-Warning ('[ARM-CLEAN06.4I] Distinct UE identity audit incomplete: ' + $Distinct4IStage)
            }
        }

        # ARM-CLEAN06.4J — one-shot transient UE Editor-only projection.
        # No SavePackage; private IDs and full property exports stay Windows-only.
        $NonArm4JTrigger = 'chore(ARM-CLEAN06.4J): verify transient non-armament canonical projections'
        $NonArm4JSubject = @(& git -C $WorktreePath log -1 --format=%s)
        if ($LASTEXITCODE -eq 0 -and $NonArm4JSubject.Count -eq 1 -and
            [string]$NonArm4JSubject[0] -ceq $NonArm4JTrigger) {
            $NonArm4JContext = 'UE5.5.4/ARM-CLEAN06.4J'
            $NonArm4JPhase = 'START'
            try {
                Publish-CommitStatus -Sha $Sha -State 'pending' -Context $NonArm4JContext -Description 'Dry-run UE transitoire : 11 ItemIds non-ARM, aucune ecriture'
                $NonArm4JScript = Join-Path $WorktreePath 'Scripts\AuditARM_CLEAN06_4J_NonArmTransient.ps1'
                if (-not (Test-Path -LiteralPath $NonArm4JScript -PathType Leaf)) {
                    throw 'ARM-CLEAN06.4J: script missing.'
                }
                $NonArm4JPhase = 'PARSE'
                $NonArm4JTokens = $null
                $NonArm4JErrors = $null
                [void][System.Management.Automation.Language.Parser]::ParseFile(
                    $NonArm4JScript, [ref]$NonArm4JTokens, [ref]$NonArm4JErrors)
                if ($NonArm4JErrors -and $NonArm4JErrors.Count -gt 0) {
                    throw 'ARM-CLEAN06.4J: invalid PowerShell syntax.'
                }
                foreach ($NonArm4JValue in @($NonArm4JScript,$WorktreePath,$EngineRoot,$StateRoot,$RunDir,$GitHubRepository)) {
                    if ($NonArm4JValue.Contains('"')) { throw 'ARM-CLEAN06.4J: unsupported path quote.' }
                }
                $NonArm4JArgs = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' +
                    $NonArm4JScript + '" -WorktreePath "' + $WorktreePath +
                    '" -EngineRoot "' + $EngineRoot + '" -StateRoot "' + $StateRoot +
                    '" -RunDir "' + $RunDir + '" -ExpectedHead "' + $Sha +
                    '" -GitHubRepository "' + $GitHubRepository + '"'
                $NonArm4JOut = Join-Path $RunDir 'ARM064J.stdout.log'
                $NonArm4JErr = Join-Path $RunDir 'ARM064J.stderr.log'
                $NonArm4JPhase = 'PROCESS_RUNNING'
                $NonArm4JProcess = Start-Process -FilePath $PsExe -ArgumentList $NonArm4JArgs -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput $NonArm4JOut -RedirectStandardError $NonArm4JErr -PassThru -Wait -ErrorAction Stop
                if ($NonArm4JProcess.ExitCode -ne 0) {
                    throw 'ARM-CLEAN06.4J: transient UE Editor dry-run returned nonzero.'
                }
                Publish-CommitStatus -Sha $Sha -State 'success' -Context $NonArm4JContext -Description '11 UObjects transitoires verifies; aucune conversion autorisee'
                Write-Host '[ARM-CLEAN06.4J] Real UE transient projections audited. Migration gate closed.'
            }
            catch {
                $NonArm4JStagePath = Join-Path $RunDir 'ARMClean064J.stage'
                $NonArm4JStage = if (Test-Path -LiteralPath $NonArm4JStagePath -PathType Leaf) {
                    ([string](Get-Content -LiteralPath $NonArm4JStagePath -TotalCount 1)).Trim()
                } else { $NonArm4JPhase }
                if ($NonArm4JStage -notin @('PREFLIGHT','HEAD_MISMATCH','WORKTREE_CHANGED',
                    'READ_06_4I','VERIFY_BINARY_SNAPSHOT','UE_TRANSIENT_PROJECTION',
                    'UE_PROJECTION_FAILED','VERIFY_RESULT','WRITE_PRIVATE_GATE',
                    'CHECK_CLEAN','ISSUE_BUILD','ISSUE_WRITE','SUCCESS','START',
                    'PARSE','PROCESS_RUNNING')) {
                    $NonArm4JStage = 'UNKNOWN'
                }
                try {
                    Publish-CommitStatus -Sha $Sha -State 'failure' -Context $NonArm4JContext -Description ('Projection 06.4J bloquee : ' + $NonArm4JStage)
                }
                catch { Write-Warning '[ARM-CLEAN06.4J] GitHub status publication failed.' }
                Write-Warning ('[ARM-CLEAN06.4J] UE transient projection incomplete: ' + $NonArm4JStage)
            }
        }

        # ARM-CLEAN06.4K — one-shot read-only semantic ownership triage in UE Editor.
        # All 50 active legacy fields require a review; no SavePackage or SaveGame reads.
        $Owner4KTrigger = 'chore(ARM-CLEAN06.4K): classify 50 live nonarm legacy field owners without writes'
        $Owner4KSubject = @(& git -C $WorktreePath log -1 --format=%s)
        if ($LASTEXITCODE -eq 0 -and $Owner4KSubject.Count -eq 1 -and
            [string]$Owner4KSubject[0] -ceq $Owner4KTrigger) {
            $Owner4KContext = 'UE5.5.4/ARM-CLEAN06.4K'
            $Owner4KPhase = 'START'
            try {
                Publish-CommitStatus -Sha $Sha -State 'pending' -Context $Owner4KContext -Description 'Inventaire UE de 50 champs non-ARM, sans conversion'
                $Owner4KScript = Join-Path $WorktreePath 'Scripts\AuditARM_CLEAN06_4K_NonArmFieldOwners.ps1'
                if (-not (Test-Path -LiteralPath $Owner4KScript -PathType Leaf)) {
                    throw 'ARM-CLEAN06.4K: script missing.'
                }
                $Owner4KPhase = 'PARSE'
                $Owner4KTokens = $null
                $Owner4KErrors = $null
                [void][System.Management.Automation.Language.Parser]::ParseFile(
                    $Owner4KScript, [ref]$Owner4KTokens, [ref]$Owner4KErrors)
                if ($Owner4KErrors -and $Owner4KErrors.Count -gt 0) {
                    throw 'ARM-CLEAN06.4K: invalid PowerShell syntax.'
                }
                foreach ($Owner4KValue in @($Owner4KScript,$WorktreePath,$EngineRoot,$StateRoot,$RunDir,$GitHubRepository)) {
                    if ($Owner4KValue.Contains('"')) { throw 'ARM-CLEAN06.4K: unsupported path quote.' }
                }
                $Owner4KArgs = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' +
                    $Owner4KScript + '" -WorktreePath "' + $WorktreePath +
                    '" -EngineRoot "' + $EngineRoot + '" -StateRoot "' + $StateRoot +
                    '" -RunDir "' + $RunDir + '" -ExpectedHead "' + $Sha +
                    '" -GitHubRepository "' + $GitHubRepository + '"'
                $Owner4KOut = Join-Path $RunDir 'ARM064K.stdout.log'
                $Owner4KErr = Join-Path $RunDir 'ARM064K.stderr.log'
                $Owner4KPhase = 'PROCESS_RUNNING'
                $Owner4KProcess = Start-Process -FilePath $PsExe -ArgumentList $Owner4KArgs -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput $Owner4KOut -RedirectStandardError $Owner4KErr -PassThru -Wait -ErrorAction Stop
                if ($Owner4KProcess.ExitCode -ne 0) {
                    throw 'ARM-CLEAN06.4K: UE ownership audit returned nonzero.'
                }
                Publish-CommitStatus -Sha $Sha -State 'success' -Context $Owner4KContext -Description '50 champs classes en revue; migration toujours interdite'
                Write-Host '[ARM-CLEAN06.4K] Read-only non-arm ownership audit complete. Migration gate closed.'
            }
            catch {
                $Owner4KStagePath = Join-Path $RunDir 'ARMClean064K.stage'
                $Owner4KStage = if (Test-Path -LiteralPath $Owner4KStagePath -PathType Leaf) {
                    ([string](Get-Content -LiteralPath $Owner4KStagePath -TotalCount 1)).Trim()
                } else { $Owner4KPhase }
                if ($Owner4KStage -notin @('PREFLIGHT','HEAD_MISMATCH','WORKTREE_CHANGED',
                    'READ_4J','READ_4I','REHASH_84','UNREAL_OWNER_AUDIT',
                    'UNREAL_OWNER_FAILED','VERIFY_HEADER','VERIFY_DOMAIN_KEYS',
                    'VERIFY_DOMAIN_TOTALS','VERIFY_ITEM_ROWS','WRITE_PRIVATE_GATE',
                    'CHECK_CLEAN','ISSUE_BUILD','ISSUE_WRITE','SUCCESS','START',
                    'PARSE','PROCESS_RUNNING')) {
                    $Owner4KStage = 'UNKNOWN'
                }
                try {
                    Publish-CommitStatus -Sha $Sha -State 'failure' -Context $Owner4KContext -Description ('Proprietes 06.4K bloquees : ' + $Owner4KStage)
                }
                catch { Write-Warning '[ARM-CLEAN06.4K] GitHub status publication failed.' }
                Write-Warning ('[ARM-CLEAN06.4K] UE field ownership audit incomplete: ' + $Owner4KStage)
            }
        }

        # ARM-CLEAN06.4L — dedicated read-only UE non-arm light/persistent FX
        # parity. Only run after regular Build and Grimrock Automation.
        $Presentation4LTrigger = 'feat(ARM-CLEAN06.4L): preserve nonarm light and persistent visual data in transient canonical projection'
        $Presentation4LSubject = @(& git -C $WorktreePath log -1 --format=%s)
        if ($LASTEXITCODE -eq 0 -and $Presentation4LSubject.Count -eq 1 -and
            [string]$Presentation4LSubject[0] -ceq $Presentation4LTrigger) {
            $Presentation4LContext = 'UE5.5.4/ARM-CLEAN06.4L'
            $Presentation4LPhase = 'START'
            try {
                Publish-CommitStatus -Sha $Sha -State 'pending' -Context $Presentation4LContext -Description 'Parite donnees UE en memoire : lumiere et VFX non-ARM'
                $Presentation4LScript = Join-Path $WorktreePath 'Scripts\AuditARM_CLEAN06_4L_NonArmPresentationParity.ps1'
                if (-not (Test-Path -LiteralPath $Presentation4LScript -PathType Leaf)) {
                    throw 'ARM-CLEAN06.4L: script missing.'
                }
                $Presentation4LPhase = 'PARSE'
                $Presentation4LTokens = $null
                $Presentation4LErrors = $null
                [void][System.Management.Automation.Language.Parser]::ParseFile(
                    $Presentation4LScript, [ref]$Presentation4LTokens, [ref]$Presentation4LErrors)
                if ($Presentation4LErrors -and $Presentation4LErrors.Count -gt 0) {
                    throw 'ARM-CLEAN06.4L: invalid PowerShell syntax.'
                }
                foreach ($Presentation4LValue in @($Presentation4LScript,$WorktreePath,$EngineRoot,$StateRoot,$RunDir,$GitHubRepository)) {
                    if ($Presentation4LValue.Contains('"')) { throw 'ARM-CLEAN06.4L: unsupported path quote.' }
                }
                $Presentation4LArgs = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' +
                    $Presentation4LScript + '" -WorktreePath "' + $WorktreePath +
                    '" -EngineRoot "' + $EngineRoot + '" -StateRoot "' + $StateRoot +
                    '" -RunDir "' + $RunDir + '" -ExpectedHead "' + $Sha +
                    '" -GitHubRepository "' + $GitHubRepository + '"'
                $Presentation4LOut = Join-Path $RunDir 'ARM064L.stdout.log'
                $Presentation4LErr = Join-Path $RunDir 'ARM064L.stderr.log'
                $Presentation4LPhase = 'PROCESS_RUNNING'
                $Presentation4LProcess = Start-Process -FilePath $PsExe -ArgumentList $Presentation4LArgs -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput $Presentation4LOut -RedirectStandardError $Presentation4LErr -PassThru -Wait -ErrorAction Stop
                if ($Presentation4LProcess.ExitCode -ne 0) {
                    throw 'ARM-CLEAN06.4L: UE presentation data parity returned nonzero.'
                }
                Publish-CommitStatus -Sha $Sha -State 'success' -Context $Presentation4LContext -Description '12 champs visuels/lumiere preserves, 88 copies; conversion interdite'
                Write-Host '[ARM-CLEAN06.4L] Non-arm presentation data parity complete. Binary gate closed.'
            }
            catch {
                $Presentation4LStagePath = Join-Path $RunDir 'ARMClean064L.stage'
                $Presentation4LStage = if (Test-Path -LiteralPath $Presentation4LStagePath -PathType Leaf) {
                    ([string](Get-Content -LiteralPath $Presentation4LStagePath -TotalCount 1)).Trim()
                } else { $Presentation4LPhase }
                if ($Presentation4LStage -notin @('PREFLIGHT','HEAD_MISMATCH','WORKTREE_CHANGED',
                    'READ_4K_LOCATE','READ_4K_MISSING','READ_4K_MULTIPLE',
                    'READ_4K_DUPLICATE_PROVENANCE','READ_4K_NO_TRUSTED_COPY',
                    'READ_4K_COMPARE_COPIES','READ_4K_CONFLICTING_COPIES',
                    'READ_4K_DUPLICATES_RECONCILED','READ_4K_GATE_EXISTS','READ_4K_GATE_READ','READ_4K_GATE_SCHEMA',
                    'READ_4K_REPORT_HASH','READ_4K_COUNTERS','READ_4K_GATE_CLOSED',
                    'VERIFY_4K_DOMAINS','REHASH_84','UNREAL_PRESENTATION_PARITY',
                    'UNREAL_PARITY_FAILED','VERIFY_RESULT','WRITE_PRIVATE_GATE',
                    'CHECK_CLEAN','ISSUE_BUILD','ISSUE_WRITE','SUCCESS','START',
                    'PARSE','PROCESS_RUNNING')) {
                    $Presentation4LStage = 'UNKNOWN'
                }
                try {
                    Publish-CommitStatus -Sha $Sha -State 'failure' -Context $Presentation4LContext -Description ('Parite 06.4L bloquee : ' + $Presentation4LStage)
                }
                catch { Write-Warning '[ARM-CLEAN06.4L] GitHub status publication failed.' }
                Write-Warning ('[ARM-CLEAN06.4L] UE parity incomplete: ' + $Presentation4LStage)
            }
        }

        # ARM-CLEAN06.4M — one-shot read-only UE projection of non-ARM
        # reading and world physics. No SavePackage and no SaveGame reads.
        $NonArm4MTrigger = 'feat(ARM-CLEAN06.4M): preserve nonarm reading and world physics policies in canonical data'
        $NonArm4MSubject = @(& git -C $WorktreePath log -1 --format=%s)
        if ($LASTEXITCODE -eq 0 -and $NonArm4MSubject.Count -eq 1 -and
            [string]$NonArm4MSubject[0] -ceq $NonArm4MTrigger) {
            $NonArm4MContext = 'UE5.5.4/ARM-CLEAN06.4M'
            $NonArm4MPhase = 'START'
            try {
                Publish-CommitStatus -Sha $Sha -State 'pending' -Context $NonArm4MContext -Description 'Parite donnees UE lecture et physique monde non-ARM'
                $NonArm4MScript = Join-Path $WorktreePath 'Scripts\AuditARM_CLEAN06_4M_NonArmReadingPhysics.ps1'
                if (-not (Test-Path -LiteralPath $NonArm4MScript -PathType Leaf)) {
                    throw 'ARM-CLEAN06.4M: script missing.'
                }
                $NonArm4MPhase = 'PARSE'
                $NonArm4MTokens = $null
                $NonArm4MErrors = $null
                [void][System.Management.Automation.Language.Parser]::ParseFile(
                    $NonArm4MScript, [ref]$NonArm4MTokens, [ref]$NonArm4MErrors)
                if ($NonArm4MErrors -and $NonArm4MErrors.Count -gt 0) {
                    throw 'ARM-CLEAN06.4M: invalid PowerShell syntax.'
                }
                foreach ($NonArm4MValue in @($NonArm4MScript,$WorktreePath,$EngineRoot,$StateRoot,$RunDir,$GitHubRepository)) {
                    if ($NonArm4MValue.Contains('"')) { throw 'ARM-CLEAN06.4M: unsupported path quote.' }
                }
                $NonArm4MArgs = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' +
                    $NonArm4MScript + '" -WorktreePath "' + $WorktreePath +
                    '" -EngineRoot "' + $EngineRoot + '" -StateRoot "' + $StateRoot +
                    '" -RunDir "' + $RunDir + '" -ExpectedHead "' + $Sha +
                    '" -GitHubRepository "' + $GitHubRepository + '"'
                $NonArm4MOut = Join-Path $RunDir 'ARM064M.stdout.log'
                $NonArm4MErr = Join-Path $RunDir 'ARM064M.stderr.log'
                $NonArm4MPhase = 'PROCESS_RUNNING'
                $NonArm4MProcess = Start-Process -FilePath $PsExe -ArgumentList $NonArm4MArgs -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput $NonArm4MOut -RedirectStandardError $NonArm4MErr -PassThru -Wait -ErrorAction Stop
                if ($NonArm4MProcess.ExitCode -ne 0) {
                    throw 'ARM-CLEAN06.4M: real UE reading/physics parity failed.'
                }
                Publish-CommitStatus -Sha $Sha -State 'success' -Context $NonArm4MContext -Description '3 champs lecture/Chaos, 33 copies, 77 tests purs; migration fermee'
                Write-Host '[ARM-CLEAN06.4M] Real UE canonical reading/physics data parity verified.'
            }
            catch {
                $NonArm4MStagePath = Join-Path $RunDir 'ARMClean064M.stage'
                $NonArm4MStage = if (Test-Path -LiteralPath $NonArm4MStagePath -PathType Leaf) {
                    ([string](Get-Content -LiteralPath $NonArm4MStagePath -TotalCount 1)).Trim()
                } else { $NonArm4MPhase }
                if ($NonArm4MStage -notin @('PREFLIGHT','HEAD_MISMATCH','WORKTREE_CHANGED',
                    'READ_4K_LOCATE','READ_4K_MISSING','READ_4K_MULTIPLE',
                    'READ_4K_DUPLICATE_PROVENANCE','READ_4K_NO_TRUSTED_COPY',
                    'READ_4K_COMPARE_COPIES','READ_4K_CONFLICTING_COPIES',
                    'READ_4K_DUPLICATES_RECONCILED','READ_4K_GATE_EXISTS',
                    'READ_4K_GATE_READ','READ_4K_GATE_SCHEMA','READ_4K_REPORT_HASH',
                    'READ_4K_COUNTERS','READ_4K_GATE_CLOSED','VERIFY_4K_DOMAINS',
                    'REHASH_84','READ_4L','UNREAL_READ_WORLD_PARITY',
                    'UNREAL_READ_WORLD_FAILED','VERIFY_RESULT','WRITE_PRIVATE_GATE',
                    'CHECK_CLEAN','ISSUE_BUILD','ISSUE_WRITE','SUCCESS',
                    'START','PARSE','PROCESS_RUNNING')) {
                    $NonArm4MStage = 'UNKNOWN'
                }
                try {
                    Publish-CommitStatus -Sha $Sha -State 'failure' -Context $NonArm4MContext -Description ('Parite 06.4M bloquee : ' + $NonArm4MStage)
                }
                catch { Write-Warning '[ARM-CLEAN06.4M] GitHub status publication failed.' }
                Write-Warning ('[ARM-CLEAN06.4M] Real UE reading/world projection incomplete: ' + $NonArm4MStage)
            }
        }

        # ARM-CLEAN06.4N — guarded generic NON-ARM throwing audit, read-only.
        $Generic4NTrigger = 'feat(ARM-CLEAN06.4N): preserve generic nonarm throwing and visual physics data'
        $Generic4NSubject = @(& git -C $WorktreePath log -1 --format=%s)
        if ($LASTEXITCODE -eq 0 -and $Generic4NSubject.Count -eq 1 -and
            [string]$Generic4NSubject[0] -ceq $Generic4NTrigger) {
            $Generic4NContext = 'UE5.5.4/ARM-CLEAN06.4N'
            $Generic4NPhase = 'START'
            try {
                Publish-CommitStatus -Sha $Sha -State 'pending' -Context $Generic4NContext -Description 'Audit UE transitoire des lancers physiques non-ARM'
                $Generic4NScript = Join-Path $WorktreePath 'Scripts\AuditARM_CLEAN06_4N_NonArmGenericThrow.ps1'
                if (-not (Test-Path -LiteralPath $Generic4NScript -PathType Leaf)) {
                    throw 'ARM-CLEAN06.4N: script missing.'
                }
                $Generic4NPhase = 'PARSE'
                $Generic4NTokens = $null
                $Generic4NErrors = $null
                [void][System.Management.Automation.Language.Parser]::ParseFile(
                    $Generic4NScript, [ref]$Generic4NTokens, [ref]$Generic4NErrors)
                if ($Generic4NErrors -and $Generic4NErrors.Count -gt 0) {
                    throw 'ARM-CLEAN06.4N: invalid PowerShell syntax.'
                }
                foreach ($Generic4NValue in @($Generic4NScript,$WorktreePath,$EngineRoot,$StateRoot,$RunDir,$GitHubRepository)) {
                    if ($Generic4NValue.Contains('"')) { throw 'ARM-CLEAN06.4N: unsupported path quote.' }
                }
                $Generic4NArgs = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' +
                    $Generic4NScript + '" -WorktreePath "' + $WorktreePath +
                    '" -EngineRoot "' + $EngineRoot + '" -StateRoot "' + $StateRoot +
                    '" -RunDir "' + $RunDir + '" -ExpectedHead "' + $Sha +
                    '" -GitHubRepository "' + $GitHubRepository + '"'
                $Generic4NOut = Join-Path $RunDir 'ARM064N.stdout.log'
                $Generic4NErr = Join-Path $RunDir 'ARM064N.stderr.log'
                $Generic4NPhase = 'PROCESS_RUNNING'
                $Generic4NProcess = Start-Process -FilePath $PsExe -ArgumentList $Generic4NArgs -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput $Generic4NOut -RedirectStandardError $Generic4NErr -PassThru -Wait -ErrorAction Stop
                if ($Generic4NProcess.ExitCode -ne 0) {
                    throw 'ARM-CLEAN06.4N: native generic throw data parity returned nonzero.'
                }
                Publish-CommitStatus -Sha $Sha -State 'success' -Context $Generic4NContext -Description '3 champs lancers non-ARM, 132 mappings, 77 tests; gate fermee'
                Write-Host '[ARM-CLEAN06.4N] Generic non-arm physical throw parity verified.'
            }
            catch {
                $Generic4NStagePath = Join-Path $RunDir 'ARMClean064N.stage'
                $Generic4NStage = if (Test-Path -LiteralPath $Generic4NStagePath -PathType Leaf) {
                    ([string](Get-Content -LiteralPath $Generic4NStagePath -TotalCount 1)).Trim()
                } else { $Generic4NPhase }
                if ($Generic4NStage -notin @('PREFLIGHT','HEAD_MISMATCH','WORKTREE_CHANGED',
                    'READ_4K_LOCATE','READ_4K_MISSING','READ_4K_MULTIPLE',
                    'READ_4K_DUPLICATE_PROVENANCE','READ_4K_NO_TRUSTED_COPY',
                    'READ_4K_COMPARE_COPIES','READ_4K_CONFLICTING_COPIES',
                    'READ_4K_DUPLICATES_RECONCILED','READ_4K_GATE_EXISTS',
                    'READ_4K_GATE_READ','READ_4K_GATE_SCHEMA','READ_4K_REPORT_HASH',
                    'READ_4K_COUNTERS','READ_4K_GATE_CLOSED','VERIFY_4K_DOMAINS',
                    'REHASH_84','READ_4M','UNREAL_GENERIC_THROW',
                    'UNREAL_THROW_FAILED','VERIFY_RESULT','WRITE_PRIVATE_GATE',
                    'CHECK_CLEAN','ISSUE_BUILD','ISSUE_WRITE','SUCCESS',
                    'START','PARSE','PROCESS_RUNNING')) {
                    $Generic4NStage = 'UNKNOWN'
                }
                try {
                    Publish-CommitStatus -Sha $Sha -State 'failure' -Context $Generic4NContext -Description ('Lancers 06.4N bloques : ' + $Generic4NStage)
                }
                catch { Write-Warning '[ARM-CLEAN06.4N] GitHub status publication failed.' }
                Write-Warning ('[ARM-CLEAN06.4N] UE generic throw data parity incomplete: ' + $Generic4NStage)
            }
        }

        # ARM-CLEAN06.4O — action semantic audit, read-only UE source capture.
        $Action4OTrigger = 'chore(ARM-CLEAN06.4O): inventory full legacy nonarm action semantics without writes'
        $Action4OSubject = @(& git -C $WorktreePath log -1 --format=%s)
        if ($LASTEXITCODE -eq 0 -and $Action4OSubject.Count -eq 1 -and
            [string]$Action4OSubject[0] -ceq $Action4OTrigger) {
            $Action4OContext = 'UE5.5.4/ARM-CLEAN06.4O'
            $Action4OPhase = 'START'
            try {
                Publish-CommitStatus -Sha $Sha -State 'pending' -Context $Action4OContext -Description 'Audit UE action et effets non-ARM, sans conversion'
                $Action4OScript = Join-Path $WorktreePath 'Scripts\AuditARM_CLEAN06_4O_NonArmActionSemantics.ps1'
                if (-not (Test-Path -LiteralPath $Action4OScript -PathType Leaf)) {
                    throw 'ARM-CLEAN06.4O: script missing.'
                }
                $Action4OPhase = 'PARSE'
                $Action4OTokens = $null
                $Action4OErrors = $null
                [void][System.Management.Automation.Language.Parser]::ParseFile(
                    $Action4OScript, [ref]$Action4OTokens, [ref]$Action4OErrors)
                if ($Action4OErrors -and $Action4OErrors.Count -gt 0) {
                    throw 'ARM-CLEAN06.4O: invalid PowerShell syntax.'
                }
                foreach ($Action4OValue in @($Action4OScript,$WorktreePath,$EngineRoot,$StateRoot,$RunDir,$GitHubRepository)) {
                    if ($Action4OValue.Contains('"')) { throw 'ARM-CLEAN06.4O: unsupported path quote.' }
                }
                $Action4OArgs = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' +
                    $Action4OScript + '" -WorktreePath "' + $WorktreePath +
                    '" -EngineRoot "' + $EngineRoot + '" -StateRoot "' + $StateRoot +
                    '" -RunDir "' + $RunDir + '" -ExpectedHead "' + $Sha +
                    '" -GitHubRepository "' + $GitHubRepository + '"'
                $Action4OOut = Join-Path $RunDir 'ARM064O.stdout.log'
                $Action4OErr = Join-Path $RunDir 'ARM064O.stderr.log'
                $Action4OPhase = 'PROCESS_RUNNING'
                $Action4OProcess = Start-Process -FilePath $PsExe -ArgumentList $Action4OArgs -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput $Action4OOut -RedirectStandardError $Action4OErr -PassThru -Wait -ErrorAction Stop
                if ($Action4OProcess.ExitCode -ne 0) {
                    throw 'ARM-CLEAN06.4O: real UE action audit returned nonzero.'
                }
                Publish-CommitStatus -Sha $Sha -State 'success' -Context $Action4OContext -Description '11 champs actions releves, donnees privees, aucune ecriture'
                Write-Host '[ARM-CLEAN06.4O] Real UE non-arm action inventory complete. Migration gate closed.'
            }
            catch {
                $Action4OStagePath = Join-Path $RunDir 'ARMClean064O.stage'
                $Action4OStage = if (Test-Path -LiteralPath $Action4OStagePath -PathType Leaf) {
                    ([string](Get-Content -LiteralPath $Action4OStagePath -TotalCount 1)).Trim()
                } else { $Action4OPhase }
                if ($Action4OStage -notin @('PREFLIGHT','HEAD_MISMATCH','WORKTREE_CHANGED',
                    'READ_4K_LOCATE','READ_4K_MISSING','READ_4K_MULTIPLE',
                    'READ_4K_DUPLICATE_PROVENANCE','READ_4K_NO_TRUSTED_COPY',
                    'READ_4K_COMPARE_COPIES','READ_4K_CONFLICTING_COPIES',
                    'READ_4K_DUPLICATES_RECONCILED','READ_4K_GATE_EXISTS',
                    'READ_4K_GATE_READ','READ_4K_GATE_SCHEMA','READ_4K_REPORT_HASH',
                    'READ_4K_COUNTERS','READ_4K_GATE_CLOSED','VERIFY_4K_DOMAINS',
                    'REHASH_84','READ_4N','UNREAL_ACTION_AUDIT',
                    'UNREAL_ACTION_FAILED','VERIFY_RESULT','WRITE_PRIVATE_GATE',
                    'CHECK_CLEAN','ISSUE_BUILD','ISSUE_WRITE','SUCCESS',
                    'START','PARSE','PROCESS_RUNNING')) {
                    $Action4OStage = 'UNKNOWN'
                }
                try {
                    Publish-CommitStatus -Sha $Sha -State 'failure' -Context $Action4OContext -Description ('Actions 06.4O bloquees : ' + $Action4OStage)
                }
                catch { Write-Warning '[ARM-CLEAN06.4O] GitHub status publication failed.' }
                Write-Warning ('[ARM-CLEAN06.4O] UE action semantic inventory incomplete: ' + $Action4OStage)
            }
        }

        # ARM-CLEAN06.4P — strict real UE non-armament equipment parity.
        $Equip4PTrigger = 'feat(ARM-CLEAN06.4P): preserve nonarm equipment hand slots resistances and meshes'
        $Equip4PSubject = @(& git -C $WorktreePath log -1 --format=%s)
        if ($LASTEXITCODE -eq 0 -and $Equip4PSubject.Count -eq 1 -and
            [string]$Equip4PSubject[0] -ceq $Equip4PTrigger) {
            $Equip4PContext = 'UE5.5.4/ARM-CLEAN06.4P'
            $Equip4PPhase = 'START'
            try {
                Publish-CommitStatus -Sha $Sha -State 'pending' -Context $Equip4PContext -Description 'Parite UE non-ARM: mains, emplacements, stats, resistances et meshes'
                $Equip4PScript = Join-Path $WorktreePath 'Scripts\AuditARM_CLEAN06_4P_NonArmEquipmentParity.ps1'
                if (-not (Test-Path -LiteralPath $Equip4PScript -PathType Leaf)) {
                    throw 'ARM-CLEAN06.4P: missing private validation script.'
                }
                $Equip4PPhase = 'PARSE'
                $Equip4PTokens = $null
                $Equip4PErrors = $null
                [void][System.Management.Automation.Language.Parser]::ParseFile(
                    $Equip4PScript, [ref]$Equip4PTokens, [ref]$Equip4PErrors)
                if ($Equip4PErrors -and $Equip4PErrors.Count -gt 0) {
                    throw 'ARM-CLEAN06.4P: invalid PowerShell syntax.'
                }
                foreach ($Equip4PValue in @($Equip4PScript,$WorktreePath,$EngineRoot,$StateRoot,$RunDir,$GitHubRepository)) {
                    if ($Equip4PValue.Contains('"')) { throw 'ARM-CLEAN06.4P: unsupported path quote.' }
                }
                $Equip4PArgs = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' +
                    $Equip4PScript + '" -WorktreePath "' + $WorktreePath +
                    '" -EngineRoot "' + $EngineRoot + '" -StateRoot "' + $StateRoot +
                    '" -RunDir "' + $RunDir + '" -ExpectedHead "' + $Sha +
                    '" -GitHubRepository "' + $GitHubRepository + '"'
                $Equip4POut = Join-Path $RunDir 'ARM064P.stdout.log'
                $Equip4PErr = Join-Path $RunDir 'ARM064P.stderr.log'
                $Equip4PPhase = 'PROCESS_RUNNING'
                $Equip4PProcess = Start-Process -FilePath $PsExe -ArgumentList $Equip4PArgs -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput $Equip4POut -RedirectStandardError $Equip4PErr -PassThru -Wait -ErrorAction Stop
                if ($Equip4PProcess.ExitCode -ne 0) {
                    throw 'ARM-CLEAN06.4P: actual UE equipment parity failed.'
                }
                Publish-CommitStatus -Sha $Sha -State 'success' -Context $Equip4PContext -Description '15 champs non-ARM, 44 copies, 209 emplacements, migration fermee'
                Write-Host '[ARM-CLEAN06.4P] UE non-armament equipment data parity verified.'
            }
            catch {
                $Equip4PStagePath = Join-Path $RunDir 'ARMClean064P.stage'
                $Equip4PStage = if (Test-Path -LiteralPath $Equip4PStagePath -PathType Leaf) {
                    ([string](Get-Content -LiteralPath $Equip4PStagePath -TotalCount 1)).Trim()
                } else { $Equip4PPhase }
                if ($Equip4PStage -notin @('PREFLIGHT','HEAD_MISMATCH','WORKTREE_CHANGED',
                    'READ_4K_LOCATE','READ_4K_MISSING','READ_4K_MULTIPLE',
                    'READ_4K_DUPLICATE_PROVENANCE','READ_4K_NO_TRUSTED_COPY',
                    'READ_4K_COMPARE_COPIES','READ_4K_CONFLICTING_COPIES',
                    'READ_4K_DUPLICATES_RECONCILED','READ_4K_GATE_EXISTS',
                    'READ_4K_GATE_READ','READ_4K_GATE_SCHEMA','READ_4K_REPORT_HASH',
                    'READ_4K_COUNTERS','READ_4K_GATE_CLOSED','VERIFY_4K_DOMAINS',
                    'REHASH_84','READ_4O','UNREAL_EQUIPMENT_PARITY',
                    'UNREAL_EQUIPMENT_FAILED','VERIFY_RESULT','WRITE_PRIVATE_GATE',
                    'CHECK_CLEAN','ISSUE_BUILD','ISSUE_WRITE','SUCCESS',
                    'START','PARSE','PROCESS_RUNNING')) {
                    $Equip4PStage = 'UNKNOWN'
                }
                try {
                    Publish-CommitStatus -Sha $Sha -State 'failure' -Context $Equip4PContext -Description ('Equipement 06.4P bloque : ' + $Equip4PStage)
                }
                catch { Write-Warning '[ARM-CLEAN06.4P] GitHub status publication failed.' }
                Write-Warning ('[ARM-CLEAN06.4P] Native UE equipment parity incomplete: ' + $Equip4PStage)
            }
        }

        # ARM-CLEAN06.4Q — true UGridItemSubsystem registry stack parity,
        # transient preview only. Never modify the level, asset or SaveGame.
        $Stack4QTrigger = 'feat(ARM-CLEAN06.4Q): validate generic nonarm stacks through canonical registry'
        $Stack4QSubject = @(& git -C $WorktreePath log -1 --format=%s)
        if ($LASTEXITCODE -eq 0 -and $Stack4QSubject.Count -eq 1 -and
            [string]$Stack4QSubject[0] -ceq $Stack4QTrigger) {
            $Stack4QContext = 'UE5.5.4/ARM-CLEAN06.4Q'
            $Stack4QPhase = 'START'
            try {
                Publish-CommitStatus -Sha $Sha -State 'pending' -Context $Stack4QContext -Description 'Registre UE: piles non-ARM N0/N1/N20/N21 et quantites'
                $Stack4QScript = Join-Path $WorktreePath 'Scripts\AuditARM_CLEAN06_4Q_NonArmStackRegistryParity.ps1'
                if (-not (Test-Path -LiteralPath $Stack4QScript -PathType Leaf)) {
                    throw 'ARM-CLEAN06.4Q: missing audit script.'
                }
                $Stack4QPhase = 'PARSE'
                $Stack4QTokens = $null
                $Stack4QErrors = $null
                [void][System.Management.Automation.Language.Parser]::ParseFile(
                    $Stack4QScript, [ref]$Stack4QTokens, [ref]$Stack4QErrors)
                if ($Stack4QErrors -and $Stack4QErrors.Count -gt 0) {
                    throw 'ARM-CLEAN06.4Q: invalid PowerShell syntax.'
                }
                foreach ($Stack4QValue in @($Stack4QScript,$WorktreePath,$EngineRoot,$StateRoot,$RunDir,$GitHubRepository)) {
                    if ($Stack4QValue.Contains('"')) { throw 'ARM-CLEAN06.4Q: unsupported path quote.' }
                }
                $Stack4QArgs = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' +
                    $Stack4QScript + '" -WorktreePath "' + $WorktreePath +
                    '" -EngineRoot "' + $EngineRoot + '" -StateRoot "' + $StateRoot +
                    '" -RunDir "' + $RunDir + '" -ExpectedHead "' + $Sha +
                    '" -GitHubRepository "' + $GitHubRepository + '"'
                $Stack4QOut = Join-Path $RunDir 'ARM064Q.stdout.log'
                $Stack4QErr = Join-Path $RunDir 'ARM064Q.stderr.log'
                $Stack4QPhase = 'PROCESS_RUNNING'
                $Stack4QProcess = Start-Process -FilePath $PsExe -ArgumentList $Stack4QArgs -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput $Stack4QOut -RedirectStandardError $Stack4QErr -PassThru -Wait -ErrorAction Stop
                if ($Stack4QProcess.ExitCode -ne 0) {
                    throw 'ARM-CLEAN06.4Q: real UE stack registry parity failed.'
                }
                Publish-CommitStatus -Sha $Sha -State 'success' -Context $Stack4QContext -Description '6 champs, 34 definitions transitoires, 44 niveaux et 33 quantites, gate fermee'
                Write-Host '[ARM-CLEAN06.4Q] Generic stack data parity verified using true item registry.'
            }
            catch {
                $Stack4QStagePath = Join-Path $RunDir 'ARMClean064Q.stage'
                $Stack4QStage = if (Test-Path -LiteralPath $Stack4QStagePath -PathType Leaf) {
                    ([string](Get-Content -LiteralPath $Stack4QStagePath -TotalCount 1)).Trim()
                } else { $Stack4QPhase }
                if ($Stack4QStage -notin @('PREFLIGHT','HEAD_MISMATCH','WORKTREE_CHANGED',
                    'READ_4K_LOCATE','READ_4K_MISSING','READ_4K_MULTIPLE',
                    'READ_4K_DUPLICATE_PROVENANCE','READ_4K_NO_TRUSTED_COPY',
                    'READ_4K_COMPARE_COPIES','READ_4K_CONFLICTING_COPIES',
                    'READ_4K_DUPLICATES_RECONCILED','READ_4K_GATE_EXISTS',
                    'READ_4K_GATE_READ','READ_4K_GATE_SCHEMA','READ_4K_REPORT_HASH',
                    'READ_4K_COUNTERS','READ_4K_GATE_CLOSED','VERIFY_4K_DOMAINS',
                    'REHASH_84','READ_4P','UNREAL_GENERIC_STACK',
                    'UNREAL_Q_ARGUMENTS','UNREAL_Q_INPUT_SCOPE','UNREAL_Q_DOMAIN',
                    'UNREAL_Q_IDENTITY','UNREAL_Q_LEGACY','UNREAL_Q_FIELD_DRIFT',
                    'UNREAL_Q_COUNTERS','UNREAL_Q_REGISTRY','UNREAL_Q_LEVEL_21',
                    'UNREAL_Q_LEVEL_CAPACITY','UNREAL_Q_QUANTITY',
                    'UNREAL_Q_IMMUTABILITY','UNREAL_Q_OUTPUT','UNREAL_Q_PROCESS',
                    'UNREAL_Q_CODE_1','UNREAL_Q_CODE_2','UNREAL_Q_CODE_3',
                    'UNREAL_Q_CODE_4','UNREAL_Q_CODE_5','UNREAL_Q_CODE_6',
                    'UNREAL_Q_CODE_7','UNREAL_Q_CODE_8','UNREAL_Q_CODE_9',
                    'VERIFY_RESULT','WRITE_PRIVATE_GATE',
                    'CHECK_CLEAN','ISSUE_BUILD','ISSUE_WRITE','SUCCESS',
                    'START','PARSE','PROCESS_RUNNING')) {
                    $Stack4QStage = 'UNKNOWN'
                }
                try {
                    Publish-CommitStatus -Sha $Sha -State 'failure' -Context $Stack4QContext -Description ('Empilement 06.4Q bloque : ' + $Stack4QStage)
                }
                catch { Write-Warning '[ARM-CLEAN06.4Q] GitHub status publication failed.' }
                Write-Warning ('[ARM-CLEAN06.4Q] UE generic stack registry parity incomplete: ' + $Stack4QStage)
            }
        }

        # ARM-CLEAN06.4R — real UE action/presentation full-data catalogue
        # staging, strictly read-only. Checked after Build and Grimrock suite.
        $Action4RTrigger = 'feat(ARM-CLEAN06.4R): stage complete nonarm combat actions in canonical catalogues'
        $Action4RSubject = @(& git -C $WorktreePath log -1 --format=%s)
        if ($LASTEXITCODE -eq 0 -and $Action4RSubject.Count -eq 1 -and
            [string]$Action4RSubject[0] -ceq $Action4RTrigger) {
            $Action4RContext = 'UE5.5.4/ARM-CLEAN06.4R'
            $Action4RPhase = 'START'
            try {
                Publish-CommitStatus -Sha $Sha -State 'pending' -Context $Action4RContext -Description 'Audit UE complet actions et presentation non-ARM, conversion interdite'
                $Action4RScript = Join-Path $WorktreePath 'Scripts\AuditARM_CLEAN06_4R_NonArmActionBridge.ps1'
                if (-not (Test-Path -LiteralPath $Action4RScript -PathType Leaf)) {
                    throw 'ARM-CLEAN06.4R: private verification script missing.'
                }
                $Action4RPhase = 'PARSE'
                $Action4RTokens = $null
                $Action4RErrors = $null
                [void][System.Management.Automation.Language.Parser]::ParseFile(
                    $Action4RScript, [ref]$Action4RTokens, [ref]$Action4RErrors)
                if ($Action4RErrors -and $Action4RErrors.Count -gt 0) {
                    throw 'ARM-CLEAN06.4R: PowerShell syntax error.'
                }
                foreach ($Action4RValue in @($Action4RScript,$WorktreePath,$EngineRoot,$StateRoot,$RunDir,$GitHubRepository)) {
                    if ($Action4RValue.Contains('"')) {throw 'ARM-CLEAN06.4R: unsafe quote in path.'}
                }
                $Action4RArgs = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' +
                    $Action4RScript + '" -WorktreePath "' + $WorktreePath +
                    '" -EngineRoot "' + $EngineRoot + '" -StateRoot "' + $StateRoot +
                    '" -RunDir "' + $RunDir + '" -ExpectedHead "' + $Sha +
                    '" -GitHubRepository "' + $GitHubRepository + '"'
                $Action4ROut = Join-Path $RunDir 'ARM064R.stdout.log'
                $Action4RErr = Join-Path $RunDir 'ARM064R.stderr.log'
                $Action4RPhase = 'PROCESS_RUNNING'
                $Action4RProcess = Start-Process -FilePath $PsExe -ArgumentList $Action4RArgs -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput $Action4ROut -RedirectStandardError $Action4RErr -PassThru -Wait -ErrorAction Stop
                if ($Action4RProcess.ExitCode -ne 0) {
                    throw 'ARM-CLEAN06.4R: native UE full action bridge failed.'
                }
                Publish-CommitStatus -Sha $Sha -State 'success' -Context $Action4RContext -Description '4 actions completes et 1 presentation preservees; runtime non bascule'
                Write-Host '[ARM-CLEAN06.4R] Full non-arm action/presentation data bridge verified.'
            }
            catch {
                $Action4RStagePath = Join-Path $RunDir 'ARMClean064R.stage'
                $Action4RStage = if (Test-Path -LiteralPath $Action4RStagePath -PathType Leaf) {
                    ([string](Get-Content -LiteralPath $Action4RStagePath -TotalCount 1)).Trim()
                } else { $Action4RPhase }
                if ($Action4RStage -notin @('PREFLIGHT','HEAD_MISMATCH','WORKTREE_CHANGED',
                    'READ_4K_LOCATE','READ_4K_MISSING','READ_4K_MULTIPLE',
                    'READ_4K_DUPLICATE_PROVENANCE','READ_4K_NO_TRUSTED_COPY',
                    'READ_4K_COMPARE_COPIES','READ_4K_CONFLICTING_COPIES',
                    'READ_4K_DUPLICATES_RECONCILED','READ_4K_GATE_EXISTS',
                    'READ_4K_GATE_READ','READ_4K_GATE_SCHEMA','READ_4K_REPORT_HASH',
                    'READ_4K_COUNTERS','READ_4K_GATE_CLOSED','VERIFY_4K_DOMAINS',
                    'REHASH_84','READ_4O','READ_4Q',
                    'READ_4Q_DUPLICATE_PROVENANCE','READ_4Q_NO_TRUSTED_COPY',
                    'READ_4Q_COMPARE_TRUSTED_COPIES','READ_4Q_CONFLICTING_COPIES',
                    'READ_4Q_DUPLICATES_RECONCILED','UNREAL_ACTION_BRIDGE',
                    'UNREAL_ACTION_BRIDGE_FAILED','VERIFY_RESULT',
                    'WRITE_PRIVATE_GATE','CHECK_CLEAN','ISSUE_BUILD','ISSUE_WRITE',
                    'SUCCESS','START','PARSE','PROCESS_RUNNING')) {
                    $Action4RStage = 'UNKNOWN'
                }
                try {
                    Publish-CommitStatus -Sha $Sha -State 'failure' -Context $Action4RContext -Description ('Actions 06.4R bloquees : ' + $Action4RStage)
                }
                catch {Write-Warning '[ARM-CLEAN06.4R] Cannot publish failed check.'}
                Write-Warning ('[ARM-CLEAN06.4R] Action bridge audit incomplete: ' + $Action4RStage)
            }
        }

        # ARM-CLEAN06.4S — immutable closed transaction plan + private
        # 84-file staging / tamper detection / full sandbox rollback.
        $Txn4STrigger = 'chore(ARM-CLEAN06.4S): rehearse sealed UE asset transaction without writes'
        $Txn4SSubject = @(& git -C $WorktreePath log -1 --format=%s)
        if ($LASTEXITCODE -eq 0 -and $Txn4SSubject.Count -eq 1 -and
            [string]$Txn4SSubject[0] -ceq $Txn4STrigger) {
            $Txn4SContext = 'UE5.5.4/ARM-CLEAN06.4S'
            $Txn4SPhase = 'START'
            try {
                Publish-CommitStatus -Sha $Sha -State 'pending' -Context $Txn4SContext -Description '84 binaires: manifeste ferme et rollback dans bac a sable prive'
                $Txn4SScript = Join-Path $WorktreePath 'Scripts\AuditARM_CLEAN06_4S_TransactionRehearsal.ps1'
                if (-not (Test-Path -LiteralPath $Txn4SScript -PathType Leaf)) {
                    throw 'ARM-CLEAN06.4S: missing sealed transaction rehearsal script.'
                }
                $Txn4SPhase = 'PARSE'
                $Txn4STokens = $null
                $Txn4SErrors = $null
                [void][System.Management.Automation.Language.Parser]::ParseFile(
                    $Txn4SScript, [ref]$Txn4STokens, [ref]$Txn4SErrors)
                if ($Txn4SErrors -and $Txn4SErrors.Count -gt 0) {
                    throw 'ARM-CLEAN06.4S: invalid PowerShell syntax.'
                }
                foreach ($Txn4SValue in @($Txn4SScript,$WorktreePath,$EngineRoot,$StateRoot,$RunDir,$GitHubRepository)) {
                    if ($Txn4SValue.Contains('"')) {throw 'ARM-CLEAN06.4S: unsafe quote in path.'}
                }
                $Txn4SArgs = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' +
                    $Txn4SScript + '" -WorktreePath "' + $WorktreePath +
                    '" -EngineRoot "' + $EngineRoot + '" -StateRoot "' + $StateRoot +
                    '" -RunDir "' + $RunDir + '" -ExpectedHead "' + $Sha +
                    '" -GitHubRepository "' + $GitHubRepository + '"'
                $Txn4SOut = Join-Path $RunDir 'ARM064S.stdout.log'
                $Txn4SErr = Join-Path $RunDir 'ARM064S.stderr.log'
                $Txn4SPhase = 'PROCESS_RUNNING'
                $Txn4SProcess = Start-Process -FilePath $PsExe -ArgumentList $Txn4SArgs -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput $Txn4SOut -RedirectStandardError $Txn4SErr -PassThru -Wait -ErrorAction Stop
                if ($Txn4SProcess.ExitCode -ne 0) {
                    throw 'ARM-CLEAN06.4S: signed sandbox transaction rehearsal failed.'
                }
                Publish-CommitStatus -Sha $Sha -State 'success' -Context $Txn4SContext -Description '84 sources/restaurations verifiees; 0 binaire et 0 SaveGame ecrits'
                Write-Host '[ARM-CLEAN06.4S] Sealed private rollback rehearsal verified; migration still blocked.'
            }
            catch {
                $Txn4SStagePath = Join-Path $RunDir 'ARMClean064S.stage'
                $Txn4SStage = if (Test-Path -LiteralPath $Txn4SStagePath -PathType Leaf) {
                    ([string](Get-Content -LiteralPath $Txn4SStagePath -TotalCount 1)).Trim()
                } else { $Txn4SPhase }
                if ($Txn4SStage -notin @('PREFLIGHT','HEAD_MISMATCH','WORKTREE_CHANGED',
                    'READ_4K_LOCATE','READ_4K_MISSING','READ_4K_MULTIPLE',
                    'READ_4K_DUPLICATE_PROVENANCE','READ_4K_NO_TRUSTED_COPY',
                    'READ_4K_COMPARE_COPIES','READ_4K_CONFLICTING_COPIES',
                    'READ_4K_DUPLICATES_RECONCILED','READ_4K_GATE_EXISTS',
                    'READ_4K_GATE_READ','READ_4K_GATE_SCHEMA','READ_4K_REPORT_HASH',
                    'READ_4K_COUNTERS','READ_4K_GATE_CLOSED','VERIFY_4K_DOMAINS',
                    'REHASH_84','READ_4R','READ_4R_DUPLICATE_PROVENANCE',
                    'READ_4R_NO_TRUSTED_COPY','READ_4R_COMPARE_COPIES',
                    'READ_4R_CONFLICTING_COPIES','READ_4R_DUPLICATES_RECONCILED',
                    'PREPARE_SANDBOX','BUILD_TRANSACTION_MANIFEST',
                    'REHEARSE_STAGING','REHEARSE_DETECTION','REHEARSE_ROLLBACK',
                    'VERIFY_ALL_AFTER_ROLLBACK','CLEAN_PRIVATE_SANDBOX',
                    'WRITE_CLOSED_GATE','VERIFY_CLEAN','ISSUE_BUILD',
                    'ISSUE_WRITE','SUCCESS','START','PARSE','PROCESS_RUNNING')) {
                    $Txn4SStage = 'UNKNOWN'
                }
                try {
                    Publish-CommitStatus -Sha $Sha -State 'failure' -Context $Txn4SContext -Description ('Transaction 06.4S bloquee : ' + $Txn4SStage)
                }
                catch {Write-Warning '[ARM-CLEAN06.4S] Status publication failed.'}
                Write-Warning ('[ARM-CLEAN06.4S] Closed sandbox transaction incomplete: ' + $Txn4SStage)
            }
        }

        # ARM-CLEAN06.4T: real UE Editor one-shot materialization.
        # Only this freshly-created ChatGPT code commit may be amended;
        # rewritten SHA has the same parent and is fully validated next cycle.
        $Asset4TTrigger = 'feat(ARM-CLEAN06.4T): materialize real canonical nonarm items in UE Editor'
        $Asset4TSubject = @(& git -C $WorktreePath log -1 --format=%s)
        if ($LASTEXITCODE -eq 0 -and $Asset4TSubject.Count -eq 1 -and
            [string]$Asset4TSubject[0] -ceq $Asset4TTrigger) {
            $Asset4TContext = 'UE5.5.4/ARM-CLEAN06.4T'
            $Asset4TStage = 'START'
            try {
                Publish-CommitStatus -Sha $Sha -State 'pending' -Context $Asset4TContext -Description 'Migration UE reelle: assets non-ARM, verif sauvegardes et Git LFS'
                $Asset4TScript = Join-Path $WorktreePath 'Scripts\ApplyARM_CLEAN06_4T_NonArmEditorMigration.ps1'
                if (-not (Test-Path -LiteralPath $Asset4TScript -PathType Leaf)) {
                    throw 'ARM-CLEAN06.4T: missing native migration script.'
                }
                $Asset4TTokens = $null
                $Asset4TErrors = $null
                [void][System.Management.Automation.Language.Parser]::ParseFile(
                    $Asset4TScript, [ref]$Asset4TTokens, [ref]$Asset4TErrors)
                if ($Asset4TErrors -and $Asset4TErrors.Count -gt 0) {
                    throw 'ARM-CLEAN06.4T: invalid PowerShell migration script.'
                }
                foreach ($Asset4TValue in @($Asset4TScript,$WorktreePath,$EngineRoot,$StateRoot,$RunDir,$GitHubRepository)) {
                    if ($Asset4TValue.Contains('"')) {throw 'ARM-CLEAN06.4T: unsafe quote in path.'}
                }
                $Asset4TArgs = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' +
                    $Asset4TScript + '" -WorktreePath "' + $WorktreePath +
                    '" -EngineRoot "' + $EngineRoot + '" -StateRoot "' + $StateRoot +
                    '" -RunDir "' + $RunDir + '" -ExpectedHead "' + $Sha +
                    '" -GitHubRepository "' + $GitHubRepository + '"'
                $Asset4TOut = Join-Path $RunDir 'ARM064T.stdout.log'
                $Asset4TErr = Join-Path $RunDir 'ARM064T.stderr.log'
                $Asset4TProcess = Start-Process -FilePath $PsExe -ArgumentList $Asset4TArgs -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput $Asset4TOut -RedirectStandardError $Asset4TErr -PassThru -Wait -ErrorAction Stop
                if ($Asset4TProcess.ExitCode -ne 0) {
                    throw 'ARM-CLEAN06.4T: native asset migration/verification failed.'
                }
                Publish-CommitStatus -Sha $Sha -State 'success' -Context $Asset4TContext -Description 'Conversion UE limitee aux vrais DataAssets, parite verifiee; aucun fallback'
                Write-Host '[ARM-CLEAN06.4T] Native asset authoring or committed verification succeeded.'
            }
            catch {
                $Asset4TStagePath = Join-Path $RunDir 'ARMClean064T.stage'
                $Asset4TStage = if (Test-Path -LiteralPath $Asset4TStagePath -PathType Leaf) {
                    ([string](Get-Content -LiteralPath $Asset4TStagePath -TotalCount 1)).Trim()
                } else { 'PROCESS_OR_PARSE' }
                if ($Asset4TStage -notin @(
                    'PREFLIGHT','HEAD_MISMATCH','WORKTREE_CHANGED',
                    'READ_4K_LOCATE','READ_4K_MISSING','READ_4K_GATE_EXISTS',
                    'READ_4K_GATE_READ','READ_4K_GATE_SCHEMA','READ_4K_REPORT_HASH',
                    'READ_4K_COUNTERS','READ_4K_GATE_CLOSED','VERIFY_4K_DOMAINS',
                    'READ_4K_DUPLICATE_PROVENANCE','READ_4K_NO_TRUSTED_COPY',
                    'READ_4K_COMPARE_COPIES','READ_4K_CONFLICTING_COPIES',
                    'READ_4K_DUPLICATES_RECONCILED','REHASH_84',
                    'PROVE_SIGNED_SOURCE','APPLY_UNREAL','VERIFY_COMMITTED',
                    'UNREAL_MATERIALIZATION_FAILED','VALIDATE_REPORT',
                    'CHECK_EXACT_UNCOMMITTED_FILESET','VERIFY_RELOADED_UNREAL',
                    'UNREAL_REOPEN_FAILED','POST_MIGRATION_AUTOMATION',
                    'POST_AUTOMATION_FAILED','REMOTE_LEASE','STAGE_REAL_UNREAL_ASSETS',
                    'AMEND_ONLY_OWN_COMMIT','PUSH_WITH_LEASE',
                    'VERIFY_COMMIT_FILESET','CHECK_CLEAN','PUBLISH_FINAL_RECEIPT',
                    'SUCCESS','PROCESS_OR_PARSE')) {
                    $Asset4TStage = 'UNKNOWN'
                }
                try {
                    Publish-CommitStatus -Sha $Sha -State 'failure' -Context $Asset4TContext -Description ('Migration 06.4T refusee: '+$Asset4TStage)
                }
                catch {Write-Warning '[ARM-CLEAN06.4T] Cannot publish failed status.'}
                Write-Warning ('[ARM-CLEAN06.4T] Native UE asset migration blocked: '+$Asset4TStage)
            }
        }

        # ARM-CLEAN06.4U: real UE full QuickItem action migration.
        # Only this freshly-created ChatGPT code commit may be amended;
        # rewritten SHA has the same parent and is fully validated next cycle.
        $Asset4UTrigger = 'feat(ARM-CLEAN06.4U): migrate authored nonarm QuickItems into real action catalogue'
        $Asset4USubject = @(& git -C $WorktreePath log -1 --format=%s)
        if ($LASTEXITCODE -eq 0 -and $Asset4USubject.Count -eq 1 -and
            [string]$Asset4USubject[0] -ceq $Asset4UTrigger) {
            $Asset4UContext = 'UE5.5.4/ARM-CLEAN06.4U'
            $Asset4UStage = 'START'
            try {
                Publish-CommitStatus -Sha $Sha -State 'pending' -Context $Asset4UContext -Description 'QuickItem: vrai catalogue actions/objets, relecture et tests UE'
                $Asset4UScript = Join-Path $WorktreePath 'Scripts\ApplyARM_CLEAN06_4U_NonArmQuickMigration.ps1'
                if (-not (Test-Path -LiteralPath $Asset4UScript -PathType Leaf)) {
                    throw 'ARM-CLEAN06.4U: missing native migration script.'
                }
                $Asset4UTokens = $null
                $Asset4UErrors = $null
                [void][System.Management.Automation.Language.Parser]::ParseFile(
                    $Asset4UScript, [ref]$Asset4UTokens, [ref]$Asset4UErrors)
                if ($Asset4UErrors -and $Asset4UErrors.Count -gt 0) {
                    throw 'ARM-CLEAN06.4U: invalid PowerShell migration script.'
                }
                foreach ($Asset4UValue in @($Asset4UScript,$WorktreePath,$EngineRoot,$StateRoot,$RunDir,$GitHubRepository)) {
                    if ($Asset4UValue.Contains('"')) {throw 'ARM-CLEAN06.4U: unsafe quote in path.'}
                }
                $Asset4UArgs = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' +
                    $Asset4UScript + '" -WorktreePath "' + $WorktreePath +
                    '" -EngineRoot "' + $EngineRoot + '" -StateRoot "' + $StateRoot +
                    '" -RunDir "' + $RunDir + '" -ExpectedHead "' + $Sha +
                    '" -GitHubRepository "' + $GitHubRepository + '"'
                $Asset4UOut = Join-Path $RunDir 'ARM064U.stdout.log'
                $Asset4UErr = Join-Path $RunDir 'ARM064U.stderr.log'
                $Asset4UProcess = Start-Process -FilePath $PsExe -ArgumentList $Asset4UArgs -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput $Asset4UOut -RedirectStandardError $Asset4UErr -PassThru -Wait -ErrorAction Stop
                if ($Asset4UProcess.ExitCode -ne 0) {
                    throw 'ARM-CLEAN06.4U: native asset migration/verification failed.'
                }
                Publish-CommitStatus -Sha $Sha -State 'success' -Context $Asset4UContext -Description '3 QuickItems complets et catalogue actions verifies en UAssets; 0 fallback'
                Write-Host '[ARM-CLEAN06.4U] Native asset authoring or committed verification succeeded.'
            }
            catch {
                $Asset4UStagePath = Join-Path $RunDir 'ARMClean064U.stage'
                $Asset4UStage = if (Test-Path -LiteralPath $Asset4UStagePath -PathType Leaf) {
                    ([string](Get-Content -LiteralPath $Asset4UStagePath -TotalCount 1)).Trim()
                } else { 'PROCESS_OR_PARSE' }
                if ($Asset4UStage -notin @(
                    'PREFLIGHT','HEAD_MISMATCH','WORKTREE_CHANGED',
                    'READ_4K_LOCATE','READ_4K_MISSING','READ_4K_GATE_EXISTS',
                    'READ_4K_GATE_READ','READ_4K_GATE_SCHEMA','READ_4K_REPORT_HASH',
                    'READ_4K_COUNTERS','READ_4K_GATE_CLOSED','VERIFY_4K_DOMAINS',
                    'READ_4K_DUPLICATE_PROVENANCE','READ_4K_NO_TRUSTED_COPY',
                    'READ_4K_COMPARE_COPIES','READ_4K_CONFLICTING_COPIES',
                    'READ_4K_DUPLICATES_RECONCILED','REHASH_84',
                    'PROVE_PARENT_4T','APPLY_UNREAL','VERIFY_COMMITTED',
                    'UNREAL_MATERIALIZATION_FAILED','VALIDATE_REPORT',
                    'CHECK_EXACT_UNCOMMITTED_FILESET','VERIFY_RELOADED_UNREAL',
                    'UNREAL_REOPEN_FAILED','POST_MIGRATION_AUTOMATION',
                    'POST_AUTOMATION_FAILED','REMOTE_LEASE','STAGE_REAL_UNREAL_ASSETS',
                    'AMEND_ONLY_OWN_COMMIT','PUSH_WITH_LEASE',
                    'VERIFY_COMMIT_FILESET','CHECK_CLEAN','PUBLISH_FINAL_RECEIPT',
                    'SUCCESS','PROCESS_OR_PARSE')) {
                    $Asset4UStage = 'UNKNOWN'
                }
                try {
                    Publish-CommitStatus -Sha $Sha -State 'failure' -Context $Asset4UContext -Description ('Migration 06.4U refusee: '+$Asset4UStage)
                }
                catch {Write-Warning '[ARM-CLEAN06.4U] Cannot publish failed status.'}
                Write-Warning ('[ARM-CLEAN06.4U] Native UE asset migration blocked: '+$Asset4UStage)
            }
        }

        # ARM-CLEAN06.4V: native nonarm equipped action and presentation migration.
        # Only this freshly-created ChatGPT code commit may be amended;
        # rewritten SHA has the same parent and is fully validated next cycle.
        $Asset4VTrigger = 'feat(ARM-CLEAN06.4V): migrate last nonarm equipment and authored presentation'
        $Asset4VSubject = @(& git -C $WorktreePath log -1 --format=%s)
        if ($LASTEXITCODE -eq 0 -and $Asset4VSubject.Count -eq 1 -and
            [string]$Asset4VSubject[0] -ceq $Asset4VTrigger) {
            $Asset4VContext = 'UE5.5.4/ARM-CLEAN06.4V'
            $Asset4VStage = 'START'
            try {
                Publish-CommitStatus -Sha $Sha -State 'pending' -Context $Asset4VContext -Description 'Deux DataAssets non-ARM et presentation complete, relecture UE'
                $Asset4VScript = Join-Path $WorktreePath 'Scripts\ApplyARM_CLEAN06_4V_NonArmEquipmentMigration.ps1'
                if (-not (Test-Path -LiteralPath $Asset4VScript -PathType Leaf)) {
                    throw 'ARM-CLEAN06.4V: missing native migration script.'
                }
                $Asset4VTokens = $null
                $Asset4VErrors = $null
                [void][System.Management.Automation.Language.Parser]::ParseFile(
                    $Asset4VScript, [ref]$Asset4VTokens, [ref]$Asset4VErrors)
                if ($Asset4VErrors -and $Asset4VErrors.Count -gt 0) {
                    throw 'ARM-CLEAN06.4V: invalid PowerShell migration script.'
                }
                foreach ($Asset4VValue in @($Asset4VScript,$WorktreePath,$EngineRoot,$StateRoot,$RunDir,$GitHubRepository)) {
                    if ($Asset4VValue.Contains('"')) {throw 'ARM-CLEAN06.4V: unsafe quote in path.'}
                }
                $Asset4VArgs = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' +
                    $Asset4VScript + '" -WorktreePath "' + $WorktreePath +
                    '" -EngineRoot "' + $EngineRoot + '" -StateRoot "' + $StateRoot +
                    '" -RunDir "' + $RunDir + '" -ExpectedHead "' + $Sha +
                    '" -GitHubRepository "' + $GitHubRepository + '"'
                $Asset4VOut = Join-Path $RunDir 'ARM064V.stdout.log'
                $Asset4VErr = Join-Path $RunDir 'ARM064V.stderr.log'
                $Asset4VProcess = Start-Process -FilePath $PsExe -ArgumentList $Asset4VArgs -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput $Asset4VOut -RedirectStandardError $Asset4VErr -PassThru -Wait -ErrorAction Stop
                if ($Asset4VProcess.ExitCode -ne 0) {
                    throw 'ARM-CLEAN06.4V: native asset migration/verification failed.'
                }
                Publish-CommitStatus -Sha $Sha -State 'success' -Context $Asset4VContext -Description '34 definitions, une action equipee et presentation UAssets; 0 fallback'
                Write-Host '[ARM-CLEAN06.4V] Native asset authoring or committed verification succeeded.'
            }
            catch {
                $Asset4VStagePath = Join-Path $RunDir 'ARMClean064V.stage'
                $Asset4VStage = if (Test-Path -LiteralPath $Asset4VStagePath -PathType Leaf) {
                    ([string](Get-Content -LiteralPath $Asset4VStagePath -TotalCount 1)).Trim()
                } else { 'PROCESS_OR_PARSE' }
                if ($Asset4VStage -notin @(
                    'PREFLIGHT','HEAD_MISMATCH','WORKTREE_CHANGED',
                    'READ_4K_LOCATE','READ_4K_MISSING','READ_4K_GATE_EXISTS',
                    'READ_4K_GATE_READ','READ_4K_GATE_SCHEMA','READ_4K_REPORT_HASH',
                    'READ_4K_COUNTERS','READ_4K_GATE_CLOSED','VERIFY_4K_DOMAINS',
                    'READ_4K_DUPLICATE_PROVENANCE','READ_4K_NO_TRUSTED_COPY',
                    'READ_4K_COMPARE_COPIES','READ_4K_CONFLICTING_COPIES',
                    'READ_4K_DUPLICATES_RECONCILED','REHASH_84',
                    'PROVE_PARENT_4U','APPLY_UNREAL','VERIFY_COMMITTED',
                    'UNREAL_MATERIALIZATION_FAILED','VALIDATE_REPORT',
                    'CHECK_EXACT_UNCOMMITTED_FILESET','VERIFY_RELOADED_UNREAL',
                    'UNREAL_REOPEN_FAILED','POST_MIGRATION_AUTOMATION',
                    'POST_AUTOMATION_FAILED','REMOTE_LEASE','STAGE_REAL_UNREAL_ASSETS',
                    'AMEND_ONLY_OWN_COMMIT','PUSH_WITH_LEASE',
                    'VERIFY_COMMIT_FILESET','CHECK_CLEAN','PUBLISH_FINAL_RECEIPT',
                    'SUCCESS','PROCESS_OR_PARSE')) {
                    $Asset4VStage = 'UNKNOWN'
                }
                try {
                    Publish-CommitStatus -Sha $Sha -State 'failure' -Context $Asset4VContext -Description ('Migration 06.4V refusee: '+$Asset4VStage)
                }
                catch {Write-Warning '[ARM-CLEAN06.4V] Cannot publish failed status.'}
                Write-Warning ('[ARM-CLEAN06.4V] Native UE asset migration blocked: '+$Asset4VStage)
            }
        }

        # ARM-CLEAN06.4W: actual level and palette canonical reference migration.
        # Only this freshly-created ChatGPT code commit may be amended;
        # rewritten SHA has the same parent and is fully validated next cycle.
        $Asset4WTrigger = 'feat(ARM-CLEAN06.4W): migrate real level and palette item references to canonical ItemIds'
        $Asset4WSubject = @(& git -C $WorktreePath log -1 --format=%s)
        if ($LASTEXITCODE -eq 0 -and $Asset4WSubject.Count -eq 1 -and
            [string]$Asset4WSubject[0] -ceq $Asset4WTrigger) {
            $Asset4WContext = 'UE5.5.4/ARM-CLEAN06.4W'
            $Asset4WStage = 'START'
            try {
                Publish-CommitStatus -Sha $Sha -State 'pending' -Context $Asset4WContext -Description '95 IDs dans 4 niveaux et 1 palette, relecture UE'
                $Asset4WScript = Join-Path $WorktreePath 'Scripts\ApplyARM_CLEAN06_4W_WorldReferenceMigration.ps1'
                if (-not (Test-Path -LiteralPath $Asset4WScript -PathType Leaf)) {
                    throw 'ARM-CLEAN06.4W: missing native migration script.'
                }
                $Asset4WTokens = $null
                $Asset4WErrors = $null
                [void][System.Management.Automation.Language.Parser]::ParseFile(
                    $Asset4WScript, [ref]$Asset4WTokens, [ref]$Asset4WErrors)
                if ($Asset4WErrors -and $Asset4WErrors.Count -gt 0) {
                    throw 'ARM-CLEAN06.4W: invalid PowerShell migration script.'
                }
                foreach ($Asset4WValue in @($Asset4WScript,$WorktreePath,$EngineRoot,$StateRoot,$RunDir,$GitHubRepository)) {
                    if ($Asset4WValue.Contains('"')) {throw 'ARM-CLEAN06.4W: unsafe quote in path.'}
                }
                $Asset4WArgs = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' +
                    $Asset4WScript + '" -WorktreePath "' + $WorktreePath +
                    '" -EngineRoot "' + $EngineRoot + '" -StateRoot "' + $StateRoot +
                    '" -RunDir "' + $RunDir + '" -ExpectedHead "' + $Sha +
                    '" -GitHubRepository "' + $GitHubRepository + '"'
                $Asset4WOut = Join-Path $RunDir 'ARM064W.stdout.log'
                $Asset4WErr = Join-Path $RunDir 'ARM064W.stderr.log'
                $Asset4WProcess = Start-Process -FilePath $PsExe -ArgumentList $Asset4WArgs -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput $Asset4WOut -RedirectStandardError $Asset4WErr -PassThru -Wait -ErrorAction Stop
                if ($Asset4WProcess.ExitCode -ne 0) {
                    throw 'ARM-CLEAN06.4W: native asset migration/verification failed.'
                }
                Publish-CommitStatus -Sha $Sha -State 'success' -Context $Asset4WContext -Description '95 references canoniques sur 5 UAssets reouverts; 0 fallback'
                Write-Host '[ARM-CLEAN06.4W] Native asset authoring or committed verification succeeded.'
            }
            catch {
                $Asset4WStagePath = Join-Path $RunDir 'ARMClean064W.stage'
                $Asset4WStage = if (Test-Path -LiteralPath $Asset4WStagePath -PathType Leaf) {
                    ([string](Get-Content -LiteralPath $Asset4WStagePath -TotalCount 1)).Trim()
                } elseif ($Asset4WErrors -and $Asset4WErrors.Count -gt 0) {
                    'POWERSHELL_PARSE'
                } else { 'PROCESS_OR_PARSE' }
                if ($Asset4WStage -notin @(
                    'START','SCRIPT_STARTED','CHECK_GIT_PARENT',
                    'CHECK_SOURCE_FILES_LFS','CHECK_COMMIT_FILESET',
                    'HYDRATE_SOURCE_FILES_LFS','LFS_DOWNLOAD_FAILED',
                    'LFS_CHECKOUT_SOURCE_FILES','LFS_CHECKOUT_FAILED',
                    'LFS_SIGNED_BACKUP_PREFLIGHT',
                    'LFS_BACKUP_SCAN_RUNS','LFS_BACKUP_READ_RECEIPT',
                    'LFS_BACKUP_CHECK_UNIQUE','LFS_BACKUP_READ_GATE',
                    'LFS_BACKUP_REHASH_RECEIPT','LFS_BACKUP_VERIFY_GATE',
                    'LFS_BACKUP_PROVE_OIDS','LFS_BACKUP_PREPARE_ENTRY',
                    'LFS_PINNED_HEAD_DIVERGED','LFS_FINAL_BINARY_UNAVAILABLE',
                    'LFS_BACKUP_READ_GIT_POINTER','LFS_BACKUP_GIT_POINTER_MISSING',
                    'LFS_BACKUP_READ_LOCAL_POINTER',
                    'LFS_BACKUP_VALIDATE_POINTER','LFS_BACKUP_LOOKUP_RECORD',
                    'LFS_BACKUP_REHASH_FILE','LFS_BACKUP_COPY_VERIFIED',
                    'LFS_SIGNED_BACKUP_MISSING',
                    'LFS_SIGNED_BACKUP_INVALID','LFS_BACKUP_OID_MISMATCH',
                    'LFS_FILE_COUNT_MISMATCH',
                    'LFS_PIN_COUNT_MISMATCH','LFS_PIN_PATH_MISMATCH',
                    'LFS_LOCAL_PLACEHOLDER_DIRTY','LFS_SNAPSHOT_OID_MISMATCH',
                    'LFS_BACKUP_COPY_HASH_MISMATCH',
                    'LFS_SIGNED_BACKUP_HYDRATED',
                    'LFS_POINTER_STILL_PRESENT','LFS_POST_RESTORE_POINTER',
                    'LFS_INVALID_PACKAGE_HEADER',
                    'LFS_SOURCE_BINARY_NOT_HYDRATED',
                    'HEAD_MISMATCH','WORKTREE_DIRTY','POWERSHELL_PARSE',
                    'SAVE_PRIVATE_BACKUPS','VERIFY_COMMITTED',
                    'APPLY_UNREAL','UNREAL_MATERIALIZATION_FAILED',
                    'FINAL_BINARY_SET','PUBLISH_FINAL_RECEIPT',
                    'EXACT_FIVE_CHANGED','VERIFY_RELOADED_UNREAL',
                    'UNREAL_REOPEN_FAILED','POST_MIGRATION_AUTOMATION',
                    'POST_AUTOMATION_FAILED','REMOTE_LEASE',
                    'STAGE_FIVE_REAL_FILES','AMEND_ONLY_OWN_COMMIT',
                    'PUSH_WITH_LEASE','SUCCESS','PROCESS_OR_PARSE')) {
                    $Asset4WStage = 'UNKNOWN'
                }
                try {
                    Publish-CommitStatus -Sha $Sha -State 'failure' -Context $Asset4WContext -Description ('Migration 06.4W refusee: '+$Asset4WStage)
                }
                catch {Write-Warning '[ARM-CLEAN06.4W] Cannot publish failed status.'}
                # The GitHub connector exposes state but not the status description.
                # Publish only the public allowlisted gate phase so the repair
                # does not require a manual gh api round-trip. Never upload logs.
                try {
                    $PhaseFolder = Join-Path $StateRoot 'ARM-CLEAN06.4W-Diagnostics'
                    $PhaseMarker = Join-Path $PhaseFolder ($Sha + '.issue-created')
                    if (-not (Test-Path -LiteralPath $PhaseMarker -PathType Leaf)) {
                        New-Item -Path $PhaseFolder -ItemType Directory -Force -ErrorAction Stop | Out-Null
                        $PhaseBody = Join-Path $RunDir 'ARM064W.failure-public.md'
                        @(
                            '# ARM-CLEAN06.4W - diagnostic automatise'
                            ''
                            ('Commit de validation : '+$Sha)
                            ('Controle : '+$Asset4WContext)
                            ('Phase publique limitee : '+$Asset4WStage)
                            ''
                            'Aucun chemin local, journal UE, identifiant d item ou contenu prive n est publie.'
                        ) | Set-Content -LiteralPath $PhaseBody -Encoding UTF8
                        $PhaseTitle = '[ARM-CLEAN06.4W] Diagnostic '+$Sha.Substring(0,12)+' '+$Asset4WStage
                        $PhaseResult = @(& gh issue create --repo $GitHubRepository --title $PhaseTitle --body-file $PhaseBody)
                        if ($LASTEXITCODE -eq 0 -and $PhaseResult.Count -eq 1) {
                            Set-Content -LiteralPath $PhaseMarker -Value $PhaseResult[0] -Encoding ASCII
                        }
                    }
                }
                catch {Write-Warning '[ARM-CLEAN06.4W] Public phase receipt unavailable; private logs remain sealed.'}
                Write-Warning ('[ARM-CLEAN06.4W] Native UE asset migration blocked: '+$Asset4WStage)
            }
        }

        # ARM-CLEAN03.3: one-shot binary migration only after full Build+Grimrock
        # success and only for the exact opt-in commit subject. Runs in the
        # isolated detached worktree; never writes to the user's checkout.
        $CompactTrigger = 'feat(ARM-CLEAN03.3): enable one-shot guarded catalogue migration'
        $SubjectLines = @(& git -C $WorktreePath log -1 --format=%s)
        if ($LASTEXITCODE -eq 0 -and $SubjectLines.Count -eq 1 -and
            [string]$SubjectLines[0] -ceq $CompactTrigger) {
            $CompactStatus = 'UE5.5.4/WeaponCompact'
            try {
                Publish-CommitStatus -Sha $Sha -State 'pending' -Context $CompactStatus -Description 'Migration compacte en cours (commandlet + suite Grimrock)'
                $CompactScript = Join-Path $WorktreePath 'Scripts\PublishARMCompactCatalogue.ps1'
                if (-not (Test-Path -LiteralPath $CompactScript -PathType Leaf)) {
                    throw 'ARM-CLEAN03: dedicated migration helper absent.'
                }
                $CompactTokens = $null
                $CompactErrors = $null
                [void][System.Management.Automation.Language.Parser]::ParseFile(
                    $CompactScript, [ref]$CompactTokens, [ref]$CompactErrors)
                if ($null -ne $CompactErrors -and $CompactErrors.Count -gt 0) {
                    throw 'ARM-CLEAN03: dedicated migration helper has invalid syntax.'
                }
                $CompactArgs = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' + $CompactScript +
                    '" -WorktreePath "' + $WorktreePath + '" -EngineRoot "' + $EngineRoot +
                    '" -ExpectedHead "' + $Sha + '" -RunDir "' + $RunDir + '"'
                $CompactOut = Join-Path $RunDir 'ARMCompactPublish.stdout.log'
                $CompactErr = Join-Path $RunDir 'ARMCompactPublish.stderr.log'
                $CompactProcess = Start-Process -FilePath $PsExe -ArgumentList $CompactArgs -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput $CompactOut -RedirectStandardError $CompactErr -PassThru -Wait -ErrorAction Stop
                if ($CompactProcess.ExitCode -ne 0) {
                    throw 'ARM-CLEAN03: migration returned a nonzero code.'
                }
                Publish-CommitStatus -Sha $Sha -State 'success' -Context $CompactStatus -Description 'Catalogue compacte publie; validation du SHA binaire au prochain cycle'
                Write-Host '[ARM-CLEAN03.3] Compact catalogue published; next SHA receives full UE validation.'
            }
            catch {
                $StagePath = Join-Path $RunDir 'ARMCompactMigration.stage'
                $SafeStage = if (Test-Path -LiteralPath $StagePath -PathType Leaf) {
                    ([string](Get-Content -LiteralPath $StagePath -TotalCount 1)).Trim()
                } else { 'PREFLIGHT' }
                if ($SafeStage -notin @('PREFLIGHT','COMMANDLET','COMMANDLET_FAILED',
                    'FILESET_UNEXPECTED','SIZE_INVALID','ASSET_SAVED','POST_AUTOMATION',
                    'POST_AUTOMATION_FAILED','PARITY_AND_AUTOMATION_OK','REMOTE_CHANGED',
                    'COMMIT_CREATED','PUSH_FAILED','SUCCESS')) {
                    $SafeStage = 'UNKNOWN'
                }
                try {
                    Publish-CommitStatus -Sha $Sha -State 'failure' -Context $CompactStatus -Description ("Migration compacte refusee : " + $SafeStage)
                }
                catch { Write-Warning '[ARM-CLEAN03.3] GitHub migration status unavailable.' }
                Write-Warning ("[ARM-CLEAN03.3] Migration not published; stage " + $SafeStage)
            }
        }
        $Result = 'success'
        Write-Host "[VALIDATION-AUTO01] SUCCES $Sha. Journaux : $RunDir"
    }
    catch {
        $RunError = $_.Exception.Message
        if ($Phase -eq 'publication') { $Result = 'infrastructure_error' }
        Write-Warning "[VALIDATION-AUTO01] ECHEC ($Phase) $Sha : $RunError"
        $PublicCause = if ($Phase -eq 'setup') {
            Get-PublicSetupCode -Message $RunError
        } else {
            $StageFile = Join-Path $ReportRoot 'Validation.stage'
            $StageCode = if (Test-Path -LiteralPath $StageFile -PathType Leaf) {
                ([string](Get-Content -LiteralPath $StageFile -TotalCount 1 -ErrorAction SilentlyContinue)).Trim()
            } else { '' }
            if ($StageCode -in @('BUILD_STARTED','BUILD_FAILED')) { 'BUILD' }
            elseif ($StageCode -like 'AUTOMATION_*') { 'AUTOMATION' }
            else { 'TESTS' }
        }
        try {
            Publish-CommitStatus -Sha $Sha -State 'failure' -Description "Echec $Phase/$PublicCause ($AutomationFilter)."
            $FailureStatusPublished = $true
        }
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
        # A failure issue can succeed even when the first commit-status POST
        # failed transiently. Retry the *same failure result* after reporting.
        # No new worktree actions, builds or duplicate issues are triggered.
        if ($Result -eq 'failure' -and -not $FailureStatusPublished) {
            for ($Retry = 0; $Retry -lt 2; $Retry++) {
                Start-Sleep -Seconds 2
                try {
                    Publish-CommitStatus -Sha $Sha -State 'failure' -Description "Echec $Phase/$PublicCause ($AutomationFilter)."
                    $FailureStatusPublished = $true
                    break
                }
                catch {
                    Write-Warning '[VALIDATION-AUTO01.12] GitHub failure status retry failed.'
                }
            }
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