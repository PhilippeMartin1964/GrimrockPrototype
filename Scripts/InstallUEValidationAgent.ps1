# VALIDATION-AUTO01.4 — Installation idempotente de l'agent Windows de validation UE.
# Installation dans la session interactive normale, sans tache SYSTEM ni runner GitHub Actions.
[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\UE_5.5',
    [ValidateRange(1, 60)][int]$IntervalMinutes = 3,
    [string]$TaskName = 'GrimrockPrototype-UEValidation',
    [string]$AutomationFilter = 'Grimrock',
    [switch]$UseRHI,
    [switch]$CheckOnly,
    [switch]$SkipInitialRun
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Quote-AgentArgument {
    param([AllowEmptyString()][string]$Value)
    if ($Value.Contains('"')) { throw 'Guillemet interdit dans les chemins/parametres du planificateur.' }
    return '"' + $Value + '"'
}

$RepoRoot = (Resolve-Path -LiteralPath (Split-Path -Parent $PSScriptRoot)).Path
$Agent = Join-Path $PSScriptRoot 'UEValidationAgent.ps1'
$Bootstrap = Join-Path $PSScriptRoot 'UEValidationBootstrap.ps1'
$SilentHelper = Join-Path $PSScriptRoot 'UEValidationSilentTask.ps1'
$WorktreePath = Join-Path (Split-Path -Parent $RepoRoot) 'GrimrockPrototype_UEValidation'
if ([string]::IsNullOrWhiteSpace($env:LOCALAPPDATA)) { throw 'LOCALAPPDATA indisponible pour le compte Windows courant.' }
$StateRoot = Join-Path $env:LOCALAPPDATA 'GrimrockPrototype\UEValidationAgent'

if ([string]::IsNullOrWhiteSpace($TaskName) -or $TaskName -match '[\\/]') { throw 'Nom de tache invalide.' }
if ([string]::IsNullOrWhiteSpace($AutomationFilter)) { throw 'AutomationFilter ne peut pas etre vide.' }
foreach ($Path in @($Agent, $Bootstrap, (Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'), (Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'))) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "Prerequis introuvable : $Path" }
}
foreach ($Command in @('git', 'gh')) {
    if (-not (Get-Command $Command -ErrorAction SilentlyContinue)) { throw "Prerequis absent : $Command (Git for Windows et GitHub CLI)." }
}
& git lfs version | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'Git LFS non installe.' }
# gh auth status peut ecrire sur stderr meme en cas de succes ; eviter la conversion en exception PS 5.1.
& gh auth status -h github.com | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'GitHub CLI non authentifie. Executer gh auth login -h github.com.' }

Import-Module ScheduledTasks -ErrorAction Stop
# Parsing reel PowerShell 5.1, avant toute inscription de tache.
foreach ($ScriptToCheck in @($PSCommandPath, $Agent, $Bootstrap, $SilentHelper)) {
    $Tokens = $null
    $ParseErrors = $null
    [void][System.Management.Automation.Language.Parser]::ParseFile($ScriptToCheck, [ref]$Tokens, [ref]$ParseErrors)
    if ($null -ne $ParseErrors -and $ParseErrors.Count -gt 0) {
        throw "Erreur syntaxique dans $ScriptToCheck : $($ParseErrors[0].Message)"
    }
}
$PsExe = Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe'
if (-not (Test-Path -LiteralPath $PsExe -PathType Leaf)) { throw "Windows PowerShell.exe introuvable : $PsExe" }
$PsArguments = @(
    '-NoLogo', '-NoProfile', '-NonInteractive', '-WindowStyle', 'Hidden', '-ExecutionPolicy', 'Bypass',
    '-File', (Quote-AgentArgument $Bootstrap), '-RepositoryRoot', (Quote-AgentArgument $RepoRoot),
    '-EngineRoot', (Quote-AgentArgument $EngineRoot), '-WorktreePath', (Quote-AgentArgument $WorktreePath),
    '-StateRoot', (Quote-AgentArgument $StateRoot), '-AutomationFilter', (Quote-AgentArgument $AutomationFilter)
)
if ($UseRHI) { $PsArguments += '-UseRHI' }
# The task action is prepared only AFTER -CheckOnly; the helper writes the
# invisible WSH launcher to the private StateRoot, never to the checkout.
$WscriptExe = Join-Path $env:SystemRoot 'System32\wscript.exe'
$LauncherPath = Join-Path $StateRoot 'UEValidationSilentLauncher.vbs'
$Trigger = New-ScheduledTaskTrigger -Once -At (Get-Date).AddMinutes(1) -RepetitionInterval (New-TimeSpan -Minutes $IntervalMinutes) -RepetitionDuration (New-TimeSpan -Days 3650)
$Identity = [System.Security.Principal.WindowsIdentity]::GetCurrent().Name
$Principal = New-ScheduledTaskPrincipal -UserId $Identity -LogonType Interactive -RunLevel Limited

