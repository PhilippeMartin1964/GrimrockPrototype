# ARM-CLEAN06.3AT — Reconstitution de sauvegarde, identité canonique

**Parent :** ARM-CLEAN06.3AS, commit `2573e2f`.

## Diagnostic

Le chargement `AGrimrockPartyPawn::LoadCurrentGameData` restaure
`FGridPartyInventoryState` puis appelle
`RehydrateLoadedItemDefinitions`, lequel délègue à
`UGridPartyInventoryComponent::RehydrateOwnedItemDefinitions`.
Le New Game chargé depuis le frontend utilise **le même point d'entrée**
via `UGrimrockStartupModeComponent::RestorePendingNewGameParty`.

Jusqu'à 06.3AS, chaque ItemId sauvegardé exigeait un
`UGridItemDefinitionAsset` renvoyé par `ResolveRuntimeItemDefinition`,
y compris les ARM et munitions déjà enregistrés dans `DA_ItemSystem`.

## Adaptation contrôlée

- Les **instances physiques** (inventaire actif/réserve, équipement,
  curseur) ayant un ItemId connu doivent résoudre exactement leur
  `ItemLevel` et respecter `MaxStack` du registre canonique.
- Un ARM enregistré N0 ou N21, une source dont le GUID est invalide, ou
  une pile canonique au-delà de la capacité sont **refusés avant toute
  modification** des définitions temporaires et sans inventer un N1.
- Les raccourcis d'équipement enregistrés sont collectés par ItemId ;
  le contrôle de leur action physique et du GUID reste à leur lecteur
  d'exécution ; aucun niveau fictif ne leur est attribué.
- La définition historique est **optionnelle** pour chaque ItemId
  canonique : si le résolveur l'a encore, elle reste enregistrée
  uniquement pour les consommateurs transitoires ; sinon le chargement
  ne dépend plus d'elle. Une définition historique dont l'ID diffère
  ne peut jamais faire autorité sur un ItemId connu.
- Les ItemId **inconnus** doivent conserver une définition historique
  de la même identité ou l'opération entière échoue.
- Les résolutions sont faites en préflight ; le registre temporaire
  n'est remplacé qu'une fois tous les contrôles réussis, avec une
  seule notification d'inventaire après le succès.
- Aucune instance `FGridItemInstance` n'est modifiée :
  `RuntimeObjectId`, propriétaire, emplacement, ItemLevel, poids/nom
  mis en cache, réserves d'armure, lumière et quantités sont préservés.

## Limitations connues et jalon 06.4

Le format SaveGame reste **27**, sans changement de schéma.
Les SaveGame antérieurs où un ARM a été persisté avec `ItemLevel=0`
ne sont **pas** convertis par ce lecteur : une migration Editor
**ponctuelle** avec sauvegarde des originaux, diagnostic par ItemId,
valeur de niveau authorée et validation des binaires est nécessaire
en 06.4 avant d'enlever les champs historiques.

La présence optionnelle d'un ancien `DA_Weapon_*` est conservée pendant
06.3 car certaines interfaces/contextes de lancer utilisent encore
ces définitions. Son absence ne bloque plus la validation de l'identité,
mais ne prouve pas encore que tous les écrans et interactions soient
parfaitement projetés sans définition. Le retrait définitif relève
de 06.5, pas de ce jalon.

## Tests

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalSavedItemRehydration` :
vrai `DA_ItemSystem`, snapshots ARM N20/N1, bouclier équipé, flèches
N0, réserve de personnages, curseur, anciennes métadonnées préservées,
définition absente/optionnelle, mélange canonique/legacy, échec
atomique si ID inconnu, ARM N0/N21 et pile canonique invalide,
raccourci lié au seul ItemId.

Conserver les tests historiques `Grimrock.TechnicalDebt.TD06_8.*`
et `Grimrock.RPG.ARMEMENT01.9.SaveRehydrate.*` ainsi que la
suite complète `Grimrock`.

**Localisation** : `UI.csv → ST_UI → WBP → Game/fr/en` inchangé,
aucune nouvelle chaîne ni ressource binaire. PIE/GPU et conversion
réelle des SaveGame ne sont pas attestés par `-NullRHI`.
