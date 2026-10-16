[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$EngineRoot,

    [switch]$SkipBuild,

    [switch]$KeepGeneratedOutputs
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

function Assert-FileExists {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,
        [Parameter(Mandatory = $true)]
        [string]$Label
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw ("Required {0} not found: {1}" -f $Label, $Path)
    }
}

function Assert-NoUnrealProcess {
    $Processes = @()
    $Processes += @(Get-Process -Name "UnrealEditor" -ErrorAction SilentlyContinue)
    $Processes += @(Get-Process -Name "UnrealEditor-Cmd" -ErrorAction SilentlyContinue)

    if ($Processes.Count -gt 0) {
        $Details = ($Processes | ForEach-Object { "{0} (PID {1})" -f $_.ProcessName, $_.Id }) -join ", "
        throw ("Unreal is still running and may lock project assets or DLLs: {0}. Close it before running the localization smoke pipeline." -f $Details)
    }
}

$ProjectRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..\..")).Path
$ProjectFiles = @(Get-ChildItem -LiteralPath $ProjectRoot -Filter *.uproject -File)
if ($ProjectFiles.Count -ne 1) {
    throw ("Expected exactly one .uproject under {0}; found {1}." -f $ProjectRoot, $ProjectFiles.Count)
}
$ProjectFile = $ProjectFiles[0]

if (-not (Test-Path -LiteralPath $EngineRoot -PathType Container)) {
    throw ("Engine root not found: {0}" -f $EngineRoot)
}
$EngineRoot = (Resolve-Path -LiteralPath $EngineRoot).Path
$EditorCmd = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$BuildBat = Join-Path $EngineRoot "Engine\Build\BatchFiles\Build.bat"
$ApplyTranslationsScript = Join-Path $PSScriptRoot "ApplyEnglishSmokeTranslations.ps1"
$ScriptValidator = Join-Path $PSScriptRoot "TestLocalizationScripts.ps1"
$LocalizationRoot = Join-Path $ProjectRoot "Content\Localization\Game"
$GameCsv = Join-Path $LocalizationRoot "Game.csv"
$EnglishPo = Join-Path $LocalizationRoot "en\Game.po"
$FrenchPo = Join-Path $LocalizationRoot "fr\Game.po"
$EnglishLocRes = Join-Path $LocalizationRoot "en\Game.locres"
$FrenchLocRes = Join-Path $LocalizationRoot "fr\Game.locres"
$EnglishArchive = Join-Path $LocalizationRoot "en\Game.archive"
$FrenchArchive = Join-Path $LocalizationRoot "fr\Game.archive"
$Manifest = Join-Path $LocalizationRoot "Game.manifest"
$ConflictReport = Join-Path $LocalizationRoot "Game_Conflicts.txt"
$LocMeta = Join-Path $LocalizationRoot "Game.locmeta"
$StringTableAsset = Join-Path $ProjectRoot "Content\Localization\StringTables\ST_UI.uasset"

$GatherConfigSource = Join-Path $ProjectRoot "Config\Localization\Game_Gather.ini"
$ExportConfigSource = Join-Path $ProjectRoot "Config\Localization\Game_Export.ini"
$ImportConfigSource = Join-Path $ProjectRoot "Config\Localization\Game_Import.ini"
$CompileConfigSource = Join-Path $ProjectRoot "Config\Localization\Game_Compile.ini"

Assert-FileExists -Path $EditorCmd -Label "UnrealEditor-Cmd.exe"
Assert-FileExists -Path $BuildBat -Label "Build.bat"
Assert-FileExists -Path $ApplyTranslationsScript -Label "English smoke translation script"
Assert-FileExists -Path $ScriptValidator -Label "localization PowerShell validator"
Assert-FileExists -Path $GatherConfigSource -Label "Gather config"
Assert-FileExists -Path $ExportConfigSource -Label "Export config"
Assert-FileExists -Path $ImportConfigSource -Label "Import config"
Assert-FileExists -Path $CompileConfigSource -Label "Compile config"
Assert-FileExists -Path (Join-Path $ProjectRoot "Localization\Source\UI.csv") -Label "UI localization CSV"
Assert-NoUnrealProcess

$Timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$ReportRoot = Join-Path $ProjectRoot ("Saved\Localization\UI-LOCALIZATION01.6\{0}" -f $Timestamp)
[void](New-Item -ItemType Directory -Path $ReportRoot -Force)
Write-Host ("Localization report directory: {0}" -f $ReportRoot)

# Unreal may rewrite GatherText config files by adding metadata comments.
# Never expose source-controlled configs to that mutation. Work on report-local
# copies and verify that the repository copies remain byte-for-byte unchanged.
$WorkingConfigRoot = Join-Path $ReportRoot "Config"
[void](New-Item -ItemType Directory -Path $WorkingConfigRoot -Force)

$GatherConfig = Join-Path $WorkingConfigRoot "Game_Gather.ini"
$ExportConfig = Join-Path $WorkingConfigRoot "Game_Export.ini"
$ImportConfig = Join-Path $WorkingConfigRoot "Game_Import.ini"
$CompileConfig = Join-Path $WorkingConfigRoot "Game_Compile.ini"

$SourceConfigPaths = @(
    $GatherConfigSource,
    $ExportConfigSource,
    $ImportConfigSource,
    $CompileConfigSource
)
$SourceConfigHashes = @{}
foreach ($SourceConfigPath in $SourceConfigPaths) {
    $SourceConfigHashes[$SourceConfigPath] = (Get-FileHash -LiteralPath $SourceConfigPath -Algorithm SHA256).Hash
}

Copy-Item -LiteralPath $GatherConfigSource -Destination $GatherConfig -Force
Copy-Item -LiteralPath $ExportConfigSource -Destination $ExportConfig -Force
Copy-Item -LiteralPath $ImportConfigSource -Destination $ImportConfig -Force
Copy-Item -LiteralPath $CompileConfigSource -Destination $CompileConfig -Force

$PowerShellExe = (Get-Command powershell.exe -ErrorAction Stop).Source

function New-FileSnapshot {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    $Exists = Test-Path -LiteralPath $Path -PathType Leaf
    $Snapshot = @{
        Exists = $Exists
        Bytes = $null
    }

    if ($Exists) {
        # Assign directly instead of returning the byte array through an if
        # expression: PowerShell enumerates an empty byte[] to no output.
        $Snapshot.Bytes = [System.IO.File]::ReadAllBytes($Path)
    }

    return $Snapshot
}

function Restore-FileSnapshot {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,
        [Parameter(Mandatory = $true)]
        [hashtable]$Snapshot
    )

    if ($Snapshot.Exists) {
        $Bytes = if ($null -eq $Snapshot.Bytes) {
            [byte[]]::new(0)
        }
        else {
            [byte[]]$Snapshot.Bytes
        }
        [System.IO.File]::WriteAllBytes($Path, $Bytes)
    }
    elseif (Test-Path -LiteralPath $Path -PathType Leaf) {
        Remove-Item -LiteralPath $Path -Force
    }
}

$GameCsvSnapshot = New-FileSnapshot -Path $GameCsv
$EnglishPoSnapshot = New-FileSnapshot -Path $EnglishPo
$FrenchPoSnapshot = New-FileSnapshot -Path $FrenchPo

# Gather/Import/Compile also rewrite tracked localization artifacts. Snapshot them
# so the default smoke run is observational and leaves the working tree clean.
$ManifestSnapshot = New-FileSnapshot -Path $Manifest
$ConflictReportSnapshot = New-FileSnapshot -Path $ConflictReport
$LocMetaSnapshot = New-FileSnapshot -Path $LocMeta
$EnglishArchiveSnapshot = New-FileSnapshot -Path $EnglishArchive
$FrenchArchiveSnapshot = New-FileSnapshot -Path $FrenchArchive
$EnglishLocResSnapshot = New-FileSnapshot -Path $EnglishLocRes
$FrenchLocResSnapshot = New-FileSnapshot -Path $FrenchLocRes

