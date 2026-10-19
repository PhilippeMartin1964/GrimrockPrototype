# RPG-PERF01 — Lectures legeres de progression et de surcharge

Date : 2026-10-19
Statut : **implementation publiee ; validation locale UE5.5.4 attendue**.

## Problème mesuré par inspection du code

La routine de rappel `AGrimrockPartyPawn::UpdateProgressionEventFeed` parcourt les
personnages actifs environ une fois par seconde. Elle utilisait
`UGridPartyInventoryComponent::GetCharacterSummary` pour lire exclusivement
`bHasUnspentProgressionPoints`, `Level` et `DisplayName`.
Le calcul du résumé inclut poids, equipement, resistances, derivation de
statistiques, portrait et autres valeurs inutiles pour ce rappel.

Pour chaque demande de translation, `AGrimrockPartyPawn::TryStartMove`
appelait `IsAnyActiveCharacterOverloaded`, qui calcule la masse transportée
jusqu'au premier personnage surcharge, puis parcourait une nouvelle fois
tous les personnages avec `GetCharacterSummary`, ce qui relançait les
calculs de poids et de caracteristiques pour composer les noms du message.

Ce constat est une **analyse statique de chemins**, pas une mesure de duree
CPU ou de frames dans PIE.

## Modifications

### Aucune nouvelle autorite / aucun cache

Les requetes spécialisées suivantes sont ajoutees à
`UGridPartyInventoryComponent` :

- `HasUnspentCharacterProgressionPoints(CharacterIndex)` : reutilise
  exactement `HasUnspentPartyPortraitProgressionPoints` (soldes
  canoniques Skill/Attribute/Talent). Aucun calcul d'equipement ou
  de portrait.
- `GetActiveCharacterDisplayName(CharacterIndex)` : conserve le nom du
  personnage et les fallback historiques `Hero_01`, `Hero_02`, etc.
  `GetCharacterSummary` utilise désormais la meme fonction, pour prevenir
  toute divergence.
- `GetOverloadedActiveCharacterIndices(OutIndices)` : une seule passe
  qui collecte les indices des personnages surcharges en ordre du groupe,
  sans calculer de `FGridInventoryCharacterSummary`.
- `IsCharacterOverloaded(CharacterIndex)` (prive) : factorise le calcul
  de poids de `IsAnyActiveCharacterOverloaded`, en gardant les memes
  fonctions `CalculateCharacterCurrentWeight`,
  `CalculateCharacterBaseMaxWeight`,
  `ComputeCharacterEquipmentStatBonus`, et le test strict
  `ResolveInventoryWeightState(...)=Overloaded`.

Le reminder lit directement `Character.Level` et le nom leger.
La translation utilise une seule enumeration des surcharges pour decider du
blocage et enumerer les personnes concernees.

Le montage des portraits, la presentation complete des personnages,
les transactions de progression, les chemins de combat et les autres
consommateurs `GetCharacterSummary` conservent leur comportement.

## Equivalence et limites

Invariants à conserver :

1. Point disponible dans la requete legere = `Summary.bHasUnspentProgressionPoints`.
2. Nom leger = `Summary.DisplayName`, y compris fallback si vide.
3. Chaque index surcharge = `Summary.bOverloaded`, dans le meme ordre.
4. **poids exactement egal au maximum : autorise** ; un depassement
   strict suffit à bloquer les mouvements, sans changer la rotation.
5. Chaque translation refusee par surcharge emet toujours un evenement
   `Party.Weight.MovementBlocked` contenant le ou les noms.
6. Aucun nouveau champ persistant ; aucun changement d'UMG / WBP / DataAsset.

Les requetes restent des calculs effectues à la demande et ne gardent
pas de cache invalide apres un changement d'equipement ou une restauration
de sauvegarde. La complexite de la collecte est proportionnelle au
nombre de personnages et aux items portes ; elle evite la construction
complete des résumés et la seconde exploration du poids au refus de mouvement.
Le gain en temps réel devra etre constate avec les outils de profilage UE,
et n'est pas estime numériquement ici.

## Tests

Nouveau fichier :
`Source/GrimrockPrototype/Private/Tests/GridRPGPerf01ReadModelTests.cpp`.

Filtres :

- `Grimrock.RPG.PERF01.ProgressionLightweightRead` :
  indices invalides, nom canonique/fallback, points disponibles au niveau 1,
  puis aucun point apres attribution, nouvelle disponibilite au niveau 2,
  et equivalence avec `GetCharacterSummary`.
- `Grimrock.RPG.PERF01.OverloadSinglePass` :
  deux personnages, poids nul, limite exacte, excedent de poids,
  surcharges multiples, ordre des indices, noms et equivalence avec
  `GetCharacterSummary`.
- `Grimrock.Runtime.PartyWeight01.MovementLock` :
  renforce pour verifier que le message contient le nom du personnage
  surcharge.
- Suites d'integration existantes : `Grimrock.UI.PartyHud01`,
  `Grimrock.UI.EventHUD01.ProgressionReminder`.

### TD04.2 (fermer Unreal Editor avant execution)

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.RPG.PERF01"

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Runtime.PartyWeight01"

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.PartyHud01"

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.EventHUD01.ProgressionReminder"
```

### Smoke PIE

1. Confirmer les badges `+` quand des points sont disponibles.
2. Verifier le rappel dans le Flux à l'apparition et a l'echeance (5 min de
   jeu actif), y compris quand plusieurs membres ont des points.
3. Provoquer un refus de translation avec un membre surcharge, verifier
   son nom et un message par tentative.
4. Retirer le surpoids et verifier la reprise immediate des translations,
   sans empecher la rotation.
5. Changer des pieces d'equipement modifiant la capacite de portage et
   verifier que les messages refletent l'etat courant.

Ne pas considérer le jalon clos avant compilation, automatisation et
verification manuelle du comportement dans PIE.
