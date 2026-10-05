# RPG03.4 — Trigger / Reaction Profile (C4)

Date : **5 octobre 2026**  
Parent : `RPG_Talents_Mechanics_v0_1.md`  
Dépendance : **RPG03.3 validé 8/8**  
Statut : **VALIDÉ — TD04.2 / Grimrock.RPG.RPG03.4 / 8 réussis / 0 warning / 0 échec / 5 octobre 2026**

## Objectif

RPG03.4 fournit un vocabulaire unique pour les réactions de combat sans coder les Talents par identifiant.

Le pipeline est :

```text
action/attaque autoritaire
        ↓
FGridCombatReactionEvent
        ↓
FGridCombatReactionResolver
        ↓
filtres data-driven
        ↓
FGridCombatReactionLedger
        ↓
FGridCombatReactionMatch
        ↓
consommation éventuelle du Status propriétaire
        +
OnCombatReactionTriggered
```

Le C4 ne crée ni second TurnManager, ni second système de Status Effect.

## Triggers

`EGridCombatReactionTrigger` :

- `ActionResolved`
- `AttackHit`
- `AttackMiss`
- `TargetDefeated`
- `DirectDamageReceived`
- `SurfaceReaction`

`SurfaceReaction` est volontairement défini dès C4 mais sera alimenté par C6.

## Limites

`EGridCombatReactionLimit` :

- `Unlimited`
- `OncePerRound`
- `OncePerAction`

Le ledger est runtime-only. Rien n'est ajouté au SaveGame.

`OncePerAction` utilise un `ActionInstanceId` racine : une attaque de zone peut donc produire plusieurs événements Hit/Kill sur plusieurs cibles tout en ne consommant qu'une seule fois une réaction limitée à l'action.

## Filtres

Un `FGridCombatReactionProfile` peut filtrer :

- `ActionIds`
- `SourcePolicies`
- `ActionTypes`
- `DamageTypes`

Les listes vides sont des wildcards.

Aucun code de production ne compare `ReactionId` à un nom de Talent.

## Anti-récursion

Les événements provenant d'une réaction portent `bReactionGenerated=true`.

Par défaut :

```text
bAllowReactionGeneratedEvents = false
```

Cela permet d'exprimer les contrats comme Riposte ou Réaction en chaîne sans boucle récursive.

Un profil peut explicitement autoriser un événement de réaction si une future mécanique en a réellement besoin.

## Authoring

Les réactions peuvent être déclarées sur :

```text
FRPGClassProgressionChoiceDefinition::CombatReactions
UGridStatusEffectDefinitionAsset::CombatReactions
```

Les choix de classe ne peuvent pas utiliser `bConsumeOwningStatus`, puisqu'ils ne possèdent pas de Status runtime.

Les Status Effects peuvent l'utiliser.

## Consume-on-action

RPG03.4 ajoute une suppression explicite :

```text
FGridStatusEffectCollection::RemoveByEffectId()
```

et les bridges lifecycle :

```text
ConsumeStatusEffectFromPartyCharacter()
ConsumeStatusEffectFromMonster()
```

Une réaction authorée dans un Status Effect avec :

```text
Trigger = ActionResolved
bConsumeOwningStatus = true
```

peut donc modéliser proprement :

- prochaine action offensive ;
- prochain sort compatible ;
- prochaine attaque d'arme ;
- rupture d'un buff après une action donnée.

La suppression passe toujours par MON16 : initiative, feedback et présentation restent centralisés.

## Événements runtime branchés

### Joueur → monstre

Les attaques joueur émettent :

- `AttackHit` ou `AttackMiss` ;
- `ActionResolved` une fois par action ;
- `TargetDefeated` pour chaque cible vaincue.

Les attaques Area utilisent un `ActionInstanceId` partagé entre toutes les cibles.

### Monstre → personnage

Les attaques de monstre émettent vers le personnage ciblé :

- `AttackHit` ou `AttackMiss` ;
- `DirectDamageReceived` si des dégâts ont réellement été appliqués.

Cela fournit le trigger nécessaire à une future Riposte et à la rupture de statuts sur dégâts directs.

### Effects Self et sorts

Les actions Self/Effect et le chemin Spellbook émettent `ActionResolved` après transaction réussie.

## Résultat générique

Chaque match produit `FGridCombatReactionMatch` puis :

```text
OnCombatReactionTriggered
```

Le résultat contient :

- `ReactionId`
- propriétaire
- Status propriétaire éventuel
- demande de consommation éventuelle
- événement complet.

Le C4 sépare donc volontairement **détection/limitation/consommation** de l'effet métier de la réaction.

## Couverture RPG02

RPG03.4 fournit directement le contrat nécessaire pour :

- Riposte : `AttackMiss + MeleeAttack + OncePerRound` ;
- Chasseur alpha : `TargetDefeated + OncePerRound` ;
- Réaction en chaîne : `SurfaceReaction + OncePerAction`, non récursive ;
- buffs "prochaine action" : `ActionResolved + consume owning status` ;
- Ombre parfaite : consommation après action offensive ou `DirectDamageReceived` ;
- Surcharge élémentaire / Imprégnation / Pas de l'ombre : consommation filtrée sur l'action compatible.

Les effets nécessitant une sélection ou transformation externe restent volontairement dans leur dépendance :

- exécution de la contre-attaque de Riposte : authoring/executor des Talents lors de RPG03.9 ;
- transfert spatial de Chasseur alpha : C8 ;
- réaction de surface : C6 ;
- pièges / entrée de cellule : C5 ;
- Diffusion vers un second allié : C7+C8.

C4 n'invente pas de raccourci spécifique pour ces systèmes.

## Automation ajoutée

Filtre :

```text
Grimrock.RPG.RPG03.4
```

Tests :

1. `ProfileValidation`
2. `FilterMatching`
3. `RecursiveIsolation`
4. `OncePerRoundLedger`
5. `OncePerActionLedger`
6. `Projection`
7. `ResolveMatches`
8. `RuntimeStatusConsumption`

Validation utilisateur reçue le 5 octobre 2026 : 8 tests réussis, 0 warning, 0 échec, 0 not run, process exit code 0.
