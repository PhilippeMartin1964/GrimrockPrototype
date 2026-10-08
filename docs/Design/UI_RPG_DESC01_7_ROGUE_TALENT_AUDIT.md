# UI-RPG-DESC01.7 — Audit documentaire des 15 Talents du Voleur

Date : **8 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
État : **VALIDÉ PAR L'UTILISATEUR — 8 octobre 2026**  
Dépendances : **UI-RPG-DESC01.5 validé / UI-RPG-DESC01.6 validé**  
Périmètre : **Voleur / 3 branches / 15 nœuds conceptuels / 15 Choice records**

## 1. Objet

Traduire les 15 Talents du Voleur dans le contrat UX validé par
`UI_RPG_DESC01_5_TALENT_UX_CONTRACT.md`, sans modifier le gameplay.

Sources vérifiées :

```text
docs/Rules/RPG_Talents_Mechanics_v0_1.md
docs/Design/RPG03_9_2_ROGUE_AUTHORING.md
Source/GrimrockPrototypeEditor/Private/RPG/RPGRogueAuthoring.cpp
Source/GrimrockPrototypeEditor/Private/Tests/RPGRPG0392RogueAuthoringTests.cpp
Source/GrimrockPrototype/Private/Tests/RPGRPG0392SupportTests.cpp
Source/GrimrockPrototype/Private/Runtime/GridLevelRuntimeActorSurfaces.cpp
Source/GrimrockPrototype/Private/Runtime/GridLevelRuntimeActor.cpp
```

Aucun C++, UMG ou DataAsset n'est modifié par ce jalon.

## 2. Synthèse de la classe

| Branche | Palier I | II | III | IV | V |
|---|---|---|---|---|---|
| **Assassin** | Attaque sournoise | Frappe dans le dos | Hémorragie | Point faible | Mise à mort / Finisseur |
| **Ombre** | Esquive | Disparition courte | Pas de l'ombre | Insaisissable | Ombre parfaite |
| **Saboteur** | Désamorçage expert | Piège rapide | Bombe fumigène | Maître des serrures | Sabotage |

Répartition UX proposée :

```text
ACTIF                  11
PASSIF                  3
RÉACTION AUTOMATIQUE    1
TOTAL                   15
```

Aucun nœud Voleur n'utilise de variantes exclusives.

**Insaisissable** est classé **RÉACTION AUTOMATIQUE** dans l'UX, même si RPG02
l'appelle historiquement « Passive » : son effet est déclenché automatiquement
par la résolution d'Esquive, Disparition courte ou Pas de l'ombre.

## 3. Règle commune de STATUT

Comme pour le Guerrier, les fiches sont indépendantes d'un personnage concret.

```text
ACQUIS
DISPONIBLE
VERROUILLÉ — niveau X requis
VERROUILLÉ — nécessite « Talent X »
VERROUILLÉ — nécessite N point(s) de Talent
INDISPONIBLE — autre choix exclusif déjà effectué
```

Le Voleur n'a actuellement aucun nœud à variantes ; le dernier statut n'est donc
pas utilisé par ses 15 Talents v0.1.

---

# 4. Branche ASSASSIN

## 4.1 Attaque sournoise — palier I / niveau 2

**TYPE**  
ACTIF

**PRINCIPE**  
Le Voleur porte une attaque avec une arme légère. Elle devient beaucoup plus
dangereuse lorsqu'il prend sa cible de vitesse, l'attaque alors qu'elle est sous
contrôle physique, ou l'attaque depuis son arc arrière.

**EFFETS**

```text
Dégâts de base : 125 % des dégâts de l'arme

Si au moins une condition suivante est vraie :
    la cible n'a pas encore agi ce round
    OU la cible subit un contrôle physique
    OU le groupe se trouve dans l'arc arrière de la cible

Dégâts : 175 % des dégâts de l'arme

Les conditions ne se cumulent pas entre elles.
```

**UTILISATION**

```text
Coût : 2 points d'action
Cible : première cible dans l'axe
Portée : 1 case
Recharge : 1 round
Condition : arme légère
```

**ACQUISITION**

```text
Niveau requis : 2
Coût : 1 point de Talent
Prérequis : aucun
```

