# ARM-CLEAN06.5H.21 — Conversion des tests Editor RatLoot / QuickItem sur le registre canonique

## Situation et frontière

Après la recette Shipping fonctionnelle du 21 novembre 2026 (notamment la flaque d'huile présente dans le bon exécutable) et la validation Windows du nettoyage H.20, la priorité est la disparition progressive des **consommateurs des 48 UGridItemDefinitionAsset historiques**, pas le rendu facultatif des carrés de ciblage jaunes/verts (issue #221).

Le rapport natif H.16 contrôlait **53 packages**, dont **48 définitions d'item anciennes**, **2 sources catalogues Editor**, **2 Readables protégés par SaveGame** et **1 redirecteur à fixer** ; 48/48 ItemIds sont déjà résolus par `DA_ItemSystem`, 7/7 sauvegardes v27 validées. Ces fichiers ne sont cependant **pas encore des candidats à suppression sans contrôle** : des tests Editor les chargent explicitement via `LoadObject<UGridItemDefinitionAsset>`, que les tests historiques considèrent comme obligatoire.

## Lot concret : retirer treize lectures de sources obsolètes

1. `GridEditorTD0737CurrentAssetNormalizationTests.cpp` : le test `Grimrock.TechnicalDebt.TD07_3_7.Normalization.RatLootDefinitions` prend désormais **Item_RatMeat** et **Item_RatTooth** dans `CanonicalStage/DA_ItemSystem` ; vérifie l'ID, les classes canoniques, les catégories Food/Component et les deux entrées exactes du butin natif du monstre. Il ne charge plus `DA_Item_RatMeat` ou `DA_Item_RatTooth` pour valider le gameplay.
2. `RPGRPG0310GlobalBalanceTests.cpp` : le contrôle `Grimrock.RPG.RPG03.10.Balance.ActionEconomy` et la fermeture des statuts `AppliedStatusClosure` utilisent **onze ItemId/ActionId canoniques Alchimiste** avec `UGridItemSubsystem::ResolveDetailedNonArmAction`. L'ancienne table de chemins `DA_Item_*` et ses `BuildQuickItemCombatActionDefinition` ne sont plus utilisés dans ces tests. Les assertions économiques (PA, coûts, portée, cooldown, quantité) et de statut restent actives.
3. Aucune altération des actions, assets, monstres, classes, UI, SaveGame, fichiers LFS, shaders ou règles d'interaction.

Ces modifications ne suppriment encore aucun package : l'audit natif exige toujours 48 sources et leur parité de référence. Elles réduisent les raisons pour lesquelles les anciennes définitions doivent rester dans le dépôt et préparent le retrait natif sécurisé.

## Validation et suite

- Vérifier que le nouveau commit passe `UE5.5.4/SilentTask` puis `UE5.5.4/LocalAuto` sur **son SHA exact**, y compris la compilation Win64 Shipping + Automation.
- Contrôler spécifiquement les trois tests ci-dessus ainsi que `Grimrock.Editor.ARM.CLEAN06_5A.NativeRetirementEvidence`.
- Continuer à convertir les chargements de sources historiques dans les autres tests/commandlets Editor. Les **deux catalogues historiques** restent sources de comparaison des outils d'authoring jusqu'à la conversion de ces outils ; les **deux Readables** doivent être conservés dans les sept sauvegardes ; le **redirecteur ThrowingKnife** nécessite une transaction native `Fix Up Redirectors` ciblée.
- Toute suppression d'un `.uasset` exige un audit avant/après sur l'exact SHA, un snapshot natif privé, l'analyse de références au groupe et une intervention Unreal Editor limitée à un manifeste fermé ; `git rm` massif interdit.

Le nombre des candidats est encore **53** à la sortie de H.21. Le test du ciblage Shipping demeure une anomalie de présentation non bloquante.
