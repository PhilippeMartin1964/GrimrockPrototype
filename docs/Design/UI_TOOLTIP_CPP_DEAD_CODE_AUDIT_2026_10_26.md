# UI-TOOLTIP — Audit réel du C++ après suppression des deux WBP historiques

**Date : 2026-10-26. Périmètre :** inventaire, info-bulle et tests Editor du dépôt `PhilippeMartin1964/GrimrockPrototype`, branche `master` examinée au commit `3e1191913d357b805a9c3ccbf2978272049c8c14`.

**Source de la décision de suppression :** l'utilisateur a confirmé avoir supprimé localement, via Unreal, `WBP_ItemTooltipComparisonRow` et `WBP_ItemTooltipStatLine` et déclaré leurs références nulles dans l'Editor. **Attention :** au moment de l'audit, `master` distant pointe toujours sur le commit précédent. Le contenu binaire supprimé localement n'est pas observable par GitHub tant que son commit n'a pas été poussé ; ce rapport ne prétend pas inspecter le contenu des WBP locaux.

**Objectif :** lister les suppressions C++ exactes avec preuve de consommation, et empêcher une « chasse au Legacy » qui détruirait les infobulles des objets sans catalogue ou la projection armure DOS2. **Aucune source C++ n'a été effacée dans ce commit d'audit.**

## A. Code mort ou redondant démontré

| Statut | Fichier | Élément / ligne dans le HEAD audité | Justification | Mesure |
| --- | --- | --- | --- | --- |
| **SUPPRIMER APRES VERIFICATION DU WBP RACINE** | `Public/UI/GridItemTooltipWidget.h` | `VerticalBox_WeaponComparisonSection` (L88-89) ; `VerticalBox_WeaponComparisons` (L90-91) | Ils servent exclusivement à masquer/vider l'ancienne comparaison d'armes détachée. Le chemin actuel remplit `VerticalBox_Comparisons` et instancie `UGridItemTooltipEquipmentComparisonWidget` ; il ne réutilise jamais ces deux anciens conteneurs. | Retirer les deux `UPROPERTY(BindWidgetOptional)` une fois les éventuelles références sérialisées du WBP racine supprimées dans l'Editor. |
| **SUPPRIMER AVEC LES PROPRIETES** | `Private/UI/GridItemTooltipWidget.cpp` | L225–231 : `SetSection(VerticalBox_WeaponComparisonSection, false);` + `VerticalBox_WeaponComparisons->ClearChildren()` | Seuls usages de ces deux pointeurs C++. Les vrais groupes actifs sont ajoutés aux lignes 239–247. | Supprimer les 6 lignes et le commentaire de migration devenu faux ; laisser `SetSection(VerticalBox_ComparisonSection, bShow)`. |
| **RETIRER APRES CONTROLE BP** | `Public/UI/GridInventoryUiTypes.h` | `FGridItemTooltipEquipmentComparison::bHasEquippedItem` (L196-197) | Valeur calculée dans `GridInventorySlotWidget.cpp` et `GridEquipmentTooltipGlobalProjection.cpp`, **jamais lue par un présentateur C++ actuel**. Sert encore aux assertions de tests historiques. | Supprimer le champ, les 2 affectations productives et migrer les assertions vers `EquippedItemDisplayText`. Avant de retirer le `UPROPERTY`, confirmer qu'aucun Blueprint natif ne lit cette propriété. |
| **RETIRER APRES CONTROLE BP** | `Public/UI/GridInventoryUiTypes.h` | `FGridItemTooltipEquipmentComparison::EquippedItemName` (L199-200) | Alimenté seulement par `GridInventorySlotWidget.cpp` et asserté dans `GridUIItem01Tests.cpp`. `UGridItemTooltipEquipmentComparisonWidget::PresentComparison` affiche `EquippedItemDisplayText`, pas `EquippedItemName`. | Remplacer l'affectation en deux temps par `Comparison.EquippedItemDisplayText = GetItemDisplayName(...)`; supprimer le champ et les assertions obsolètes après vérification BP. |
| **OPTIMISATION SECONDAIRE** | `Public/UI/GridInventoryUiTypes.h` | `FGridItemTooltipStatLine::DeltaText` (L141-142) | `GridInventorySlotWidget.cpp` s'en sert comme **étape intermédiaire** pour produire `DeltaDisplayText`. Le rendu natif et les comparaisons utilisent `DeltaDisplayText`. Aucun lecteur C++ productif de `Stat.DeltaText` trouvé. | Après vérification BP, convertir les constructions en `FText LocalDelta` puis ne garder que `DeltaDisplayText`. Ne pas toucher aux signes de deltas sans tests. |

