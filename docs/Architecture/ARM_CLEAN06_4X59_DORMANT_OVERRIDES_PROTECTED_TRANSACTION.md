# ARM-CLEAN06.4X.59 — nettoyage protégé des trois overrides dormants

Parent binaire : `d59d3cb25dd43d02c6b14c8cad340bdf23d81896` (X.58B, 1992 tests réussis dont 30 avec avertissements, SilentTask + LocalAuto SUCCESS, éditeur/PIE 8/8 validés par le propriétaire).

L'audit X.19 a établi trois listes héritées inactives dans le seul DA_GridLevel_00 :

| GUID | Cellule | Liste inactive | Ancien ID |
|---|---|---|---|
| 0CB018F04E6C3DCB059133A0CA12214F | 16,25 | ReceptacleRules.AcceptedItems | Item_Torch |
| 12F822EE439884334380F4AED3223847 | 11,24 | AcceptedKeys.AcceptedKeyItems | Key_Copper |
| 22CCD36F42A9F3FE8C7C3B852E2F71FA | 9,23 | AcceptedKeys.AcceptedKeyItems | Key_Iron |

Décision : vider uniquement ces listes dormantes, ne pas écrire un nouvel ID dormant, ne pas activer un override. Les autres valeurs (capacité, bAcceptAnyItem, état initial, clés actives, liens, scripts, cellules) restent identiques. La définition partagée est l'autorité tant que l'override est désactivé. Selected Object utilise déjà les helpers X.49/X.50 pour recréer des règles canoniques à l'activation ; aucun changement de UI/runtime nécessaire.

## Preuves préparées

FGridEditorNativeDormantOverridesPlan reprend les helpers de comparaison/export X.58, pas son écriture. Charge les quatre niveaux, exige le cutover X.58B complet, trois GUID/cellules exacts et leurs définitions, interrupteurs désactivés, listes d'origine exactes ou toutes vides. Toute autre liste/ID d'acceptation, override actif, incohérence ou état partiel bloque. Compare tous les champs réfléchis de FGridObjectBehaviorParams avant/après, et vérifie que les helpers d'authoring reconstituent les trois règles canoniques exactes. Test Grimrock.RPG.ARM.CLEAN06_4X59.NativeDormantOverridesPlan : projection complète, parité de tous les champs monde, lecture seule, idempotence, refus actif/mauvaise cellule/règle inattendue/état partiel. X.48 impose désormais zéro ancien pointeur initial et 8 ou 5 références résiduelles, les cinq loots restent inchangés.

## Transaction native

GridArmClean064X59NativeDormantOverrides : Preflight/Verify lecture seule ; Apply seulement avec snapshot externe byte-equal du seul niveau 00, SHA opérateur, plan valide et comparaison complète du manifeste des quatre niveaux. Aucun autre asset sauvé. Script ApplyARM_CLEAN06_4X59_NativeDormantOverrides.ps1 reprend les garde-fous X.58 éprouvés : statuts exacts, master propre et épinglé, SHA256 des quatre niveaux, build, préflight et verify, snapshot, apply, verify dans processus frais, full Automation Grimrock, index/LFS, publication d'un seul binaire, rollback borné avant commit. Sauvegarde privée sous ARM_CLEAN06_4X59. Mode par défaut en lecture seule. Aucun SaveGame touché.

Après les deux statuts Windows du SHA code, exécuter le script avec -RepoRoot D:\Development\GrimrockPrototype -EngineRoot D:\UE_5.5 -StateRoot D:\GrimrockNativeSnapshots -ExpectedHead SHA40_DU_CODE -Apply -Publish, UE fermé. Attendre les deux statuts du SHA binaire puis tester torche, mauvaises offrandes, serrures cuivre/fer avec bonne/mauvaise clé et activation/désactivation sur instances de test sans enregistrer le niveau natif.

Aucun texte UI/WBP modifié ; localisation inchangée. **Avant transaction : 8 références restantes. Après X.59B : 5, toutes dans les loots.** Ce n'est pas la fin de 06.4 : 4 identités de loot hors registre, 10 identités alchimie, catalogues historiques, WBP et v27 restent à traiter. Aucune suppression de classe/champ/asset dans ce lot.
