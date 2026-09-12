# PRESSURE-MON01 — Optional Monster Pressure Plate Activation

## Objectif

Une `PressurePlate` peut désormais choisir si la présence physique d'un monstre sur sa cellule participe à son activation.

Cette règle est **optionnelle** et désactivée par défaut afin de préserver le comportement des niveaux existants.

## Donnée d'authoring

`FGridPressurePlateWeightParams` expose :

```cpp
bool bActivateWhenMonsterPresent = false;
```

La valeur appartient aux règles de la plaque, au même niveau que `bActivateWhenPartyPresent` et `bUseItemWeight`. Elle est donc Definition-owned par défaut et voyage aussi avec la structure complète `PressurePlateWeight` déjà utilisée par l'override d'instance.

## Règle runtime

L'état pressé est l'OR des règles activées :

```text
Pressed = PartyPresent
       OR MonsterPresent
       OR SufficientItemWeight
```

`MonsterPresent` n'est évalué que lorsque `bActivateWhenMonsterPresent` vaut `true`.

La présence d'un monstre provient exclusivement de `UGridMonsterOccupancySubsystem`, qui reste l'autorité des cellules dynamiquement occupées par les monstres.

Une **réservation de destination ne presse pas la plaque**. Le monstre doit réellement avoir terminé son déplacement et sa nouvelle occupation doit avoir été commitée.

## Synchronisation avec les mouvements

Le sous-système d'occupation rafraîchit les PressurePlates concernées lors de :

- l'enregistrement/spawn d'un monstre sur une cellule ;
- la validation d'un déplacement, sur la cellule quittée puis sur la cellule atteinte ;
- la suppression/désinscription d'un monstre de sa cellule.

Cette centralisation couvre également les chemins runtime qui utilisent directement l'occupation sans dépendre d'un composant de déplacement particulier.

Les événements existants restent inchangés : un changement d'état continue d'émettre `Activated` ou `Deactivated`, puis les liens Event -> Command / Event -> Lua existants prennent le relais.

## Limites volontaires de PRESSURE-MON01

La présence d'un monstre est binaire. PRESSURE-MON01 n'introduit aucun poids de monstre : un rat et un gros monstre pressent de la même manière une plaque configurée pour accepter les monstres.

Le poids des monstres pourra être généralisé ultérieurement si un puzzle concret l'exige, sans modifier le contrat introduit ici.

## Validation Automation

Filtre :

```text
Grimrock.PressurePlate.PRESSURE_MON01
```

Le test couvre : option désactivée par défaut, réservation sans activation, activation à l'occupation commitée, désactivation au départ de la cellule et absence d'activation sur une seconde plaque ayant explicitement refusé les monstres.
