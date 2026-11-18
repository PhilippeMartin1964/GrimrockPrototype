# ARM-CLEAN06.4X.66 — Native alchemy cutover

Parent X.65: `d944814a072f94b7483b23a63ea4fc204d4257c5` validated SilentTask + LocalAuto.

## Scope

- Exact 38 -> 48 item definitions, 4 -> 14 detailed actions.
- Ten new UGridConsumableDefinitionAsset packages, existing DA_ItemSystem and DA_ActionCatalogue only. Compact actions, weapon/visual/presentation catalogues and previous ordered definitions remain unchanged.
- Four distinct source-aware elemental elixir action IDs; all other reflected action fields retained. All ten old source assets remain present for proof and later retirement.
- Native verification reconstructs the historical prefix on a transient root then independently projects and compares every canonical authored field/action. Reject partial registry states, unexpected paths or dirty source packages. Historical X.62 commandlet still requires its actual 38-entry root; tests use a verified transient view after native cutover.
- Earlier loot, action, and reference tests recognize only the additional exact 10-item extension. No generic lower-bound relaxation.

## Local operation

`Scripts/ApplyARM_CLEAN06_4X66_NativeAlchemyCutover.ps1` defaults to read-only. Apply and Publish must be requested together, with explicit SaveDirectory. Close the editor and game. Use the precise validated code SHA. Use a private StateRoot outside the repository and save directory.

The script captures and converts all .sav files in the selected directory into a private X.64 snapshot, compares SHA256 receipts and checks that the live save set has not changed. It verifies 12 package paths, LFS, old bytes, expected preflight, full UE automation, freshly reopened verification and normalized manifest. Only after successful native proof, it installs changed save copies using compare-and-replace; no-op saves are untouched. Before binary commit, failure restores known changed saves and two source packages and removes only ten previously absent paths (unknown concurrent changes cause refusal). An existing binary commit is never reset or force-pushed. The binary commit contains only 12 uassets, not saves or receipts.

The chosen SaveDirectory must be the actual active save location; no other install/profile directories are silently scanned or changed. Original saves remain in the private snapshot. If a crash/interruption occurs, do not launch the game or reset assets independently of saves; inspect the private backups/receipts and exact Git state first.

PowerShell (replace code SHA only with the exact validated commit, adjust SaveDirectory to the actual save directory):

```powershell
Set-Location D:\Development\GrimrockPrototype
git pull --ff-only
.\Scripts\ApplyARM_CLEAN06_4X66_NativeAlchemyCutover.ps1 -RepoRoot (Get-Location).Path -EngineRoot D:\UE_5.5 -StateRoot D:\GrimrockNativeSnapshots -ExpectedHead '<validated X66 code SHA>' -SaveDirectory 'D:\Development\GrimrockPrototype\Saved\SaveGames' -Apply -Publish
```

No private save contents in public logs or Git. After binary push check exact SHA, both Windows statuses, and manual PIE: grenadier fire bomb and transmuter oil starter, all four elixir action/element pairs, PA/consumption/status durations and cooldowns, save reload and hotbar. Missing Status_Silence stays a preserved removal-only reference; no invented effect. No UI localization changes. No proof of visual/editor PIE by NullRHI.

## Exit gate

The native migration has not been executed by the code publication. Persistent registry is 38 until X.66B. Do not delete old definitions or historical save fields. Global 107/107 previous reference migration stays complete; alchemy deletion, catalogue/icon retirement and saved world-pointer cleanup remain.
