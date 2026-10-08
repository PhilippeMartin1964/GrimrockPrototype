# UI-RPG-DESC01.6 — Audit documentaire des 15 Talents du Guerrier

Date : **8 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
État : **DRAFT — validation utilisateur requise**  
Dépendance : **UI-RPG-DESC01.5 validé**  
Périmètre : **Guerrier / 3 branches / 15 nœuds conceptuels / 17 Choice records concrets**

## 1. Objet

Traduire les 15 Talents du Guerrier dans le contrat UX validé par
`UI_RPG_DESC01_5_TALENT_UX_CONTRACT.md`, sans modifier le gameplay.

Sources vérifiées :

```text
docs/Rules/RPG_Talents_Mechanics_v0_1.md
Source/GrimrockPrototypeEditor/Private/RPG/RPGWarriorAuthoring.cpp
Source/GrimrockPrototypeEditor/Private/Tests/RPGRPG0391WarriorAuthoringTests.cpp
docs/Design/RPG03_4_TRIGGER_REACTION_PROFILE.md
Source/GrimrockPrototype/Private/Runtime/Combat/GridTurnManagerReactions.cpp
```

Aucun C++, UMG ou DataAsset n'est modifié par ce jalon.

## 2. Synthèse de la classe

| Branche | Palier I | II | III | IV | V |
|---|---|---|---|---|---|
| **Gardien** | Posture défensive | Coup de bouclier | Interception | Rempart | Forteresse |
| **Brise-ligne** | Coup puissant | Brise-armure | Balayage | Exécution | Ravage |
| **Maître d'armes** | Spécialisation martiale | Riposte | Second souffle | Maîtrise critique | Seigneur de guerre |

Répartition UX proposée :

```text
ACTIF                  10
PASSIF                  3
RÉACTION AUTOMATIQUE    2
TOTAL                   15
```

Un seul nœud conceptuel possède des variantes :

```text
Spécialisation martiale
    -> Tranchant
    -> Perforant
    -> Contondant
```

Le runtime authoré contient donc 17 Choice records pour 15 nœuds conceptuels.

## 3. Règle commune de STATUT

Les fiches ci-dessous définissent le contenu **indépendamment du personnage**.

La section STATUT est toujours calculée à l'exécution selon DESC01.5 :

```text
ACQUIS
DISPONIBLE
VERROUILLÉ — niveau X requis
VERROUILLÉ — nécessite « Talent X »
VERROUILLÉ — nécessite N point(s) de Talent
INDISPONIBLE — autre variante déjà choisie
```

L'audit ne fige donc jamais un statut particulier pour Elias ou pour un niveau donné.

---

# 4. Branche GARDIEN

## 4.1 Posture défensive — palier I / niveau 2

**TYPE**  
ACTIF

**PRINCIPE**  
Le Guerrier adopte une posture défensive pendant 2 rounds. Réutiliser la capacité
avant son expiration rafraîchit sa durée ; les bonus ne se cumulent pas.

**EFFETS**

```text
Dégâts physiques reçus : -20 %
Esquive : +2
Dégâts d'arme infligés : -10 %
Durée : 2 rounds
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

**Audit source** : conforme RPG02 / authoring. L'effet est porté par le statut
joueur « Garde », avec rafraîchissement de durée.

---

## 4.2 Coup de bouclier — palier II / niveau 6

**TYPE**  
ACTIF

**PRINCIPE**  
Le Guerrier frappe la première cible située dans l'axe avec son bouclier.
La capacité exige qu'un bouclier soit équipé.

**EFFETS**

```text
Dégâts : 80 % des dégâts de l'arme
Type : physique, contondant
Si l'armure physique de la cible est épuisée après les dégâts :
    Étourdi pendant 1 tour
    -> la prochaine activation de la cible est perdue
