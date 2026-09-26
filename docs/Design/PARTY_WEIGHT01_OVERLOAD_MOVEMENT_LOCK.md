# PARTY-WEIGHT01 — Overloaded Party Movement Lock

Date : **26 septembre 2026**  
Statut : **C++ prêt pour validation UE5.5.4**

## Décision gameplay

Si au moins un personnage actif du groupe est en surcharge :

```text
CurrentWeight > MaxWeight
```

le groupe ne peut plus changer de cellule.

Sont bloqués :

- Forward ;
- Backward ;
- Strafe Left ;
- Strafe Right.

La rotation sur place reste autorisée.

## Autorité

Le calcul du poids reste exclusivement dans `UGridPartyInventoryComponent`.

```text
UGridPartyInventoryComponent
    -> CalculateCharacterCurrentWeight()
    -> CalculateCharacterBaseMaxWeight()
    -> equipment CarryWeightBonus
    -> ResolveInventoryWeightState()
    -> IsAnyActiveCharacterOverloaded()
```

Aucun second seuil n'est introduit dans le Pawn.

`Heavy` ne bloque pas le mouvement. Seul `Overloaded` le bloque, donc une charge exactement égale à `MaxWeight` reste mobile.

## Mouvement

Le verrou est appliqué dans le point commun :

```cpp
AGrimrockPartyPawn::TryStartMove()
```

avant :

- les contrôles spatiaux d'exploration ;
- `UGridTurnManagerComponent::RequestPartyTranslation()`.

Conséquences :

- même règle en exploration et en combat ;
- aucune dépense de translation/AP combat lorsque la surcharge refuse le mouvement ;
- aucun faux `BlockedMoveFeedback` de collision murale ;
- les rotations ne sont pas affectées ;
- les pits, téléporteurs et autres relocations forcées ne passent pas par ce verrou joueur.

## Automation

Filtre :

```text
Grimrock.Runtime.PartyWeight01
```

Test :

```text
Grimrock.Runtime.PartyWeight01.MovementLock
```

Le test vérifie :

- `CurrentWeight == MaxWeight` ne bloque pas ;
- `CurrentWeight > MaxWeight` bloque ;
- les quatre translations sont refusées ;
- la cellule et la position ne changent pas ;
- aucun feedback spatial n'est démarré ;
- la rotation reste autorisée ;
- le mouvement redevient immédiatement possible après retrait du poids excédentaire.
