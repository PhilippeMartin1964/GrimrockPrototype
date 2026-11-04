[CmdletBinding()]
param([string]$TaskName = 'GrimrockPrototype-UEValidation')
$ErrorActionPreference = 'Stop'
$Task = Get-ScheduledTask -TaskName $TaskName -ErrorAction SilentlyContinue
if (-not $Task) { Write-Host "Tache absente : $TaskName"; return }
Stop-ScheduledTask -TaskName $TaskName -ErrorAction SilentlyContinue
Unregister-ScheduledTask -TaskName $TaskName -Confirm:$false
Write-Host "Tache supprimee : $TaskName"
Write-Host 'Le worktree et les rapports restent volontairement conserves. Aucune suppression automatique.'