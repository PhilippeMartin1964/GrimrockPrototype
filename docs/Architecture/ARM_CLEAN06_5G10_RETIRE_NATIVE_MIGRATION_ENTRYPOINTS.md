# ARM-CLEAN06.5G.10 — Retrait des points d'entrée natifs de conversion terminés

## Périmètre

Après la validation de 5G.9, suppression des 17 scripts ApplyARM_CLEAN06 et de leurs 17 classes UCommandlet Editor-only (34 fichiers C++). Ces exécutables ponctuels peuvent encore écrire des packages historiques et n'ont plus d'appel dans l'agent ou ValidateUE. Leurs références exécutables recherchées par noms exacts ne présentent plus de dépendance hors du lot ; les anciens documents restent des archives historiques, pas des instructions réexécutables. Les deux scripts permanents de validation et les trois auto-tests de rollback/audit/sauvegarde ont été relus sur le SHA parent, sans appel à ces entrées.

## Garanties

- Les helpers de planification/parité, fixtures et tests Automation existants restent intacts ; aucun test de gameplay supprimé.
- Les audits read-only, les utilitaires de copies de sauvegarde, l'audit de retrait 5A et les procédures rollback restent en place jusqu'à vérification de leurs dépendances.
- Aucun asset, SaveGame, WBP, localisation, manifeste ou copie privée supprimé. Le code retiré reste accessible dans l'historique Git (parent : 2683d5d602299e5f8d9124303aeb061d360e3f02).
- Les deux statuts Windows du SHA publié doivent être SUCCESS avant le lot suivant. Cette opération ne démontre pas encore zéro legacy.

## Fichiers retirés

- `Scripts/ApplyARM_CLEAN06_4T_NonArmEditorMigration.ps1`
- `Scripts/ApplyARM_CLEAN06_4U_NonArmQuickMigration.ps1`
- `Scripts/ApplyARM_CLEAN06_4V_NonArmEquipmentMigration.ps1`
- `Scripts/ApplyARM_CLEAN06_4W_WorldReferenceMigration.ps1`
- `Scripts/ApplyARM_CLEAN06_4X37_ArmWorldProfiles.ps1`
- `Scripts/ApplyARM_CLEAN06_4X46_NativeLoosePointers.ps1`
- `Scripts/ApplyARM_CLEAN06_4X54_NativeSharedRules.ps1`
- `Scripts/ApplyARM_CLEAN06_4X57_NativePalette.ps1`
- `Scripts/ApplyARM_CLEAN06_4X58_NativeInitialContent.ps1`
- `Scripts/ApplyARM_CLEAN06_4X59_NativeDormantOverrides.ps1`
- `Scripts/ApplyARM_CLEAN06_4X5_PhysicalLevelMigration.ps1`
- `Scripts/ApplyARM_CLEAN06_4X60_NativeStoneLoot.ps1`
- `Scripts/ApplyARM_CLEAN06_4X61_NativeLootCutover.ps1`
- `Scripts/ApplyARM_CLEAN06_4X66_NativeAlchemyCutover.ps1`
- `Scripts/ApplyARM_CLEAN06_4X72_ReceptacleRules.ps1`
- `Scripts/ApplyARM_CLEAN06_4X83_LockRules.ps1`
- `Scripts/ApplyARM_CLEAN06_5D3_HandThrow.ps1`
- `Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridArmClean064TNonArmMaterializeCommandlet.cpp`
- `Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridArmClean064UNonArmQuickMaterializeCommandlet.cpp`
- `Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridArmClean064VNonArmEquipmentMaterializeCommandlet.cpp`
- `Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridArmClean064WWorldItemReferencesCommandlet.cpp`
- `Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridArmClean064X37NativeArmWorldProfilesCommandlet.cpp`
- `Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridArmClean064X46NativeLoosePointersCommandlet.cpp`
- `Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridArmClean064X54NativeSharedRulesCommandlet.cpp`
- `Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridArmClean064X57NativePaletteCommandlet.cpp`
- `Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridArmClean064X58NativeInitialContentCommandlet.cpp`
- `Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridArmClean064X59NativeDormantOverridesCommandlet.cpp`
- `Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridArmClean064X5PhysicalLevelsCommandlet.cpp`
- `Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridArmClean064X60NativeStoneLootCommandlet.cpp`
- `Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridArmClean064X61NativeLootCutoverCommandlet.cpp`
- `Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridArmClean064X66NativeAlchemyCutoverCommandlet.cpp`
- `Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridArmClean064X71ReceptacleRulesCommandlet.cpp`
- `Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridArmClean064X81LockRulesCommandlet.cpp`
- `Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridArmClean065D3HandThrowCommandlet.cpp`
- `Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridArmClean064TNonArmMaterializeCommandlet.h`
- `Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridArmClean064UNonArmQuickMaterializeCommandlet.h`
- `Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridArmClean064VNonArmEquipmentMaterializeCommandlet.h`
- `Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridArmClean064WWorldItemReferencesCommandlet.h`
- `Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridArmClean064X37NativeArmWorldProfilesCommandlet.h`
- `Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridArmClean064X46NativeLoosePointersCommandlet.h`
- `Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridArmClean064X54NativeSharedRulesCommandlet.h`
- `Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridArmClean064X57NativePaletteCommandlet.h`
- `Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridArmClean064X58NativeInitialContentCommandlet.h`
- `Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridArmClean064X59NativeDormantOverridesCommandlet.h`
- `Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridArmClean064X5PhysicalLevelsCommandlet.h`
- `Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridArmClean064X60NativeStoneLootCommandlet.h`
- `Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridArmClean064X61NativeLootCutoverCommandlet.h`
- `Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridArmClean064X66NativeAlchemyCutoverCommandlet.h`
- `Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridArmClean064X71ReceptacleRulesCommandlet.h`
- `Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridArmClean064X81LockRulesCommandlet.h`
- `Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridArmClean065D3HandThrowCommandlet.h`

## Suite

Retirer progressivement les lecteurs d'audit obsolètes et leurs dépendances uniquement lorsque les contrôles permanents et les preuves nécessaires sont préservés ; traiter ensuite le schéma legacy, les assets et les fixtures encore liés. Recette PIE/Save/Continue/Cook/Shipping 06.6 distincte.
