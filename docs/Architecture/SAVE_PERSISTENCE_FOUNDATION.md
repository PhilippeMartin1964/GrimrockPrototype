# Sauvegarde et persistance — Fondation d’architecture

Date de référence : **27 août 2026 — TD07.3.3.6 IMPLÉMENTÉ / À VALIDER**

## Politique prototype autoritaire

Tant que GrimrockPrototype reste un **prototype**, aucune compatibilité arrière des sauvegardes n'est exigée.

Git conserve l'historique du code et du contenu. Une sauvegarde créée avec un ancien schéma peut être supprimée. Le runtime ne maintient aucune migration uniquement pour permettre de relire une phase antérieure du prototype.

## Contrat courant

```text
UGrimrockPartySaveGame::CurrentSaveVersion = 18

SaveVersion == 18
    -> validation du schéma courant
    -> restore

SaveVersion != 18
    -> rejet
    -> aucune migration
```

La v10 a été la rupture volontaire TD07.3.2. TD07.3.3.2 a ouvert la v11 après suppression du bridge d'attributs. TD07.3.3.3 a ouvert la v12 après séparation des ressources mutables. TD07.3.3.4 a ouvert la v13 après suppression des caches de poids. TD07.3.3.5 B1 a ouvert la v14 lorsque `Level` est devenu Transient. B2 ouvre la v15 après suppression physique de `ClassProgressionStates`. TD07.3.3.6 ouvre la v16 lorsque `SkillRanks` devient durable et que `CharacterSkillStates` est supprimé. TD07.3.3.7 ouvre la v17 lorsque `KnownSpellIds` devient durable et que `CharacterSpellbookStates` est supprimé. TD07.3.3.8 ouvre la v18 lorsque `Character.StatusEffects` devient durable et que `CharacterStatusEffectStates` est supprimé ; `DefinitionAsset` reste transient et rehydraté. La v17 et toutes les générations antérieures sont désormais incompatibles.

Il n'existe plus de :

```text
MinimumCompatibleSaveVersion
FRPGSaveMigrationService
FRPGSaveMigrationReport
PrepareLoadedSave()
ResetLegacyDungeonSnapshots()
```

## Validation du schéma courant

```cpp
UGrimrockPartySaveGame::ValidateCurrentState(FText& OutError) const
```

est la frontière de validation Save.

Elle ne migre et ne modifie jamais le snapshot. Elle vérifie notamment :

- version exacte ;
- validité de Experience et cohérence du cache runtime Level reconstruit ;
- CharacterId et progression active ;
- notifications Level Up ;
- Spellbooks ;
- Skills ;
- variables de niveau.

La restauration des Status Effects et notifications Level Up reste assurée par leurs services de domaine. Les Skills sont désormais directement présents dans `FGridCharacterInventoryState::SkillRanks`; aucune restauration Skill séparée n'existe. La projection de progression de classe est reconstruite depuis `SelectedClassProgressionChoiceIds`.

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

TD07.3.3 poursuit cette normalisation. TD07.3.3.4 a supprimé les caches de poids. TD07.3.3.5 a normalisé Level et la progression de classe. TD07.3.3.6 rend `SkillRanks` durable et supprime `CharacterSkillStates`. Le schéma courant est v18 exact-match.

## Dungeon state

`UGridLevelAsset` décrit l'état initial. `FGridDungeonRuntimeState` décrit l'état vivant : présence d'objets, portes, items, réceptacles, monstres/placements/encounters, variables de niveau et permissions runtime persistantes.

`bLevelVariablesInitialized` reste temporairement présent car il participe encore au lifecycle runtime courant. Il n'est pas supprimé opportunistement par TD07.3.2.

## RPG state

`FGridPartyInventoryState` reste l'autorité du groupe et du CharacterPool.

Les snapshots séparés encore présents :

```text
PendingLevelUpNotifications
```

seront réaudités par TD07.3.3. L'objectif est de supprimer les doubles représentations sans perdre d'état gameplay réellement mutable.

## Politique de sauvegarde en combat

`GridCombatSavePolicy` reste valide : le système refuse une sauvegarde durable lorsque l'état de combat n'est pas sérialisable de façon cohérente et utilise le checkpoint pré-combat.

Ce comportement n'est pas une compatibilité arrière et n'est pas remis en cause par TD07.3.

## Validation TD07.3.2

Validation réelle du 27 août 2026 :

```text
TD07.3.2 contract       6 Success / 0 warning / 0 Failed
MON19.2 Save            2 Success / 0 warning / 0 Failed
MON20.9 Skills          7 Success / 0 warning / 0 Failed
MON16.7                 10 Success / 0 warning / 0 Failed
MON18.8                 11 Success / 0 warning / 0 Failed
TD01.1 Receptacles      2 Succeeded with warnings / 0 Failed
Win64 Shipping          [OK] Cook / package validated
```

Les deux warnings TD01.1 sont émis par le fixture transient de test et ne correspondent ni à un échec fonctionnel ni à une migration Save.

TD07.3.2 est clos. TD07.3.3 poursuit la normalisation de l'état du personnage.

## Validation

Pendant TD07.3 :

```text
capture/restore courant
validation structurelle stricte
version ancienne rejetée
Automation ciblée
PIE lorsque le comportement joueur est concerné
Shipping après changement de schéma
```

## Invariants

- une seule autorité par donnée ;
- identités valides et non ambiguës ;
- aucune donnée dérivée persistée lorsqu'elle peut être reconstruite ;
- aucune compatibilité arrière maintenue pendant le prototype ;
- version Save exacte ou rejet ;
- validation sans mutation ;
- restore atomique/fail-closed sur snapshot courant incohérent ;
- Git, et non le runtime, conserve l'histoire du prototype.
