# ARMEMENT01.17.1.4 — Corriger l'usage InstancedStaticMeshes du seul matériau d'huile.
# Aucune intervention sur la copie de développement Windows de l'utilisateur.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$WorktreePath,
    [Parameter(Mandatory=$true)][string]$EngineRoot,
    [Parameter(Mandatory=$true)][string]$ReportRoot,
    [Parameter(Mandatory=$true)][string]$ExpectedHead
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$Target = 'Content/GrimrockPrototype/Art/Materials/Surfaces/M_Surface_Oil.uasset'
$TestName = 'Grimrock.RPG.ARMEMENT01.17.1.4.InstancedMaterialUsage'
function GitChecked {
    param([string[]]$Arguments)
    $Lines = @(& git -C $WorktreePath @Arguments)
    if ($LASTEXITCODE -ne 0) { throw ('Échec Git : ' + $Arguments[0]) }
    return @($Lines)
}
function Changes { return @(GitChecked -Arguments @('status', '--porcelain', '--untracked-files=all')) }

if ($ExpectedHead -notmatch '^[a-f0-9]{40}$') { throw 'SHA invalide.' }
if (-not (Test-Path -LiteralPath (Join-Path $WorktreePath '.git'))) { throw 'Worktree de validation absent.' }
$Head = @(GitChecked -Arguments @('rev-parse', 'HEAD'))[0].Trim()
if ($Head -ne $ExpectedHead) { throw 'SHA du worktree inattendu.' }
$Origin = @(GitChecked -Arguments @('remote', 'get-url', 'origin'))[0].Trim()
if ($Origin -notmatch '(?i)github\.com[:/]PhilippeMartin1964/GrimrockPrototype(\.git)?$') { throw 'Origine Git inattendue.' }
if (@(Changes).Count -ne 0) { throw 'Worktree modifié : réparation refusée.' }
$MaterialPath = Join-Path $WorktreePath $Target
if (-not (Test-Path -LiteralPath $MaterialPath -PathType Leaf) -or
    (Get-Item -LiteralPath $MaterialPath).Length -lt 1024) { throw 'Matériau Unreal absent ou incomplet.' }
$Attribute = @(GitChecked -Arguments @('check-attr', 'filter', '--', $Target))
if ($Attribute.Count -ne 1 -or $Attribute[0] -notmatch ': filter: lfs$') { throw 'Matériau sans attribut Git LFS.' }

$Indexes = @(Get-ChildItem -LiteralPath $ReportRoot -Filter 'index.json' -File -Recurse -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending)
if ($Indexes.Count -eq 0) { Write-Host 'Rapport Automation introuvable.'; exit 2 }
$Report = Get-Content -LiteralPath $Indexes[0].FullName -Raw -ErrorAction Stop | ConvertFrom-Json
$Failed = @($Report.tests | Where-Object { [string]$_.state -match '^(?i:fail|failed|error)$' })
if ([int]$Report.failed -ne 1 -or $Failed.Count -ne 1 -or
    [string]$Failed[0].fullTestPath -cne $TestName) {
    Write-Host 'Échecs non limités au test InstancedMaterialUsage : aucune modification.'
    exit 2
}

$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Project = Join-Path $WorktreePath 'GrimrockPrototype.uproject'
if (-not (Test-Path -LiteralPath $Editor -PathType Leaf) -or
    -not (Test-Path -LiteralPath $Project -PathType Leaf)) { throw 'Éditeur UE ou projet absent.' }
$Output = Join-Path (Split-Path -Parent $ReportRoot) 'OilISMCommandlet.stdout.log'
$Errors = Join-Path (Split-Path -Parent $ReportRoot) 'OilISMCommandlet.stderr.log'
$Arguments = '"' + $Project + '" -run=GridOilSurfaceMaterialAuthoring -FixInstancedUsage -Unattended -NoSplash -NoP4 -NullRHI'
$Process = Start-Process -FilePath $Editor -ArgumentList $Arguments -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput $Output -RedirectStandardError $Errors -PassThru -Wait -ErrorAction Stop
if ($Process.ExitCode -ne 0) { throw 'Commandlet Unreal ISM en échec (journaux locaux).' }

$Modified = @(Changes)
if ($Modified.Count -ne 1 -or $Modified[0] -cne (' M ' + $Target)) {
    throw 'Liste de modifications inattendue : aucun commit.'
}
GitChecked -Arguments @('fetch', '--no-tags', 'origin', 'master') | Out-Null
$Remote = @(GitChecked -Arguments @('rev-parse', 'refs/remotes/origin/master'))[0].Trim()
if ($Remote -ne $ExpectedHead) { throw 'master a changé durant la génération.' }
GitChecked -Arguments @('add', '--', $Target) | Out-Null
$Staged = @(GitChecked -Arguments @('diff', '--cached', '--name-only', '--diff-filter=ACMR'))
if ($Staged.Count -ne 1 -or $Staged[0] -cne $Target) { throw 'Index Git inattendu.' }
GitChecked -Arguments @('-c', 'user.name=Grimrock UE Automation',
    '-c', 'user.email=grimrock-ue-automation@users.noreply.github.com',
    'commit', '-m', 'ARMEMENT01.17.1.4ISM: activer le shader ISM de la flaque') | Out-Null
$Commit = @(GitChecked -Arguments @('rev-parse', 'HEAD'))[0].Trim()
$Pushed = $false
for ($Attempt = 1; $Attempt -le 3; ++$Attempt) {
    & git -C $WorktreePath push origin 'HEAD:refs/heads/master' | Out-Null
    if ($LASTEXITCODE -eq 0) { $Pushed = $true; break }
    if ($Attempt -lt 3) { Start-Sleep -Seconds (2 * $Attempt) }
}
if (-not $Pushed) { throw 'Push non forcé impossible ; commit conservé localement.' }
Write-Host ('[ARMEMENT01.17.1.4] Permutation ISM publiée : ' + $Commit)
exit 0