```

**UTILISATION**

```text
Coût : 2 points d'action
Cible : première cible dans l'axe
Portée : 1 case
Recharge : 2 rounds
Condition : bouclier équipé
```

**ACQUISITION**

```text
Niveau requis : 6
Coût : 1 point de Talent
Prérequis : Posture défensive
```

**Audit source** : conforme. L'ArmorGate physique est explicitement testé.

---

## 4.3 Interception — palier III / niveau 10

**TYPE**  
RÉACTION AUTOMATIQUE

**PRINCIPE**  
Une fois par round, Interception se déclenche lorsqu'un autre allié du rang avant
est touché par une attaque physique ciblée. Le Guerrier doit lui-même être au rang
avant. Les attaques de zone, dégâts périodiques et surfaces ne passent pas par ce
chemin d'interception.

**EFFETS**

```text
50 % des dégâts finaux sont retirés aux dégâts subis par l'allié.
Ces dégâts redirigés sont appliqués au Guerrier.
Ils sont d'abord absorbés par sa propre armure physique, puis par ses PV.
```

**UTILISATION**  
Aucune : réaction automatique, sans coût en points d'action.

**ACQUISITION**

```text
Niveau requis : 10
Coût : 1 point de Talent
Prérequis : Coup de bouclier
```

**Audit source** : cœur mécanique conforme. Voir **ARBITRAGE WARRIOR-01** pour le
cas de plusieurs Guerriers éligibles.

---

## 4.4 Rempart — palier IV / niveau 14

**TYPE**  
PASSIF

**PRINCIPE**  
Le Guerrier tire davantage de protection de son équipement défensif. Le bonus
augmente son pool d'armure physique de référence ; il ne crée pas une seconde
réserve d'armure parallèle.

**EFFETS**

```text
Armure physique de référence provenant de l'équipement et du bouclier : +25 %
```

**UTILISATION**  
Aucune : bonus passif permanent tant que le Talent est acquis.

**ACQUISITION**

```text
Niveau requis : 14
Coût : 1 point de Talent
Prérequis : Interception
```

**Audit source** : conforme. Le modifier authoré est
`PhysicalArmorReferencePercentModifier = 25`.

---

## 4.5 Forteresse — palier V / niveau 18

**TYPE**  
ACTIF

**PRINCIPE**  
Le Guerrier consolide immédiatement la ligne de front puis la protège pendant
2 rounds.

**EFFETS**

```text
Pour chaque allié vivant du rang avant :
    restaure 40 % de son armure physique de référence
    applique Fortifié pendant 2 rounds

Fortifié :
    dégâts physiques reçus : -25 %
```

**UTILISATION**

```text
Coût : 3 points d'action
Cible : alliés vivants du rang avant
Recharge : 5 rounds
```

**ACQUISITION**

```text
Niveau requis : 18
Coût : 1 point de Talent
Prérequis : Rempart
```

**Audit source** : conforme. L'action cible explicitement le rang avant et la
restauration de 40 % du pool de référence est testée.

---

# 5. Branche BRISE-LIGNE

## 5.1 Coup puissant — palier I / niveau 2

**TYPE**  
ACTIF

**PRINCIPE**  
Le Guerrier effectue une attaque lourde plus puissante mais moins précise.
Une arme lourde est requise.

**EFFETS**

```text
Dégâts : 150 % des dégâts de l'arme
Précision : -2
Coup critique : autorisé
Contrôle supplémentaire : aucun
```

**UTILISATION**

```text
Coût : 3 points d'action
Cible : première cible dans l'axe
Portée : 1 case
Recharge : 1 round
Condition : arme lourde équipée
```

**ACQUISITION**

```text
Niveau requis : 2
Coût : 1 point de Talent
Prérequis : aucun
```

**Audit source** : conforme. Le tag d'arme lourde et le malus de Précision sont
authorés séparément mais convergent vers la même mécanique.

---

## 5.2 Brise-armure — palier II / niveau 6

**TYPE**  
ACTIF

**PRINCIPE**  
Le Guerrier porte une attaque normale puis ajoute un choc spécialement destiné à
détruire l'armure physique de la cible.

**EFFETS**

```text
Attaque principale : 100 % des dégâts de l'arme
Dégâts supplémentaires contre l'armure physique :
    50 % des dégâts bruts de l'attaque principale
L'excédent de ces dégâts d'armure ne déborde jamais sur les PV.
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
Niveau requis : 6
Coût : 1 point de Talent
Prérequis : Coup puissant
```

**Audit source** : conforme. L'effet secondaire cible exclusivement le pool
d'armure physique.

---

## 5.3 Balayage — palier III / niveau 10

**TYPE**  
ACTIF

**PRINCIPE**  
Le Guerrier balaie les ennemis regroupés autour d'une cellule proche. Chaque
ennemi est résolu comme une attaque indépendante.

**EFFETS**

```text
Chaque ennemi de la zone :
    85 % des dégâts de l'arme
    jet d'attaque indépendant
