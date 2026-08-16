# MON15.7 — Équilibrage et clôture de MON15

Statut : **VALIDÉ ET CLOS sous Unreal Engine 5.5.4**  
Date : **16 août 2026**

---

## 1. Objectif

MON15.7 ne crée pas de nouveau système de progression. Il fixe les paramètres de production du vertical slice et verrouille les hypothèses d'équilibrage de MON15 avant le passage à MON16.

Périmètre :

- figer la courbe XP actuelle ;
- choisir la récompense finale du Rat Géant ;
- définir un rythme de progression de référence ;
- retirer la valeur temporaire utilisée pendant les validations MON15.5 / MON15.6 ;
- ajouter une garde Automation sur l'asset de production ;
- exécuter les régressions finales ;
- documenter la clôture de MON15.

---

## 2. Courbe XP finale

La courbe MON15.1 est conservée :

```text
XP cumulative niveau L = 1000 * (L - 1) * L / 2
```

Seuils principaux :

```text
Niveau 1      0 XP
Niveau 2   1000 XP
Niveau 3   3000 XP
Niveau 4   6000 XP
Niveau 5  10000 XP
Niveau 10 45000 XP
Niveau 20 190000 XP
```

Le niveau maximum reste 20.

La courbe n'est pas modifiée en MON15.7 afin de préserver le contrat strict `Level <-> Experience` de SaveVersion 4.

---

## 3. Récompense finale du Rat Géant

Valeur de production :

```text
DA_MON_RatGiant.ExperienceReward = 500
```

Cette valeur est un **pool total de groupe** conformément à MON15.2.

Historique :

```text
10 XP   = valeur historique MON1 / première validation MON15.2
1000 XP = valeur temporaire utilisée pour accélérer les PIE MON15.5 / MON15.6
500 XP  = valeur de production MON15.7
```

Le C++ ne contient aucun cas particulier `RatGiant`; la valeur reste portée par le DataAsset.

---

## 4. Pacing de référence

### Solo

```text
1 rat    500 XP  -> niveau 1
2 rats  1000 XP  -> niveau 2
6 rats  3000 XP  -> niveau 3
12 rats 6000 XP  -> niveau 4
20 rats 10000 XP -> niveau 5
```

### Groupe de quatre personnages actifs

```text
500 / 4 = 125 XP par personnage
8 rats   -> niveau 2
24 rats  -> niveau 3
48 rats  -> niveau 4
80 rats  -> niveau 5
```

Ces valeurs sont des rat-equivalents de balance, pas une prescription de contenu.

Le wall-clock réel ne sera fixé qu'en MON22, lorsque les familles de monstres, sorts, effets de statut et rencontres seront représentatifs du slice 45–90 minutes.

---

## 5. Tests dédiés

Suite finale :

```text
Grimrock.RPG.MON15.7.FrozenCurve
Grimrock.RPG.MON15.7.SoloRatPacing
Grimrock.RPG.MON15.7.PartyRatPacing
Grimrock.RPG.MON15.7.ProductionRatAsset
```

Résultat : **4/4 Success**.

`ProductionRatAsset` charge réellement :

```text
/Game/GrimrockPrototype/Monsters/RatGiant/Data/DA_MON_RatGiant
```

et confirme :

```text
MonsterId = MON_RatGiant
ExperienceReward = 500
```

---

## 6. Validation PIE

Le PIE final a confirmé :

```text
Rat 1 : 0 -> 500 XP, niveau 1, aucune montée
Rat 2 : 500 -> 1000 XP, niveau 2, Level Up 1 -> 2
Rat 3 : 1000 -> 1500 XP, toujours niveau 2
```

Le Level Up déclenché pendant le combat est différé puis présenté au safe point de victoire. La modal est utilisable, le choix est confirmable et `ModalGuard Restored` restitue correctement les contrôles.

---

## 7. Régression finale

Campagne fournie le 16 août 2026 :

```text
95 tests
95 Success
0 Fail
0 Error
0 Skipped
0 NotRun
```

Les 42 tests MON15.1 à MON15.7 sont verts. La campagne couvre également les régressions monstres sélectionnées, notamment MON9, MON13.5 RealPIEIntegration et MON14.1–14.4.

`CC5` et `MON12.ActionCatalog` avaient déjà été validés durant la campagne MON15.6 ; ils ne figurent pas dans le dernier log de 95 tests. MON15.7 n'a modifié aucun de leurs chemins runtime.

---

## 8. Asset versionné

Le changement de production est versionné dans :

```text
9b1fc8cf4bdd1a51f8af66c347b37fe62b42c074
Balance Rat Giant XP for MON15.7
```

Le pointeur Git LFS de `DA_MON_RatGiant.uasset` a été mis à jour.

---

## 9. Frontière du jalon

MON15.7 ne change pas :

- la formule de partage de l'XP ;
- le niveau maximum ;
- le SaveVersion ;
- les règles de recalcul des stats ;
- la progression de classe ;
- le pipeline de mort/loot ;
- les règles de combat.

---

## 10. Décision finale

**MON15.7 — VALIDÉ ET CLOS.**

Avec la clôture de MON15.1 à MON15.6 déjà acquise, **MON15 — XP & Level Progression est VALIDÉ ET CLOS**.

Suite :

```text
MON16.1 — Status Effect Definition & Runtime State
```