Les deux premiers sont les **candidats de suppression les plus directs**. Les champs `USTRUCT(BlueprintReadOnly)` nécessitent une validation de compilation de `WBP_ItemTooltip` et `WBP_ItemTooltipNativeComparison` avant suppression, car la recherche GitHub ne révèle pas les accès Blueprint compilés.

## B. C++ de tests et d'audit devenu périmé

| Priorité | Fichier | Problème exact | Correctif |
| --- | --- | --- | --- |
| **HAUTE** | `Source/GrimrockPrototypeEditor/Private/Tests/GridEditorUIAssetOrg01MigrationAuditTests.cpp` | Les deux entrées `ItemTooltipComparisonRow` et `ItemTooltipStatLine` de `Moves[]` exigent chacune qu'un des packages source/cible existe. Une suppression légitime fait donc échouer le test. | **Retirer exactement ces deux entrées** (garder les autres contrôles de migration). |
| **HAUTE** | `Source/GrimrockPrototypeEditor/Private/Tests/GridEditorUIWBPAudit02InventoryTests.cpp` | `ExpectedCount = 46` (L17) et l'assertion d'inventaire (fin du test) datent d'avant la suppression des deux WBP. | Nouveau référentiel attendu **44** si aucun autre ajout/retrait n'a été effectué. Renommer le texte de l'assertion, puis valider le nombre réel dans l'AssetRegistry UE. |
| **MOYENNE** | `Source/GrimrockPrototypeEditor/Private/Tests/GridEditorUITooltipClean01AuditTests.cpp` | Le tableau d'audit contient toujours les deux WBP retirés : l'état `ALREADY_ABSENT` est correct mais le rapport « référencements à inspecter » est obsolète après nettoyage. | **Conserver un test de non-régression** imposant l'absence des deux packages, ou déplacer ces entrées vers un test de nettoyage permanent. Garder le rapport pour les trois WBP actifs. |
| **FAIBLE** | `Source/GrimrockPrototypeEditor/Private/Tests/GridEditorUIAssetClean01ReferenceAuditTests.cpp` | `ItemTooltipComparisonRow` reste un candidat historique d'audit ; l'absence est seulement journalisée. | Retirer l'entrée du tableau historique si l'audit doit porter exclusivement sur les assets restants ; sinon la garder comme archive, sans assertion de présence. |
| **CONSERVER** | `Source/GrimrockPrototypeEditor/Private/Tests/UILOCALIZATION033WbpOwnershipRegressionTests.cpp` | Les chemins d'anciens WBP figurent dans des tableaux `RequirePathsAbsent` vérifiant des chaînes **absentes** du manifeste de localisation. | **Ne pas supprimer automatiquement** : ce sont des gardes négatifs, non des références actives ni une preuve que les anciens WBP existent. Une simplification éventuelle est documentaire et distincte. |

## C. C++ à conserver impérativement

