[CmdletBinding()]
param(
    [string]$ExpectedHead,
    [string]$ReceiptPath,
    [string]$EngineRoot='D:\UE_5.5',
    [switch]$SelfTest
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'ARM_CLEAN06_4X66_Rollback.ps1')
$ConversionHead='475f7f3f04c60ea50bd9ed44ef2bee69c7116ba0'

function FullPath([string]$Path) { [IO.Path]::GetFullPath($Path).TrimEnd('\','/') }
function AssertSet([string]$Directory,$Rows,[bool]$Converted) {
    Assert-ArmX66OrdinaryPath $Directory
    if (-not (Test-Path -LiteralPath $Directory -PathType Container)) { throw 'Save directory missing.' }
    $Entries=@(Get-ChildItem -LiteralPath $Directory -Force)
    if ($Entries.Count -ne @($Rows).Count) { throw 'Save set changed.' }
    foreach ($Row in $Rows) {
        $File=Join-Path $Directory $Row.name
        $Hash=if ($Converted) { $Row.converted_sha256 } else { $Row.original_sha256 }
        $Size=if ($Converted) { $Row.converted_bytes } else { $Row.original_bytes }
        if ((Get-ArmX66FileState $File) -cne $Hash -or (Get-Item -LiteralPath $File).Length -ne $Size) {
            throw ('Save bytes differ from receipt: '+$Row.name)
        }
    }
}
function CheckRows($Rows) {
    if (@($Rows).Count -ne 7) { throw 'Expected seven protected saves.' }
    $Names=@{}
    foreach ($Row in $Rows) {
        if ($Row.name -cnotmatch '^[A-Za-z0-9_-]+\.sav$' -or $Row.name -match '^(CON|PRN|AUX|NUL|COM[0-9]|LPT[0-9])\.sav$' -or $Names.ContainsKey($Row.name) -or
            $Row.original_sha256 -cnotmatch '^[0-9a-f]{64}$' -or $Row.converted_sha256 -cnotmatch '^[0-9a-f]{64}$' -or
            $Row.original_bytes -le 0 -or $Row.converted_bytes -le 0) { throw 'Invalid receipt row.' }
        $Names[$Row.name]=$true
        if ($Row.original_sha256 -ceq $Row.converted_sha256 -and $Row.original_bytes -ne $Row.converted_bytes) { throw 'Inconsistent unchanged row.' }
    }
    $Changed=@($Rows | Where-Object { $_.original_sha256 -cne $_.converted_sha256 })
    if ($Changed.Count -ne 1) { throw 'Expected precisely one converted save.' }
    return $Changed[0]
}
function InstallOne([string]$Live,[string]$Original,[string]$Converted,$Rows,[scriptblock]$PostCheck) {
    $Changed=CheckRows $Rows
    AssertSet $Live $Rows $false
    AssertSet $Original $Rows $false
    AssertSet $Converted $Rows $true
    $Target=Join-Path $Live $Changed.name
    $Backup=Join-Path $Original $Changed.name
    $Candidate=Join-Path $Converted $Changed.name
    try {
        Replace-ArmX66CheckedFile $Candidate $Target $Changed.original_sha256 $Changed.converted_sha256
        AssertSet $Live $Rows $true
        AssertSet $Original $Rows $false
        AssertSet $Converted $Rows $true
        & $PostCheck
    } catch {
        $Failure=$_
        # Only restore receipt-proven bytes; never overwrite unknown progress.
        $Current=Get-ArmX66FileState $Target
        if ($Current -ceq $Changed.converted_sha256) {
            Replace-ArmX66CheckedFile $Backup $Target $Changed.converted_sha256 $Changed.original_sha256
        } elseif ($Current -cne $Changed.original_sha256) {
            throw 'Unknown live bytes preserved. Manual recovery evidence required.'
        }
        AssertSet $Live $Rows $false
        throw $Failure
    }
}
function CheckColdReport($Report,$Rows,$Changed) {
    if ($Report.schema -cne 'ARM-CLEAN06.5A/retirement-evidence/v1' -or $Report.deletion_authorized -or
        $Report.save_count -ne 7 -or $Report.decode_failures -ne 0 -or @($Report.saves).Count -ne 7 -or
        $Report.native.canonical_count -ne 48 -or $Report.native.resolved_item_levels -ne 466) { throw 'Cold audit refused.' }
    $Paths=@(
        '/Game/GrimrockPrototype/Core/DataAssets/Tests/Readables/DA_Scroll_Test.DA_Scroll_Test',
        '/Game/GrimrockPrototype/Core/DataAssets/Tests/Readables/DA_BookPage_Test.DA_BookPage_Test'
    )
    foreach ($Row in $Rows) {
        $Match=@($Report.saves | Where-Object { $_.filename -ceq $Row.name })
        if ($Match.Count -ne 1) { throw 'Cold audit slot mismatch.' }
        $S=$Match[0]
        if (-not $S.decoded -or $S.version -ne 27 -or $S.bytes -ne $Row.converted_bytes -or
            @($S.unknown_item_ids).Count -ne 0 -or $S.projectile_definition_pointers -ne 0 -or
            $S.pending_definition_pointers -ne 0 -or $S.readable_ids_without_pointer -ne 0) { throw 'Cold audit contains unresolved data.' }
        if ($Row.name -ceq $Changed.name) {
            if ($S.readable_content_pointers -ne 2 -or @($S.decoded_references).Count -ne 2 -or
                @($S.raw_path_candidates).Count -ne 2) { throw 'Expected two restored readable references.' }
            foreach ($P in $Paths) {
                if (@($S.decoded_references | Where-Object { $_.field -ceq 'ReadableContentAsset' -and $_.object -ceq $P -and -not $_.identity_conflict }).Count -ne 1 -or
                    @($S.raw_path_candidates | Where-Object { $_.object -ceq $P }).Count -ne 1) { throw 'Restored readable target mismatch.' }
            }
        } elseif ($S.readable_content_pointers -ne 0 -or @($S.raw_path_candidates).Count -ne 0 -or @($S.decoded_references).Count -ne 0) {
            throw 'Unexpected references in an unchanged save.'
        }
    }
}
if ($SelfTest) {
    $Root=Join-Path ([IO.Path]::GetTempPath()) ('Arm5BInstallTest-'+[guid]::NewGuid().ToString('N'))
    if (Test-Path -LiteralPath $Root) { throw 'Fixture collision.' }
    [void](New-Item -ItemType Directory -Path $Root)
    try {
        $Live=Join-Path $Root 'Live'; $Original=Join-Path $Root 'Original'; $Converted=Join-Path $Root 'Converted'
        foreach ($Dir in @($Live,$Original,$Converted)) { [void](New-Item -ItemType Directory -Path $Dir) }
        $Rows=@()
        for ($I=0;$I -lt 7;$I++) {
            $Name='Fixture'+$I+'.sav'
            $A=Join-Path $Original $Name; $B=Join-Path $Converted $Name
            [IO.File]::WriteAllText($A,'before'+$I)
            Copy-Item -LiteralPath $A -Destination (Join-Path $Live $Name)
            Copy-Item -LiteralPath $A -Destination $B
            if ($I -eq 0) { [IO.File]::WriteAllText($B,'after') }
            $Rows+=[pscustomobject]@{name=$Name;original_sha256=(Get-ArmX66FileState $A);converted_sha256=(Get-ArmX66FileState $B);original_bytes=(Get-Item -LiteralPath $A).Length;converted_bytes=(Get-Item -LiteralPath $B).Length}
        }
        $Failed=$false
        try { InstallOne $Live $Original $Converted $Rows { throw 'Injected failure after replacement' } } catch { $Failed=$true }
        if (-not $Failed) { throw 'Missing injected failure.' }
        AssertSet $Live $Rows $false
        InstallOne $Live $Original $Converted $Rows {}
        AssertSet $Live $Rows $true
        $Failed=$false
        try { InstallOne $Live $Original $Converted $Rows {} } catch { $Failed=$true }
        if (-not $Failed) { throw 'Reinstallation must refuse changed live set.' }
        AssertSet $Live $Rows $true
        AssertSet $Original $Rows $false
        AssertSet $Converted $Rows $true
        Write-Host '[OK] 5B save installation, rollback and replay refusal on synthetic bytes.'
    } finally { Remove-Item -LiteralPath $Root -Recurse -Force }
    return
}

if ($ExpectedHead -cnotmatch '^[0-9a-f]{40}$' -or [string]::IsNullOrWhiteSpace($ReceiptPath)) { throw 'Exact HEAD and private receipt required.' }
$Repo=FullPath (Split-Path -Parent $PSScriptRoot)
$Live=Join-Path $Repo 'Saved\SaveGames'
$Git=(Get-Command git.exe -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source
function InvokeCheckedGit([string[]]$Arguments) {
    $Lines=@(& $Git -C $Repo @Arguments)
    if ($LASTEXITCODE -ne 0) { throw 'Git check failed.' }
    $Lines
}
function CheckGit {
    if ((InvokeCheckedGit @('rev-parse','HEAD')) -cne $ExpectedHead -or
        (InvokeCheckedGit @('branch','--show-current')) -cne 'master' -or
        @(InvokeCheckedGit @('status','--porcelain','--untracked-files=all')).Count -ne 0) { throw 'Expected exact clean master.' }
    $Remote=@(InvokeCheckedGit @('ls-remote','origin','refs/heads/master'))
    if ($Remote.Count -ne 1 -or ($Remote[0] -split '\s+')[0] -cne $ExpectedHead) { throw 'Origin master changed.' }
}
function CheckClosed {
    if (@(Get-Process -Name UnrealEditor,UnrealEditor-Cmd,GrimrockPrototype,GrimrockPrototype-Win64-Shipping -ErrorAction SilentlyContinue).Count -ne 0) { throw 'Close UE and the game.' }
}
CheckGit
$Statuses=@(& gh.exe api ('repos/PhilippeMartin1964/GrimrockPrototype/commits/'+$ExpectedHead+'/status'))
if ($LASTEXITCODE -ne 0) { throw 'Cannot retrieve exact SHA validation.' }
$Status=($Statuses -join [Environment]::NewLine) | ConvertFrom-Json
foreach ($Context in @('UE5.5.4/SilentTask','UE5.5.4/LocalAuto')) {
    if (@($Status.statuses | Where-Object { $_.context -ceq $Context -and $_.state -ceq 'success' }).Count -ne 1) { throw ('Windows gate not acquired: '+$Context) }
}
CheckClosed
$ReceiptPath=FullPath $ReceiptPath
Assert-ArmX66OrdinaryPath $ReceiptPath
$Snapshot=Split-Path -Parent $ReceiptPath
if ((Split-Path -Leaf $ReceiptPath) -cne 'verified-copies.json' -or
    (Split-Path -Leaf $Snapshot) -cnotmatch '^ARM_CLEAN06_5B_475f7f3f04c6_[0-9a-f]{32}$' -or
    $Snapshot -ieq $Repo -or $Snapshot.StartsWith($Repo+'\',[StringComparison]::OrdinalIgnoreCase) -or
    $Repo.StartsWith($Snapshot+'\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Private 5B receipt location refused.' }
$Receipt=Get-Content -LiteralPath $ReceiptPath -Raw -Encoding UTF8 | ConvertFrom-Json
if ($Receipt.schema -cne 'ARM-CLEAN06.5B/readable-copy-receipt/v1' -or $Receipt.head -cne $ConversionHead -or
    -not $Receipt.live_saves_unchanged -or $Receipt.installed -or $Receipt.save_count -ne 7 -or
    (FullPath $Receipt.snapshot) -ine $Snapshot -or (FullPath $Receipt.source_directory) -ine $Live) { throw 'Receipt mismatch.' }
$Rows=@($Receipt.files)
$Changed=CheckRows $Rows
$Original=Join-Path $Snapshot 'Original'; $Converted=Join-Path $Snapshot 'Converted'
AssertSet $Live $Rows $false
AssertSet $Original $Rows $false
AssertSet $Converted $Rows $true

# Audit exact candidate bytes in a fresh UE process before writing anything live.
$Run=Join-Path $Snapshot ('InstallEvidence_'+[guid]::NewGuid().ToString('N'))
if (Test-Path -LiteralPath $Run) { throw 'Evidence directory collision.' }
$AuditCopies=Join-Path $Run 'AuditCopies'
[void](New-Item -ItemType Directory -Path $AuditCopies -Force)
foreach ($Row in $Rows) { Copy-Item -LiteralPath (Join-Path $Converted $Row.name) -Destination (Join-Path $AuditCopies $Row.name) }
AssertSet $AuditCopies $Rows $true
$Project=Join-Path $Repo 'GrimrockPrototype.uproject'
$Build=Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$Editor=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
& $Build GrimrockPrototypeEditor Win64 Development "-Project=$Project" -WaitMutex -NoHotReloadFromIDE | Out-Host
if ($LASTEXITCODE -ne 0) { throw 'Local build failed.' }
CheckGit
CheckClosed
$ReportPath=Join-Path $Run 'retirement-evidence.json'
& $Editor $Project '-run=GridArmClean065RetirementAudit' '-ReadOnly' "-Copies=$AuditCopies" "-Output=$ReportPath" '-Unattended' '-NoP4' '-NoSplash' '-NoSound' '-NullRHI' "-abslog=$(Join-Path $Run 'audit.log')" | Out-Host
if ($LASTEXITCODE -ne 0) { throw 'Cold candidate audit failed.' }
CheckColdReport (Get-Content -LiteralPath $ReportPath -Raw -Encoding UTF8 | ConvertFrom-Json) $Rows $Changed
AssertSet $AuditCopies $Rows $true
CheckGit
CheckClosed
$Intent=[pscustomobject]@{schema='ARM-CLEAN06.5B/install-intent/v1';head=$ExpectedHead;source_receipt=$ReceiptPath;changed_slot=$Changed.name;files=$Rows}
$Intent | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $Run 'install-intent.json') -Encoding UTF8
Write-Host ('[5B] PRIVATE EVIDENCE '+$Run)
InstallOne $Live $Original $Converted $Rows {
    CheckGit
    AssertSet $AuditCopies $Rows $true
    [pscustomobject]@{schema='ARM-CLEAN06.5B/install-receipt/v1';head=$ExpectedHead;save_count=7;changed_count=1;restored_readables=2;originals_preserved=$true;installed=$true;native_audit=$ReportPath;files=$Rows} |
        ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $Run 'installed.json') -Encoding UTF8
}
Write-Host ('[OK] 5B INSTALLED: one save, six unchanged, original copies preserved. '+(Join-Path $Run 'installed.json'))