**Audit source** : conforme. Le bonus conditionnel est un unique +50 points au
coefficient d'arme dès qu'au moins une des trois conditions est satisfaite.

---

## 4.2 Frappe dans le dos — palier II / niveau 6

**TYPE**  
PASSIF

**PRINCIPE**  
Les attaques de mêlée du Voleur avec une arme légère deviennent plus efficaces
lorsque le groupe se trouve dans l'arc arrière de la cible.

**EFFETS**

```text
Condition :
    attaque de mêlée
    arme légère
    cible attaquée depuis son arc arrière
    action non-AoE

Dégâts infligés : +20 %
Chance de critique : +20 points de pourcentage
```

**UTILISATION**  
Aucune : bonus passif conditionnel.

**ACQUISITION**

```text
Niveau requis : 6
Coût : 1 point de Talent
Prérequis : Attaque sournoise
```

**Audit source** : conforme.

---

## 4.3 Hémorragie — palier III / niveau 10

**TYPE**  
ACTIF

**PRINCIPE**  
Le Voleur effectue une attaque tranchante ou perforante destinée à faire saigner
une cible dont la protection physique est épuisée.

**EFFETS**

```text
Attaque : 100 % des dégâts de l'arme
Armes admises : tranchantes ou perforantes

Si l'armure physique est épuisée après l'attaque :
    Saignement pendant 3 tours
    Dégâts périodiques : 2 dégâts physiques par tick
    Cumul : 1 maximum
    Réapplication : rafraîchit la durée
```

**UTILISATION**

```text
Coût : 2 points d'action
Cible : première cible dans l'axe
Portée : 1 case
Recharge : 2 rounds
```

**ACQUISITION**

```text
Niveau requis : 10
Coût : 1 point de Talent
Prérequis : Frappe dans le dos
```

**Audit source** : conforme. L'application de Saignement passe par l'ArmorGate
physique après une attaque réussie.

---

## 4.4 Point faible — palier IV / niveau 14

**TYPE**  
ACTIF

**PRINCIPE**  
Le Voleur exploite une cible dont l'armure physique est déjà entièrement détruite
afin de la rendre plus vulnérable aux dégâts physiques.

**EFFETS**

```text
Dégâts directs : aucun
Condition : armure physique de la cible = 0

Exposé physiquement pendant 2 rounds :
    dégâts physiques reçus : +20 %
```

**UTILISATION**

```text
Coût : 1 point d'action
Cible : un ennemi
Portée : 1 case
Recharge : 3 rounds
```

**ACQUISITION**

```text
Niveau requis : 14
Coût : 1 point de Talent
Prérequis : Hémorragie
```

**Audit source** : conforme.

---

## 4.5 Mise à mort / Finisseur — palier V / niveau 18

**TYPE**  
ACTIF

**PRINCIPE**  
Le Voleur tente d'achever une cible déjà privée de son armure physique et
gravement blessée. La capacité ne se déclenche jamais automatiquement.

**EFFETS**

```text
Conditions de cible :
    armure physique = 0
    PV <= 30 % des PV maximum

Dégâts : 220 % des dégâts de l'arme
Si la cible survit : aucun effet secondaire automatique
```

**UTILISATION**

```text
Coût : 3 points d'action
Cible : première cible dans l'axe
Portée : 1 case
Recharge : 4 rounds
```

**ACQUISITION**

```text
Niveau requis : 18
Coût : 1 point de Talent
Prérequis : Point faible
```

**Audit source** : mécanique conforme. Voir **ROGUE-01** pour le nom canonique.

---

# 5. Branche OMBRE

## 5.1 Esquive — palier I / niveau 2

**TYPE**  
ACTIF

**PRINCIPE**  
Le Voleur adopte brièvement une posture d'esquive améliorée.

**EFFETS**

```text
Esquive : +4
Durée : 1 round
Cumul : non
Réapplication : rafraîchit la durée
```

**UTILISATION**

```text
Coût : 1 point d'action
Cible : soi-même
Recharge : 3 rounds
```

**ACQUISITION**

```text
Niveau requis : 2
Coût : 1 point de Talent
Prérequis : aucun
```

**Audit source** : conforme.

---

## 5.2 Disparition courte — palier II / niveau 6

