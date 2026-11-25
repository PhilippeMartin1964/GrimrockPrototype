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
    $LastCode = -1
    for ($Attempt = 0; $Attempt -lt 3; ++$Attempt) {
        & gh api -X POST "repos/$GitHubRepository/statuses/$Sha" -f "state=$State" -f "context=$Context" -f "description=$Description" --silent
        $LastCode = $LASTEXITCODE
        if ($LastCode -eq 0) { return }
        if ($Attempt -lt 2) {
            Write-Warning "[VALIDATION-AUTO01] Publication statut GitHub echouee; nouvelle tentative bornee."
            Start-Sleep -Seconds (2 * ($Attempt + 1))
        }
    }
    throw "Publication du statut GitHub impossible apres 3 essais (code $LastCode)."
}

function Get-PublicSetupCode {
    param([string]$Message)
    if ($Message -match '(?i)Publication du statut GitHub') { return 'GITHUB_STATUS' }
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
            if ($Line -match '\bLNK(?:2019|2001)\b') {
                foreach ($Symbol in @('UWidgetTree','UPanelWidget','UWidgetBlueprint','UWidget','UK2Node_Variable','UBlueprint','GetObjectsWithOuter','FProperty','FObjectPropertyBase','UEdGraphNode','UEdGraph')) {
                    if ($Line.Contains($Symbol)) {
                        $Record = 'Edition liens : ' + $Symbol
                        if (-not $Safe.Contains($Record)) { $Safe.Add($Record) }
                    }
                }
            }
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
        $KnownStages = @('PREFLIGHT',
            'BUILD_STARTED','BUILD_FAILED','BUILD_OK',
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
                        if ($TestPath -ceq 'Grimrock.RPG.ARM.CLEAN06_5D3.RealHandThrowProjection' -and
                            $State -match '^(?i:fail|failed|error)$' -and
                            $Test.PSObject.Properties['entries']) {
                            foreach ($Entry in @($Test.entries)) {
                                if ($null -eq $Entry -or $null -eq $Entry.event) { continue }
                                if ([string]$Entry.event.type -notmatch '^(?i:error|fatal|warning)$') { continue }
                                $Message = [string]$Entry.event.message
                                $Category = 'OTHER'
                                if ($Message -match 'Native registry or exact production manifest rejected') { $Category = 'ROOT_MANIFEST' }
                                elseif ($Message -match 'Identity/visual/native source mismatch') { $Category = 'SOURCE_IDENTITY' }
                                elseif ($Message -match 'Specialized projectile mismatch') { $Category = 'SPECIALIZED_POLICY' }
                                elseif ($Message -match 'Ordinary physical/visual source rejected') { $Category = 'ORDINARY_SOURCE' }
                                elseif ($Message -match 'Hand-toss eligibility differs|Physical eligibility matches original|Explicit physical one-hand permission|Passive off-hand source') { $Category = 'PHYSICAL_PERMISSION' }
                                elseif ($Message -match 'Previously authored flight data contradicts source|Unconverted policy contains unexpected authored values') { $Category = 'AUTHORED_CONFLICT' }
                                elseif ($Message -match 'Resolved level lost ordinary flight data|Runtime resolves physical level') { $Category = 'LEVEL_RESOLUTION' }
                                elseif ($Message -match 'Partial migration or unexpected active item count') { $Category = 'COUNTS' }
                                elseif ($Message -match 'Projected root validates|Runtime accepts projected root') { $Category = 'PREVIEW_REGISTRY' }
                                elseif ($Message -match 'Exact speed|Exact signed arc|Exact lifetime|Exact impact offset|Flight rotation|Flight scale|Flight angular axis|Flight forward axis|Tumble|Spin') { $Category = 'FIELD_PARITY' }
                                elseif ($Message -match 'Original catalogues untouched') { $Category = 'SOURCE_MUTATION' }
                                $Record = 'ARM-CLEAN06.5D3 projection : ' + $Category
                                if (-not $Safe.Contains($Record)) { $Safe.Add($Record) }
                            }
                        }
                        # 5G85: fixed assertion categories, never raw messages.
                        if ($TestPath -ceq 'Grimrock.Editor.ARM.CLEAN06.5G85.WorldPointerPreflight' -and
                            $State -match '^(?i:fail|failed|error)$' -and $Test.PSObject.Properties['entries']) {
                            foreach ($Entry in @($Test.entries)) {
                                if ($null -eq $Entry -or $null -eq $Entry.event) { continue }
                                if ([string]$Entry.event.type -notmatch '^(?i:error|fatal|warning)$') { continue }
                                $Message=[string]$Entry.event.message
                                $Category='OTHER'
                                if ($Message -match 'Canonical world preflight accepts') { $Category='PHYSICAL_GATE' }
                                elseif ($Message -match 'Transient archive encodes|Decoded archive re-encodes') { $Category='ENCODE' }
                                elseif ($Message -match 'archive decodes') { $Category='DECODE' }
                                elseif ($Message -match 'All current saved properties round-trip') { $Category='REFLECTED_PARITY' }
                                elseif ($Message -match 'Projection covers both physical rows') { $Category='ROW_COUNT' }
                                elseif ($Message -match 'Private physical projection survives') { $Category='PROJECTION_PARITY' }
                                elseif ($Message -match 'Projection excludes both retired pointers') { $Category='POINTER_EXCLUSION' }
                                elseif ($Message -match 'Physical level change alters') { $Category='LEVEL_PROJECTION' }
                                elseif ($Message -match 'Invalid non-ARM level refuses') { $Category='INVALID_LEVEL' }
                                $Record='ARM-CLEAN06.5G85 synthetic proof : '+$Category
                                if (-not $Safe.Contains($Record)) { $Safe.Add($Record) }
                            }
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
- Suite : $PublicFilter
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

$PublicFilter = if ($AutomationFilter -cmatch '^Grimrock(?:\.[A-Za-z0-9_]+)*(?:\+Grimrock(?:\.[A-Za-z0-9_]+)*)*$' -and
    $AutomationFilter.Length -le 100) { $AutomationFilter } else { 'filtre local' }

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

    $RepositoryRoot = (Resolve-Path -LiteralPath $RepositoryRoot).Path
    if ([string]::IsNullOrWhiteSpace($WorktreePath)) {
        $WorktreePath = Join-Path (Split-Path -Parent $RepositoryRoot) 'GrimrockPrototype_UEValidation'
    }
    $WorktreePath = [IO.Path]::GetFullPath($WorktreePath)
    $UserRoot = $RepositoryRoot.TrimEnd('\')
    $ValidationRoot = $WorktreePath.TrimEnd('\')
    if ($ValidationRoot -ieq $UserRoot -or
        $ValidationRoot.StartsWith($UserRoot + '\', [StringComparison]::OrdinalIgnoreCase) -or
        $UserRoot.StartsWith($ValidationRoot + '\', [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Le worktree de validation doit etre hors du checkout utilisateur.'
    }
    $StateRoot = [IO.Path]::GetFullPath($StateRoot).TrimEnd('\')
    foreach ($Root in @($UserRoot, $ValidationRoot)) {
        if ($StateRoot -ieq $Root -or $StateRoot.StartsWith($Root + '\', [StringComparison]::OrdinalIgnoreCase)) {
            throw 'Les journaux et fichiers de l agent doivent etre hors des checkouts.'
        }
    }
    foreach ($Root in @($UserRoot, $ValidationRoot, $StateRoot)) {
        for ($Directory = [IO.DirectoryInfo]::new($Root); $null -ne $Directory; $Directory = $Directory.Parent) {
            if (Test-Path -LiteralPath $Directory.FullName) {
                $Item = Get-Item -LiteralPath $Directory.FullName -Force
                if (-not $Item.PSIsContainer -or ($Item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                    throw 'Repertoire de validation non ordinaire : verification du chemin impossible.'
                }
            }
        }
    }
    if ($GitHubRepository -cne 'PhilippeMartin1964/GrimrockPrototype') {
        throw 'Depot GitHub des statuts inattendu.'
    }

    # Transcript de l'agent des le preflight, meme si git/gh/UE echoue avant les tests.
    New-Item -ItemType Directory -Force -Path $StateRoot | Out-Null
    try {
        Start-Transcript -Path (Join-Path $StateRoot 'Agent.latest.log') -Force -ErrorAction Stop | Out-Null
        $TranscriptStarted = $true
    }
    catch { Write-Warning "Transcript indisponible : $($_.Exception.Message)" }

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
    $AgentFile = [IO.Path]::GetFileName($PSCommandPath)
    if ($AgentFile -match '^UEValidationAgent-([0-9a-f]{40})[.]ps1$' -and $Matches[1] -cne $Sha) {
        throw 'origin/master a change depuis le bootstrap. Le prochain cycle chargera le nouvel agent.'
    }
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
        Publish-CommitStatus -Sha $Sha -State 'pending' -Description "Validation Windows en cours : $PublicFilter"

        if (Test-Path -LiteralPath $WorktreePath) {
            if (-not (Test-Path -LiteralPath (Join-Path $WorktreePath '.git') -PathType Leaf)) {
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
            $Branch = @(Invoke-Checked -Executable 'git' -Arguments @('-C', $WorktreePath, 'branch', '--show-current'))
            if ($Branch.Count -ne 0) { throw 'Worktree doit etre detache avant validation.' }
            $Dirty = @(Invoke-Checked -Executable 'git' -Arguments @('-C', $WorktreePath, 'status', '--porcelain', '--untracked-files=all'))

            if ($Dirty.Count -ne 0) {
                throw 'Worktree de validation modifie : aucune restauration automatique.'
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
        # Validate Shipping on every new SHA in the dedicated worktree.
        # This is a compile-only gate; packaging is validated independently.
        $ValidateArguments += ' -BuildShipping'
        if ($UseRHI) { $ValidateArguments += ' -UseRHI' }
        $RunStderrLog = Join-Path $RunDir 'Agent.stderr.log'
        $Process = Start-Process -FilePath $PsExe -ArgumentList $ValidateArguments -WorkingDirectory $WorktreePath `
            -RedirectStandardOutput $RunLog -RedirectStandardError $RunStderrLog -WindowStyle Hidden -Wait -PassThru -ErrorAction Stop
        $ExitCode = $Process.ExitCode
        $Result = 'failure'

        if ($ExitCode -ne 0) { throw "ValidateUE.ps1 a retourne le code $ExitCode. Voir $RunLog et $RunStderrLog." }
        $StageFile = Join-Path $ReportRoot 'Validation.stage'
        if (-not (Test-Path -LiteralPath $StageFile -PathType Leaf) -or
            (Get-Content -LiteralPath $StageFile -Raw).Trim() -cne 'SUCCESS') {
            throw 'ValidateUE.ps1 n a pas confirme la fin de toutes les validations.'
        }
        $FinalHead = @(Invoke-Checked -Executable 'git' -Arguments @('-C', $WorktreePath, 'rev-parse', 'HEAD'))
        if ($FinalHead.Count -ne 1 -or $FinalHead[0] -cne $Sha) {
            throw 'Le SHA du worktree a change pendant la validation.'
        }
        $FinalChanges = @(Invoke-Checked -Executable 'git' -Arguments @('-C', $WorktreePath, 'status', '--porcelain', '--untracked-files=all'))
        if ($FinalChanges.Count -ne 0) {
            throw 'Worktree de validation modifie pendant les tests : validation refusee.'
        }
        $Phase = 'publication'
        Publish-CommitStatus -Sha $Sha -State 'success' -Description "Editor + Shipping + Automation OK ($PublicFilter)"

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
            Publish-CommitStatus -Sha $Sha -State 'failure' -Description "Echec $Phase/$PublicCause ($PublicFilter)."
            $FailureStatusPublished = $true
        }
        catch { Write-Warning "Impossible de publier le statut d'echec : $($_.Exception.Message)" }
        # Les incidents de recuperation Git/LFS restent reessayables au cycle suivant.
        # Une erreur de compilation/test est definitive pour ce SHA, sans issues multiples.
        if ($Result -eq 'failure') {
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
                    Publish-CommitStatus -Sha $Sha -State 'failure' -Description "Echec $Phase/$PublicCause ($PublicFilter)."
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
