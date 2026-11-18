# ARM-CLEAN06.4X66F -- local transaction helpers. No side effects at import.

function Assert-ArmX66OrdinaryPath([string]$Path) {
    $Current = [IO.Path]::GetFullPath($Path)
    while ($true) {
        try {
            $Item = Get-Item -LiteralPath $Current -Force -ErrorAction Stop
            break
        } catch [System.Management.Automation.ItemNotFoundException] {
            $Parent = Split-Path -Parent $Current
            if ([string]::IsNullOrEmpty($Parent) -or $Parent -eq $Current) { throw 'No ordinary ancestor.' }
            $Current = $Parent
        }
    }
    while ($null -ne $Item) {
        if (($Item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw 'Reparse point refused during native transaction.'
        }
        if ($Item -is [IO.FileInfo]) { $Item = $Item.Directory } else { $Item = $Item.Parent }
    }
}

function Get-ArmX66FileState([string]$Path) {
    Assert-ArmX66OrdinaryPath $Path
    if (-not (Test-Path -LiteralPath $Path)) { return 'ABSENT' }
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw 'Expected an ordinary file or absence.' }
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

function Invoke-ArmX66RollbackActions([System.Collections.IDictionary]$Actions) {
    # A failed SaveGames check must never short-circuit safe package recovery.
    $Issues = New-Object 'System.Collections.Generic.List[string]'
    foreach ($Name in $Actions.Keys) {
        try { & ($Actions[$Name]) | Out-Null }
        catch { $Issues.Add(([string]$Name + ': ' + $_.Exception.Message)) }
    }
    return [pscustomobject]@{ Success=($Issues.Count -eq 0); Issues=@($Issues.ToArray()) }
}

function Restore-ArmX66NativeFile {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory=$true)][string]$Live,
        [Parameter(Mandatory=$true)][string]$Before,
        [AllowEmptyString()][string]$After = '',
        [AllowEmptyString()][string]$Backup = '',
        [Parameter(Mandatory=$true)][string]$Archive
    )
    $Current = Get-ArmX66FileState $Live
    if ($Before -cne 'ABSENT') {
        if ($Before -cnotmatch '^[0-9a-f]{64}$' -or [string]::IsNullOrEmpty($Backup) -or
            (Get-ArmX66FileState $Backup) -cne $Before) { throw 'Original private package snapshot mismatch.' }
    }
    if ($Current -ceq $Before) { return }
    if ($After -cnotmatch '^[0-9a-f]{64}$' -or $Current -cne $After) {
        throw 'Native package state has no matching candidate receipt; preserved for audit.'
    }
    if ((Get-ArmX66FileState $Archive) -cne 'ABSENT') { throw 'Rollback candidate archive already exists.' }
    $ArchiveParent = Split-Path -Parent $Archive
    [void](New-Item -ItemType Directory -Path $ArchiveParent -Force)
    Copy-Item -LiteralPath $Live -Destination $Archive
    if ((Get-ArmX66FileState $Archive) -cne $After -or (Get-ArmX66FileState $Live) -cne $After) {
        throw 'Candidate preservation failed or concurrent change; refusing replacement.'
    }
    if ($Before -ceq 'ABSENT') {
        # Only an exact receipt-proven newly created native package; never a save.
        Remove-Item -LiteralPath $Live
    }
    else {
        $Temp = Join-Path (Split-Path -Parent $Live) ('.armx66-rollback-'+[guid]::NewGuid().ToString('N')+'.tmp')
        try {
            Copy-Item -LiteralPath $Backup -Destination $Temp
            if ((Get-ArmX66FileState $Temp) -cne $Before -or (Get-ArmX66FileState $Live) -cne $After) {
                throw 'Package compare-and-replace refused.'
            }
            [IO.File]::Replace($Temp,$Live,$null)
        } finally {
            if (Test-Path -LiteralPath $Temp -PathType Leaf) { Remove-Item -LiteralPath $Temp }
        }
    }
    if ((Get-ArmX66FileState $Live) -cne $Before) { throw 'Native rollback state mismatch.' }
}