**TYPE**  
ACTIF

**PRINCIPE**  
Le Voleur devient temporairement caché. Tant que l'effet tient, les attaques
hostiles ciblées ne peuvent pas le sélectionner directement. L'effet cesse à sa
prochaine activation ou dès qu'il résout sa première action offensive.

**EFFETS**

```text
Ciblage hostile direct : bloqué

Ne protège pas contre :
    attaques de zone
    dégâts périodiques
    surfaces

Fin de l'effet :
    prochaine activation du Voleur
    OU première action offensive résolue
```

**UTILISATION**

```text
Coût : 2 points d'action
Cible : soi-même
Recharge : 4 rounds
```

**ACQUISITION**

```text
Niveau requis : 6
Coût : 1 point de Talent
Prérequis : Esquive
```

**Audit source** : conforme. Le statut Caché utilise une expiration à la prochaine
activation et une consommation sur action offensive.

---

## 5.3 Pas de l'ombre — palier III / niveau 10

**TYPE**  
ACTIF

**PRINCIPE**  
Le Voleur prépare sa prochaine attaque de mêlée avec une arme légère en augmentant
temporairement son allonge. L'effet est consommé par cette attaque.

**EFFETS**

```text
Prochaine attaque compatible :
    attaque de mêlée
    arme légère
    portée : +1 case

Consommation :
    après résolution de la prochaine attaque de mêlée offensive avec arme légère
```

**UTILISATION**

```text
Coût : 1 point d'action
Cible : soi-même
Recharge : 2 rounds
Durée authorée : 1 tour
```

**ACQUISITION**

```text
Niveau requis : 10
Coût : 1 point de Talent
Prérequis : Disparition courte
```

**Audit source** : le +1 case et la consommation sont authorés/testés. RPG02 ajoute
explicitement « peut être exécutée depuis le rang arrière » ; voir **ROGUE-02**.

---

## 5.4 Insaisissable — palier IV / niveau 14

**TYPE**  
RÉACTION AUTOMATIQUE

**PRINCIPE**  
Après la réussite d'une des trois techniques d'évasion de la branche — Esquive,
Disparition courte ou Pas de l'ombre — le Voleur gagne automatiquement un bref
bonus défensif et d'initiative.

**EFFETS**

```text
Déclencheurs :
    Esquive
    Disparition courte
    Pas de l'ombre

Esquive : +2
Initiative : +4
Durée : 1 round
Cumul : non
```

**UTILISATION**  
Aucune : réaction automatique à la résolution des trois actions listées.

**ACQUISITION**

```text
Niveau requis : 14
Coût : 1 point de Talent
Prérequis : Pas de l'ombre
```

**Audit source** : conforme. Le classement UX « RÉACTION AUTOMATIQUE » reflète
mieux son fonctionnement réel que l'ancien libellé générique « Passive ».

---

## 5.5 Ombre parfaite — palier V / niveau 18

**TYPE**  
ACTIF

**PRINCIPE**  
Le Voleur entre dans une furtivité renforcée pendant 2 rounds. Tant que l'effet
tient, il ne peut pas être sélectionné directement par une attaque hostile.
Sa première action offensive bénéficie d'un puissant bonus puis rompt l'effet.
Recevoir des dégâts directs rompt également l'effet après résolution.

**EFFETS**

```text
Ciblage hostile direct : bloqué
Durée maximale : 2 rounds

Première action offensive :
    dégâts infligés : +50 %
    Précision : +2
    puis l'effet prend fin

Dégâts directs reçus :
    l'effet prend fin après leur résolution
```

**UTILISATION**

```text
Coût : 2 points d'action
Cible : soi-même
Recharge : 5 rounds
```

**ACQUISITION**

```text
Niveau requis : 18
Coût : 1 point de Talent
Prérequis : Insaisissable
```

**Audit source** : conforme.

---

# 6. Branche SABOTEUR

## 6.1 Désamorçage expert — palier I / niveau 2

**TYPE**  
PASSIF

**PRINCIPE**  
Le Voleur devient plus efficace pour détecter/désamorcer les pièges et peut
transformer certains petits échecs en échecs sans conséquence immédiate.

**EFFETS**

