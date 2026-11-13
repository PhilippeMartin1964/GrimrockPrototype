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
        try {
            Publish-EffectsInventoryIssue -Sha $Sha -WorktreePath $WorktreePath -NotBeforeUtc $EffectsInventoryStartUtc
        }
        catch {
            Write-Warning '[EFFECTS-ORG01.1] La publication du rapport est non bloquante.'
        }

        # ARM-CLEAN06.2E — one-shot semantic census in native UE. NO asset writes.
        # Triggered only on the exact authored subject and the isolated SHA.
        $SemanticsTrigger = 'fix(ARM-CLEAN06.2E.4.2): locate qualification runtime failure safely'
        $SemanticsSubject = @(& git -C $WorktreePath log -1 --format=%s)
        if ($LASTEXITCODE -eq 0 -and $SemanticsSubject.Count -eq 1 -and
            [string]$SemanticsSubject[0] -ceq $SemanticsTrigger) {
            $SemanticsContext = 'UE5.5.4/ARM-CLEAN06.2E.4.2'
            $SemanticsPhase = 'PREFLIGHT'
            try {
                Publish-CommitStatus -Sha $Sha -State 'pending' -Context $SemanticsContext -Description 'Audit UE des mains/familles en lecture seule'
                $SemanticsScript = Join-Path $WorktreePath 'Scripts\PreflightARM_CLEAN06_2_HandSemantics.ps1'
                if (-not (Test-Path -LiteralPath $SemanticsScript -PathType Leaf)) {
                    throw 'ARM-CLEAN06.2E: missing semantic preflight.'
                }
                $SemanticsPhase = 'PARSE'
                $SemanticsTokens = $null
                $SemanticsErrors = $null
                [void][System.Management.Automation.Language.Parser]::ParseFile(
                    $SemanticsScript, [ref]$SemanticsTokens, [ref]$SemanticsErrors)
                if ($SemanticsErrors -and $SemanticsErrors.Count -gt 0) {
                    throw 'ARM-CLEAN06.2E: invalid PowerShell syntax.'
                }
                foreach ($Value in @($SemanticsScript,$WorktreePath,$EngineRoot,$RunDir)) {
                    if ($Value.Contains('"')) { throw 'ARM-CLEAN06.2E: quoted path unsupported.' }
                }
                $SemanticsArgs = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' +
                    $SemanticsScript + '" -WorktreePath "' + $WorktreePath +
                    '" -EngineRoot "' + $EngineRoot + '" -ExpectedHead "' + $Sha +
                    '" -RunDir "' + $RunDir + '" -GitHubRepository "' + $GitHubRepository + '"'
                $SemanticsPhase = 'COMMANDLET'
                $SemanticsProcess = Start-Process -FilePath $PsExe -ArgumentList $SemanticsArgs -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput (Join-Path $RunDir 'ARMClean062E.stdout.log') -RedirectStandardError (Join-Path $RunDir 'ARMClean062E.stderr.log') -PassThru -Wait -ErrorAction Stop
                if ($SemanticsProcess.ExitCode -ne 0) {
                    throw 'ARM-CLEAN06.2E: native read-only semantic audit rejected.'
                }
                Publish-CommitStatus -Sha $Sha -State 'success' -Context $SemanticsContext -Description 'Qualification E4.2 traçable, 0 ecriture'
            }
            catch {
                $SemanticsStageFile = Join-Path $RunDir 'ARMClean062E.stage'
                $SemanticsStage = if (Test-Path -LiteralPath $SemanticsStageFile -PathType Leaf) {
                    ([string](Get-Content -LiteralPath $SemanticsStageFile -TotalCount 1)).Trim()
                } else { $SemanticsPhase }
                if ($SemanticsStage -notin @('PREFLIGHT','PARSE','COMMANDLET','REPORT','CHECK_CLEAN','ISSUE','SUCCESS',
                    'RECONCILE','QUALIFY','QUALIFY_HELPER','QUALIFY_PARSE','QUALIFY_SCHEMA','QUALIFY_EXEC','QUALIFY_REPORT',
                    'QUALIFY_CLASS_HIERARCHY','QUALIFY_EXPECTED_CLASS',
                    'E4_INIT','E4_INIT_FACTS','E4_ROW_START','E4_ROW_CLASS',
                    'E4_ROW_OPTOUT','E4_ROW_SLOTS','E4_ROW_SEMANTICS','E4_ROW_FACT',
                    'E4_ASSERT','E4_OUTPUT_MODEL','E4_OUTPUT_JSON','E4_EXEC',
                    'E4_BAD_PACKAGE_COUNT','E4_CLASS_COUNTS','E4_OPTOUT_COUNTS',
                    'E4_HAND_COUNTS','E4_KIND_COUNTS','E4_FOCUS_COUNTS','CMD_ARGUMENT','SOURCE_CATALOGUE','SOURCE_ARCHETYPE','ITEM_IDS','ASSET_COUNT',
                    'OUTPUT_JSON','REPORT_ABSENT','NATIVE_PROCESS','UNREAL_EXIT_255')) {
                    $SemanticsStage = 'UNKNOWN'
                }
                try {
                    Publish-CommitStatus -Sha $Sha -State 'failure' -Context $SemanticsContext -Description ('Audit UE 06.2E bloque : ' + $SemanticsStage)
                }
                catch { Write-Warning '[ARM-CLEAN06.2E] Failed to publish semantic audit status.' }
                Write-Warning ('[ARM-CLEAN06.2E] Read-only semantic census blocked at ' + $SemanticsStage)
            }
        }

        # ARM-CLEAN06.2C — exact one-shot read-only canonical number projection.
        # This is explicitly NOT an editor asset migration or a gameplay cutover.
        $ProjectionTrigger = 'chore(ARM-CLEAN06.2C): prove transient compact catalogue numeric parity'
        $ProjectionSubject = @(& git -C $WorktreePath log -1 --format=%s)
        if ($LASTEXITCODE -eq 0 -and $ProjectionSubject.Count -eq 1 -and
            [string]$ProjectionSubject[0] -ceq $ProjectionTrigger) {
            $ProjectionContext = 'UE5.5.4/ARM-CLEAN06.2C'
            $ProjectionPhase = 'PREFLIGHT'
            try {
                Publish-CommitStatus -Sha $Sha -State 'pending' -Context $ProjectionContext -Description 'Projection numerique canonique UE en lecture seule'
                $ProjectionScript = Join-Path $WorktreePath 'Scripts\PreflightARM_CLEAN06_2_Catalogue.ps1'
                if (-not (Test-Path -LiteralPath $ProjectionScript -PathType Leaf)) {
                    throw 'ARM-CLEAN06.2C: script missing.'
                }
                $ProjectionTokens = $null
                $ProjectionParseErrors = $null
                [void][System.Management.Automation.Language.Parser]::ParseFile(
                    $ProjectionScript, [ref]$ProjectionTokens, [ref]$ProjectionParseErrors)
                if ($ProjectionParseErrors -and $ProjectionParseErrors.Count -gt 0) {
                    throw 'ARM-CLEAN06.2C: invalid PowerShell syntax.'
                }
                foreach ($Value in @($ProjectionScript,$WorktreePath,$EngineRoot,$RunDir)) {
                    if ($Value.Contains('"')) { throw 'ARM-CLEAN06.2C: quoted path unsupported.' }
                }
                $ProjectionPhase = 'PROCESS'
                $ProjectionArgs = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' +
                    $ProjectionScript + '" -WorktreePath "' + $WorktreePath +
                    '" -EngineRoot "' + $EngineRoot + '" -ExpectedHead "' + $Sha +
                    '" -RunDir "' + $RunDir + '" -GitHubRepository "' + $GitHubRepository + '"'
                $ProjectionOut = Join-Path $RunDir 'ARM062C.stdout.log'
                $ProjectionErr = Join-Path $RunDir 'ARM062C.stderr.log'
                $ProjectionProcess = Start-Process -FilePath $PsExe -ArgumentList $ProjectionArgs -WorkingDirectory $WorktreePath -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput $ProjectionOut -RedirectStandardError $ProjectionErr -ErrorAction Stop
                if ($ProjectionProcess.ExitCode -ne 0) {
                    throw 'ARM-CLEAN06.2C: transient numeric projection rejected.'
                }
                Publish-CommitStatus -Sha $Sha -State 'success' -Context $ProjectionContext -Description 'Parite numerique UE 82x20; 260 passifs; 0 asset sauve'
            }
            catch {
                $ProjectionStagePath = Join-Path $RunDir 'ARMClean062C.stage'
                $ProjectionStage = if (Test-Path -LiteralPath $ProjectionStagePath -PathType Leaf) {
                    ([string](Get-Content -LiteralPath $ProjectionStagePath -TotalCount 1 -ErrorAction SilentlyContinue)).Trim()
                } else { $ProjectionPhase }
                if ($ProjectionStage -notin @('PREFLIGHT','COMMANDLET','REPORT',
                    'CHECK_CLEAN','ISSUE','SUCCESS','PROCESS')) {
                    $ProjectionStage = 'UNKNOWN'
                }
                try {
                    Publish-CommitStatus -Sha $Sha -State 'failure' -Context $ProjectionContext -Description ('Projection UE refusee : ' + $ProjectionStage)
                }
                catch { Write-Warning '[ARM-CLEAN06.2C] Projection status publication failed.' }
                Write-Warning ('[ARM-CLEAN06.2C] Read-only projection blocked: ' + $ProjectionStage)
            }
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