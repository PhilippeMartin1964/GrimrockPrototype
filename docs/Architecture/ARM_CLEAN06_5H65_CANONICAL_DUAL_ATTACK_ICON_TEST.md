# ARM-CLEAN06.5H.65 — Icône contextuelle du coup double depuis ItemSystem

## Migration

`Grimrock.RPG.ARMEMENT.LOT02.19.2.1.ContextualDualAttackIcons` n'ouvre plus les trois anciens DataAssets Dagger, Main-gauche et Round Shield. Le monde MON12 reçoit un `FGridCanonicalLootTestScope` et les trois armes physiques N1 proviennent de `UGridItemSubsystem`.

Les deux instances équipées portent uniquement leur identité, leur niveau, le GUID déterministe et le slot. Le bouclier reste passif. Aucune définition n'est enregistrée dans l'inventaire ; le contrôle d'absence de pointeur historique remplace l'ancien `RegisterItemDefinition`.

L'icône alternative, nécessaire au test du `Image_ActionIcon` du WBP, provient de `FGridResolvedItem::ItemIcon` de la Main-gauche, et non de son ancien `DA_Weapon_MainGauche_01`.

## Contrats intacts

La première action demeure `FGridDualWieldPolicy::ActionId`, utilise un seul élément Image, ne duplique pas les raccourcis ni les ressources physiques et ne crée aucun label `x2`. Le test vérifie la substitution d'icône et sa restauration, puis le retrait du coup double dès qu'un bouclier est mis en OffHand. Les contrôles WBP existants restent intacts ; aucun nouveau texte visible, aucune clé ST_UI et aucun changement de `UI.csv`, `ST_UI.uasset`, WBP ou Game/fr/en.

Le test de deux attaques avec audio séquencé `Grimrock.RPG.ARMEMENT.LOT02.19.2.N1DualWieldTwoProfilesOneCost` reste volontairement indépendant : il utilise encore des copies de définitions historiques afin d'injecter des échantillons de son distincts sans modifier les assets de production. Son remplacement nécessite une extension transitoire des présentations du catalogue canonique, pas la suppression de ses quatre assertions audio.

Aucun runtime, DataAsset, `.umap`, `.sav`, Niagara ou HUD de production modifié. Les 53 packages historiques H.23 restent protégés.

H.66 requiert `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` tous deux SUCCESS sur H.65 (Editor, Shipping, Automation `Grimrock.RPG.ARMEMENT.LOT02.19.2.1.ContextualDualAttackIcons`). Les sept SaveGames privés doivent encore être réaudités avant toute suppression.
