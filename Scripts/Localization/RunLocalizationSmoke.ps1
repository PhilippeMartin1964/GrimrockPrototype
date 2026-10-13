[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$EngineRoot,

    [switch]$SkipBuild
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
$LocalizationRoot = Join-Path $ProjectRoot "Content\Localization\Game"
$EnglishPo = Join-Path $LocalizationRoot "en\Game.po"
$FrenchPo = Join-Path $LocalizationRoot "fr\Game.po"
$EnglishLocRes = Join-Path $LocalizationRoot "en\Game.locres"
$FrenchLocRes = Join-Path $LocalizationRoot "fr\Game.locres"
$Manifest = Join-Path $LocalizationRoot "Game.manifest"
$StringTableAsset = Join-Path $ProjectRoot "Content\Localization\StringTables\ST_UI.uasset"

$GatherConfig = Join-Path $ProjectRoot "Config\Localization\Game_Gather.ini"
$ExportConfig = Join-Path $ProjectRoot "Config\Localization\Game_Export.ini"
$ImportConfig = Join-Path $ProjectRoot "Config\Localization\Game_Import.ini"
$CompileConfig = Join-Path $ProjectRoot "Config\Localization\Game_Compile.ini"

Assert-FileExists -Path $EditorCmd -Label "UnrealEditor-Cmd.exe"
Assert-FileExists -Path $BuildBat -Label "Build.bat"
Assert-FileExists -Path $ApplyTranslationsScript -Label "English smoke translation script"
Assert-FileExists -Path $GatherConfig -Label "Gather config"
Assert-FileExists -Path $ExportConfig -Label "Export config"
Assert-FileExists -Path $ImportConfig -Label "Import config"
Assert-FileExists -Path $CompileConfig -Label "Compile config"
Assert-FileExists -Path (Join-Path $ProjectRoot "Localization\Source\UI.csv") -Label "UI localization CSV"
Assert-NoUnrealProcess

$Timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$ReportRoot = Join-Path $ProjectRoot ("Saved\Localization\UI-LOCALIZATION01.6\{0}" -f $Timestamp)
[void](New-Item -ItemType Directory -Path $ReportRoot -Force)
Write-Host ("Localization report directory: {0}" -f $ReportRoot)

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

Invoke-UnrealStep -Label "Gather" -Arguments @("-run=GatherText", ("-config={0}" -f $GatherConfig))
Assert-FileExists -Path $Manifest -Label "generated localization manifest"

Invoke-UnrealStep -Label "Export_PO" -Arguments @("-run=GatherText", ("-config={0}" -f $ExportConfig))
Assert-FileExists -Path $EnglishPo -Label "English PO"
Assert-FileExists -Path $FrenchPo -Label "French PO"

Write-Host ""
Write-Host "=== Apply_English_Smoke_Translations ==="
& $ApplyTranslationsScript -PoPath $EnglishPo

Invoke-UnrealStep -Label "Import_PO" -Arguments @("-run=GatherText", ("-config={0}" -f $ImportConfig))
Invoke-UnrealStep -Label "Compile_LocRes" -Arguments @("-run=GatherText", ("-config={0}" -f $CompileConfig))
Assert-FileExists -Path $EnglishLocRes -Label "English locres"
Assert-FileExists -Path $FrenchLocRes -Label "French locres"

Invoke-UnrealStep -Label "Runtime_Culture_Smoke" -Arguments @("-run=UILocalizationEnglishSmoke")

Write-Host ""
Write-Host "[OK] UI-LOCALIZATION01.6 localization pipeline and English smoke test completed successfully."
Write-Host ("Report: {0}" -f $ReportRoot)