Alliés : aucun dégât de zone
```

**UTILISATION**

```text
Coût : 3 points d'action
Cellule cible : portée 1 case
Zone : rayon 1 case autour de la cellule cible
Recharge : 2 rounds
```

**ACQUISITION**

```text
Niveau requis : 10
Coût : 1 point de Talent
Prérequis : Brise-armure
```

**Audit source** : conforme. `bAffectsAlliesInArea=false`.

---

## 5.4 Exécution — palier IV / niveau 14

**TYPE**  
ACTIF

**PRINCIPE**  
Le Guerrier tente d'achever une cible déjà gravement affaiblie. La capacité n'est
utilisable que lorsque la cible a perdu toute son armure physique et ne possède
plus que 35 % de ses PV maximum ou moins.

**EFFETS**

```text
Dégâts : 200 % des dégâts de l'arme
Déclenchement automatique : non
```

**UTILISATION**

```text
Coût : 2 points d'action
Cible : première cible dans l'axe
Portée : 1 case
Recharge : 2 rounds
Conditions de cible :
    armure physique = 0
    PV <= 35 % des PV maximum
```

**ACQUISITION**

```text
Niveau requis : 14
Coût : 1 point de Talent
Prérequis : Balayage
```

**Audit source** : conforme. Les conditions sont authorées dans le filtre de cible
et sont réévaluées à la requête.

---

## 5.5 Ravage — palier V / niveau 18

**TYPE**  
ACTIF

**PRINCIPE**  
Le Guerrier frappe violemment tous les ennemis d'une petite zone. La cible
principale peut être mise à terre si son armure physique est déjà épuisée après
les dégâts.

**EFFETS**

```text
Tous les ennemis de la zone :
    140 % des dégâts de l'arme

Cible principale uniquement, si armure physique = 0 après les dégâts :
    À terre pendant 1 tour
    -> la prochaine activation est perdue

