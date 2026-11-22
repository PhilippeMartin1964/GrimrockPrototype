# ARM-CLEAN06.5H.25 — Atomic historical package retirement receipt

## Authority and prior evidence

Retirement performed on the explicit user request of 2026-11-22. The independently generated *read-only* native report **did not itself authorize deletion**. It showed 48 native item identities, 22 ARM, 466 valid level resolutions, zero external referencer edges, 48 matching obsolete definitions, 53 classified candidates, seven unchanged decoded v27 SaveGames and zero unknown item IDs. It also demonstrated that the ThrowingKnife redirector references only the obsolete target.

This separate atomic Git transaction removes **exactly 48 historical `GridItemDefinitionAsset` packages and their single orphaned ThrowingKnife redirector**, without editing any surviving binary, SaveGame or level. The source state and every Git LFS pointer are recoverable at parent commit:

`c2bd170cc7bcbb50d568718c353f4b1d22a452f6`

Rollback **of the entire transaction**, if necessary, must be done via a new safe revert commit; never perform an unqualified reset of a user's checkout. Before any history garbage collection, Git LFS payloads remain reachable through this parent.

## Never removed in this transaction

- `DA_ItemSystem`, its canonical item and armament definitions, menus, levels, meshes, effects and all runtime packages
- Historical `DA_WeaponCatalogue` and `DA_WeaponVisualCatalogue` — the separate protected old authoring references
- `DA_BookPage_Test` and `DA_Scroll_Test` — two Readable assets directly named in a v27 private save
- All seven private SaveGames and their copied SHA256 audit receipts

## Git-tracked file manifests (blob hashes of the original LFS pointers)

All 49 paths were checked against a complete untruncated 4694-entry Git tree before the transaction. Exactly these paths, and no others, were deleted:

