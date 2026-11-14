# ARM-CLEAN06.3AG — Attribution canonique des métadonnées d'instance

**Dépendance :** 06.3AF, commit `190bfdd`, build UE5.5.4 et
suite complète `Grimrock` validés sur Windows.

## Pourquoi

`UGridPartyInventoryComponent::ApplyItemDefinitionToInstance` alimentait
encore `FGridItemInstance.Weight` avec l'ancien
`UGridItemDefinitionAsset::TryGetWeaponCatalogueProfile`, alors que les
lecteurs du poids porté, des plaques de pression et des tooltips
consultaient déjà `DA_ItemSystem` pour les identités enregistrées.

`FGridItemInstanceMaterializationReadModel` est un petit adaptateur
transitoire, sans changement de schéma. Il remplit uniquement les champs
de cache `Weight` et, si vide, `DisplayName` avec les valeurs
canonisées au niveau effectif. La décision de catégorie ARM/non-ARM
vient du registre, non d'un `bUseWeaponCatalogue` périmé.

- ARM connu N1–N20 : niveau **préservé**, masse et nom initial lus
  exclusivement dans `UGridItemSubsystem::ResolveItem`.
- Nouveau ARM à niveau authoré 0 : hérite **une seule fois** de
  `DefaultWeaponItemLevel` comme avant, sans forcer N1.
- Flèches connues : niveau réel **0**, aucune promotion accidentelle.
- ARM connu mais invalide : refus sans modification d'instance et
  **aucun** recours à un ancien profil d'arme.
- Objet hors registre : chemin temporaire historique inchangé.
- Nom d'instance préexistant : **conservé** en attente de la conversion
  SaveGame (il peut contenir une personnalisation). Les lecteurs
  canoniques ne doivent pas lui redonner autorité.
- GUID, propriétaire, slot, quantité, réserves d'armure consommées,
  lumière et champs de lecture : jamais changés par cet adaptateur.

Les deux points d'entrée
`CanAddItemToCharacterInventory` et
`AddItemToCharacterInventory` ne permettent plus à une identité
canonique de contourner un échec de résolution sous prétexte que
le vieux flag `bUseWeaponCatalogue` est faux.

La normalisation des piles (quantités et taille maximale) reste
exclusivement à sa place dans `ApplyItemDefinitionToInstance` après
la résolution des métadonnées. Aucun effet sur la consommation
des armes, la physique, le combat ou la version de sauvegarde.

## Tests UE5.5.4

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalInstanceMaterialization` charge
le vrai `DA_ItemSystem`, contrôle ARM014/031/075/081 à N1 et N20
et héritage N6, flèches N0, anciens noms/masses volontairement erronés,
rejet sans mutation, GUID, ownership, lumière, dettes de protections,
masse égale au lecteur d'inventaire, et identité Legacy non enregistrée.

Conserver les suites existantes
`Grimrock.RPG.ARMEMENT01.5.*`, `Grimrock.Item.LIGHT02.*`,
ainsi que les tests de Save/Continue et l'intégralité de `Grimrock`.

Un commit par jalon ; vérifier `UE5.5.4/SilentTask` et
`UE5.5.4/LocalAuto` **sur le SHA exact** après publication.

## Localisation et contenu

Aucun `.uasset`, `.umap`, WBP, ressource de langue ou SaveGame
n'est modifié.

**UI.csv → ST_UI → WBP → Game/fr/en : inchangé.**
