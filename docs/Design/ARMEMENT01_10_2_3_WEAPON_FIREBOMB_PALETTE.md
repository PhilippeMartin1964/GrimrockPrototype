# ARMEMENT01.10.2.3 — Weapon & FireBomb in the Grid Editor palette

## Contract
Only the editor grouping PaletteCategory is changed. No gameplay type,
weapon statistics, ItemDefinitionId, WorldObjectDefinition, kit/talent
progression, existing level, spawn, equipped item, or save is changed.

Target direct-collectible palette entries:
- Weapon_Dagger_01 (ARM001) -> Weapon
- Weapon_ShortBow_01 (ARM045) -> Weapon
- Weapon_RoundShield_01 (ARM075) -> Weapon
- Weapon_StraightSword_01 (ARM014) -> Weapon
- Item_Bomb_Fire -> Items

Each references only its real DefaultItemDefinition; no companion
DefaultWorldObjectDefinition and no duplicate Palette.Icon. The ItemDefinition
owns the icon and visual mesh. Historical EntryIds persist unchanged.

## Guarded migration of the already-modified local palette

The user's DA_ObjectPalette_Default.uasset already has uncommitted changes.
The GitHub code commit deliberately DOES NOT include a replacement .uasset.
The new GridWeaponFireBombPalette commandlet edits the local palette only
when explicitly asked to -Apply and when all five real ItemDefinitions exist.
The ARM014 item exists on the user's machine, but it is not in the current
remote master until the local artwork is approved and explicitly committed.

The commandlet:
1. Validates the original production palette before making a proposal.
2. Loads and validates each target item, including the canonical ID.
3. Copies all entries. It preserves unrelated entries, their order, data and
   identities. Existing requested entries are kept with the same EntryId.
4. Refuses duplicates, wrong definitions, dual-definition collectibles,
   any duplicate alias for an item, or nonstandard categories.
5. Allows only Items -> Weapon for the four targeted weapons; the bomb must
   be in Items. Adds missing entries in the correct category.
6. Runs complete UGridObjectPaletteAsset::ValidatePalette on the proposed copy.
7. Without -Apply, makes zero writes. With -Apply, saves only the existing
   DA_ObjectPalette_Default package, or nothing if already compliant.
8. Never stages, commits, pushes or resets any source-controlled assets.

Historical ARMEMENT01.7.1 authoring and its tests accept both Items
(legacy) and Weapon (migrated) for the three original pilots. The Grid Editor
sort order now recognizes Weapon immediately after Items.

## Local commands

Close Unreal Editor before -Apply.

~~~powershell
cd D:\Development\GrimrockPrototype
git status --short
git pull --ff-only origin master

# Read-only preflight
.\Scripts\ApplyARMEMENT011023WeaponFireBombPalette.ps1 -EngineRoot D:\UE_5.5

# Inspect ADD/MOVE/ALREADY messages, then apply explicitly
.\Scripts\ApplyARMEMENT011023WeaponFireBombPalette.ps1 -EngineRoot D:\UE_5.5 -Apply
~~~

The script checks the SHA256 hash for a read-only run, records UE output under
Saved/ARMEMENT01/ARMEMENT01_10_2_3_WeaponFireBombPalette.log, builds
Development Editor and, after Apply, runs automation filters:
- Grimrock.RPG.ARMEMENT01.10
- Grimrock.RPG.ARMEMENT01.7.Palette

Regression tests verify an in-memory migration, strict category and
ItemDefinition references, repeated idempotence, duplicate alias rejection,
and fire-bomb selection/placement through the actual editor API.

## Manual PIE review

- Weapon tab: dagger, bow, round shield and Straight Sword.
- Items tab: Fire Bomb with its own icon.
- Place each on an ordinary floor cell; inspect editor preview/placement.
- PIE: recover fire bomb; check inventory and stack/consume behavior.
- N1 fire bomb in inventory does not by itself grant the level-2 Grenadier
  talent or recipe. Keep that progression rule unchanged.
- Verify unrelated user edits to stairs, dungeon, map and borders survive.

## Git safety

Do NOT run git add . or git commit -a. Do NOT restore/reset the user's
dirty palette, dungeon, stairs, border, maps or local ARM014 artwork.

The palette was already locally modified before this ticket. Its eventual
binary Git LFS commit must be reviewed together with the user's earlier
palette edits; Git cannot split .uasset changes by logical entry.
Do not claim local compilation or PIE succeeded until local logs/screenshots
have been inspected.
