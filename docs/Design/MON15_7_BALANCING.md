# MON15.7 — Équilibrage et clôture de MON15

Statut : **IMPLEMENTÉ — EN ATTENTE DE VALIDATION UE5.5.4 ET MISE À JOUR DE DA_MON_RatGiant**  
Date : **16 août 2026**

---

## 1. Objectif

MON15.7 ne crée pas un nouveau système de progression. Il fixe les paramètres de production du vertical slice et verrouille les hypothèses d'équilibrage de MON15 avant le passage à MON16.

Le périmètre est volontairement réduit :

- figer la courbe XP actuelle ;
- choisir la récompense finale du Rat Géant ;
- définir un rythme de progression observable ;
- éliminer les valeurs temporaires utilisées pendant MON15.5 / MON15.6 ;
- ajouter une garde Automation sur l'asset de production ;
- exécuter les régressions finales combat / inventaire / save ;
- documenter la clôture de MON15.

---

## 2. Courbe XP finale du vertical slice

La courbe MON15.1 est conservée sans modification :

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

### Pourquoi ne pas modifier la courbe en MON15.7 ?

MON15.6 vient de figer un contrat SaveVersion 4 qui valide strictement la cohérence `Level` / `Experience` avec cette courbe. Changer maintenant les seuils rendrait certaines sauvegardes v4 incohérentes et imposerait immédiatement une nouvelle migration SaveVersion.

MON15.7 traite donc l'équilibrage par les récompenses data-driven des monstres, pas par une nouvelle courbe.

---

## 3. Récompense finale du Rat Géant

Valeur MON15.7 :

```text
DA_MON_RatGiant.ExperienceReward = 500
```

Cette valeur est un **pool total de groupe**, conformément au contrat MON15.2. Elle n'est pas attribuée intégralement à chaque personnage.

Historique :

```text
10 XP   = première valeur de validation MON1 / MON15.2, beaucoup trop lente avec la courbe finale
1000 XP = valeur temporaire utilisée pour accélérer les tests PIE MON15.5 / MON15.6
500 XP  = valeur de production MON15.7
```

Le C++ ne contient aucun cas particulier `RatGiant`; la valeur reste portée par le DataAsset.

---

## 4. Rythme de progression de référence

### Prototype actuel — un personnage actif

Avec 500 XP par Rat Géant :

```text
1 rat    500 XP  -> niveau 1
2 rats  1000 XP  -> niveau 2
6 rats  3000 XP  -> niveau 3
12 rats 6000 XP  -> niveau 4
20 rats 10000 XP -> niveau 5
```

Cela signifie :

```text
L1 -> L2 : 2 rats supplémentaires
L2 -> L3 : 4 rats supplémentaires
L3 -> L4 : 6 rats supplémentaires
L4 -> L5 : 8 rats supplémentaires
```

Le premier ennemi ne provoque donc plus automatiquement un Level Up, contrairement à la fixture 1000 XP.

### Groupe de référence — quatre personnages actifs

Le partage MON15.2 donne exactement :

```text
500 / 4 = 125 XP par personnage
```

Rythme équivalent si tous les XP provenaient uniquement de Rats Géants :

```text
8 rats   -> niveau 2
24 rats  -> niveau 3
48 rats  -> niveau 4
80 rats  -> niveau 5
```

Ces nombres représentent des **rat-equivalents**, pas une prescription de contenu. Les monstres plus dangereux, élites et boss devront fournir des récompenses supérieures.

---

## 5. Temps moyen par niveau

MON15.7 ne fige pas une durée en secondes dans le code. La durée réelle dépend encore de systèmes qui seront ajoutés avant MON22 : plusieurs familles de monstres, sorts, effets de statut, recrutement, composition du donjon et boss.

La cible de rythme du vertical slice est donc exprimée de façon robuste :