Alliés : aucun dégât de zone
```

**UTILISATION**

```text
Coût : 4 points d'action
Cellule cible : portée 1 case
Zone : rayon 1 case autour de la cellule cible
Recharge : 5 rounds
```

**ACQUISITION**

```text
Niveau requis : 18
Coût : 1 point de Talent
Prérequis : Exécution
```

**Audit source** : conforme. Le statut À terre est limité à la cible primaire et
utilise l'ArmorGate physique.

---

# 6. Branche MAÎTRE D'ARMES

## 6.1 Spécialisation martiale — palier I / niveau 2

**TYPE**  
PASSIF

**PRINCIPE**  
Le Guerrier choisit définitivement une spécialisation parmi trois familles
d'armes physiques. Le bonus s'applique uniquement lorsqu'il utilise une arme du
type choisi.

**EFFETS communs**

```text
Précision : +1
Dégâts finaux de l'arme : +10 %
```

**VARIANTES**

### Tranchant

```text
Condition : arme infligeant des dégâts physiques tranchants
Précision : +1
Dégâts finaux de l'arme : +10 %
```

### Perforant

```text
Condition : arme infligeant des dégâts physiques perforants
Précision : +1
Dégâts finaux de l'arme : +10 %
```

### Contondant

```text
Condition : arme infligeant des dégâts physiques contondants
Précision : +1
Dégâts finaux de l'arme : +10 %
```

**UTILISATION**  
Aucune : bonus passif conditionné par l'arme utilisée.

**ACQUISITION**

```text
Niveau requis : 2
Coût : 1 point de Talent
Prérequis : aucun
Exclusivité : une seule variante peut être acquise
```

**Audit source** : conforme. Trois ChoiceIds exclusifs accordent tous le même
prérequis logique « Spécialisation martiale » pour la suite de la branche.

---

## 6.2 Riposte — palier II / niveau 6

**TYPE**  
RÉACTION AUTOMATIQUE

**PRINCIPE**  
Une fois par round, lorsqu'une attaque de mêlée ciblée contre le Guerrier échoue,
le Guerrier effectue immédiatement une contre-attaque contre son assaillant.
Une Riposte ne peut pas déclencher une nouvelle Riposte.

**EFFETS**

```text
Contre-attaque : 75 % des dégâts de l'arme
Portée de réaction : 1 case
Coût en points d'action : 0
Fréquence : 1 fois par round
Récursion : interdite
```

**UTILISATION**  
Aucune : réaction automatique.

**ACQUISITION**

```text
Niveau requis : 6
Coût : 1 point de Talent
Prérequis : une variante de Spécialisation martiale acquise
```

**Audit source** : le sens « l'ennemi manque le Guerrier » est confirmé par le
pipeline runtime des attaques de monstre. Voir **ARBITRAGE WARRIOR-02** concernant
l'autorisation actuelle des mains nues.

---

## 6.3 Second souffle — palier III / niveau 10

**TYPE**  
ACTIF

**PRINCIPE**  
Le Guerrier récupère immédiatement une partie de ses propres PV. La capacité ne
restaure ni mana ni armure.

**EFFETS**

```text
PV restaurés : 20 % des PV maximum
Arrondi : supérieur
Minimum restauré : 1 PV
Mana restauré : 0
Armure restaurée : 0
```

**UTILISATION**

```text
Coût : 1 point d'action
Cible : soi-même
Recharge : 4 rounds
Condition spécifiée par RPG02 : indisponible à PV maximum
```

**ACQUISITION**

```text
Niveau requis : 10
Coût : 1 point de Talent
Prérequis : Riposte
```

**Audit source** : restauration 20 % et recharge 4 authorées et testées. Voir
**ARBITRAGE WARRIOR-03** pour la condition « indisponible à PV maximum ».

---

## 6.4 Maîtrise critique — palier IV / niveau 14

**TYPE**  
PASSIF

**PRINCIPE**  
Le Guerrier perfectionne la spécialisation martiale choisie au palier I. Le bonus
ne s'applique qu'aux attaques correspondant à cette spécialisation.

**EFFETS**

```text
Chance de critique : +10 points de pourcentage
Multiplicateur de dégâts critiques : +25 points de pourcentage
Condition : arme correspondant à la spécialisation martiale acquise
```

**UTILISATION**  
Aucune : bonus passif conditionnel.

**ACQUISITION**

```text
Niveau requis : 14
Coût : 1 point de Talent
Prérequis : Second souffle
```

**Audit source** : conforme. Trois profils conditionnels existent, un par ChoiceId
de spécialisation, sans créer trois Talents conceptuels supplémentaires.

---

## 6.5 Seigneur de guerre — palier V / niveau 18

**TYPE**  
ACTIF

**PRINCIPE**  
Le Guerrier galvanise tous les alliés actifs pendant 2 rounds. Réutiliser l'effet
rafraîchit sa durée ; il ne se cumule pas avec lui-même.

**EFFETS**

```text
Tous les alliés actifs :
    Précision : +2
    Initiative : +4
Durée : 2 rounds
Cumul : non
Réapplication : rafraîchit la durée
```

**UTILISATION**

```text
Coût : 2 points d'action
Cible : tous les alliés actifs
Recharge : 4 rounds
```

**ACQUISITION**

```text
Niveau requis : 18
Coût : 1 point de Talent
Prérequis : Maîtrise critique
```

**Audit source** : conforme. Le statut authoré porte bien Précision +2 et
Initiative +4.

---

# 7. Chaînes de progression validées

## Gardien

```text
Posture défensive
    -> Coup de bouclier
        -> Interception
            -> Rempart
                -> Forteresse
```

## Brise-ligne

```text
Coup puissant
    -> Brise-armure
        -> Balayage
            -> Exécution
                -> Ravage
```

## Maître d'armes

```text
Spécialisation martiale
    -> Riposte
        -> Second souffle
            -> Maîtrise critique
                -> Seigneur de guerre