function Invoke-NativeStep {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Label,
        [Parameter(Mandatory = $true)]
        [string]$FilePath,
        [Parameter(Mandatory = $true)]
        [string[]]$ArgumentList
    )

    $SafeLabel = ($Label -replace '[^A-Za-z0-9._-]', '_')
    $LogPath = Join-Path $ReportRoot ("{0}.log" -f $SafeLabel)

    Write-Host ""
    Write-Host ("=== {0} ===" -f $Label)
    Write-Host ("Executable : {0}" -f $FilePath)
    Write-Host ("Log        : {0}" -f $LogPath)

    $ExitCode = $null
    $PreviousErrorActionPreference = $ErrorActionPreference
    try {
        # Native tools may legitimately write warnings to stderr. Do not let
        # Windows PowerShell turn those lines into terminating script errors;
        # the native exit code remains authoritative.
        $ErrorActionPreference = "Continue"
        & $FilePath @ArgumentList 2>&1 | Tee-Object -FilePath $LogPath
        $ExitCode = $LASTEXITCODE
    }
    catch {
        throw ("Unable to execute step '{0}'. See log: {1}. {2}" -f $Label, $LogPath, $_.Exception.Message)
    }
    finally {
        $ErrorActionPreference = $PreviousErrorActionPreference
    }

    if ($null -eq $ExitCode) {
        throw ("Step '{0}' did not return a native process exit code. See log: {1}" -f $Label, $LogPath)
    }

    if ([int]$ExitCode -ne 0) {
        throw ("Step '{0}' failed with exit code {1}. See log: {2}" -f $Label, $ExitCode, $LogPath)
    }

    Write-Host ("[OK] {0}" -f $Label)
}

function Invoke-UnrealStep {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Label,
        [Parameter(Mandatory = $true)]
        [string[]]$Arguments
    )

    $CommonArgs = @("-unattended", "-nop4", "-nosplash", "-nullrhi", "-utf8output")
    Invoke-NativeStep -Label $Label -FilePath $EditorCmd -ArgumentList (@($ProjectFile.FullName) + $Arguments + $CommonArgs)
}

function Assert-GatherIntegrity {
    param(
        [Parameter(Mandatory = $true)]
        [string]$ManifestPath,
        [Parameter(Mandatory = $true)]
        [string]$ConflictReportPath
    )

    Assert-FileExists -Path $ManifestPath -Label "generated localization manifest"
    Assert-FileExists -Path $ConflictReportPath -Label "generated localization conflict report"

    $ManifestContent = [System.IO.File]::ReadAllText($ManifestPath, [System.Text.Encoding]::UTF8)

    $ForbiddenManifestPatterns = [ordered]@{
        "UMG TextBlock placeholder key" = "TextBlockDefaultValue"
        "UMG TextBlock placeholder source" = '"Text": "Text Block"'
        "test fixture subtree" = "/Game/GrimrockPrototype/Core/DataAssets/Tests/"
        "test note fixture" = "DA_Item_TestNote"
        "test book page fixture" = "DA_BookPage_Test"
        "test scroll fixture" = "DA_Scroll_Test"
        "reflected spell school metadata" = "EGridSpellSchool"
        "reflected skill attribute metadata" = "ERPGSkillGoverningAttribute"
    }

    foreach ($Pair in $ForbiddenManifestPatterns.GetEnumerator()) {
        if ($ManifestContent.Contains([string]$Pair.Value)) {
            throw ("Localization manifest contains forbidden {0}: {1}" -f $Pair.Key, $Pair.Value)
        }
    }

    $RequiredStableKeys = @(
        "CharacterCreation.Attribute.Dexterity",
        "Spellbook.School.Fire"
    )
    foreach ($RequiredStableKey in $RequiredStableKeys) {
        if (-not $ManifestContent.Contains($RequiredStableKey)) {
            throw ("Localization manifest is missing required ST_UI key: {0}" -f $RequiredStableKey)
        }
    }

    $ConflictContent = [System.IO.File]::ReadAllText($ConflictReportPath, [System.Text.Encoding]::UTF8)
    if (-not [string]::IsNullOrWhiteSpace($ConflictContent)) {
        throw ("Localization conflict report is not empty: {0}" -f $ConflictReportPath)
    }

    Write-Host "[OK] Gather integrity guards."
}

Invoke-NativeStep -Label "Validate_PowerShell" -FilePath $PowerShellExe -ArgumentList @(
    "-NoProfile",
    "-NonInteractive",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    $ScriptValidator
)

