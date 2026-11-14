# ARM-CLEAN06.3AE — Prévisualisations et comparaisons d'équipement

**Point de départ :** ARM-CLEAN06.3AD, commit `46452a0`, build et suite `Grimrock` verts.

## Cause et solution

Trois endroits de prévisualisation conservaient des lectures incohérentes : le détail des statistiques dans `GridInventorySlotWidget.cpp` employait directement `EquipmentStatBonus.ArmorBonus` de l'ancien `DA_Weapon_*` ; la prévisualisation globale testait la réserve d'armure avant d'ajouter les résistances ; le nom des objets déplacés était lu dans l'ancienne définition. Les comparaisons d'armes du scénario global ne transmettaient pas non plus le registre à `FGridWeaponTooltipProjection::BuildComparison`.

`FGridEquipmentPreviewItemReadModel` est un modèle **transitoire en lecture seule** alimenté par les lecteurs déjà partagés depuis 06.3AD :
- Protection nominale physique et magique : le `UGridItemSubsystem::ResolveItem` **du niveau réel N1–N20**, sans ancienne copie pour une identité enregistrée.
- Protection physique restante : déplétion réelle des compteurs de l'instance, sans refill implicite.
- Bonus de caractéristiques et résistances : les champs historiques provisoires, **car ils n'ont pas encore d'autorité dans le modèle canonique**.
- Nom affiché de l'objet déplacé : le `DisplayName` canonique de l'identité et du niveau effectifs ; fallback provisoire uniquement pour identités non enregistrées.

Le calcul hypothétique cumule chaque contribution comme les totaux d'équipement du personnage ; l'absence éventuelle d'une réserve d'armure ne fait plus disparaître une résistance réelle dans l'aperçu. Les mêmes lectures alimentent la section des statistiques d'un item et les comparaisons simples. `FGridEquipmentTooltipGlobalProjection::BuildScenario` transmet aussi le registre existant à la comparaison `N1–N20` des armes.

**Invariants :** aucune mutation de `FGridItemInstance`, aucune nouvelle réserve, aucun changement de chance ou de formule de dégâts, aucune écriture de l'inventaire ou de SaveGame, aucun nouveau DataAsset et aucun WBP modifié.

## Tests

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalEquipmentPreviewReadModel` charge la **vraie** racine `DA_ItemSystem` et vérifie ARM075, ARM081, ARM014 et ARM013 à N1 et N20, anciennes armures/noms volontairement contradictoires, déplétion, Force/résistances conservées, niveau canonique N0 invalide, object legacy non enregistré et invariance d'identité. Les tests existants `Grimrock.UI.EquipmentPreview043A.*` et `Grimrock.RPG.ARM.CLEAN06_3.CanonicalWeaponTooltipProfiles` restent dans la suite complète.

## Localisation / interface

Relire `docs/UI_WBP_LOCALIZATION_WORKFLOW.md` avant et après toute modification de texte WBP. Ici **aucune clé, aucun libellé, aucune formule de texte ni aucune présentation du Designer ne change** : seules les valeurs et noms métier viennent de la bonne autorité.
**UI.csv → ST_UI → WBP → Game/fr/en : inchangé.** Aucune matérialisation d'asset String Table n'est requise.

## Validation

Un seul commit atomique sur `master`, puis les statuts `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` **sur le SHA exact**. Le succès `-NullRHI` valide l'automatisation C++ mais pas la disposition visuelle PIE des infobulles.
