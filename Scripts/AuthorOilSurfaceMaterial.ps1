# ARMEMENT01.17.1.4 — Creation automatique de deux assets Unreal (M et MI d'huile).
# Le seul declencheur autorise est un echec exclusif du test ProductionMaterial.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$WorktreePath,
    [Parameter(Mandatory=$true)][string]$EngineRoot,
    [Parameter(Mandatory=$true)][string]$ReportRoot,
    [Parameter(Mandatory=$true)][string]$ExpectedHead
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$Material = 'Content/GrimrockPrototype/Art/Materials/Surfaces/M_Surface_Oil.uasset'
$Instance = 'Content/GrimrockPrototype/Art/Materials/Surfaces/MI_Surface_Oil.uasset'
$TestName = 'Grimrock.RPG.ARMEMENT01.17.1.4.ProductionMaterial'
$StageFile = Join-Path (Split-Path -Parent $ReportRoot) 'OilMaterial.stage'
function Stage {
    param([string]$Name)
    Set-Content -LiteralPath $StageFile -Value $Name -Encoding ASCII
}
Stage -Name 'PREFLIGHT'
function GitChecked {
    param([string[]]$Arguments)
    $Lines = @(& git -C $WorktreePath @Arguments)
    if ($LASTEXITCODE -ne 0) { throw ("Commande Git echouee : " + $Arguments[0]) }
    return @($Lines)
}
function Changes { return @(GitChecked -Arguments @('status', '--porcelain', '--untracked-files=all')) }
if ($ExpectedHead -notmatch '^[0-9a-f]{40}$') { throw 'SHA attendu invalide.' }
if (-not (Test-Path -LiteralPath (Join-Path $WorktreePath '.git'))) { throw 'Worktree introuvable.' }
$Head = @(GitChecked -Arguments @('rev-parse', 'HEAD'))[0].Trim()
if ($Head -ne $ExpectedHead) { throw 'Revision du worktree differente de celle testee.' }
$Origin = @(GitChecked -Arguments @('remote', 'get-url', 'origin'))[0].Trim()
if ($Origin -notmatch '(?i)github\.com[:/]PhilippeMartin1964/GrimrockPrototype(\.git)?$') { throw 'Origin GitHub inattendu.' }
if (@(Changes).Count -ne 0) { throw 'Worktree deja modifie : aucune ecriture autorisee.' }
foreach ($Target in @($Material, $Instance)) {
    if (Test-Path -LiteralPath (Join-Path $WorktreePath $Target) -PathType Leaf) { exit 2 }
    $Attribute = @(GitChecked -Arguments @('check-attr', 'filter', '--', $Target))
    if ($Attribute.Count -ne 1 -or $Attribute[0] -notmatch ': filter: lfs$') {
        throw 'Attribut Git LFS manquant pour un asset.'
    }
}
$Reports = @(Get-ChildItem -LiteralPath $ReportRoot -Recurse -File -Filter 'index.json' -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending)
if ($Reports.Count -eq 0) { exit 2 }
$Report = Get-Content -LiteralPath $Reports[0].FullName -Raw | ConvertFrom-Json
$Failed = @($Report.tests | Where-Object { [string]$_.state -match '^(?i:fail|failed|error)$' })
if ([int]$Report.failed -ne 1 -or $Failed.Count -ne 1 -or [string]$Failed[0].fullTestPath -cne $TestName) {
    Write-Host 'Erreur non exclusivement liee au materiau, aucune ecriture.'
    exit 2
}
Stage -Name 'REPORT_ACCEPTED'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Project = Join-Path $WorktreePath 'GrimrockPrototype.uproject'
if (-not (Test-Path -LiteralPath $Editor -PathType Leaf) -or -not (Test-Path -LiteralPath $Project -PathType Leaf)) {
    throw 'Executable Unreal ou projet manquant.'
}
$LogDir = Split-Path -Parent $ReportRoot
$Output = Join-Path $LogDir 'OilMaterialCommandlet.stdout.log'
$Errors = Join-Path $LogDir 'OilMaterialCommandlet.stderr.log'
$Args = '"' + $Project + '" -run=GridOilSurfaceMaterialAuthoring -Unattended -NoSplash -NoP4 -NullRHI'
Stage -Name 'COMMANDLET_STARTED'
$Process = Start-Process -FilePath $Editor -ArgumentList $Args -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput $Output -RedirectStandardError $Errors -PassThru -Wait -ErrorAction Stop
if ($Process.ExitCode -ne 0) { throw "Commandlet Unreal en echec ($($Process.ExitCode)). Logs prives." }
Stage -Name 'COMMANDLET_COMPLETED'
$Changes = @(Changes)
if ($Changes.Count -ne 2) { throw 'Nombre de fichiers modifies inattendu. Aucun commit.' }
foreach ($Change in $Changes) {
    $Path = $Change.Substring(3)
    if ($Path -notin @($Material, $Instance) -or -not $Change.StartsWith('?? ')) {
        throw 'Fichier inattendu dans le worktree. Aucun commit.'
    }
}
foreach ($Target in @($Material, $Instance)) {
    $Path = Join-Path $WorktreePath $Target
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf) -or (Get-Item -LiteralPath $Path).Length -lt 1024) {
        throw 'Asset Unreal absent ou trop petit. Aucun commit.'
    }
}
Stage -Name 'ASSETS_VERIFIED'
GitChecked -Arguments @('fetch', '--no-tags', 'origin', 'master') | Out-Null
$Remote = @(GitChecked -Arguments @('rev-parse', 'refs/remotes/origin/master'))[0].Trim()
if ($Remote -ne $ExpectedHead) { throw 'master a change ; aucun commit.' }
GitChecked -Arguments @('add', '--', $Material, $Instance) | Out-Null
$Staged = @(GitChecked -Arguments @('diff', '--cached', '--name-only', '--diff-filter=ACMR'))
if ($Staged.Count -ne 2 -or @($Staged | Where-Object { $_ -notin @($Material, $Instance) }).Count -gt 0) {
    throw 'Index Git inattendu : publication refusee.'
}
GitChecked -Arguments @('-c', 'user.name=Grimrock UE Automation',
    '-c', 'user.email=grimrock-ue-automation@users.noreply.github.com',
    'commit', '-m', 'ARMEMENT01.17.1.4B: materialiser les deux assets huile') | Out-Null
$Published = @(GitChecked -Arguments @('rev-parse', 'HEAD'))[0].Trim()
Stage -Name 'COMMIT_CREATED'
GitChecked -Arguments @('push', 'origin', 'HEAD:refs/heads/master') | Out-Null
Stage -Name 'SUCCESS'
Write-Host "[ARMEMENT01.17.1.4] Materiaux M/MI publies sur master : $Published"
exit 0
