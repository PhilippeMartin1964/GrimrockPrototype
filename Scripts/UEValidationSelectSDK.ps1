param([Parameter(Mandatory = $true)][string]$WorktreePath)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# Isolated UEValidation worktree only. Never edit the user's global UBT XML.
$KitsX86 = [Environment]::GetEnvironmentVariable('ProgramFiles(x86)')
if ([string]::IsNullOrWhiteSpace($KitsX86)) { throw 'SDK_SELECTION: Windows Kits root not found.' }
$KitsRoot = Join-Path $KitsX86 'Windows Kits\10'
$IncludeRoot = Join-Path $KitsRoot 'Include'
$LibRoot = Join-Path $KitsRoot 'Lib'
if (-not (Test-Path -LiteralPath $IncludeRoot -PathType Container)) {
    throw 'SDK_SELECTION: Windows SDK include root missing.'
}

$Candidates = @(
    foreach ($Entry in @(Get-ChildItem -LiteralPath $IncludeRoot -Directory)) {
        if ($Entry.Name -notmatch '^\d+\.\d+\.\d+\.\d+$') { continue }
        $Version = [version]$Entry.Name
        if ($Version -lt [version]'10.0.19041.0') { continue }
        $Required = @(
            (Join-Path $Entry.FullName 'ucrt\assert.h'),
            (Join-Path $Entry.FullName 'ucrt\stddef.h'),
            (Join-Path $Entry.FullName 'um\Windows.h'),
            (Join-Path $Entry.FullName 'shared\winapifamily.h'),
            (Join-Path $LibRoot ($Entry.Name + '\um\x64\kernel32.lib')),
            (Join-Path $LibRoot ($Entry.Name + '\ucrt\x64\ucrt.lib'))
        )
        $Complete = $true
        foreach ($File in $Required) {
            if (-not (Test-Path -LiteralPath $File -PathType Leaf)) { $Complete = $false; break }
        }
        if ($Complete) { [pscustomobject]@{ Version = $Version; Name = $Entry.Name } }
    }
)
$Chosen = @($Candidates | Sort-Object Version -Descending | Select-Object -First 1)
if ($Chosen.Count -ne 1) {
    throw 'SDK_SELECTION: no complete Windows SDK (UCRT, Win32 headers, x64 libraries).'
}

$ConfigPath = Join-Path $WorktreePath 'Saved\UnrealBuildTool\BuildConfiguration.xml'
$Marker = 'UEVALIDATION_MANAGED_WINDOWS_SDK_V1'
if (Test-Path -LiteralPath $ConfigPath -PathType Leaf) {
    $Previous = Get-Content -LiteralPath $ConfigPath -Raw
    if (-not $Previous.Contains($Marker)) {
        throw 'SDK_SELECTION: refusing to overwrite unmanaged worktree BuildConfiguration.xml.'
    }
}
$Config = @(
    '<?xml version="1.0" encoding="utf-8"?>',
    '<Configuration xmlns="https://www.unrealengine.com/BuildConfiguration">',
    "  <!-- $Marker -->",
    '  <WindowsPlatform>',
    "    <WindowsSdkVersion>$($Chosen[0].Name)</WindowsSdkVersion>",
    '  </WindowsPlatform>',
    '</Configuration>'
) -join [Environment]::NewLine
$Parsed = [xml]$Config
$Node = $Parsed.SelectSingleNode("//*[local-name()='WindowsSdkVersion']")
if ($null -eq $Node -or $Node.InnerText -cne $Chosen[0].Name) {
    throw 'SDK_SELECTION: invalid generated XML.'
}
[IO.Directory]::CreateDirectory((Split-Path -Parent $ConfigPath)) | Out-Null
$TempPath = $ConfigPath + '.tmp'
try {
    Set-Content -LiteralPath $TempPath -Value $Config -Encoding UTF8
    Move-Item -LiteralPath $TempPath -Destination $ConfigPath -Force
}
finally {
    Remove-Item -LiteralPath $TempPath -Force -ErrorAction SilentlyContinue
}
# Pass the validated SDK explicitly to the next PowerShell process and then
# to UnrealBuildTool. The XML remains an isolated fallback; on some 5.5
# installations UBT otherwise selects the latest (incomplete) Windows Kit.
$env:GRIMROCK_VALIDATION_WINDOWS_SDK = $Chosen[0].Name
Write-Host "[VALIDATION-AUTO01] SDK_SELECTION COMPLETE version=$($Chosen[0].Name)"
