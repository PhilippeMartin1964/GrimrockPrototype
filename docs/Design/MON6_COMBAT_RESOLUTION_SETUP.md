# MON6 — Attaque de mêlée et résolution des dégâts

## 1. Objectif

MON6 raccorde la première attaque réelle du Rat géant au gestionnaire de tours MON5.

Le jalon ajoute :

- `FGridCombatResolver`, résolveur déterministe et indépendant des Actors ;
- `FGridPartyTargetSelector`, choix d'une cible vivante dans la formation ;
- `UGridMonsterCombatComponent`, composant natif de combat du monstre ;
- l'action `MeleeAttack` dans la file du TurnManager ;
- l'attaque `Attack_Bite` ;
- la réduction de l'armure physique puis des points de vie ;
- la défaite lorsque tous les personnages sont à zéro point de vie ;
- une graine aléatoire reproductible ;
- un impact unique, compatible avec un futur Anim Notify ;
- trois Automation Tests MON6.

Aucun asset d'animation n'est requis pour valider MON6. Tant que le montage de morsure n'est pas prêt, un timer applique automatiquement l'impact et termine l'action.

---

## 2. Architecture

```text
UGridTurnManagerComponent
    ↓ prépare MeleeAttack
UGridMonsterCombatComponent
    ↓ choisit la cible et applique l'impact
FGridCombatResolver
    ↓ résultat pur et déterministe
FGridCharacterInventoryState::DerivedStats
```

Les responsabilités restent séparées :

- le `TurnManager` décide quand le Rat agit et dépense les PA ;
- le `MonsterBehavior` fournit le chemin ;
- le `MonsterMovement` exécute les rotations et déplacements ;
- le `MonsterCombat` applique une attaque à la cible choisie ;
- le `CombatResolver` calcule le résultat sans modifier le monde.

---

## 3. Compilation

Fermer Unreal Engine puis compiler :

```text
Configuration : Development Editor
Plateforme    : Win64
Cible         : GrimrockPrototypeEditor
```

Une compilation complète est nécessaire, car MON6 ajoute un nouveau `UActorComponent` natif et étend plusieurs `USTRUCT`.

Après compilation, rouvrir Unreal Engine puis compiler et sauvegarder :

```text
BP_MON_RatGiant
BP_GridLevelRuntimeActor
BP_GrimrockPartyPawn
```

---

## 4. Composant natif du Rat

`AGridMonsterActor` crée automatiquement :

```text
MonsterCombat
```

Dans `BP_MON_RatGiant`, la hiérarchie doit donc contenir :

```text
BP_MON_RatGiant
├── SceneRoot
├── CollisionComponent
├── SkeletalMeshComponent
├── MonsterCombat          ← composant C++ hérité
├── MonsterMovement
└── MonsterBehavior
```

Ne pas ajouter un second `Grid Monster Combat Component` dans le Blueprint.

Réglages initiaux du composant hérité :

```text
Auto Initialize      = true
Front Line Slot Count = 3
```

Les indices de formation sont provisoirement interprétés ainsi :

```text
Première ligne : indices 0, 1 et 2
Seconde ligne  : indices 3 et suivants
```

Le Rat choisit aléatoirement, avec le flux déterministe du combat, un personnage vivant de première ligne. Il ne vise la seconde ligne que si aucun personnage de première ligne n'est vivant.

---

## 5. DataAsset du Rat géant

Ouvrir :

```text
DA_MON_RatGiant
```

### Statistiques

Conserver ou renseigner :

```text
Max Health             = 8
Physical Armor         = 0
Magical Armor          = 0
Initiative             = 12
Accuracy               = 2
Evasion                = 1
Action Points Per Turn = 2
```

### Attaque `Attack_Bite`

Dans le tableau `Attacks`, créer ou corriger l'entrée suivante :

```text
Attack Id            = Attack_Bite
Display Name         = Morsure
Damage Type          = Physical
Physical Subtype     = Piercing
Min Damage           = 1
Max Damage           = 4
Damage Bonus         = 1
Accuracy Bonus       = 0
Range Cells          = 1
Action Point Cost    = 1
Expected Duration    = 0.55
Impact Time Seconds  = 0.25
Attack Montage       = None
Impact Notify Name   = Monster.AttackImpact
Complete Notify Name = Monster.ActionComplete
```

Cette configuration représente :

```text
1d4 + 1 dégâts physiques perforants
```

