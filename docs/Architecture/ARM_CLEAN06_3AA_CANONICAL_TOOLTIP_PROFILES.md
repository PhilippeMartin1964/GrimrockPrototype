# ARM-CLEAN06.3AA — Profil d'arme des infobulles et comparaison

## Portée

Le lecteur `FGridWeaponTooltipProjection` ne consulte plus les profils N1–N20 des anciens `DA_Weapon_*` **pour une identité enregistrée dans `DA_ItemSystem`**. Les deux parcours, détail d'une arme et comparaison avec l'équipement porté, utilisent le niveau réel de `FGridItemInstance` et les données numériques de `UGridItemSubsystem::ResolveItem`.

Une projection temporaire de même forme que `FGridWeaponLevelProfile` permet de conserver **exactement** les règles de présentation existantes (NdM+K, min/max, précision, PA, portée, type de dégâts, attribut, réserve physique/magique, comparaisons/deltas) sans créer d'autres DataAssets ni nouvelle autorité persistante. Le `ArchetypeId` et les champs de combat/protection viennent du seul `FGridResolvedItem.Weapon`. Les armes passives restent sans dégâts fictifs. Les niveaux invalides d'une identité connue ne réactivent jamais les copies anciennes. Pour les items non enregistrés, le contrat ancien est temporairement conservé jusqu'à la conversion binaire 06.4.

`UGridInventorySlotWidget::GetTooltipView` transmet le registre déjà résolu au lecteur de profil, pour le candidat et l'item équipé. Le Designer UMG n'est pas modifié ; les formules de couleur et les comparateurs ne changent pas.

## Checklist localisation WBP

`docs/UI_WBP_LOCALIZATION_WORKFLOW.md` relu avant et après l'étape. Les textes des faits, facteurs et comparaisons réutilisent exclusivement les clés `Inventory.Tooltip.Weapon.*`, `Inventory.Tooltip.Global.*` et les formats existants. **UI.csv inchangé → ST_UI inchangée → WBP inchangé → Game/fr/en inchangé** : aucune clé, nouvelle étiquette, format `FText` ou `SetText` n'est ajouté. Le contenu métier numérique est nouvellement résolu depuis le seul DataAsset canonique. Aucune matérialisation `ST_UI` n'est nécessaire sans nouvelle clé.

## Validation

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalWeaponTooltipProfiles` utilise le vrai `DA_ItemSystem` ; ARM014, ARM054, ARM075, ARM081 aux niveaux N1 et N20 sont testées avec une ancienne définition sans catalogue, plus profils passifs sans dégâts, comparaison sans legacy, ItemLevel=0 refusé, identité inconnue non inventée. Le build UE5.5.4 et la suite complète `Grimrock` restent les critères Windows du SHA exact. Les textes et deltas visibles seront à confronter en PIE ; `-NullRHI` ne démontre pas le rendu UMG.
