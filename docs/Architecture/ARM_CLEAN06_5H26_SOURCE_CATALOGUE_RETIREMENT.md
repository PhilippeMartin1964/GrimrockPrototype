# ARM-CLEAN06.5H.26 — Final authoring-catalogue retirement

The separate Git commit removes **only** two obsolete authoring assets that had zero external referencer edges in the native ARM-CLEAN06.5A audit:

| Asset removed | Previous Git blob SHA |
|---|---|
| `Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.uasset` | `29837374cb8fb459d3368783283dabca98ce313a` |
| `Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.uasset` | `8231c04fbe254a8a6932822716c45e3d2b0355d6` |

**Rollback parent (full Git LFS contents preserved):** `af7cfc3b1ced0d04459f83e7c169ebdd6915b4db`.

The active authoritative catalogue remains `DA_ItemSystem` and its canonical staged data. The old importer/baseline code is archival tooling, not an active importer, and cannot recreate an authoritative gameplay catalogue.

The seven private v27 SaveGames and their two serialised `DA_BookPage_Test` and `DA_Scroll_Test` Readables are not touched. The new automated native-audit and package-registry tests fail on any reintroduced historical definition, catalogue or redirector. UE 5.5.4 automated validation is required on this exact SHA; PIE/cook/save-continue are distinct release checks.