`Attack Montage = None` est volontaire pour la première validation. Le timer de secours joue le rôle de l'animation.

---

## 6. TurnManager

Dans le composant `TurnManager` de `BP_GridLevelRuntimeActor`, conserver les réglages MON5 et vérifier :

```text
Encounter Random Seed = 1337
```

La même graine et le même état initial produisent la même séquence :

- choix de cible ;
- D20 ;
- dégâts.

Changer la graine permet de tester une autre séquence reproductible.

---

## 7. Formule MON6

### Jet d'attaque

```text
Jet total = D20 + Accuracy du monstre + Accuracy Bonus de l'attaque
Défense   = 10 + Evasion de la cible
```

Règles naturelles :

```text
D20 = 1  → échec automatique
D20 = 20 → réussite automatique et critique
```

Le critique double :

```text
jet de dégâts + Damage Bonus
```

### Dégâts

Ordre d'application :

```text
Dégâts bruts
    ↓
Damage Multiplier
    ↓
Résistance en pourcentage
    ↓
Armure physique ou magique
    ↓
Points de vie
```

Une attaque `Physical` consomme l'armure physique. Les autres types consomment l'armure magique.

Exemple :

```text
Morsure = 5 dégâts
Armure physique = 3
Santé = 10

Résultat :
Armure physique : 3 → 0
Santé           : 10 → 8
```

Les valeurs de `FGridDamageResistanceSet` sont désormais interprétées comme des pourcentages par le combat :

```text
50  = réduction de 50 %
0   = aucune modification
-50 = vulnérabilité de 50 %
```

Le résolveur limite la valeur entre `-100` et `100`.

---

## 8. Politique d'action du Rat

Avec deux PA et une morsure coûtant un PA :

### Rat adjacent

```text
Turn éventuel — 0 PA
MeleeAttack     — 1 PA
```

Le Rat n'effectue qu'une morsure dans cette première version, même s'il lui reste un PA après l'attaque.

### Rat à deux cellules

```text
Move        — 1 PA
MeleeAttack — 1 PA
```

### Rat à trois cellules ou davantage

```text
Move — 1 PA
Move — 1 PA
```

La morsure aura lieu pendant une manche suivante.

Un mur ou une porte fermée entre le Rat et le groupe interdit l'attaque, même lorsque les deux cellules sont voisines.

---

## 9. Premier test PIE — Rat adjacent

1. Placer le Rat sur une cellule adjacente au groupe.
2. Vérifier qu'aucun mur et qu'aucune porte fermée ne les sépare.
3. Lancer le PIE et cliquer dans le viewport.
4. Appuyer sur `NumPad 1` pour démarrer le combat.
5. Attendre `PlayerPhase`.
6. Appuyer sur `NumPad 2`.

Résultat attendu :

```text
EnemyPhase
→ Turn éventuel
→ MeleeAttack / Attack_Bite
→ impact vers 0.25 seconde
→ fin vers 0.55 seconde
→ EndingRound
→ PlayerPhase suivante
```

Le log doit contenir une ligne de la forme :

```text
[GridMonsterCombat] Attack Monster=... Attack=Attack_Bite Target=0 Name=... Natural=... Total=... Defense=... Hit=true/false Critical=true/false Raw=... ArmorPhysical=... Health=... HP=...->...
```

Un échec affiche `Hit=false` et n'enlève ni armure ni santé.

---

## 10. Vérifier les statistiques de la cible

Pendant le PIE, sélectionner :

```text
BP_GrimrockPartyPawn
→ PartyInventoryComponent
→ Party Inventory State
→ Active Characters
→ personnage ciblé
→ Derived Stats
```

Observer :

```text
Current Health
Physical Armor
Magical Armor
Evasion
```

Après une morsure réussie, l'armure physique diminue d'abord. La santé ne diminue que si les dégâts dépassent l'armure restante.

Le `TurnManager` expose aussi :

```text
Last Attack Result
Last Target Character Index
Active Attack Definition
Active Attack Impact Committed
```

`Active Attack Impact Committed` ne doit passer à `true` qu'une seule fois par attaque.

---

## 11. Test déplacement puis attaque

Placer le Rat à exactement deux cellules du groupe, dans un couloir libre.

Démarrer le combat puis terminer la phase du joueur.

Résultat attendu :

```text
Move d'une cellule
MeleeAttack
```

Le Rat dépense exactement deux PA.