```text
Jets de Pièges : +2

Marge d'échec sûr : 2
    un échec de 1 ou 2 points est signalé comme « échec sûr »
```

**UTILISATION**  
Aucune : bonus appliqué aux tests de compétence concernés.

**ACQUISITION**

```text
Niveau requis : 2
Coût : 1 point de Talent
Prérequis : aucun
```

**Audit source** : le +2 et la marge d'échec sûr sont authorés/testés. RPG02 ajoute
« une fois par piège » et « le piège reste armé sans se déclencher » ; voir
**ROGUE-03**.

---

## 6.2 Piège rapide — palier II / niveau 6

**TYPE**  
ACTIF

**PRINCIPE**  
Le Voleur pose un piège temporaire dans une cellule proche. Le premier ennemi qui
entre dans cette cellule déclenche le piège, puis celui-ci disparaît.

**EFFETS**

```text
Durée du piège : 3 rounds
Déclenchement : premier ennemi entrant

Dégâts :
    6 + modificateur de Dextérité
    type : physique, perforant

Si l'armure physique est épuisée après les dégâts :
    Immobilisé pendant 1 round
    -> déplacements volontaires bloqués

Après déclenchement :
    piège consommé
```

**UTILISATION**

```text
Coût : 2 points d'action
Cible : cellule
Portée : 1 case
Recharge : 2 rounds
```

**ACQUISITION**

```text
Niveau requis : 6
Coût : 1 point de Talent
Prérequis : Désamorçage expert
```

**Audit source** : conforme. Le piège temporaire est un état runtime de niveau
sauvegardable.

---

## 6.3 Bombe fumigène — palier III / niveau 10

**TYPE**  
ACTIF

**PRINCIPE**  
Le Voleur crée une zone de fumée qui gêne les lignes de tir et rend les occupants
plus difficiles à toucher à distance.

**EFFETS**

```text
Surface : fumée
Durée : 2 rounds
Zone : rayon 1 case

Fumée strictement entre tireur et cible :
    bloque la ligne de vue ciblée à distance

Cible occupant une cellule de fumée :
    Esquive : +2 contre les attaques à distance

Ne rend pas les AoE, dégâts périodiques ou surfaces non ciblables.
```

**UTILISATION**

```text
Coût : 2 points d'action
Cellule cible : portée 3 cases
Zone : rayon 1 case
Recharge : 3 rounds
```

**ACQUISITION**

```text
Niveau requis : 10
Coût : 1 point de Talent
Prérequis : Piège rapide
```

**Audit source** : conforme. Le runtime possède une règle générique de fumée pour
les attaques à distance et fournit aussi l'état de ligne de vue aux sorts ciblés.

---

## 6.4 Maître des serrures — palier IV / niveau 14

**TYPE**  
PASSIF

**PRINCIPE**  
Le Voleur améliore ses tests de Crochetage et compte comme légèrement plus
expérimenté pour les prérequis accordés par cette compétence.

**EFFETS**

```text
Jets de Crochetage : +2

Rang effectif pour les prérequis de Crochetage :
    +1
    maximum : rang 5

Marge d'échec sûr : 2
    un échec de 1 ou 2 points est signalé comme « échec sûr »
```

**UTILISATION**  
Aucune : bonus appliqué aux tests et prérequis de Crochetage.

**ACQUISITION**

```text
Niveau requis : 14
Coût : 1 point de Talent
Prérequis : Bombe fumigène
```

**Audit source** : projection conforme. RPG02 précise qu'un petit échec « ne
bloque/jamme jamais la serrure » ; voir **ROGUE-03**.

---

## 6.5 Sabotage — palier V / niveau 18

**TYPE**  
ACTIF

**PRINCIPE**  
Le Voleur tente de saboter une créature mécanique ou un mécanisme du monde.
La tentative s'appuie sur sa compétence Mécanique et la difficulté propre à la
cible.

**EFFETS — combat**

```text
Cibles admises :
    Mechanical
    Construct

Test :
    Intelligence + Mécanique
    contre la difficulté de la cible

En cas de succès :
    Saboté pendant 2 rounds
    Précision : -2
    Initiative : -4

En cas d'échec :
    l'action est dépensée
    aucun effet Saboté n'est appliqué
```

