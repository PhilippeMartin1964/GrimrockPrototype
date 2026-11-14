# ARM-CLEAN06.3K — Métadonnées d'item dans l'interface d'inventaire

**Précondition :** réussite de la compilation et de `Grimrock` pour ARM-CLEAN06.3J sur le SHA de publication.

## Migration

Le titre du slot (`GetDisplayNameText`), le nom, la description et le poids unitaire/total de `GetTooltipView`, ainsi que le nom d'un équipement comparé, lisent `UGridItemSubsystem::ResolveItem(ItemDefinitionId, ItemLevel)` pour les identités enregistrées dans `DA_ItemSystem`.

Le `DisplayName`, le `Description` et le `WeightKg` viennent du seul DataAsset canonique concret. Le niveau réel de l'instance est utilisé ; aucun niveau N1 n'est supposé. Un ARM connu mais invalide affiche son ItemId technique, une description vide et un poids zéro (sans reprendre silencieusement les snapshots `FGridItemInstance.DisplayName/Weight` ni l'ancien `DA_Weapon_*`). Les identités hors registre conservent temporairement leurs valeurs historiques. Les formatages de quantité et les textes de labels existants ne sont pas modifiés.

**Limite :** cette étape n'a pas migré les faits d'attaque ni les résistances ou les textes d'usage provenant des anciennes `UGridItemDefinitionAsset`. Aucun `WBP` ou `.uasset` n'est modifié.

## Contrôle de localisation UI

La procédure `docs/UI_WBP_LOCALIZATION_WORKFLOW.md` est relue : **UI.csv inchangé → ST_UI inchangée → WBP inchangé → Game/fr/en inchangé**. Les chaînes dynamiques affichées proviennent des `FText` DataAsset de l'objet et des formats existants. Aucune nouvelle clé, String Table matérialisable ou nouveau `TextBlock` n'est introduit. La validation visuelle demeure une étape PIE distincte du contrôle Automation.

## Tests

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalInventoryMetadata` compare les métadonnées du vrai registre pour ARM014, ARM031, ARM075 et ARM081, N1/N20, face à un ancien DataAsset volontairement contradictoire, à des snapshots obsolètes et à N0 invalide ; il vérifie le maintien des noms, descriptions et poids legacy pour une identité non enregistrée.

La compilation UE5.5.4 et la suite complète `Grimrock` doivent réussir sur le SHA exact de ce jalon avant toute étape supplémentaire.
