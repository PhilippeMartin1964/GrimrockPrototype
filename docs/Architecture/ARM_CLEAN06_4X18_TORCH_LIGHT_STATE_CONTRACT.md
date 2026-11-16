# ARM-CLEAN06.4X.18 — Torche : état lumineux indépendant du conteneur

## Incident

PIE : la torche est manipulable dans support, sac et mains
après X.17, mais s'éteint dès qu'elle est déposée au sol,
puis reste éteinte après ramassage et rééquipement.

La cause est précise : `TryDropItemInstanceAtCell`
initialise un ItemId canonique non-ARM avec
`InitializeFromItemDefinitionId` (configuration
lumineuse et Niagara vides), puis n'applique que
`ApplyCanonicalWorldVisual` (mesh). Le `true`
de `bLightsEnabled` ne peut ensuite pas créer
un PointLight sans `FGridLightEmitterConfig`.
Au ramassage `AreItemLightsEnabled` retourne faux.

## Contrat de manipulation

**Une transaction n'allume ni n'éteint une torche.**
L'autorité est `FGridItemInstance::bLightsEnabled` :
- support et sol : flamme + PointLight local si allumée ;
- sac et curseur : état mémorisé mais aucune lumière
  diffusée dans le monde ;
- mains : état mémorisé, lumière du groupe via
  `UGridPartyIlluminationComponent` si allumée ;
- Save/Continue : même état sérialisé ;
- une torche explicitement éteinte ne se rallume pas
  simplement parce qu'elle change de support ;
- pas de carburant, usure, pluie, auto-extinction ni
  action Allumer/Éteindre nouvellement introduits
  dans ce correctif ; ces mécaniques nécessitent
  un design et une étape dédiés.

## Correctif

L'initialisation d'une goutte non-ARM canonique
appelle `InitializeFromCanonicalItem` avec identité,
niveau et GUID de l'instance, avant de restaurer son
état de lumière. Ceci fournit le **mesh + LightEmitter
+ VisualEffects + physique** depuis le seul registre.
Une erreur d'initialisation échoue avant toute
publication d'acteur de ramassage. Les ARM et
identités non enregistrées conservent leur chemin.

## Validation

`Grimrock.RPG.ARM.CLEAN06_4X18.TorchFloorDropAndPickupLightState`
teste la vraie `DA_Item_Torch`, l'état lumineux du
PointLight sur le sol, la capture d'état de niveau,
le ramassage avec GUID, le sac, le MainHand et
l'activation du proxy de lumière du groupe.
Deuxième torche explicitement éteinte : sol et
ramassage doivent la laisser éteinte.

La validation automatique `-NullRHI` n'évalue pas
le rendu final de la flamme/ombres. Une recette PIE
reste requise : support -> sac -> main -> sol ->
sac -> main -> support, avec sauvegarde/chargement
sur une copie uniquement. Aucun vrai `.uasset`,
`.umap` ou `.sav` modifié.