**EFFETS — hors combat**

```text
Un objet explicitement sabotable possède sa propre difficulté.
Après réussite du test, il reçoit l'événement générique « Saboté ».
Les conséquences sont définies par ses liens / sa logique de niveau.
```

**UTILISATION**

```text
Coût : 2 points d'action en combat
Cible combat : Mechanical ou Construct
Portée : 1 case
Recharge : 3 rounds
```

**ACQUISITION**

```text
Niveau requis : 18
Coût : 1 point de Talent
Prérequis : Maître des serrures
```

**Audit source** : primitives de combat et de monde présentes. Leur disponibilité
réelle dépend du catalogue de compétences de production ; voir **ROGUE-04**.

---

# 7. Chaînes de progression validées

## Assassin

```text
Attaque sournoise
    -> Frappe dans le dos
        -> Hémorragie
            -> Point faible
                -> Mise à mort / Finisseur
```

## Ombre

```text
Esquive
    -> Disparition courte
        -> Pas de l'ombre
            -> Insaisissable
                -> Ombre parfaite
```

## Saboteur

```text
Désamorçage expert
    -> Piège rapide
        -> Bombe fumigène
            -> Maître des serrures
                -> Sabotage
```

# 8. Concordance mécanique / authoring

| Talent | RPG02 vs authoring | Décision UX |
|---|---|---|
| Attaque sournoise | Conforme | ACTIF |
| Frappe dans le dos | Conforme | PASSIF |
| Hémorragie | Conforme | ACTIF |
| Point faible | Conforme | ACTIF |
| Mise à mort / Finisseur | Mécanique conforme, nom divergent | ACTIF |
| Esquive | Conforme | ACTIF |
| Disparition courte | Conforme | ACTIF |
| Pas de l'ombre | +1 portée conforme ; clause rang arrière à clarifier | ACTIF |
| Insaisissable | Mécanique conforme ; RPG02 dit Passive | RÉACTION AUTOMATIQUE |
| Ombre parfaite | Conforme | ACTIF |
| Désamorçage expert | Primitive Skill conforme ; conséquence métier incomplète | PASSIF |
| Piège rapide | Conforme | ACTIF |
| Bombe fumigène | Conforme | ACTIF |
| Maître des serrures | Primitive Skill conforme ; conséquence métier incomplète | PASSIF |
| Sabotage | Primitives conformes ; dépendance Skills production | ACTIF |

Aucune nouvelle mécanique n'est inventée dans les fiches ci-dessus.

# 9. Arbitrages et dépendances validés

## ROGUE-01 — « Mise à mort » ou « Finisseur »

RPG02 nomme le Talent :

```text
Mise à mort
```

L'authoring et le DataAsset utilisent :

```text
Finisseur
```

La mécanique est identique.

**Décision validée :** retenir **Mise à mort** comme nom joueur canonique. L'authoring « Finisseur » sera harmonisé ultérieurement lors de la phase d'implémentation.

## ROGUE-02 — Pas de l'ombre et « rang arrière »

RPG02 dit que la prochaine attaque légère de mêlée peut être exécutée « depuis le
rang arrière » et gagne +1 case de portée.

L'authoring/runtime vérifié encode clairement :

```text
arme légère
attaque de mêlée
portée +1 case
consommation après l'attaque
```

Aucune règle spécifique de sortie « front row / rear row » n'a été identifiée dans
ce chemin d'attaque ; le modèle de rang sert surtout au ciblage des membres du
groupe par les ennemis et à certains effets comme Interception.

**Décision validée :** pour la v0.1, la fiche joueur retient uniquement « prochaine attaque de mêlée avec arme légère : portée +1 case ». La mention historique « depuis le rang arrière » n'ajoute pas de règle runtime supplémentaire ; elle pourra être réévaluée si un vrai système de restriction de rang est introduit.

## ROGUE-03 — échecs sûrs Pièges / Crochetage

Le moteur de Skill Check sait produire :

```text
SafeFailureMargin = 2
bSafeFailure = true
```

pour un échec de 1 ou 2 points.

Mais la documentation d'authoring précise explicitement que **le consommateur
métier** doit décider de la conséquence. Le moteur générique ne connaît pas une
serrure ou un piège particulier.

