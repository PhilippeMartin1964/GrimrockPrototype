# VALIDATION-AUTO01.1 — desinstallation idempotente, ne supprime jamais d'autres taches.
[CmdletBinding()]
param([string]$TaskName = 'GrimrockPrototype-UEValidation')
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$Task = Get-ScheduledTask -TaskName $TaskName -ErrorAction SilentlyContinue
if (-not $Task) { Write-Host "Tache absente : $TaskName"; return }
$Agent = Join-Path $PSScriptRoot 'UEValidationAgent.ps1'
$Owned = @($Task.Actions | Where-Object { $_.Arguments -and $_.Arguments.Contains(('"' + $Agent + '"')) })
if ($Owned.Count -eq 0) { throw "Refus de supprimer une tache etrangere : $TaskName" }
Stop-ScheduledTask -TaskName $TaskName -ErrorAction SilentlyContinue
Unregister-ScheduledTask -TaskName $TaskName -Confirm:$false -ErrorAction Stop
Write-Host "Tache supprimee : $TaskName"
Write-Host 'Le worktree et les rapports sont conserves volontairement. Aucune suppression automatique.'