# ARM-CLEAN06.4X.7 — initialisation de l'acteur d'item canonique

**Statut du jalon :** nouvelle API opt-in + test runtime sur vrais acteurs.
Aucune migration des placements existants, des SaveGames, des niveaux,
des palettes ou des Blueprints dans cette étape.

**Base :** ARM-CLEAN06.4X.6, commit validé
`68ef913cad264c8858ff46e76a5950b221246363`,
`UE5.5.4/SilentTask=SUCCESS` et `UE5.5.4/LocalAuto=SUCCESS`.

## Contrat

`AGridItemActor::InitializeFromCanonicalItem(Instance, Registry)` :

- exige une `FGridItemInstance` valide : GUID, `ItemId`, niveau
  physique N1–N20 pour ARM et N0 pour non-ARM, quantité positive ;
- utilise exclusivement `UGridItemSubsystem::ResolveItem` pour
  l'identité, les tags, le mesh, le transform, la lumière, Niagara,
  la politique de scintillement et de masse, et l'inclinaison ;
- échoue **avant toute mutation** si l'identité ou le niveau est
  inconnu, ou si la projection canonique est invalide. Aucun ARM
  N0 ne devient arbitrairement N1 ;
- ne garde aucun `UGridItemDefinitionAsset` historique. La projection
  `FGridResolvedItem` est **transiente** et non sauvegardée, et sert
  aux composants de présentation, à la masse Chaos et à l'inclinaison
  qui consultaient jusque-là le vieux pointeur ;
- laisse l'état effectif des lumières au propriétaire de l'instance.
  La création de l'acteur désactive initialement la lumière ;
  le système appelant doit réappliquer
  `FGridItemInstance::bLightsEnabled` ;
- remet le scintillement de l'objet à l'état initial désactivé
  pour éviter de transporter l'affichage d'un autre objet.

Les anciens chemins `InitializeFromItemDefinition` et
`InitializeFromItemDefinitionId` restent disponibles mais réinitialisent
explicitement la projection transiente si un acteur change d'origine.
Cette compatibilité est **temporaire** ; elle n'est pas un nouveau
fallback vers le catalogue legacy pour une identité canonique connue.

Les méthodes de scintillement, de masse et de nudge consultent
désormais la projection si elle existe ; sinon, le comportement
historique déjà testé est conservé pour les instances non encore
basculées. Cela ne modifie ni le schéma SaveGame ni les
`UGridLevelAsset` sérialisés.

## Test UE

`Grimrock.RPG.ARM.CLEAN06_4X7.CanonicalItemActorInitialization`
crée un monde transitoire avec les **34 définitions réellement
enregistrées**, puis initialise un acteur pour chaque famille :
**22 ARM N1 et 12 non-ARM N0**, dont les munitions.

Il contrôle l'identité, le GUID, le niveau, les tags, le mesh,
le transform, les configurations lumière/Niagara, l'état initial
de la lumière, et l'absence de pointeur historique. Il vérifie
le rejet sans mutation d'un ARM N0 ou d'un ItemId inconnu.
Aucun vrai niveau, DataAsset, SaveGame ou Blueprint n'est enregistré.

La compilation UE5.5.4 et la suite `Grimrock` complète doivent
réussir sur le commit de ce jalon. La validation `-NullRHI`
ne démontre pas le rendu Niagara, les matériaux de scintillement
ou la simulation Chaos en PIE.

## Étape suivante

Basculer **progressivement** les constructions de pickups monde,
placements et réceptacles depuis leurs `CanonicalItemId` et
niveaux effectifs déjà enregistrés en X.5, conserver l'identité
runtime et les liens d'énigmes, rétablir exactement la lumière
et les textes/overrides par instance, puis vérifier en PIE
avant le retrait des pointeurs historiques et la migration
séparée des SaveGames sur copies.