# Microsoft expose -AllowStartIfOnBatteries (pas -DisallowStartIfOnBatteries),
# et -DontStopIfGoingOnBatteries (pas -StopIfGoingOnBatteries).
# Sonder les parametres disponibles evite les echecs entre versions de Windows.
$SettingsArguments = @{
    MultipleInstances = 'IgnoreNew'
    ExecutionTimeLimit = (New-TimeSpan -Hours 12)
    StartWhenAvailable = $true
}
$AvailableSettings = (Get-Command New-ScheduledTaskSettingsSet -ErrorAction Stop).Parameters
if ($AvailableSettings.ContainsKey('AllowStartIfOnBatteries')) {
    $SettingsArguments.AllowStartIfOnBatteries = $true
} else {
    Write-Warning 'Option batterie indisponible : Windows peut differer le demarrage sur batterie.'
}
if ($AvailableSettings.ContainsKey('DontStopIfGoingOnBatteries')) {
    $SettingsArguments.DontStopIfGoingOnBatteries = $true
} else {
    Write-Warning 'Option batterie indisponible : Windows peut arreter une validation sur batterie.'
}
$Settings = New-ScheduledTaskSettingsSet @SettingsArguments

# Ne jamais ecraser une tache appartenant a une autre application.
$Existing = Get-ScheduledTask -TaskName $TaskName -ErrorAction SilentlyContinue
if ($Existing) {
    $OwnAction = @($Existing.Actions | Where-Object {
        ($_.Execute -ieq $PsExe -and $_.Arguments.Contains((Quote-AgentArgument $Bootstrap))) -or
        ($_.Execute -ieq $WscriptExe -and $_.Arguments -ceq ('//B //Nologo "' + $LauncherPath + '"') -and
            $_.WorkingDirectory -ieq $RepoRoot)
    })
    if ($OwnAction.Count -eq 0) { throw "Tache existante non reconnue : $TaskName. Aucun remplacement effectue." }
}

Write-Host "[VALIDATION-AUTO01.4] Preflight OK : $Identity"
Write-Host "Bootstrap   : $Bootstrap"
Write-Host "Agent       : $Agent"
Write-Host "Engine      : $EngineRoot"
Write-Host "Worktree    : $WorktreePath"
Write-Host "Etat/Logs   : $StateRoot"
Write-Host "Intervalle  : $IntervalMinutes minute(s) ; filtre=$AutomationFilter"
if ($CheckOnly) {
    Write-Host '[VALIDATION-AUTO01.4] CheckOnly : aucune tache creee ou modifiee.'
    return
}

New-Item -ItemType Directory -Force -Path $StateRoot | Out-Null
. $SilentHelper
$Action = New-UEValidationSilentAction -RepositoryRoot $RepoRoot -StateRoot $StateRoot -PowerShellArguments ($PsArguments -join ' ')
Register-ScheduledTask -TaskName $TaskName -Action $Action -Trigger $Trigger -Settings $Settings -Principal $Principal -Description 'Validation automatique UE 5.5.4 de origin/master dans un worktree dedie.' -Force | Out-Null
$Registered = Get-ScheduledTask -TaskName $TaskName -ErrorAction Stop
if (-not $Registered) { throw "Echec de verification de l'inscription de la tache $TaskName." }
if (@($Registered.Actions).Count -ne 1) { throw 'VALIDATION-AUTO01.11: incorrect task action count.' }
Assert-UEValidationSilentAction -Action $Registered.Actions[0] -RepositoryRoot $RepoRoot -StateRoot $StateRoot
Write-Host "[VALIDATION-AUTO01.4] Tache installee : $TaskName"
if (-not $SkipInitialRun) {
    # La reinstallation pendant un build UE ne doit pas lancer une seconde validation.
    $CurrentTask = Get-ScheduledTask -TaskName $TaskName -ErrorAction Stop
    if ($CurrentTask.State -eq 'Running') {
        Write-Host '[VALIDATION-AUTO01.4] Validation deja en cours. Instance existante conservee ; prochaines executions invisibles.'
    } else {
        try { Start-ScheduledTask -TaskName $TaskName -ErrorAction Stop; Write-Host '[VALIDATION-AUTO01.4] Verification automatique declenchee.' }
        catch { Write-Warning "Tache installee mais demarrage refuse : $($_.Exception.Message). Elle sera reessayee au prochain declenchement." }
    }
}
Write-Host "Verification : Get-ScheduledTask -TaskName '$TaskName' | Get-ScheduledTaskInfo"
Write-Host "Journal agent : $StateRoot\Agent.latest.log"
Write-Host "Journal bootstrap : $StateRoot\Bootstrap.latest.log"
Write-Host '[VALIDATION-AUTO01.11] Lanceur wscript.exe sans console (mode 0 et attente du processus enfant).'
Write-Host 'Une instance PowerShell deja en cours peut conserver son comportement jusqu a sa fin.'