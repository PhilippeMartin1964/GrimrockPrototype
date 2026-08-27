# Sauvegarde et persistance — Fondation d’architecture

Date de référence : **27 août 2026 — TD07.3.1**

## Politique prototype autoritaire

Tant que GrimrockPrototype reste un **prototype**, aucune compatibilité arrière des sauvegardes n'est exigée.

Git conserve l'historique du code et du contenu. Une ancienne sauvegarde incompatible peut être supprimée. Le projet ne maintient pas de migration uniquement pour permettre de relire une ancienne phase du prototype.

Le contrat cible devient :

```text
Save schema courant
    version exacte attendue -> chargement
    version différente       -> rejet

aucune chaîne de migration v1 -> vN
aucune valeur legacy reconstruite
aucun champ conservé seulement pour anciennes saves
```

Cette politique sera réévaluée lorsque le prototype sera validé et que le projet entrera en phase de développement produit.

## Implémentation encore présente avant TD07.3.2

Le code courant contient toujours temporairement :

```text
UGrimrockPartySaveGame::CurrentSaveVersion = 9
UGrimrockPartySaveGame::MinimumCompatibleSaveVersion = 1
FRPGSaveMigrationService
migrations v1-v8
```

TD07.3.1 ne les supprime pas encore : il caractérise le modèle. **TD07.3.2 doit retirer cette compatibilité historique.**

## Frontière persistante cible

Un SaveGame prototype doit contenir uniquement :

- identités stables nécessaires à la résolution du contenu courant ;
- état réellement mutable du groupe et du donjon ;
- ressources courantes qui ne sont pas recalculables ;
- progression choisie par le joueur ;
- position / orientation / présence / état logique nécessaires au Continue.

Ne doivent pas être persistés comme autorités :

- pointeurs vers DataAssets lorsqu'un ID runtime suffit ;
- noms, icônes ou textes recalculables ;
- poids recalculables ;
- statistiques dérivées qui peuvent être reconstruites ;
- duplications runtime/save de la même structure sans nécessité ;
- marqueurs servant uniquement à distinguer un ancien snapshot.

## Dungeon state

`UGridLevelAsset` décrit l'état initial. `FGridDungeonRuntimeState` décrit l'état vivant : présence d'objets, portes, items, réceptacles, monstres/placements/encounters, variables de niveau et permissions runtime persistantes.

Le principe reste valide. TD07.3 auditera cependant les champs qui existent uniquement pour reconnaître ou normaliser d'anciens snapshots, notamment `bLevelVariablesInitialized` et `ResetLegacyDungeonSnapshots`.

## RPG state

`FGridPartyInventoryState` reste l'autorité du groupe et du CharacterPool.

Les snapshots séparés actuellement présents :

```text
ClassProgressionStates
PendingLevelUpNotifications
CharacterStatusEffectStates
CharacterSpellbookStates
CharacterSkillStates
```

sont réaudités par TD07.3.3. L'objectif n'est pas de supprimer une donnée gameplay nécessaire, mais de supprimer les doubles représentations et de rapprocher les données durables de leur autorité réelle.

## Politique de sauvegarde en combat

`GridCombatSavePolicy` reste valide : le système refuse une sauvegarde durable lorsque l'état de combat n'est pas sérialisable de façon cohérente et utilise le checkpoint pré-combat.

Ce comportement n'est pas une compatibilité arrière et n'est pas remis en cause par TD07.3.

## Validation

Pendant TD07.3 :

```text
capture/restore courant
validation structurelle stricte
ancienne version rejetée
Automation ciblée
PIE lorsque le comportement joueur est concerné
Shipping après changement de schéma
```

Les anciens tests de migration restent historiques jusqu'à leur suppression dans TD07.3.2.

## Invariants

- une seule autorité par donnée ;
- identités valides et non ambiguës ;
- aucune donnée dérivée persistée lorsqu'elle peut être reconstruite ;
- aucune compatibilité arrière maintenue pendant le prototype ;
- restore atomique/fail-closed sur snapshot courant incohérent ;
- Git, et non le runtime, conserve l'histoire du prototype.
