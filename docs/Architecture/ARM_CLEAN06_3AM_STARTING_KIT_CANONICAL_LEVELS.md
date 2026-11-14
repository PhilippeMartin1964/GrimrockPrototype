# ARM-CLEAN06.3AM — Niveau fixe canonique lors de la création des kits

**Dépendance :** ARM-CLEAN06.3AL (`f715646`), statut Windows des deux validations vérifié avant l'étape.

## Anomalie restante

Le frontend utilisait déjà `FGridStartingKitDisplayReadModel` et montrait le
niveau d'arme canonique, même lorsque le `DA_Weapon_*` possédait un
`bUseWeaponCatalogue` obsolète. Mais
`FRPGStartingEquipmentService::TryBuildKit` matérialisait le vrai objet
d'après ce booléen et `TryGetWeaponCatalogueProfile` de la définition.
Une épée enregistrée avec ce drapeau à faux pouvait devenir un faux N0
ou être rejetée, tandis que des flèches avec le drapeau à vrai étaient
promues à un faux N1. Le nom et le poids statiques étaient déjà projetés
depuis le registre (06.3Y), mais le **choix du niveau** ne l'était pas.

## Correction

Pour chaque `ItemId` enregistré dans `DA_ItemSystem`,
`TryBuildKit` délègue désormais le niveau de **nouvelle création authorée**
à `FGridWorldItemLevelReadModel::ResolveAuthoredPlacement` :
- ARM réellement enregistré : N1–N20 conservé ; N0 auteur hérite
  exactement une fois de `DefaultWeaponItemLevel` si valide.
- Non-ARM canonique : le N0 est réel ; aucun niveau de munition inventé.
- Niveau canonique invalide : refus **avant tout commit** des objets du
  kit, sans retour au catalogue historique.
- Identité hors registre : parcours antérieur inchangé, notamment contrôle
  historique des niveaux pour les anciennes définitions.

`FGridStartingKitStaticMetadataReadModel` reste le lecteur statique
des noms/poids et restitue le résultat du registre pour tout item connu.
Les garanties transactionnelles, l'identité GUID, l'ordre de slots,
les quantités, les mains et la sélection des kits sont inchangés.
Aucun `.uasset`, `.umap`, SaveGame ni WBP n'est modifié ici.

## Tests

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalStartingKitMaterializedLevels`
charge la vraie racine `DA_ItemSystem`, crée un kit transitoire comprenant
ARM014 authoré N0→N6, ARM014 N20 et flèches N0 avec les **drapeaux
historiques volontairement inversés**. Compare chaque instance à
`ResolveItem` (niveau, identité, nom et poids) ; vérifie également
le rejet atomique d'un ARM N0 invalide et de flèches N1.
Les tests `Grimrock.RPG.ARMEMENT01.9.StartingKits.*`,
`Grimrock.RPG.ARMEMENT01.9.SixClassKits.*` et la suite complète
`Grimrock` restent les garde-fous des kits de production.

**Validation requise sur le SHA publié :** `UE5.5.4/SilentTask` et
`UE5.5.4/LocalAuto` success. La qualité visuelle en PIE reste distincte.

**Localisation :** aucun texte joueur / nouvelle interface ;
`UI.csv → ST_UI → WBP → Game/fr/en` inchangé.