RPG02 ajoute deux promesses métier :

```text
Désamorçage expert :
    une fois par piège, le piège reste armé mais ne se déclenche pas

Maître des serrures :
    la serrure ne bloque/jamme jamais sur un échec sûr
```

Ces conséquences ne sont pas établies par le seul `FRPGSkillProgressionModifier`.

**Décision validée :** conserver ces règles comme **comportement cible RPG02**, mais les reporter au futur runtime métier Pièges/Crochetage. L'UI ne devra pas les présenter comme garanties tant que ce branchement n'existe pas.

## ROGUE-04 — catalogue Skills de production

Le document RPG03.9.2 indique que le dépôt ne contient pas encore de
`URPGSkillAsset` de production pour :

```text
Pièges
Crochetage
Mécanique
```

Les contrats sont testés avec des définitions transitoires.

Conséquences :

- Désamorçage expert a son modificateur, mais dépend du futur Skill Pièges ;
- Maître des serrures dépend du futur Skill Crochetage ;
- Sabotage de combat dépend du futur Skill Mécanique pour résoudre réellement son
  test Intelligence + Mécanique ;
- le sabotage d'objet de monde possède déjà les primitives
  `bCanBeSabotaged / SabotageDifficulty / Sabotaged`, mais le caller doit encore
  fournir le Skill Check.

**Décision validée :** ne pas masquer cette dépendance. Elle reste inscrite pour DESC01.12 et reliée explicitement à l'effort global **Skills / Audit données compétences**.

# 10. Vocabulaire Voleur à normaliser

À employer :

```text
arme légère
arc arrière
Précision
Esquive
Initiative
armure physique
dégâts physiques
dégâts bruts
PV / PV maximum
Saignement
Exposé physiquement
Caché
Allonge de l'ombre
Insaisissable
Ombre parfaite
Immobilisé
Saboté
Pièges
Crochetage
Mécanique
fumée
```

À ne pas exposer :

```text
Weapon.Light
RearArc
Accuracy
Evasion
InitiativeModifier
PhysicalArmor
RawDamage
MaxHP
Status_*
Skill_*
Trap_Quick
Surface_Smoke
RequirementGrant
SafeFailureMargin
bSafeFailure
bBlockDirectHostileTargeting
```

Le terme technique « échec sûr » peut rester dans la documentation de règle, mais
la fiche joueur finale devra l'expliquer en langage métier lorsque le comportement
Pièges/Crochetage sera réellement branché.

# 11. Lien avec l'effort Skills

DESC01.7 ne crée aucun Skill et ne modifie pas RPG-SKILL01.

Il révèle néanmoins une dépendance à ne pas perdre :

```text
Talents Voleur
    -> Désamorçage expert -> Skill Pièges
    -> Maître des serrures -> Skill Crochetage
    -> Sabotage -> Skill Mécanique
```

Cette dépendance doit être reportée dans DESC01.12 afin que le chantier Talent ne
prétende pas être fonctionnel indépendamment du catalogue de compétences.

# 12. Contrat de données attendu plus tard

Comme DESC01.6, ce jalon ne définit pas encore la structure C++ finale.

Les 15 fiches montrent que le futur read-model devra pouvoir représenter au moins :

```text
type UX
principe
effets conditionnels
utilisation
conditions d'équipement
conditions de cible
statuts appliqués
effets périodiques
surfaces
pièges
skill checks
effets hors combat
acquisition
```

La forme C++ ne sera définie qu'en DESC01.13 après l'audit 90/90.

# 13. Critères de validation UI-RPG-DESC01.7

Le jalon a été validé par l'utilisateur le **8 octobre 2026**. Ont été approuvés :

- les 15 fiches Voleur ;
- la classification **11 ACTIF / 3 PASSIF / 1 RÉACTION AUTOMATIQUE** ;
- Insaisissable comme RÉACTION AUTOMATIQUE ;
- les trois chaînes de progression ;
- le vocabulaire joueur ;
- les décisions/reports ROGUE-01 à ROGUE-04.

Après validation :

```text
UI-RPG-DESC01.8 — audit documentaire des 15 Talents du Rôdeur
```