| Deleted package | Previous Git blob SHA |
|---|---|
| `Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Antidote.uasset` | `584c89783db88aa33c1627a0f91a4c6befa0facd` |
| `Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Arrows_01.uasset` | `9d37362a07f9af6988f29dd31029426d5224aea4` |
| `Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_BlueGem.uasset` | `be6c84f497aff41fbfc61c9fb444c7fd2a1155de` |
| `Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Bomb_Fire.uasset` | `30820c90a5db85a962de47a90f4d121c6e070493` |
| `Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Bomb_Toxic.uasset` | `5bae9d69275b388156b370d8f97c9035a7c1ae09` |
| `Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_CopperKey.uasset` | `f20322507e675dd62dca386377660599b0b4756d` |
| `Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_DefensiveElixir_Fire.uasset` | `84242d842c0400982d827d99c830aa9285653d10` |
| `Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_DefensiveElixir_Ice.uasset` | `f3e65c46a47ff1d2ff0692235ab6686ad92c53fe` |
| `Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_DefensiveElixir_Lightning.uasset` | `1e6868b17ec5b6f2c334dfb62b350183a8b340ba` |
| `Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_DefensiveElixir_Poison.uasset` | `126d20cea265a9d36605a668bf5efe1d68c913c5` |
| `Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_DisarmTools_01.uasset` | `9a9292ae3d346db31fadaa2f88abb0016b6b04f6` |
| `Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_EmptyVial.uasset` | `fca1724d7cb023da8e2939c83b95c2b0d24f83eb` |
| `Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Flask_Acid.uasset` | `6c1843968a9129a1c4789442127fb1697551e94a` |
| `Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Flask_CorrosiveCloud.uasset` | `7d27a700deb97fb8ca006c4130051f41846fd19e` |
| `Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Flask_Oil.uasset` | `bed7c70a64036fbbb956eb87a2204b6b4f7713f8` |
| `Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_GoblinKnife.uasset` | `59c80c77b915d8b83f43f7ce7f9dbc00876e649c` |
| `Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_IronKey.uasset` | `4053207299be2f03c6b3c58fd1550600efcb56db` |
| `Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Panacea.uasset` | `c730dc2980f19ccf791945bbc4b05e2403a03007` |
| `Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Potion_Healing_01.uasset` | `3e9dd199883f9415e9a8e55f0f687f4731b15700` |
| `Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_RatMeat.uasset` | `67d5120ba36222dde4c4425a8b0bddc888f6eee2` |
| `Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_RatTooth.uasset` | `15d4ce65128154c2e121466c75c976caf34dbc46` |
| `Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Stone.uasset` | `7a29e89a948410cd158c056dba4e749ce4752dca` |
| `Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_SurfaceCatalyst_01.uasset` | `fcbb4fcbdd937ffaf9fc23032c94116da24a35e7` |
| `Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Torch.uasset` | `be74ac9cac32f24e78ae2b60f27237d47a6b8e40` |
| `Content/GrimrockPrototype/Core/DataAssets/Tests/Items/DA_Item_TestNote.uasset` | `ff3c811c7d72fb26ba0d302e567e9415a8ad8598` |
| `Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_ArcaneOrb_01.uasset` | `50a53a2a9d8d74908ab5c3422b47a282b64783c5` |
| `Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_ArcaneWand_01.uasset` | `03fef1887bea6fedd869696fa915c45d2804fe5d` |
| `Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_AssassinDagger_01.uasset` | `124d02890e94b5199833876586c0414c6164b180` |
| `Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_Dagger_01.uasset` | `b5f2db760f123dc71b4be40193434a969e638254` |
| `Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_Dart_01.uasset` | `c027e1e74b2ac3bfee1590e3ebcc0dde6c88092f` |
| `Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_ElementalStaff_01.uasset` | `49ea78b2bca2326006868751b2a80b23a26d1d55` |
| `Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_FireWand_01.uasset` | `afa2729ae65fcf4d54f86c2adfabfd52c31c02f1` |
| `Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_GreatAxe_01.uasset` | `6ec758e63681a0e7237d0d24eef62b8d2c55f1cb` |
| `Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_HolyAegis_01.uasset` | `47bf6e9f05e3f7639206d28a2d0d2295da40fa8c` |
| `Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_HolyScepter_01.uasset` | `2f8029bf70a096698db4a96f1be17a5321dead1a` |
| `Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_Javelin_01.uasset` | `d6b448ecaa1829a7399fe8f4f9314251f699f51c` |
| `Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_LongSword_01.uasset` | `f70b6526548be73a32f80c4de64e161fde646113` |
| `Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_Mace_01.uasset` | `061e6ffbd4d40e6a58627205dc992c3269cc8ce2` |
| `Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_MainGauche_01.uasset` | `dd1cb4d7023439be89e7af40b1aa792edf7ae9c1` |
| `Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_RecurveBow_01.uasset` | `076585c0982cb7e6961a157a40932d5012ae9e34` |
| `Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_RoundShield_01.uasset` | `056d37ba8758a17698abf009c9eda7d91492ceb6` |
| `Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_ShortBow_01.uasset` | `74e9cc74444638e6a03b778d835fe4e6ed1938e7` |
| `Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_ShortSpear_01.uasset` | `23a128c19ccab35513f2c57cc4adbeeed5df1213` |
| `Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_Shuriken.uasset` | `bdf2be16b3fdc915627b971d3545017216ac6576` |
| `Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_Stiletto_01.uasset` | `e22728b952ce0674d672f06b8b2ad4c32a25b3f2` |
| `Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_StraightSword_01.uasset` | `e4196ad3255cb70861bd18f97ac014863176bf92` |
| `Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_ThrowingKnife.uasset` | `d927e85e3ce8dcc53633564f24348c7ceacd9285` |
| `Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_ThrowingKnife_01.uasset` | `6ea7d51efdc9bb3bad53b171ce24a306e2df53f0` |
| `Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_WarPick_01.uasset` | `5479827e2912da0d11b3c8862663c71ac3f4c8b7` |

## Acceptance and limits

The Editor Automation test `Grimrock.Editor.ARM.CLEAN06_5A.NativeRetirementEvidence` now requires zero obsolete definitions and zero ThrowingKnife redirectors, while still requiring 48 canonical identities, 22 armaments, and two protected Readables. Existing world/save and production integration tests remain mandatory.

UE 5.5.4 Windows validation for the resulting SHA is mandatory before regarding this removal as accepted. This is a **version-controlled removal of unreferenced packages**, not a claimed UE Editor fix-up operation. Visual PIE, Cook/Package and a Save/Continue trial remain separate release-level checks.