```

La première transition de Maître d'armes utilise un RequirementId logique commun :
n'importe laquelle des trois spécialisations satisfait le prérequis de Riposte.

# 8. Concordance mécanique / authoring

| Talent | RPG02 vs authoring | Décision UX |
|---|---|---|
| Posture défensive | Conforme | ACTIF |
| Coup de bouclier | Conforme | ACTIF |
| Interception | Conforme sur le cas nominal | RÉACTION AUTOMATIQUE |
| Rempart | Conforme | PASSIF |
| Forteresse | Conforme | ACTIF |
| Coup puissant | Conforme | ACTIF |
| Brise-armure | Conforme | ACTIF |
| Balayage | Conforme | ACTIF |
| Exécution | Conforme | ACTIF |
| Ravage | Conforme | ACTIF |
| Spécialisation martiale | Conforme, 3 ChoiceIds | PASSIF + VARIANTES |
| Riposte | Conforme sur le cas nominal | RÉACTION AUTOMATIQUE |
| Second souffle | Effet principal conforme ; disponibilité à PV max à confirmer | ACTIF |
| Maîtrise critique | Conforme | PASSIF |
| Seigneur de guerre | Conforme | ACTIF |

Aucune nouvelle mécanique n'est inventée dans les fiches ci-dessus.

# 9. Arbitrages ouverts

## WARRIOR-01 — plusieurs Intercepteurs

**RPG02** dit qu'Interception redirige 50 % des dégâts lorsqu'un allié du rang
avant est touché.

**Runtime actuel** : si plusieurs personnages possèdent une Interception éligible,
le code choisit **un seul intercepteur déterministe** puis s'arrête.

Décision requise avant DESC01.12 :

- **A — canoniser le comportement actuel** : une seule Interception peut répondre à
  une même attaque ;
- **B — changer ultérieurement la mécanique**.

Tant que ce point n'est pas arbitré, la fiche joueur ne doit pas mentionner le cas
multi-Guerriers.

## WARRIOR-02 — Riposte à mains nues

**RPG02** spécifie une contre-attaque à 75 % des dégâts de l'arme mais ne précise
pas le comportement sans arme.

**Authoring actuel** contient :

```text
bAllowUnarmed = true
```

Décision requise :

- **A — autoriser officiellement la Riposte à mains nues** et le documenter ;
- **B — exiger une arme équipée** et corriger ultérieurement l'authoring.

Aucune formulation joueur ne doit trancher ce point avant décision.

## WARRIOR-03 — Second souffle à PV maximum

**RPG02** spécifie explicitement :

```text
Indisponible à PV max.
```

L'authoring de l'action encode la restauration de 20 % et la recharge, mais aucun
filtre dédié à « PV < PV maximum » n'a été identifié dans la définition du Talent
ou son test d'authoring.

Décision / vérification requise avant DESC01.12 :

- confirmer qu'un garde-fou runtime générique interdit déjà l'action à PV maximum ;
- sinon enregistrer un écart mécanique à corriger dans un ticket ultérieur.

L'audit conserve la règle RPG02 comme comportement cible.

# 10. Vocabulaire Guerrier à normaliser

Le futur texte joueur doit employer :

```text
Précision
Esquive
armure physique
dégâts bruts
PV / PV maximum
Étourdi
À terre
Garde
Fortifié
points d'action
round / tour selon l'unité gameplay réelle
```

À ne pas exposer :

```text
Accuracy
Evasion
PhysicalArmor
RawDamage
MaxHP
Status_Guarded
Status_Stunned
Status_Fortified
Status_KnockedDown
Status_Warlord
bSkipActivation
ChoiceId / RequirementId
```

# 11. Contrat de données attendu plus tard

DESC01.6 **ne définit pas encore le read-model**. Il identifie seulement ce qu'il
faudra pouvoir projeter à partir des données autoritaires :

```text
nom
type UX
statut runtime
principe
effets structurés
utilisation structurée
acquisition structurée
variantes éventuelles
```

La forme C++ de cette projection sera décidée seulement en DESC01.13 après les
six audits de classe et la normalisation croisée DESC01.12.

# 12. Critères de validation UI-RPG-DESC01.6

Le jalon est validé lorsque l'utilisateur approuve :

- les 15 fiches Guerrier ;
- la classification 10 ACTIF / 3 PASSIF / 2 RÉACTION AUTOMATIQUE ;
- la présentation de Spécialisation martiale comme PASSIF + VARIANTES ;
- les trois chaînes de progression ;
- le vocabulaire joueur ;
- les trois arbitrages WARRIOR-01 à WARRIOR-03, ou leur report explicite à
  DESC01.12.

Après validation :

```text
UI-RPG-DESC01.7 — audit documentaire des 15 Talents du Voleur
```
