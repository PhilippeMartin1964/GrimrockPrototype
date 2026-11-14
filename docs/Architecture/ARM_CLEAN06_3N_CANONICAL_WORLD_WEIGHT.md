# ARM-CLEAN06.3N — Masse unifiée des pickups et réceptacles

**Prérequis** : ARM-CLEAN06.3M validé automatiquement sur son SHA exact.

## Contrat

`AGridLevelRuntimeActor::GetWorldItemWeightAtCell`, utilisé pour les plaques de pression, et `AGridReceptacleActor::GetContainedTotalWeight` utilisent désormais le **même** `FGridItemWeightReadModel` que le poids porté depuis 06.3I. Chaque appel construit une vue `FGridItemInstance` transitoire comportant le GUID, l'`ItemId`, l'`ItemLevel` fixé et la quantité du vrai objet ; la fonction ne modifie aucun état persisté.

Pour un ItemId présent dans `DA_ItemSystem`, `UGridItemSubsystem::ResolveItem` est seul propriétaire de `WeightKg`. Aucune copie `Entry.ItemDefinitionAsset->Weight` ni `FGridContainedReceptacleItem.Weight` ne peut influer sur une plaque ou un réceptacle canonique ; un niveau invalide ne retombe pas sur ces copies. Un pickup enregistré peut être pesé même si son ancienne définition n'est plus disponible.

Pour les identités **non migrées**, l'ancien chemin par définition et le snapshot de poids du réceptacle demeurent temporairement. Les quantités sont multipliées comme auparavant. Le filtrage des arêtes, la cellule effectivement occupée par Chaos et la logique de pression ne sont pas modifiés.

Aucun `.uasset`, `.umap`, SaveGame, seuil de pression, effet de lumière, blueprint ou texte UI n'est modifié.

## Tests

`Grimrock.RPG.ARM.CLEAN06_3.WorldAndReceptacleWeightParity` lit le vrai registre ARM014/ARM031/ARM075/ARM081 N1/N20, compare des snapshots historiques volontairement contradictoires, teste l'empilement x3 et un N0 invalide sans fallback. La suite complète `Grimrock` reste l'autorité de non-régression pour la logique des plaques et réceptacles.

**Condition de sortie :** `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` verts sur le SHA unique de cette étape. Le comportement physique réel des objets au sol doit être couvert ultérieurement par PIE GPU.
