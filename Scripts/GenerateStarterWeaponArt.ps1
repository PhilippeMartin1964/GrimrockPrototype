[CmdletBinding()]
param([string]$BlenderExe='blender',
      [string]$OutputDir='',
      [switch]$InspectOnly)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$Repo=Split-Path -Parent $PSScriptRoot
if (-not $OutputDir) {
    $OutputDir=Join-Path $Repo 'Saved\ARMEMENT-LOT02\LOT02_9_ArtSource'
}
$PythonFile=Join-Path $PSScriptRoot 'GenerateStarterWeaponArt.py'
$Report=Join-Path $OutputDir 'LOT02_9_ArtProduction.csv'
if ((& git -C $Repo branch --show-current).Trim() -ne 'master') {
    throw 'Art authoring is restricted to master.'
}
$Expected=@('ARM038','ARM048','ARM054','ARM057','ARM058','ARM061','ARM062','ARM067','ARM069','ARM070','ARM081')
if (-not $InspectOnly) {
    if (Test-Path -LiteralPath $Report) {
        throw 'Previous art batch found. Use -InspectOnly or move the existing folder.'
    }
    $Exe=Get-Command $BlenderExe -ErrorAction SilentlyContinue
    if (-not $Exe -and -not (Test-Path -LiteralPath $BlenderExe)) {
        $Found=@(Get-ChildItem 'C:\Program Files\Blender Foundation' -Directory -ErrorAction SilentlyContinue |
            ForEach-Object { Join-Path $_.FullName 'blender.exe' } |
            Where-Object { Test-Path $_ })
        if ($Found.Count -eq 0) { throw 'Blender not found; pass -BlenderExe with its full path.' }
        $BlenderExe=$Found[-1]
    }
    New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
    & $BlenderExe --background --factory-startup --python $PythonFile -- $OutputDir
    if ($LASTEXITCODE -ne 0) {
        throw "Blender returned $LASTEXITCODE. Partial files retained for inspection."
    }
}
if (-not (Test-Path -LiteralPath $Report)) { throw 'No production manifest found.' }
$Rows=@(Import-Csv -LiteralPath $Report)
if ($Rows.Count -ne 11 -or (@($Rows.ArchetypeId | Sort-Object) -join ',') -cne (($Expected | Sort-Object) -join ',')) {
    throw 'Expected 11 unique approved source models.'
}
foreach ($Row in $Rows) {
    foreach ($Column in @('FBX','IconRGBA','BaseColor','Normal','ORM','BlenderSource')) {
        $File=Join-Path $OutputDir $Row.$Column
        if (-not (Test-Path -LiteralPath $File -PathType Leaf) -or
            (Get-Item -LiteralPath $File).Length -eq 0) {
            throw "Missing asset $($Row.ArchetypeId) $Column"
        }
    }
}
Write-Host '[LOT02.9] 11/11 Blender art source sets exist, awaiting visual review and UE import.'
$Rows | Format-Table ArchetypeId,Name,ReviewStatus -AutoSize
