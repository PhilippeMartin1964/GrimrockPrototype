# MON21.6.3 — Topology-Aware Reveal

Date : **28 septembre 2026**  
Statut : **IMPLÉMENTÉ — VALIDATION LOCALE UTILISATEUR REQUISE**

## 1. Objectif

MON21.6.3 transforme l’état Unknown/Explored de MON21.6.2 en révélation automatique autour du groupe, sans introduire de rendu Map ni de persistance disque.

## 2. Rayon

`FGridMapRevealService::RevealRadiusCells = 1.25f`.

Le contrat est volontairement borné entre 1 et √2 : cellule courante + quatre cardinales, jamais les diagonales.

## 3. Topologie

Une cellule voisine est révélable uniquement si :

- les deux cellules existent et ne sont pas `Empty` ;
- aucun mur `Solid` n’existe sur l’un ou l’autre côté de leur frontière commune ;
- si une porte est authorée sur cette frontière, le `UGridDoorSystemComponent` existe, indexe cette porte et indique que le passage n’est plus bloqué.

`bBlocksOccupancy` n’intervient pas : visibilité cartographique et occupation gameplay sont deux notions distinctes.

Une donnée de porte présente sans autorité runtime exploitable est traitée fail-closed.

## 4. Déclenchement

`AGridLevelRuntimeActor::HandlePartyCellChanged()` appelle désormais `RevealMapAroundCell(NewCellX, NewCellY)`.

Le `BeginPlay()` du Pawn appelant déjà `HandlePartyCellChanged(Current, Current)`, la zone initiale est révélée sans mécanisme supplémentaire. Chaque déplacement réussi étend ensuite l’exploration.

Il n’y a aucun Tick Map.

## 5. Frontières conservées

- secret discovery : MON21.6.4 ;
- SaveGame disque : MON21.6.5 ;
- read model : MON21.6.6 ;
- rendu / WBP : MON21.6.8.

Le SaveGame reste v22.

## 6. Validation MON21.6.2 actée

Validation utilisateur du 28 septembre 2026 :

```text
Filter                  Grimrock.Map.MON21_6_2
Succeeded               4
Succeeded with warnings 0
Failed                  0
Not run                 0
Process exit code        0
Report                   TD04-20260928-082142
```

MON21.6.2 est donc **VALIDÉ**.

## 7. Automation MON21.6.3

Filtre : `Grimrock.Map.MON21_6_3`

```text
Reveal.RadiusAndOccupancy
Reveal.StructuralWallsBlockBothSides
Reveal.DoorTopology
Reveal.RuntimeCellChangeHook
```

Aucun résultat MON21.6.3 n’est déclaré avant retour du harness local UE5.5.4.

## 8. Stop condition

MON21.6.3 est implémenté lorsque la révélation initiale et après mouvement utilise le rayon 1.25, respecte murs/portes, ignore les blockers d’occupation et reste cumulative/idempotente.

Prochaine tranche après validation : **MON21.6.4 — Secret Discovery**.