À trois cellules ou davantage, il doit effectuer au maximum deux déplacements et ne pas mordre pendant la même phase ennemie.

---

## 12. Test de la formation

Préparer plusieurs personnages vivants.

### Première ligne disponible

Au moins un personnage parmi les indices `0..2` possède :

```text
Current Health > 0
```

Le log `Target=` doit toujours désigner l'un de ces indices.

### Première ligne vaincue

Mettre les personnages `0..2` à zéro point de vie et conserver un personnage vivant à l'indice `3` ou plus.

Le Rat doit alors sélectionner la seconde ligne.

### Défaite

Lorsque tous les personnages atteignent :

```text
Current Health = 0
```

le combat passe à :

```text
Defeat
```

et les entrées d'exploration sont restituées.

---

## 13. Impact unique

L'impact peut être demandé par :

- le timer de secours ;
- `NotifyActiveAttackImpact` ;
- un futur Anim Notify ;
- le timeout final de sécurité.

Le booléen interne `bActiveAttackImpactCommitted` empêche toute seconde application. Une morsure ne doit donc jamais soustraire deux fois les dégâts.

---

## 14. Raccordement futur d'un montage de morsure

Lorsque l'animation sera prête :

1. créer un `AnimMontage` de morsure ;
2. l'assigner à `Attack_Bite.Attack Montage` ;
3. placer un Notify à l'instant du contact ;
4. depuis l'Animation Blueprint, récupérer le `GridMonsterActor` propriétaire ;
5. récupérer son composant `MonsterCombat` ;
6. appeler :

```text
Notify Attack Impact
```

7. placer un second Notify à la fin et appeler :

```text
Notify Action Complete
```

Les timers restent actifs comme filet de sécurité. L'impact reste unique, même si le Notify et le timer arrivent presque simultanément.

Aucun montage n'est demandé pour valider MON6.

---

## 15. Tests automatisés

Dans :

```text
Tools
→ Session Frontend
→ Automation
```

lancer :

```text
Grimrock.Monsters.MON6.CombatResolver
Grimrock.Monsters.MON6.PartyTargetSelector
Grimrock.Monsters.MON6.DirectMeleePlanner
```

### CombatResolver

Vérifie :

- échec automatique sur 1 naturel ;
- réussite et critique sur 20 naturel ;
- absorption par l'armure physique ;
- absorption élémentaire par l'armure magique ;
- résistance positive ;
- vulnérabilité négative.

### PartyTargetSelector

Vérifie :

- priorité à la première ligne ;
- repli sur la seconde ligne ;
- absence de cible lorsque tout le groupe est vaincu.

### DirectMeleePlanner

Vérifie :

- rotation puis morsure lorsque le Rat est adjacent ;
- déplacement puis morsure à deux cellules ;
- deux déplacements sans morsure à trois cellules.

---

## 16. Filtres Output Log

```text
GridMonsterCombat
GridTurnManager
GridTurnManagerInput
```

---

## 17. Validation MON6

MON6 est validé lorsque :

1. le projet compile en `Development Editor / Win64` ;
2. `MonsterCombat` apparaît comme composant C++ hérité du Rat ;
3. `Attack_Bite` possède les valeurs indiquées ;
4. les trois tests automatisés réussissent ;
5. un Rat adjacent prépare `MeleeAttack` au lieu de `Wait` ;
6. la morsure coûte un PA ;
7. le Rat à deux cellules se déplace puis mord ;
8. le Rat plus éloigné ne dépasse pas deux déplacements ;
9. un mur ou une porte fermée empêche la morsure ;
10. le Rat préfère une cible vivante de première ligne ;
11. il utilise la seconde ligne lorsque la première est vaincue ;
12. le jet d'attaque et la défense apparaissent dans le log ;
13. un échec n'applique aucun dégât ;
14. l'armure physique absorbe les dégâts avant la santé ;
15. l'impact n'est appliqué qu'une fois ;
16. la phase ennemie continue après l'attaque ;
17. tous les personnages à zéro santé déclenchent `Defeat` ;
18. le comportement MON5 reste valide.

---

## 18. Suite

Le jalon suivant pourra traiter :

- les attaques des personnages contre les monstres ;
- les états `Hurt` et `Dead` avec animations ;
- l'expérience et la victoire automatique après la mort des Rats ;
- les résistances et vulnérabilités propres aux monstres ;
- les sons, VFX et Montages définitifs.
