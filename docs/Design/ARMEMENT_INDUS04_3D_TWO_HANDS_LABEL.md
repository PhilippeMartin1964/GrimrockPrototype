# ARMEMENT-INDUS04.3D — Libelle de maniement a deux mains

**Statut : implementation C++ et source de localisation publiees ; validation locale UE5.5.4 et PIE attendue.**

## Incident

Le haut du tooltip (champ `Text_Equipment` de `WBP_ItemTooltip`) affiche `FGridItemTooltipView::CompatibleSlotsText`. Pour la Grande Hache ARM031, le code proposait uniquement `MainHand` (traduit « Main directrice »), emplacement technique de stockage, alors que cette arme monopolise **les deux mains**.

Le sous-titre `Deux mains — consequences globales` dans la section de comparaison est **un autre concept** et reste intact : il decrit les changements simulés pour le personnage.

## Correction minimale

Dans `GridInventorySlotWidget.cpp`, `BuildCompatibleEquipmentSlotsText` teste en premier `GetEffectiveHandUsage() == TwoHanded` et la compatibilite reelle avec `MainHand`, puis retourne la nouvelle cle `Inventory.Tooltip.Equipment.TwoHands` pour le seul **libelle superieur**.

Les autres items conservent la construction de la liste des slots reelement compatibles via `FGridEquipmentTooltipGlobalProjection::GatherAllowedSlots`. Aucun nouveau `EGridEquipmentSlot` n'est cree ; aucun changement d'equipement, de simulation globale, de debit de ressource ni de sauvegarde.

Nouvelle ligne `Localization/Source/UI.csv` :

```csv
"Inventory.Tooltip.Equipment.TwoHands","Deux mains","Mode de maniement d'une arme à deux mains dans le tooltip d'inventaire"
```

Le C++ lit `FUILocalizationText::Get` et alimente le `Text_Equipment` deja existant. **Aucun WBP binaire ni Graph modifie.**

## Controle local

1. Fermer UE, `git pull --ff-only origin master`.
2. Executer `Scripts/Localization/RunLocalizationSmoke.ps1 -EngineRoot D:\UE_5.5`, materialisant dans le vrai `Content/Localization/StringTables/ST_UI.uasset` la nouvelle cle. Ne pas restaurer le `ST_UI.uasset` deja modifie localement.
3. Dans UE, ouvrir `ST_UI` et confirmer `Inventory.Tooltip.Equipment.TwoHands` et sa valeur `Deux mains` avant la recette PIE.
4. Relancer `Scripts/ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.EquipmentTooltip043D"` : **1/1** attendu. Ce test lit les vrais assets Grande Hache N1 et Pic de guerre N1, sans mutation.
5. Relancer les regressions `Grimrock.UI.EquipmentTooltip043B` (4/4), `Grimrock.UI.WeaponTooltip04` (9/9), `Grimrock.UI.Item01` (5/5).
6. En PIE, survoler Grande Hache : titre superieur **Deux mains**, comparaison globale inchangee. Survoler Pic de guerre : **Main directrice, Main secondaire**, comparaisons existantes conservees.
7. `git status --short`, conserver les edits locaux UMG et `ST_UI.uasset` ; committer les binaires dans des commits locaux distincts apres verification.

La presence de la cle dans le CSV n'est **pas** une preuve de materialisation ST_UI. Le smoke de localisation ne prouve pas une traduction anglaise complete : controler `Game.po` en et `Game.locres` en avant de declarer le texte traduit en anglais.

**Hors champ :** narratif Excel, hauteur du tooltip, refonte RPG de la protection physique, suppression d'anciens widgets.
