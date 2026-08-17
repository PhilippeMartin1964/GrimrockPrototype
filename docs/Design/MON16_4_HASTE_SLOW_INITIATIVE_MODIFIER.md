# MON16.4 — Haste / Slow & InitiativeModifier

## Statut

**VALIDÉ ET CLOS — 17 août 2026.**

Base :

```text
e8ffd4308fe5dc20f2393dc9e1e027b78aaa8eda
Close MON16.3 periodic status damage
```

Commit logique d'implémentation :

```text
b926dc584a1f38ca0aed3d4a53cd8c2b79ca23e5
Add MON16.4 status initiative modifiers
```

Correctif de compilation UE5.5.4 :

```text
53d86f47cbbb0440f4807e434fe9aaf593a70112
Fix MON16.4 UE5.5 guard include
```

MON16.4 donne aux effets de statut leur première projection sur l'ordre d'initiative sans créer de second moteur d'initiative. Le champ `FGridCombatantInitiativeEntry::InitiativeModifier`, préparé depuis MON12/MON16.1, reste l'unique autorité de modification runtime.

## 1. Périmètre

Implémenté et validé :

- contribution d'initiative data-driven par définition d'effet ;
- valeur positive pour accélérer, négative pour ralentir ;
- contribution multipliée par `StackCount` ;
- somme algébrique de plusieurs effets actifs ;
- saturation `int32` ;
- projection personnages et monstres ;
- prise en compte d'un statut déjà actif lorsque l'ordre est publié ;
- application/réapplication en plein combat avec réordonnancement immédiat des activations futures ;
- retrait automatique de la contribution à l'expiration `Turns` / `Rounds` ;
- préservation du jet initial et du total roulé ;
- active combatant et tours déjà consommés jamais déplacés rétroactivement.

Hors périmètre conservé :

- aucun changement de PA/PAM ;
- aucun multiplicateur de vitesse d'animation ou déplacement ;
- aucune action/spell ajoutée pour appliquer Haste ou Slow ;
- aucun HUD/icône/WBP ;
- aucune persistance des statuts avant MON16.7 ;
- aucun Stun/Silence/Immobilize avant MON16.5.

## 2. Réutilisation de MON12

MON12 fournit déjà :

```text
FGridCombatantInitiativeEntry::InitiativeModifier
FGridCombatantInitiativeEntry::GetEffectiveInitiativeTotal()
UGridTurnManagerComponent::SetCombatantInitiativeModifier()
UGridTurnManagerComponent::ReorderFutureInitiativeEntries()
FGridInitiativeOrderBuilder::Sort()
```

La règle existante est conservée :

```text
EffectiveInitiative = InitiativeTotal + InitiativeModifier
```

avec saturation `int32` dans `GetEffectiveInitiativeTotal()`.

`SetCombatantInitiativeModifier()` ne relance jamais le d20 d'initiative. Il modifie seulement le champ runtime puis retrie les entrées `Waiting` qui n'ont pas encore agi.

## 3. Résolution des statuts vers l'initiative

Resolver pur :

```text
FGridStatusEffectInitiativeResolver
```

Calcul :

```text
EffectContribution = Definition.InitiativeModifier * StackCount
TotalModifier       = somme des EffectContribution
```

La somme et chaque contribution sont saturées dans l'intervalle `int32`.

Un effet dont `InitiativeModifier == 0` n'a aucun impact sur l'ordre.

Configurations de référence :

```text
Haste -> InitiativeModifier positif
Slow  -> InitiativeModifier négatif
```

Les noms `Haste` et `Slow` ne sont jamais testés par le code de production : ils restent de simples configurations de `UGridStatusEffectDefinitionAsset`.

## 4. Stacking

La contribution est par stack actif :

```text
InitiativeModifier=+4, StackCount=1 -> +4
InitiativeModifier=+4, StackCount=2 -> +8
InitiativeModifier=+4, StackCount=3 -> +12
```

Pour un Haste/Slow classique, le design peut utiliser `NoStack`, `RefreshDuration` ou `ReplaceIfStronger` avec `MaxStacks=1`.

`Potency` n'entre jamais dans ce calcul. Elle conserve uniquement son rôle MON16.2 pour `ReplaceIfStronger`.

## 5. Projection au démarrage du combat

`UGridStatusEffectLifecycleSubsystem` écoute :

```text
OnTurnOrderChanged
```

Quand le TurnManager publie l'ordre fraîchement roulé, le subsystem relit les collections de statuts autoritatives et projette leurs contributions via `SetCombatantInitiativeModifier()`.

Cela garantit qu'un statut déjà actif avant la publication de l'ordre est pris en compte sans modifier le code de rolling du TurnManager.

Une garde de réentrance empêche les broadcasts produits par `SetCombatantInitiativeModifier()` de provoquer une boucle.

## 6. Application en plein round

