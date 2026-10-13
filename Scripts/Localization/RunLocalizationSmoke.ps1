param(
    [Parameter(Mandatory = $true)]
    [string]$EngineRoot
)

$ErrorActionPreference = "Stop"

$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$ProjectFile = Get-ChildItem -LiteralPath $ProjectRoot -Filter *.uproject -File | Select-Object -First 1
if (-not $ProjectFile) {
    throw "No .uproject file found under $ProjectRoot"
}

$EditorCmd = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
if (-not (Test-Path -LiteralPath $EditorCmd)) {
    throw "UnrealEditor-Cmd.exe not found: $EditorCmd"
}

function Invoke-Unreal {
    param([string[]]$Arguments)

    & $EditorCmd $ProjectFile.FullName @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Unreal command failed with exit code $LASTEXITCODE: $($Arguments -join ' ')"
    }
}

$CommonArgs = @("-unattended", "-nop4", "-nosplash", "-nullrhi", "-utf8output")

Write-Host "=== Sync ST_UI from UI.csv ==="
Invoke-Unreal (@("-run=UIStringTableAuthoring") + $CommonArgs)

Write-Host "=== Gather localization target ==="
Invoke-Unreal (@("-run=GatherText", "-config=$ProjectRoot\Config\Localization\Game_Gather.ini") + $CommonArgs)

Write-Host "=== Export PO files ==="
Invoke-Unreal (@("-run=GatherText", "-config=$ProjectRoot\Config\Localization\Game_Export.ini") + $CommonArgs)

Write-Host "=== Apply English smoke translations ==="
& (Join-Path $PSScriptRoot "ApplyEnglishSmokeTranslations.ps1") -PoPath (Join-Path $ProjectRoot "Content\Localization\Game\en\Game.po")

Write-Host "=== Import PO files ==="
Invoke-Unreal (@("-run=GatherText", "-config=$ProjectRoot\Config\Localization\Game_Import.ini") + $CommonArgs)

Write-Host "=== Compile locres ==="
Invoke-Unreal (@("-run=GatherText", "-config=$ProjectRoot\Config\Localization\Game_Compile.ini") + $CommonArgs)

Write-Host "=== Runtime culture smoke test ==="
Invoke-Unreal (@("-run=UILocalizationEnglishSmoke") + $CommonArgs)

Write-Host "[OK] UI-LOCALIZATION01.6 localization pipeline and English smoke test completed successfully."