- le joueur solo de développement doit voir un premier Level Up après environ deux Rats Géants, pas après un seul ;
- un groupe complet ne doit pas monter de niveau après chaque rencontre mineure ;
- le niveau 2 doit rester atteignable tôt dans le slice ;
- le niveau 3 doit nécessiter plusieurs rencontres ou des récompenses de monstres plus importants ;
- le wall-clock réel sera mesuré pendant MON22 sur le slice 45–90 minutes.

Cette politique évite de coder aujourd'hui un objectif temporel qui deviendrait faux dès l'ajout de MON16–MON20.

---

## 6. Tests MON15.7

Nouvelle suite :

```text
Grimrock.RPG.MON15.7.FrozenCurve
Grimrock.RPG.MON15.7.SoloRatPacing
Grimrock.RPG.MON15.7.PartyRatPacing
Grimrock.RPG.MON15.7.ProductionRatAsset
```

### FrozenCurve

Verrouille les seuils de la courbe finale et le niveau maximum 20.

### SoloRatPacing

Verrouille les rat-equivalents cumulés :

```text
L2 = 2
L3 = 6
L4 = 12
L5 = 20
```

### PartyRatPacing

Verrouille le pool de 500 XP, sa divisibilité exacte par quatre et les rat-equivalents d'un groupe complet.

### ProductionRatAsset

Charge réellement :

```text
/Game/GrimrockPrototype/Monsters/RatGiant/Data/DA_MON_RatGiant
```

et exige :

```text
MonsterId = MON_RatGiant
ExperienceReward = 500
```

Ce test doit volontairement échouer tant que l'asset n'a pas été remis à sa valeur de production MON15.7.

---

## 7. Contenu UE à modifier manuellement

Le DataAsset est binaire et doit être modifié depuis Unreal Editor :

```text
DA_MON_RatGiant
  Monster | Rewards
    Experience Reward = 500
```

La valeur temporaire `1000` ne doit plus rester dans la copie de travail.

Après sauvegarde de l'asset, le fichier attendu est :

```text
Content/GrimrockPrototype/Monsters/RatGiant/Data/DA_MON_RatGiant.uasset
```

Ce changement `.uasset` est exceptionnellement indispensable : `ExperienceReward` est volontairement data-driven et ne doit pas être remplacé par un override C++.

---

## 8. Régressions finales attendues

Avant clôture MON15 :

```text
Grimrock.RPG.MON15
Grimrock.CharacterCreation.CC5
Grimrock.Monsters.MON9
Grimrock.Monsters.MON12.ActionCatalog
```

et, si la campagne globale reste raisonnable à exécuter :

```text
Grimrock.Monsters
```

Le PIE final doit confirmer :

- 1 rat en solo : 500 XP, pas de Level Up ;
- 2e rat : 1000 XP cumulés, Level Up 1 -> 2 ;
- la modal reste fonctionnelle ;
- après confirmation/annulation, le gameplay reprend ;
- Save -> Continue conserve le nouvel état.

---

## 9. Frontière de MON15.7

MON15.7 ne change pas :

- la formule de partage de l'XP ;
- le niveau maximum ;
- le SaveVersion ;
- les règles de recalcul de stats ;
- la progression de classe ;
- le pipeline de mort/loot ;
- les règles de combat.

Le jalon ne doit pas anticiper MON16 ou MON17.

---

## 10. Porte de sortie

MON15.7 et MON15 pourront être déclarés **VALIDÉS ET CLOS** lorsque :

- compilation UE5.5.4 réussie ;
- les 4 tests MON15.7 sont verts ;
- `ProductionRatAsset` confirme 500 XP ;
- la campagne `Grimrock.RPG.MON15` est verte ;
- les régressions CC5 / MON9 / MON12 ActionCatalog sont vertes ;
- le PIE 1 rat / 2 rats confirme le rythme attendu ;
- la documentation de synthèse MON15 est mise à jour ;
- `00_PROJECT_OVERVIEW.md`, `PROJECT_COMPLETION_ROADMAP.md` et `99_DECISIONS_LOG.md` reflètent la clôture.

Suite après clôture : **MON16 — Status Effects**.