- `UGridItemTooltipWidget` + `PresentFromInventorySlot`, `PresentTooltipView`, `PopulateLegacyStats`, `PopulateWeaponFacts`, `PopulateEquipmentComparisons` : tous participent au rendu natif actuel. Le nom *Legacy* désigne ici les **items hors catalogue** et leurs lignes statistiques, pas l'ancien WBP supprimé.
- `UGridItemTooltipWeaponLineWidget::PresentLegacyStat` : toujours appelé pour les stats générales, icônes et comparaisons non cataloguées. Ne pas confondre avec `WBP_ItemTooltipStatLine`.
- `UGridItemTooltipEquipmentComparisonWidget::PresentComparison` : remplaçant actuel de l'ancien widget ; conserve les lignes et faits d'armes par slot.
- `UGridItemTooltipWidget::ShouldOmitLegacyArmor` et `UGridItemTooltipEquipmentComparisonWidget::ShouldOmitLegacyArmor` : utilisées pour empêcher les doublons entre les bonus génériques et la véritable protection du bouclier ; tests de non-régression déjà existants.
- `FGridWeaponTooltipProjection` et `FGridEquipmentTooltipGlobalProjection` : deux projections différentes, toutes deux consommées par `GetTooltipView`. Le maximum du Bouclier rond N1 reste **5** ; la réserve est **2/5** après 3 points absorbés, puis **5/5** après victoire/repos. La comparaison globale utilise l'armure **restante effective**, pas 5 plus une seconde réserve.
- `UGridInventorySlotWidget::GetTooltipText()` : ce n'est **pas** une duplication morte de l'info-bulle visuelle. L'action de contexte **Examine** appelle toujours ce texte dans `GridInventoryWidget.cpp` (L1602–1607) pour `PresentItemExamination`. Le retirer casserait cet écran.
- `FGridItemTooltipStatLine`, `FGridItemTooltipEquipmentComparison`, `WeaponLineWidgetClass`, `ComparisonRowWidgetClass`, `VerticalBox_StatLines`, `VerticalBox_WeaponFacts`, `VerticalBox_Comparisons`, `VerticalBox_ComparisonSection` : structures et composants toujours exploités.
- `StatIconTextures`, `SizeBox_StatIcon`, `Image_StatIcon`, `GetFallbackIconStatId` : indispensables pour les icônes 48×48 et les nouvelles lignes `WeaponPhysicalArmorRemaining`. Ne pas supprimer.

## D. Ordre de travail recommandé (tickets atomiques)

1. **UI-TOOLTIP-CPP-CLEAN02.1 — Nettoyage des conteneurs morts** : retirer les deux champs `VerticalBox_WeaponComparisons`/`WeaponComparisonSection` et leurs seules opérations associées, si le Designer ne les utilise plus. Aucun changement fonctionnel, ni de bouclier, ni de UMG binaire dans le commit C++.
2. **UI-TOOLTIP-CPP-CLEAN02.2 — Tests d'inventaire alignés** : retirer les deux entrées du tableau de migration et ajuster la baseline UI-WBP-AUDIT02 après constat `Widgets=44`. Adapter le test d'audit Clean01 pour confirmer leur absence ; conserver les tests négatifs de localisation.
3. **UI-TOOLTIP-CPP-CLEAN02.3 — Read-model réduit** : retirer `bHasEquippedItem`, `EquippedItemName` uniquement après confirmation qu'ils ne sont plus lus en Blueprint. Optionnel : supprimer `DeltaText` au profit du texte de delta formaté, dans une intervention séparée.

### Validation obligatoire des suppressions

Filtre initial conseillé pour les tickets : `Grimrock.UI.WeaponTooltip04`, `Grimrock.UI.EquipmentTooltip043B`, `Grimrock.UI.TooltipClean01`, `Grimrock.UI.Item01`, `Grimrock.UI.Item01.Clean01`, `Grimrock.Editor.UIWBPAudit02`, `Grimrock.Editor.UIAssetOrg01`, `Grimrock.Editor.UITooltipClean01`, et smoke PIE (survol + comparaison Dague/Grande Hache/Bouclier, Examine, icônes, descriptions, 2/5→5/5).

