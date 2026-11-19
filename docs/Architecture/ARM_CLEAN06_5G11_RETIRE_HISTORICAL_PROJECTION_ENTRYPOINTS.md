# ARM-CLEAN06.5G.11 — Retrait des points d'entrée d'audit historiques

Après validation de 5G.10, retirer les 13 commandlets d'audit/projection E, H à R et X62, ainsi que les 12 scripts correspondants. Leurs responsabilités étaient des preuves ponctuelles de la conversion déjà réalisée ; leurs points d'entrée ne sont plus appelés par l'agent ni par les contrôles permanents. La recherche exacte des noms des classes et scripts n'a identifié aucun appel hors de ce périmètre (hors documentation historique). Les preuves déjà produites, helpers de parité, fixtures, tests et historiques Git ne sont pas effacés.

Conserver explicitement les trois commandlets Baseline, 4X64 SaveCopy et 5A RetirementAudit et leurs scripts, les trois auto-tests de sauvegarde/rollback/audit de ValidateUE, les snapshots et les outils de qualification/relecture de preuves. Aucune écriture ou suppression de packages, sauvegardes, localisation ou WBP. Aucun changement de runtime. Attendre les deux statuts Windows du présent SHA avant le lot suivant.

## Périmètre retiré (38 fichiers)

- `Scripts/AuditARM_CLEAN06_4H_WorldReferences.ps1`
- `Scripts/AuditARM_CLEAN06_4I_DistinctItemIds.ps1`
- `Scripts/AuditARM_CLEAN06_4J_NonArmTransient.ps1`
- `Scripts/AuditARM_CLEAN06_4K_NonArmFieldOwners.ps1`
- `Scripts/AuditARM_CLEAN06_4L_NonArmPresentationParity.ps1`
- `Scripts/AuditARM_CLEAN06_4M_NonArmReadingPhysics.ps1`
- `Scripts/AuditARM_CLEAN06_4N_NonArmGenericThrow.ps1`
- `Scripts/AuditARM_CLEAN06_4O_NonArmActionSemantics.ps1`
- `Scripts/AuditARM_CLEAN06_4P_NonArmEquipmentParity.ps1`
- `Scripts/AuditARM_CLEAN06_4Q_NonArmStackRegistryParity.ps1`
- `Scripts/AuditARM_CLEAN06_4R_NonArmActionBridge.ps1`
- `Scripts/CheckARM_CLEAN06_4E_PresentationParity.ps1`
- `Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridArmClean064EPresentationParityCommandlet.cpp`
- `Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridArmClean064HReferenceAuditCommandlet.cpp`
- `Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridArmClean064IIdentityGapCommandlet.cpp`
- `Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridArmClean064JNonArmProjectionCommandlet.cpp`
- `Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridArmClean064KNonArmFieldOwnershipCommandlet.cpp`
- `Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridArmClean064LNonArmPresentationParityCommandlet.cpp`
- `Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridArmClean064MNonArmReadingPhysicsCommandlet.cpp`
- `Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridArmClean064NNonArmGenericThrowCommandlet.cpp`
- `Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridArmClean064ONonArmActionSemanticsCommandlet.cpp`
- `Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridArmClean064PNonArmEquipmentParityCommandlet.cpp`
- `Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridArmClean064QNonArmStackParityCommandlet.cpp`
- `Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridArmClean064RNonArmFullActionBridgeCommandlet.cpp`
- `Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridArmClean064X62AlchemyAuditCommandlet.cpp`
- `Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridArmClean064EPresentationParityCommandlet.h`
- `Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridArmClean064HReferenceAuditCommandlet.h`
- `Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridArmClean064IIdentityGapCommandlet.h`
- `Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridArmClean064JNonArmProjectionCommandlet.h`
- `Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridArmClean064KNonArmFieldOwnershipCommandlet.h`
- `Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridArmClean064LNonArmPresentationParityCommandlet.h`
- `Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridArmClean064MNonArmReadingPhysicsCommandlet.h`
- `Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridArmClean064NNonArmGenericThrowCommandlet.h`
- `Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridArmClean064ONonArmActionSemanticsCommandlet.h`
- `Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridArmClean064PNonArmEquipmentParityCommandlet.h`
- `Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridArmClean064QNonArmStackParityCommandlet.h`
- `Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridArmClean064RNonArmFullActionBridgeCommandlet.h`
- `Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridArmClean064X62AlchemyAuditCommandlet.h`

## Suite

Démêler les helpers et tests de conversion du contrôle permanent ; retirer ensuite les références et chemins legacy encore présents dans le code. Ne pas déclarer zéro legacy avant preuve native et recette 06.6.