if (-not $SkipBuild) {
    $EditorTarget = "{0}Editor" -f $ProjectFile.BaseName
    Invoke-NativeStep -Label "Build_Editor" -FilePath $BuildBat -ArgumentList @(
        $EditorTarget,
        "Win64",
        "Development",
        ("-Project={0}" -f $ProjectFile.FullName),
        "-WaitMutex",
        "-NoHotReloadFromIDE"
    )
}
else {
    Write-Host "[SKIP] Editor build requested by -SkipBuild."
}

Invoke-UnrealStep -Label "Sync_ST_UI" -Arguments @("-run=UIStringTableAuthoring")
Assert-FileExists -Path $StringTableAsset -Label "materialized ST_UI String Table asset"

try {
    Invoke-UnrealStep -Label "Gather" -Arguments @("-run=GatherText", ("-config={0}" -f $GatherConfig))
}
finally {
    if (-not $KeepGeneratedOutputs) {
        Restore-FileSnapshot -Path $GameCsv -Snapshot $GameCsvSnapshot
    }
}
Assert-GatherIntegrity -ManifestPath $Manifest -ConflictReportPath $ConflictReport

try {
    Invoke-UnrealStep -Label "Export_PO" -Arguments @("-run=GatherText", ("-config={0}" -f $ExportConfig))
    Assert-FileExists -Path $EnglishPo -Label "English PO"
    Assert-FileExists -Path $FrenchPo -Label "French PO"

    Write-Host ""
    Write-Host "=== Apply_English_Smoke_Translations ==="
    & $ApplyTranslationsScript -PoPath $EnglishPo -CsvPath (Join-Path $ProjectRoot "Localization\Source\UI.csv")

    Invoke-UnrealStep -Label "Import_PO" -Arguments @("-run=GatherText", ("-config={0}" -f $ImportConfig))
}
finally {
    if (-not $KeepGeneratedOutputs) {
        Restore-FileSnapshot -Path $EnglishPo -Snapshot $EnglishPoSnapshot
        Restore-FileSnapshot -Path $FrenchPo -Snapshot $FrenchPoSnapshot
    }
}
Invoke-UnrealStep -Label "Compile_LocRes" -Arguments @("-run=GatherText", ("-config={0}" -f $CompileConfig))
Assert-FileExists -Path $EnglishLocRes -Label "English locres"
Assert-FileExists -Path $FrenchLocRes -Label "French locres"
Assert-FileExists -Path $LocMeta -Label "localization target metadata"

Invoke-UnrealStep -Label "Runtime_Culture_Smoke" -Arguments @("-run=UILocalizationEnglishSmoke")

foreach ($SourceConfigPath in $SourceConfigPaths) {
    $CurrentHash = (Get-FileHash -LiteralPath $SourceConfigPath -Algorithm SHA256).Hash
    if ($CurrentHash -ne $SourceConfigHashes[$SourceConfigPath]) {
        throw ("Source-controlled localization config was modified during smoke execution: {0}" -f $SourceConfigPath)
    }
}
Write-Host "[OK] Source-controlled localization configs remained unchanged."
if (-not $KeepGeneratedOutputs) {
    Restore-FileSnapshot -Path $Manifest -Snapshot $ManifestSnapshot
    Restore-FileSnapshot -Path $ConflictReport -Snapshot $ConflictReportSnapshot
    Restore-FileSnapshot -Path $LocMeta -Snapshot $LocMetaSnapshot
    Restore-FileSnapshot -Path $EnglishArchive -Snapshot $EnglishArchiveSnapshot
    Restore-FileSnapshot -Path $FrenchArchive -Snapshot $FrenchArchiveSnapshot
    Restore-FileSnapshot -Path $EnglishLocRes -Snapshot $EnglishLocResSnapshot
    Restore-FileSnapshot -Path $FrenchLocRes -Snapshot $FrenchLocResSnapshot
    Write-Host "[OK] Volatile generated localization outputs restored after smoke validation."
}

Write-Host ""
Write-Host "[OK] UI-LOCALIZATION01.6 localization pipeline and English smoke test completed successfully."
Write-Host ("Report: {0}" -f $ReportRoot)
