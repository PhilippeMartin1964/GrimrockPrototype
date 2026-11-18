# ARM-CLEAN06.4X.60 — cutover de Stone et inventaire exact des quatre loots hors registre

**Parent :** `1cf5ace7e979eac8843bd9e556a382cb4bc0c8f2`, X.59B validé par les deux statuts Windows et la recette propriétaire 8/8 ; 1993 tests réussis dont 30 avec avertissements. 102 références supprimées sur les 107 recensées par X.48.

## Périmètre

Le vrai GoblinThrower a trois loots : GoblinKnife (25 %), Stone (50 %), EmptyVial (25 %), chacun quantité 1. RatGiant a Item_RatMeat et Item_RatTooth. Seul Stone est déjà résolu dans le registre de 34 définitions. Les quatre autres restent exactement sur leur source native à ce stade. Le plan ne crée ni ne supprime une définition ; aucune modification de probabilité, poids, mesh, XP, attaque ou sauvegarde.

**Pourquoi ne pas enregistrer les quatre en bloc ?** GoblinKnife est Weapon sans archétype et sans action (X.19) : aucun ARM ne peut être choisi de façon automatique. Item_RatMeat est Food ; l'ancien modèle d'interaction autorise Consume sur Food, tandis que le canonique exige une action explicitement résolue. Le comportement réel de consommation, les tags de catégorie, les données d'usage et de présentation doivent être vérifiés avant enregistrement. EmptyVial et RatTooth nécessitent eux aussi une projection parité exacte du type, pile, métadonnées, capacités et physique. Aucune hypothèse de gameplay ni action inventée. Les dix autres sources alchimie restent hors lot.

## Code et tests

- Le composant de mort ne recherche plus un ancien DataAsset par ID si l'ItemId est enregistré. Une source directe encore présente peut servir à l'ancien héritage du niveau ARM lors de la transition ; une entrée canonique physique fixe ne dépend plus de cette source. L'état de lumière provient de la résolution canonique, avec HasEmitter et bDefaultEnabled. Le placement utilise le chemin canonique déjà validé. Les sources non enregistrées restent sur leur chemin actuel jusqu'au cutover.
- FGridMonsterLootEntry exporte ses méthodes pour le préflight intermodule. Aucun changement de schéma.
- FGridEditorNativeStoneLootPlan contrôle les deux vrais monstres, les cinq vrais IDs et l'ordre, les niveaux, les quantités et chances MON17.7 du Goblin, XP 125/500, l'appartenance au registre, le mesh, l'icône, le nom, la masse et lumière de Stone. Projette uniquement LootTable[1] vers Stone/N0 sans pointeur ; 128 seeds vérifient les mêmes décisions, quantités et tirages sur l'ensemble du Goblin. Manifeste réfléchi complet des deux monstres (normalisé uniquement sur cette ligne) et des cinq définitions anciennes. Le rapport de préflight exporte les propriétés réelles des quatre sources bloquées, sans les modifier.
- Tests Grimrock.RPG.ARM.CLEAN06_4X60.NativeStoneLootPlan et PointerlessLootDeath (mort effective, acteur monde, identité, niveau, masse, exactly once). X.48 exige zéro référence dans les niveaux et 5/4 loots. Les quatre anciens audits TD07 reconnaissent exclusivement le nouveau Stone du package exact et de la ligne exacte ; aucun assouplissement global.

## Transaction native sûre

Commandlet GridArmClean064X60NativeStoneLoot et Scripts/ApplyARM_CLEAN06_4X60_NativeStoneLoot.ps1 : Preflight/Verify lecture seule ; Apply après SHA code exact et deux statuts Windows, sauvegarde privée SHA256 de DA_MON_GoblinThrower, rapport de parité complet avant/après et rechargement frais, suite Grimrock, contrôle index Git LFS, rollback avant commit si nécessaire. Seul DA_MON_GoblinThrower.uasset autorisé. Le préflight affiche les exports exacts et les conserve en preflight.json sous le snapshot privé ARM_CLEAN06_4X60. Aucun ancien commandlet de matérialisation ne doit être relancé.

Après le SHA binaire et les deux statuts, recette ciblée : Goblin peut encore lâcher Stone, Knife, Vial avec règles existantes ; Stone visible, ramassable, empilable et physique niveau N0, aucune duplication au Save/Continue. Le rapport preflight.json (ou journal PowerShell avec SOURCE) fournit l'évidence pour préparer les quatre autres projections. **Avant Apply : 5 loots legacy ; après X.60B : 4.** 06.4 partielle, 06.5 bloquée, SaveGame v27 préservé. Aucun UI.csv/ST_UI/WBP/Game/fr/en modifié.