MON16.4 ajoute deux points d'entrée C++ au subsystem de lifecycle :

```text
TryApplyStatusEffectToPartyCharacter(...)
TryApplyStatusEffectToMonster(...)
```

Ils réutilisent directement :

```text
FGridStatusEffectCollection::TryApply()
```

puis, seulement si la mutation a réussi, recalculent le modificateur d'initiative du combattant concerné.

Conséquences validées :

- un Haste appliqué à un combattant futur peut le faire remonter immédiatement ;
- un Slow peut le faire descendre immédiatement ;
- le combattant actuellement actif ne bouge jamais rétroactivement ;
- un combattant ayant déjà terminé son tour n'est pas déplacé dans le round courant ;
- le round suivant utilise naturellement le modificateur encore actif.

## 7. Expiration

Après chaque :

```text
AdvanceDuration(Turns)
AdvanceDuration(Rounds)
```

le subsystem recalcule l'`InitiativeModifier` depuis la collection restante.

Donc :

```text
expiration Haste -> contribution positive supprimée
expiration Slow  -> contribution négative supprimée
```

Le recalcul se fait après les éventuels DoT MON16.3 et après le retrait de l'effet expiré.

## 8. Personnages et monstres

Le calcul est identique pour les deux catégories :

```text
Party Character.StatusEffects
        -> FGridStatusEffectInitiativeResolver
        -> SetCombatantInitiativeModifier(Party, ...)

Monster.StatusEffects
        -> FGridStatusEffectInitiativeResolver
        -> SetCombatantInitiativeModifier(Monster, ...)
```

Aucun système parallèle de stats ou d'initiative n'est ajouté.

## 9. Fichiers

Ajoutés :

```text
Source/GrimrockPrototype/Public/RPG/StatusEffects/GridStatusEffectInitiativeResolver.h
Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectInitiativeResolver.cpp
Source/GrimrockPrototype/Private/Tests/RPGMON164StatusEffectInitiativeTests.cpp
docs/Design/MON16_4_HASTE_SLOW_INITIATIVE_MODIFIER.md
docs/Design/MON16_4_VALIDATION_CHECKLIST.md
```

Modifiés :

```text
Source/GrimrockPrototype/Public/RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.h
Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.cpp
```

Aucun `.uasset`, `.umap`, WBP, Build.cs ou SaveGame.

## 10. Validation UE5.5.4 — 17 août 2026

Le premier build a révélé un include incorrect pour `TGuardValue` :

```text
Misc/GuardValue.h -> introuvable sous UE5.5.4
```

Le correctif `53d86f47...` utilise l'include UE5.5.4 approprié :

```text
Templates/UnrealTemplate.h
```

Le log utilisateur suivant exécute ensuite la campagne Automation complète avec le nouveau code chargé.

### MON16.4 ciblé

```text
ActiveCombatantStability      Success
AggregateModifier             Success
FutureHasteReorder            Success
FutureSlowReorder             Success
MonsterParity                 Success
NoParallelSystem              Success
ReapplicationUpdatesModifier  Success
RoundExpiration               Success
StackScalingAndSaturation     Success
TurnExpiration                Success
TurnOrderBroadcastProjection  Success
```

Bilan : **11/11 Success**.

Traces runtime représentatives :

```text
Haste futur     : Modifier 0 -> +12, EffectiveTotal=22
Slow futur      : Modifier 0 -> -15, EffectiveTotal=5
Actif stable    : Modifier 0 -> +100, EffectiveTotal=130 sans déplacement rétroactif
Réapplication   : Modifier +4 -> +8
Expiration round: Modifier +12 -> 0
Expiration turn : Modifier +9 -> 0
Monster parity  : Modifier 0 -> -10, EffectiveTotal=5
```

### Régressions demandées

```text
MON16.3 : 11/11 Success
MON16.2 : 10/10 Success
MON16.1 :  7/7 Success
MON15   : 42/42 Success
MON14   : 19/19 Success
```

La campagne complète fournie contient **134 tests terminés, 134 Success, 0 Fail et 0 Error Automation**.

## 11. Contrat gelé MON16.4

À partir de cette clôture :

- `InitiativeModifier` reste l'unique champ runtime de Haste/Slow ;
- le jet initial et `InitiativeTotal` restent inchangés ;
- seuls les combattants futurs `Waiting` peuvent être réordonnés en plein round ;
- l'actif et les tours déjà consommés ne bougent jamais rétroactivement ;
- les contributions de statut s'additionnent et se multiplient par stack ;
- l'expiration retire automatiquement la contribution ;
- personnages et monstres partagent le même resolver ;
- aucun hard-code `Haste` / `Slow` n'est introduit ;
- aucune persistance de statut n'est incluse avant MON16.7.

Prochaine étape : **MON16.5 — Stun / Silence / Immobilize**.
