# MON15 — XP & Level Progression — Clôture

Statut : **VALIDÉ ET CLOS sous Unreal Engine 5.5.4**  
Date : **16 août 2026**

---

## 1. Résultat

MON15 ferme la boucle RPG allant de la mort d'un monstre à une progression persistante et exploitable par le combat :

```text
Monster death
    -> XP data-driven
    -> partage déterministe
    -> Level Up
    -> recalcul des statistiques
    -> progression de classe
    -> choix joueur
    -> projection des capacités
    -> Save / Continue / migration
```

Le système réutilise l'état canonique existant de `FGridCharacterInventoryState` :

```cpp
int32 Level = 1;
int32 Experience = 0;
```

Aucun second modèle parallèle de niveau ou d'expérience n'a été introduit.

---

## 2. MON15.1 — Modèle XP / niveau

Contrat final :

```text
XP cumulative niveau L = 1000 * (L - 1) * L / 2
Niveau minimum = 1
Niveau maximum = 20
XP maximum = 190000
```

Seuils de référence :

```text
L1       0 XP
L2    1000 XP
L3    3000 XP
L4    6000 XP
L5   10000 XP
L10  45000 XP
L20 190000 XP
```

`URPGCharacterRulesLibrary` reste l'autorité unique pour la courbe, la normalisation et la cohérence `Level <-> Experience`.

---

## 3. MON15.2 — Attribution XP

`UGridMonsterDefinitionAsset::ExperienceReward` est la source data-driven de la récompense.

La récompense :

- n'est appliquée qu'après une mort réellement committée ;
- est exactly-once via le pipeline de mort existant ;
- est partagée entre les personnages actifs éligibles ;
- utilise un ordre stable pour le reste de division ;
- exclut les personnages au plafond ;
- reste indépendante du loot et de son éventuel échec.

---

## 4. MON15.3 — Level Up

`FRPGLevelUpService` détecte un ou plusieurs seuils franchis et recalcule les statistiques via les règles RPG existantes.

Politique finale :

- plusieurs niveaux peuvent être gagnés en une transaction ;
- le déficit absolu de PV/mana est conservé ;
- un personnage mort reste à 0 PV ;
- les statistiques stockées restent les statistiques de base ;
- les bonus d'équipement ne sont jamais incorporés dans la base ;
- une progression invalide n'entraîne pas de mutation partielle du niveau/statistiques.

---

## 5. MON15.4 / MON15.5 — Progression de classe et UI

`URPGClassAsset` porte les grants par niveau et les choix de progression.

Le système gère :

- points de choix ;
- niveau minimum ;
- prérequis ;
- coût ;
- requirements accordés ;
- validation des définitions ;
- transaction atomique de plusieurs choix ;
- isolation par `CharacterId` ;
- projection immédiate vers le catalogue d'actions MON12.

La modal Level Up :

- peut être confirmée ou annulée sans mutation partielle ;
- est différée pendant le combat ;
- se présente au premier safe point ;
- restaure correctement les entrées souris/clavier et la pause ;
- coalesce les notifications successives du même personnage lorsque nécessaire.

---

## 6. MON15.6 — SaveVersion 4 et migration

`UGrimrockPartySaveGame::CurrentSaveVersion` est `4`.

La sauvegarde v4 persiste :

- `Level` et `Experience` cohérents ;
- les choix de progression confirmés, indexés par `CharacterId` ;
- les notifications Level Up encore en attente.

Les sauvegardes v1-v3 sont migrées avec une politique conservative de progression maximale. Un snapshot v4 incohérent est rejeté au lieu d'être réparé silencieusement.

Le scénario `PendingLevelUps=1 -> Continue -> Restored Pending=1` a été validé en PIE réel.

Un deadlock UI découvert après la première validation MON15.6 a également été corrigé : l'overlay `Partie chargée / 100 %` est désormais retiré avant qu'une modal Level Up restaurée puisse mettre le monde en pause. La modal est cliquable et le gameplay reprend après `ModalGuard Restored`.

---

## 7. MON15.7 — Équilibrage final

La courbe MON15.1 est figée pour le vertical slice afin de ne pas invalider le contrat SaveVersion 4.

La récompense de production du Rat Géant est :

```text
DA_MON_RatGiant.ExperienceReward = 500
```

Il s'agit d'un **pool de groupe**, pas d'une récompense par personnage.

Pacing de référence solo :

```text
1 rat   ->  500 XP -> niveau 1
2 rats  -> 1000 XP -> niveau 2
6 rats  -> 3000 XP -> niveau 3
12 rats -> 6000 XP -> niveau 4
20 rats ->10000 XP -> niveau 5
```

Pacing équivalent d'un groupe de quatre personnages éligibles :

```text
500 / 4 = 125 XP chacun
8 rats  -> niveau 2
24 rats -> niveau 3
48 rats -> niveau 4
```

Ces nombres sont des budgets de balance en équivalents Rat Géant. Le wall-clock réel par niveau sera mesuré lors de MON22.

---

## 8. Validation PIE finale

Le scénario de production avec `DA_MON_RatGiant` à 500 XP a produit :

```text
Rat 1 : Previous=0    New=500  LevelStored=1
Rat 2 : Previous=500  New=1000 LevelStored=2
Rat 3 : Previous=1000 New=1500 LevelStored=2
```

À 1000 XP, la notification est différée pendant le combat puis présentée à la victoire. Le choix est confirmable et `ModalGuard Restored` rend le contrôle au gameplay.

---

## 9. Validation Automation finale

La campagne finale du 16 août 2026 contient :

```text
95 tests exécutés
95 Success
0 Fail
0 Error
0 Skipped
0 NotRun
```

MON15 représente 42 tests :

```text
MON15.1   4
MON15.2   5
MON15.3   6
MON15.4   7
MON15.5   8
MON15.6   8
MON15.7   4
---------
Total    42
```

La même campagne couvre aussi les régressions monstres présentes dans la sélection, notamment MON9 13/13, MON13.5 RealPIEIntegration et MON14.1–14.4.

`CC5` et `MON12.ActionCatalog` avaient été validés dans la campagne précédente de MON15.6. Ils ne figurent pas dans ce dernier log de 95 tests ; MON15.7 n'a modifié aucun de leurs chemins runtime.

---

## 10. Commits de fin de jalon

```text
d3af5c30644681f07b5e6290d37acc85937f3807
Implement MON15.7 progression balancing

9b1fc8cf4bdd1a51f8af66c347b37fe62b42c074
Balance Rat Giant XP for MON15.7
```

Le DataAsset reste versionné par Git LFS.

---

## 11. Observations non bloquantes

Restent hors périmètre MON15 :

- certains anciens slots v4 incomplets sont rejetés lors de l'inspection du menu principal, conformément à la validation stricte v4 ;
- `Item_RatMeat` / `Item_RatTooth` peuvent encore produire des warnings de résolution selon le contexte du niveau ; le pipeline XP reste volontairement indépendant du loot.

---

## 12. Décision finale

```text
MON15.1 — VALIDÉ ET CLOS
MON15.2 — VALIDÉ ET CLOS
MON15.3 — VALIDÉ ET CLOS
MON15.4 — VALIDÉ ET CLOS
MON15.5 — VALIDÉ ET CLOS
MON15.6 — VALIDÉ ET CLOS
MON15.7 — VALIDÉ ET CLOS
```

**MON15 — XP & Level Progression est VALIDÉ ET CLOS.**

Prochain jalon autoritaire :

```text
MON16.1 — Status Effect Definition & Runtime State
```