**Bilan :** 2 propriétés mortes de présentation + leur code de nettoyage, 2 champs USTRUCT redondants sous réserve d'accès BP, 1 texte intermédiaire optionnel, 2 tests historiquement faux après les suppressions, 2 audits historiques à actualiser. **Aucune classe native entière à supprimer.**

**Risque restant :** l'Editor local connaît les suppressions binaires, mais la version `master` inspectée ne les contient pas nécessairement. Ne jamais exécuter de nettoyage de `.uasset` par GitHub ni modifier les quatre fichiers locaux UI sans intervention de l'utilisateur.


## UI-TOOLTIP-CPP-CLEAN02 — Mise en œuvre autorisée après publication des suppressions WBP

Les trois étapes sont publiées en commits atomiques sur `master` :

1. **CLEAN02.1** : les deux champs `VerticalBox_WeaponComparisonSection` et `VerticalBox_WeaponComparisons` et leurs seules opérations de vidage/masquage ont été supprimés de `GridItemTooltipWidget`. Un test de réflexion natif contrôle leur absence et la présence du conteneur actif.
2. **CLEAN02.2** : l'audit de migration ne réclame plus les deux WBP supprimés ; `UI-WBP-AUDIT02` a un référentiel de 44 widgets ; l'audit tooltip vérifie maintenant l'absence des anciens chemins source **et** Inventory, et la présence des trois WBP actifs dont la racine réside dans `UI/Inventory`.
3. **CLEAN02.3** : le read-model retire `bHasEquippedItem`, `EquippedItemName` et `FGridItemTooltipStatLine::DeltaText`. Les productions C++ et les tests sont alignés ; le chemin **Examine** réutilise `DeltaDisplayText`, déjà utilisé par les widgets natifs. `FGridWeaponTooltipComparisonFact::DeltaText` reste intact, car les faits d'armes possèdent leur propre contrat. Un test éditeur compile les trois Blueprints actifs.

**Attention :** UE5.5.4 Windows / PIE doivent encore confirmer le build et la compilation réelle des Blueprints après retrait des `UPROPERTY`. Ces propriétés étaient exposées aux Blueprints, même si aucun lecteur C++ de production ne les consommait. Pas de modification d'assets binaires au travers de GitHub, ni de changement des mécanismes de protection/recharge du Bouclier rond.


## Correctif CLEAN02.2.1 — Inventaire AssetRegistry (26 octobre 2026)

**Logs TD04-20261026-143012 :** le filtre `Grimrock.Editor.UIWBPAudit02.Inventory` a trouvé **48 assets WBP**, tandis que la baseline **44**, calculée par soustraction arithmétique depuis une ancienne liste de 46, était incorrecte. L'échec relevé (48 ≠ 44) était **uniquement l'assertion de cardinalité** ; les autres tests du ticket sont passés (24/25).

Le test UI-WBP-AUDIT02 ne bloque donc plus sur un total global fixe. Il **consigne 48 comme dernière observation**, à titre informatif, et impose désormais :
- l'existence des **trois WBP actifs** `Inventory/WBP_ItemTooltip`, `Inventory/WBP_ItemTooltipNativeLine` et `Inventory/WBP_ItemTooltipNativeComparison` ;
- l'**absence** de `WBP_ItemTooltipComparisonRow` et `WBP_ItemTooltipStatLine`, tant dans `UI/Inventory` que dans les anciens chemins `UI` ;
- l'absence de redirectors WBP, la génération des deux rapports d'audit et l'inventaire Source/Config fonctionnel.

La variation du nombre total de widgets n'est **pas une preuve de régression** : de nouveaux WBP peuvent être ajoutés au projet. Les règles spécifiques priment sur une valeur historique. Cela corrige l'assertion sans restaurer ni modifier un ancien widget et sans toucher au système de protection du bouclier.

**Validation locale attendue :** `Grimrock.Editor.UIWBPAudit02` après récupération du commit correctif. Les 24 autres tests ont déjà été validés par l'utilisateur et ne sont pas modifiés.
