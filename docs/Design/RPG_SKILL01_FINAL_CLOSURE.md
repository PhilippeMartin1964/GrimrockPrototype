# RPG-SKILL01 — Clôture finale Skill Point Economy & Allocation

Date : **8 octobre 2026**  
État : **VALIDÉ / CLOS**

## Résultat

Le joueur peut désormais attribuer ses points de compétence directement dans
la page COMPÉTENCES.

```text
Level
  -> Skill Points accordés

SkillRanks
  -> Skill Points dépensés

FRPGSkillPointService
  -> solde dérivé
  -> plafond de Rank
  -> achat +1
  -> remboursement sûr -1

WBP_GridSkills
  -> solde / Rang max

WBP_RPGSkillEntry
  -> [ − ] [ + ]
```

## Règles validées

```text
Niveau 1      = 4 Skill Points
Niveaux 2-20  = +1 par niveau
Niveau 20     = 23 points accordés

1 point       = +1 Rank

Rank cap
1-4           = 2
5-9           = 3
10-14         = 4
15-20         = 5
```

Le `MaxRank` du `URPGSkillAsset` reste également contraignant.

## Autorité

Aucune monnaie Skill Point n'est sauvegardée.

```text
Granted   = Level + 3
Spent     = somme des SkillRanks
Remaining = Granted - Spent
```

`FGridCharacterInventoryState::SkillRanks` reste l'autorité durable.

Toute allocation joueur passe par :

```text
FRPGSkillPointService::TryPurchaseNextRank()
FRPGSkillPointService::TryRefundPurchasedRank()
```

UMG n'écrit jamais directement dans `SkillRanks`.

## Safe Undo

Le bouton `−` corrige uniquement une attribution effectuée pendant la session
courante de COMPÉTENCES.

```text
rang à l'ouverture = plancher
+                  = rang achetable et annulable
-                  = retour possible jusqu'au plancher
fermer / rouvrir K = nouveau plancher
```

Il ne s'agit donc pas d'un système de respec gratuit.

## Validation locale

```text
Filter                 : Grimrock.RPG.SKILL01
Succeeded              : 8
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
```

Build Development Editor : **OK**.

## Validation PIE

Validé manuellement :

```text
+ augmente le Rank
+ consomme un Skill Point
- rembourse le point
- diminue le Rank
- se désactive au plancher de session
fermer puis rouvrir K rend le rang précédent non remboursable
```

## Matérialisation

Commit UMG final :

```text
aba11f5502376a77ab6b98749a3a0cbbfb9e4f7f
RPG-SKILL01.3B materialize safe Skill allocation undo
```

Le diff final de ce commit contient uniquement
`WBP_RPGSkillEntry.uasset`. L'état de `WBP_GridSkills` nécessaire au
fonctionnement a été confirmé par le test PIE.

## Jalons

```text
RPG-SKILL01.1  CLOS — économie + transaction
RPG-SKILL01.2  CLOS — allocation + en UMG
RPG-SKILL01.3A CLOS — Safe Undo source
RPG-SKILL01.3B CLOS — matérialisation −/+ et PIE
RPG-SKILL01    CLOS
```

## Hors périmètre

RPG-SKILL01 ne traite pas :

- les points de caractéristiques aux niveaux 4/8/12/16/20 ;
- la suppression de la popup Level Up ;
- l'enrichissement des descriptions de Talents/variantes ;
- les icônes Skills/Talents ;
- un système de respec permanent.
