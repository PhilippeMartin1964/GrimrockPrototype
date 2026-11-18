[CmdletBinding()]
param()
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'ARM_CLEAN06_4X66_Rollback.ps1')
function Assert-True([bool]$Condition,[string]$Message) { if (-not $Condition) { throw $Message } }
function Expect-Failure([scriptblock]$Action) {
    $Failed = $false
    try { & $Action | Out-Null } catch { $Failed = $true }
    Assert-True $Failed 'Expected refusal did not occur.'
}
$Root = Join-Path ([IO.Path]::GetTempPath()) ('ArmX66RollbackTest-'+[guid]::NewGuid().ToString('N'))
if (Test-Path -LiteralPath $Root) { throw 'Fixture collision.' }
[void](New-Item -ItemType Directory -Path $Root)
try {
    $Original = Join-Path $Root 'backup.uasset'
    $Live = Join-Path $Root 'live.uasset'
    $Candidate = Join-Path $Root 'candidate.uasset'
    $Archive = Join-Path $Root 'archive\live.uasset'
    [IO.File]::WriteAllText($Original,'original bytes')
    [IO.File]::WriteAllText($Live,'candidate bytes')
    $Before = Get-ArmX66FileState $Original
    $After = Get-ArmX66FileState $Live

    # Inject both save rollback and save-set verification failures, with a
    # real synthetic native file restored between them.
    $Calls = New-Object 'System.Collections.Generic.List[string]'
    $Result = Invoke-ArmX66RollbackActions ([ordered]@{
        RestoreSaves={ $Calls.Add('RestoreSaves'); throw 'Injected save restoration failure' }
        RestorePackages={
            $Calls.Add('RestorePackages')
            Restore-ArmX66NativeFile -Live $Live -Before $Before -After $After -Backup $Original -Archive $Archive
        }
        VerifySaves={ $Calls.Add('VerifySaves'); throw 'Live save set changed.' }
        VerifyPackages={
            $Calls.Add('VerifyPackages')
            Assert-True ((Get-ArmX66FileState $Live) -ceq $Before) 'Existing package was not restored.'
        }
    })
    Assert-True (-not $Result.Success -and @($Result.Issues).Count -eq 2) 'Rollback must report both failed save phases.'
    Assert-True (($Calls -join ',') -ceq 'RestoreSaves,RestorePackages,VerifySaves,VerifyPackages') 'All phases must run.'
    Assert-True ((Get-ArmX66FileState $Live) -ceq $Before) 'Existing package not restored after save failure.'
    Assert-True ((Get-ArmX66FileState $Archive) -ceq $After) 'Existing package candidate not preserved.'

    [IO.File]::WriteAllText($Candidate,'new candidate bytes')
    $NewHash = Get-ArmX66FileState $Candidate
    $NewArchive = Join-Path $Root 'archive\candidate.uasset'
    Restore-ArmX66NativeFile -Live $Candidate -Before 'ABSENT' -After $NewHash -Archive $NewArchive
    Assert-True ((Get-ArmX66FileState $Candidate) -ceq 'ABSENT') 'New package still present.'
    Assert-True ((Get-ArmX66FileState $NewArchive) -ceq $NewHash) 'New candidate archive missing.'

    # Unknown current bytes, unproven candidate and corrupt backup must be
    # preserved rather than overwritten. Nothing here touches actual saves.
    [IO.File]::WriteAllText($Live,'user change')
    $Unknown = Get-ArmX66FileState $Live
    Expect-Failure { Restore-ArmX66NativeFile -Live $Live -Before $Before -After $After -Backup $Original -Archive (Join-Path $Root 'unknown.uasset') }
    Expect-Failure { Restore-ArmX66NativeFile -Live $Live -Before $Before -Backup $Original -Archive (Join-Path $Root 'unproven.uasset') }
    Assert-True ((Get-ArmX66FileState $Live) -ceq $Unknown) 'Unknown bytes were modified.'
    [IO.File]::WriteAllText($Live,'candidate bytes')
    [IO.File]::WriteAllText($Original,'corrupted snapshot')
    Expect-Failure { Restore-ArmX66NativeFile -Live $Live -Before $Before -After $After -Backup $Original -Archive (Join-Path $Root 'corrupt.uasset') }
    Assert-True ((Get-ArmX66FileState $Live) -ceq $After) 'Corrupt snapshot changed candidate.'

    $Success = Invoke-ArmX66RollbackActions ([ordered]@{ Verify={ Assert-True $true 'unused' } })
    Assert-True ($Success.Success -and @($Success.Issues).Count -eq 0) 'Successful rollback result invalid.'
    Write-Host '[OK] X.66 rollback fault injection and content guards.'
} finally {
    # Only this freshly allocated synthetic test directory.
    Remove-Item -LiteralPath $Root -Recurse -Force
}
