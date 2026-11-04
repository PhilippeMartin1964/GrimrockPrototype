# VALIDATION-AUTO01 — installation unique de la surveillance Windows.
# S'execute avec le compte Windows interactif. Aucun privilege administrateur necessaire en principe.
[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\UE_5.5',
    [int]$IntervalMinutes = 3,
    [string]$TaskName = 'GrimrockPrototype-UEValidation',
    [string]$AutomationFilter = 'Grimrock',
    [switch]$UseRHI
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$RepoRoot = (Resolve-Path -LiteralPath (Split-Path -Parent $PSScriptRoot)).Path
$Agent = Join-Path $PSScriptRoot 'UEValidationAgent.ps1'
$WorktreePath = Join-Path (Split-Path -Parent $RepoRoot) 'GrimrockPrototype_UEValidation'
$StateRoot = Join-Path $env:LOCALAPPDATA 'GrimrockPrototype\UEValidationAgent'

if ($IntervalMinutes -lt 1 -or $IntervalMinutes -gt 60) { throw 'IntervalMinutes doit etre compris entre 1 et 60.' }
foreach ($Path in @($Agent, (Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'), (Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'))) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "Prerequis introuvable : $Path" }
}
foreach ($Command in @('git', 'gh')) {
    if (-not (Get-Command $Command -ErrorAction SilentlyContinue)) { throw "Prerequis absent : $Command (Git for Windows et GitHub CLI)." }
}
& git lfs version *> $null
if ($LASTEXITCODE -ne 0) { throw 'Git LFS non installe.' }
& gh auth status -h github.com *> $null
if ($LASTEXITCODE -ne 0) { throw 'GitHub CLI non authentifie. Une seule fois : gh auth login -h github.com' }

New-Item -ItemType Directory -Force -Path $StateRoot | Out-Null
$PsExe = Join-Path $PSHOME 'powershell.exe'
if (-not (Test-Path -LiteralPath $PsExe)) { throw "Windows PowerShell.exe introuvable : $PsExe" }
$PsArguments = @(
    '-NoLogo', '-NoProfile', '-NonInteractive', '-ExecutionPolicy', 'Bypass',
    '-File', ('"' + $Agent + '"'), '-RepositoryRoot', ('"' + $RepoRoot + '"'),
    '-EngineRoot', ('"' + $EngineRoot + '"'), '-WorktreePath', ('"' + $WorktreePath + '"'),
    '-StateRoot', ('"' + $StateRoot + '"'), '-AutomationFilter', ('"' + $AutomationFilter + '"')
)
if ($UseRHI) { $PsArguments += '-UseRHI' }
$Action = New-ScheduledTaskAction -Execute $PsExe -Argument ($PsArguments -join ' ') -WorkingDirectory $RepoRoot
$Trigger = New-ScheduledTaskTrigger -Once -At (Get-Date).AddMinutes(1) -RepetitionInterval (New-TimeSpan -Minutes $IntervalMinutes) -RepetitionDuration (New-TimeSpan -Days 3650)
$Identity = [System.Security.Principal.WindowsIdentity]::GetCurrent().Name
$Principal = New-ScheduledTaskPrincipal -UserId $Identity -LogonType Interactive -RunLevel Limited
$Settings = New-ScheduledTaskSettingsSet -MultipleInstances IgnoreNew -ExecutionTimeLimit (New-TimeSpan -Hours 12) -StartWhenAvailable -DisallowStartIfOnBatteries:$false -StopIfGoingOnBatteries:$false
Register-ScheduledTask -TaskName $TaskName -Action $Action -Trigger $Trigger -Settings $Settings -Principal $Principal -Description 'Validation automatique UE 5.5.4 de origin/master sur worktree isole, avec statut GitHub.' -Force | Out-Null
Start-ScheduledTask -TaskName $TaskName
Write-Host "[VALIDATION-AUTO01] Installe et lance : $TaskName"
Write-Host "Periodique : $IntervalMinutes minute(s) ; user = $Identity"
Write-Host "Code utilisateur preserve : $RepoRoot"
Write-Host "Worktree de validation : $WorktreePath"
Write-Host "Rapports + etat : $StateRoot"
Write-Host "Verification : Get-ScheduledTask -TaskName '$TaskName' | Get-ScheduledTaskInfo"
Write-Host 'Pas de GitHub Actions runner expose pour ce depot public.'