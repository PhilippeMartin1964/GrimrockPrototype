# UI-RPG-DESC01.9 — Audit documentaire des 15 Talents du Mage

Date : **8 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
État : **VALIDÉ PAR L'UTILISATEUR — 8 octobre 2026**  
Dépendances : **UI-RPG-DESC01.5 / .6 / .7 / .8 validés**  
Périmètre : **Mage / 3 branches / 15 nœuds conceptuels / 21 Choice records concrets**

## 1. Objet

Traduire les 15 Talents conceptuels du Mage dans le contrat UX validé par
`UI_RPG_DESC01_5_TALENT_UX_CONTRACT.md`, sans modifier le gameplay.

Sources vérifiées :

```text
docs/Rules/RPG_Talents_Mechanics_v0_1.md
docs/Rules/RPG_Class_Progression_1_20_v0_1.md
docs/Design/RPG03_9_4D_MAGE_EVOKER_BRANCH.md
docs/Design/RPG03_9_4E_MAGE_ARCANIST_BRANCH.md
docs/Design/RPG03_9_4F2_MAGE_SURFACE_WEAVER_BRANCH.md
docs/Design/UI_RPG06_1_SKILLS_AUDIT_CATALOG_AUTHORING.md
Source/GrimrockPrototypeEditor/Private/RPG/RPGMageAuthoring.cpp
Source/GrimrockPrototypeEditor/Private/Tests/RPGRPG0394MageAuthoringTests.cpp
Source/GrimrockPrototype/Private/Tests/RPGRPG0394F1SurfaceWeaverSupportTests.cpp
```

Aucun C++, UMG ou DataAsset n'est modifié par ce jalon.

## 2. Synthèse de la classe

| Branche | Palier I | II | III | IV | V |
|---|---|---|---|---|---|
| **Évocateur** | Affinité élémentaire | Surcharge élémentaire | Explosion contrôlée | Chaîne élémentaire | Cataclysme |
| **Arcaniste** | Bouclier arcanique | Dissipation | Manipulation runique | Téléportation courte | Maîtrise de l'Arcane |
| **Tisseur de surfaces** | Imprégnation | Conversion élémentaire | Conduction | Surface persistante | Architecte du terrain |

Répartition UX proposée :

```text
ACTIF                   1
SORT ACTIF              8
PASSIF                  6
TOTAL                   15
```

Deux nœuds conceptuels possèdent des variantes :

```text
Affinité élémentaire
    -> Feu
    -> Glace
    -> Air
    -> Terre

Imprégnation
    -> Feu
    -> Glace
    -> Air
    -> Terre
```

Ces deux ensembles de variantes sont **indépendants**.

Nombre de Choice records :

```text
Évocateur            8
Arcaniste            5
Tisseur de surfaces  8
TOTAL               21
```

## 3. Règle commune de STATUT

Les fiches sont indépendantes d'un personnage concret.

```text
ACQUIS
DISPONIBLE
VERROUILLÉ — niveau X requis
VERROUILLÉ — nécessite « Talent X »
VERROUILLÉ — nécessite N point(s) de Talent
INDISPONIBLE — autre variante déjà choisie
```

Affinité élémentaire et Imprégnation utilisent chacune leur propre groupe
d'exclusivité. Une variante choisie dans l'une ne verrouille pas l'autre.

---

# 4. Branche ÉVOCATEUR

## 4.1 Affinité élémentaire — palier I / niveau 2

**TYPE**  
PASSIF

**PRINCIPE**  
Le Mage choisit une école élémentaire de prédilection. Les sorts appartenant à
cette école infligent davantage de dégâts. Le choix est exclusif dans la branche
Évocateur.

**EFFETS communs**

```text
Sorts de l'école choisie :
    dégâts infligés : +15 %
```

**VARIANTES**

### Feu

```text
École : Feu
Dégâts des sorts de Feu : +15 %
```

### Glace

```text
École : Glace
Dégâts des sorts de Glace : +15 %
```

### Air

```text
École : Air
Dégâts des sorts d'Air : +15 %
```

### Terre

```text
École : Terre
Dégâts des sorts de Terre : +15 %

« Terre » reste une école magique.
Elle n'introduit pas un nouveau type de dégâts.
```

**UTILISATION**  
Aucune : bonus passif conditionné par l'école du sort.

**ACQUISITION**

```text
Niveau requis : 2
Coût : 1 point de Talent
Prérequis : aucun
Exclusivité : une seule affinité Évocateur
```

**Audit source** : conforme. Les quatre variantes partagent un alias logique
commun utilisé par Surcharge élémentaire. Voir **MAGE-01** pour Terre et
**MAGE-02** pour l'indépendance avec Imprégnation.

---

## 4.2 Surcharge élémentaire — palier II / niveau 6

**TYPE**  
ACTIF

**PRINCIPE**  
Le Mage se surcharge brièvement d'énergie élémentaire. Son prochain sort
appartenant à son Affinité élémentaire inflige des dégâts supplémentaires puis
consomme la surcharge. Si aucun sort correspondant n'est lancé à temps, l'effet
expire à la fin du tour.

**EFFETS**

```text
Prochain sort correspondant à l'Affinité :
    dégâts infligés : +35 %

Affinité passive + Surcharge :
    bonus total possible sur ce sort : +50 %

Consommation :
    après résolution du premier sort correspondant

Durée maximale :
    1 tour
```

**UTILISATION**

```text
Coût : 1 point d'action
Mana : 4
Cible : soi-même
Recharge : 3 rounds
```

**ACQUISITION**

```text
Niveau requis : 6
Coût : 1 point de Talent
Prérequis : une variante d'Affinité élémentaire acquise
```

**Audit source** : conforme. L'action elle-même est une capacité d'activation,
pas un sort offensif ; le buff ne modifie que le prochain sort de l'école choisie.

---

## 4.3 Explosion contrôlée — palier III / niveau 10

**TYPE**  
PASSIF

**PRINCIPE**  
Le Mage maîtrise mieux les dégâts directs de ses propres sorts de zone afin de
protéger son groupe. Cette protection ne neutralise ni les surfaces ni les états
créés par ces sorts.

**EFFETS**

```text
Pour les sorts de zone du Mage :

Dégâts directs subis par le Mage lui-même :
    -100 % -> aucun dégât direct

Dégâts directs subis par ses alliés :
    -50 %

Surfaces créées :
    inchangées

États appliqués :
    inchangés
```

**UTILISATION**  
Aucune : bonus passif appliqué aux sorts de zone.

**ACQUISITION**

```text
Niveau requis : 10
Coût : 1 point de Talent
Prérequis : Surcharge élémentaire
```

**Audit source** : conforme. Le filtre exige simultanément une source Spell et un
ciblage Area.

---

## 4.4 Chaîne élémentaire — palier IV / niveau 14

**TYPE**  
SORT ACTIF

**PRINCIPE**  
Le Mage frappe une cible principale puis propage l'énergie élémentaire vers
jusqu'à deux autres ennemis proches. Chaque saut part de la cible précédente et
une même cible ne peut être touchée qu'une fois.

**EFFETS**

```text
Nombre maximal de cibles : 3
Distance maximale entre deux cibles successives : 1 case

Dégâts par cible :
    7
    + modificateur d'Intelligence
    + rang d'Arcane

Type de dégâts :
    Feu      -> Feu
    Glace    -> Glace
    Air      -> Foudre
    Terre    -> Physique

Jet pour toucher : aucun
Coup critique : impossible
```

**UTILISATION**

```text
Coût : 3 points d'action
Mana : 8
Cible primaire : ennemi à portée
Portée : 5 cases
Ligne de vue : requise
Recharge : 3 rounds
```

**ACQUISITION**

```text
Niveau requis : 14
Coût : 1 point de Talent
Prérequis : Explosion contrôlée
```

**Audit source** : conforme à l'interprétation déterministe Earth du runtime.
Voir **MAGE-01**.

---

## 4.5 Cataclysme — palier V / niveau 18

**TYPE**  
SORT ACTIF

**PRINCIPE**  
Le Mage déclenche une vaste explosion élémentaire autour d'une cellule visible.
Tous les ennemis de la zone subissent les dégâts élémentaires. La zone peut
également englober le groupe ; Explosion contrôlée, qui est un prérequis de cette
branche, protège alors le Mage des dégâts directs et réduit de moitié ceux de ses
alliés.

**EFFETS**

```text
Dégâts directs :
    12
    + modificateur d'Intelligence
    + rang d'Arcane

Type et contrôle selon l'Affinité :

Feu :
    dégâts de Feu
    si armure magique épuisée après dégâts :
        Brûlure pendant 2 tours

Glace :
    dégâts de Glace
    si armure magique épuisée après dégâts :
        Ralenti pendant 2 rounds
        Initiative : -6

Air :
    dégâts de Foudre
    si armure magique épuisée après dégâts :
        Étourdi pendant 1 tour

Terre :
    dégâts physiques
    si armure magique épuisée après dégâts :
        Immobilisé pendant 1 round

Jet pour toucher : aucun
Coup critique : impossible

Avec Explosion contrôlée :
    Mage : 0 % des dégâts directs alliés
    autres alliés : 50 % des dégâts directs normaux

Les surfaces et états éventuels ne sont pas réduits par Explosion contrôlée.
```

**UTILISATION**

```text
Coût : 4 points d'action
Mana : 16
Cellule cible : portée 5 cases
Zone : rayon 2 cases
Ligne de vue : requise
Recharge : 5 rounds
```

**ACQUISITION**

```text
Niveau requis : 18
Coût : 1 point de Talent
Prérequis : Chaîne élémentaire
```

**Audit source** : l'authoring teste explicitement le friendly fire de l'action.
Voir **MAGE-01** pour Terre et **MAGE-03** pour la formulation RPG02 « Chaque hostile ».

---

# 5. Branche ARCANISTE

## 5.1 Bouclier arcanique — palier I / niveau 2

**TYPE**  
SORT ACTIF

**PRINCIPE**  
Le Mage restaure immédiatement une partie de l'armure magique d'un membre vivant
du groupe, lui-même compris, sans pouvoir dépasser son armure magique de référence.

**EFFETS**

```text
Armure magique restaurée :
    6
    + modificateur d'Intelligence du Mage
    + rang d'Arcane

Maximum :
    armure magique de référence de la cible
```

**UTILISATION**

```text
Coût : 2 points d'action
Mana : 5
Cible : soi-même ou un allié
Portée : 3 cases
Recharge : 2 rounds
```

**ACQUISITION**

```text
Niveau requis : 2
Coût : 1 point de Talent
Prérequis : aucun
```

**Audit source** : conforme. Le ciblage Ally existant accepte une sélection
explicite du Mage lui-même.

---

## 5.2 Dissipation — palier II / niveau 6

**TYPE**  
SORT ACTIF

**PRINCIPE**  
Le Mage dissipe un effet magique amovible. Sur un allié, il retire un effet
négatif ; sur un ennemi, il retire un effet positif.

**EFFETS**

```text
Cible alliée :
    retire 1 effet négatif magique amovible

Cible ennemie :
    retire 1 effet positif magique amovible

Priorité :
    effet de puissance la plus élevée
    puis départage déterministe en cas d'égalité
```

Le détail technique de départage par identifiant interne n'appartient pas au texte
joueur.

**UTILISATION**

```text
Coût : 2 points d'action
Mana : 6
Cible : allié ou ennemi
Portée : 4 cases
Ligne de vue : requise
Recharge : 2 rounds
```

**ACQUISITION**

```text
Niveau requis : 6
Coût : 1 point de Talent
Prérequis : Bouclier arcanique
```

**Audit source** : conforme. Le terme « magique » signifie ici « marqué comme
dissipable par la primitive Dissipation », et non nécessairement « créé par un sort ».

---

## 5.3 Manipulation runique — palier III / niveau 10

**TYPE**  
PASSIF

**PRINCIPE**  
Le Mage comprend mieux les runes et les constructions arcaniques. Il bénéficie
d'un meilleur score dans ses interactions runiques et ses sorts arcaniques
infligent davantage de dégâts aux cibles associées aux runes ou aux constructions.

**EFFETS**

```text
Tests de Runes :
    +2

Sorts de l'école Arcane contre une cible Rune ou Construction :
    dégâts infligés : +20 %

Interactions de mécanisme runique :
    +2 au test
    aucune réussite automatique
```

**UTILISATION**  
Aucune : bonus passif.

**ACQUISITION**

```text
Niveau requis : 10
Coût : 1 point de Talent
Prérequis : Dissipation
```

**Audit source** : conforme. Le Skill de production **Runes** existe désormais
dans le catalogue UI-RPG06.2A.

---

## 5.4 Téléportation courte — palier IV / niveau 14

**TYPE**  
SORT ACTIF

**PRINCIPE**  
Le Mage téléporte instantanément tout le groupe vers une cellule proche visible,
libre et marchable. Il ne s'agit pas d'une translation normale sur la grille.

**EFFETS**

```text
Déplace : groupe entier
Distance maximale : 2 cases

Destination obligatoire :
    visible
    libre
    marchable

Ne traverse pas :
    mur solide
    porte fermée
    frontière de niveau

Ne déclenche pas automatiquement :
    téléporteur de cellule
    transition de niveau
    événement ordinaire de translation

PAM dépensé : 0
```

**UTILISATION**

```text
Coût : 3 points d'action
Mana : 8
Cible : cellule
Portée : 2 cases
Ligne de vue : requise
Recharge : 4 rounds
```

**ACQUISITION**

```text
Niveau requis : 14
Coût : 1 point de Talent
Prérequis : Manipulation runique
```

**Audit source** : conforme.

---

## 5.5 Maîtrise de l'Arcane — palier V / niveau 18

**TYPE**  
PASSIF

**PRINCIPE**  
Le Mage réduit le coût, augmente la portée et renforce les dégâts de tous ses
sorts appartenant à l'école Arcane.

**EFFETS**

```text
Sorts de l'école Arcane :

Coût en mana :
    -1
    minimum 1 si le sort possède normalement un coût positif
    un sort gratuit reste gratuit

Portée :
    +1 case
    maximum 32 cases

Dégâts infligés :
    +15 %
```

**UTILISATION**  
Aucune : bonus passif.

**ACQUISITION**

```text
Niveau requis : 18
Coût : 1 point de Talent
Prérequis : Téléportation courte
```

**Audit source** : conforme.

---

# 6. Branche TISSEUR DE SURFACES

## 6.1 Imprégnation — palier I / niveau 2

**TYPE**  
SORT ACTIF

**PRINCIPE**  
Le Mage choisit une affinité propre à la branche Tisseur de surfaces puis imprègne
l'arme d'un membre du groupe. La prochaine attaque d'arme réussie de la cible
inflige des dégâts élémentaires supplémentaires calculés avec l'Intelligence du
Mage qui a lancé Imprégnation.

**EFFETS communs**

```text
Durée maximale : 2 rounds
Déclenchement : prochaine attaque d'arme réussie
Après déclenchement : Imprégnation consommée

Dégâts supplémentaires :
    3 + modificateur d'Intelligence du Mage source
```

**VARIANTES**

### Feu

```text
Dégâts supplémentaires : Feu
```

### Glace

```text
Dégâts supplémentaires : Glace
```

### Air

```text
Dégâts supplémentaires : Foudre
```

### Terre

```text
Dégâts supplémentaires : Physiques
```

**UTILISATION**

```text
Coût : 1 point d'action
Mana : 4
Cible : soi-même ou un allié
Portée : 3 cases
Recharge : 1 round
```

**ACQUISITION**

```text
Niveau requis : 2
Coût : 1 point de Talent
Prérequis : aucun
Exclusivité : une seule affinité Tisseur
```

**Audit source** : conforme. L'affinité est celle du **Mage source**, pas celle de
la cible imprégnée. Voir **MAGE-01** et **MAGE-02**.

---

## 6.2 Conversion élémentaire — palier II / niveau 6

**TYPE**  
SORT ACTIF

**PRINCIPE**  
Le Mage transforme les surfaces d'une petite zone selon l'affinité choisie pour
sa branche Tisseur de surfaces.

**EFFETS**

```text
Affinité Feu :
    Huile -> Feu
    Poison -> Feu

Affinité Glace :
    Eau -> Glace

Affinité Air :
    Eau -> Eau électrifiée
    Sang -> Eau électrifiée

Affinité Terre :
    Eau -> Poison
    cellule neutre -> Huile

Conversion invalide :
    aucune transformation
```

**UTILISATION**

```text
Coût : 2 points d'action
Mana : 5
Cellule cible : portée 4 cases
Zone : rayon 1 case
Recharge : 2 rounds
```

**ACQUISITION**

```text
Niveau requis : 6
Coût : 1 point de Talent
Prérequis : une variante d'Imprégnation acquise
```

**Audit source** : conforme.

---

## 6.3 Conduction — palier III / niveau 10

**TYPE**  
PASSIF

**PRINCIPE**  
Les attaques élémentaires du Mage infligent davantage de dégâts lorsqu'elles
exploitent une surface ou un état compatible avec leur élément. Les contrôles
restent soumis à leurs règles normales de protection après les dégâts.

**EFFETS**

```text
Bonus de dégâts si interaction compatible :
    +20 %

Compatibilités Feu :
    Huile
    Poison
    Brûlure / état Feu

Compatibilités Glace :
    Eau
    Ralenti

Compatibilités Foudre :
    Eau
    Sang
    Eau électrifiée
    Étourdi

Compatibilités Terre :
    dégâts Physiques ou Poison d'un sort de l'école Terre
    avec Eau, Huile, Poison ou Immobilisé

Contrôles associés :
    ne sont pas créés par Conduction
    suivent l'application normale de l'attaque après les dégâts
```

**UTILISATION**  
Aucune : bonus passif conditionnel.

**ACQUISITION**

```text
Niveau requis : 10
Coût : 1 point de Talent
Prérequis : Conversion élémentaire
```

**Audit source** : conforme.

---

## 6.4 Surface persistante — palier IV / niveau 14

**TYPE**  
PASSIF

**PRINCIPE**  
Les surfaces créées ou converties par les sorts du Mage durent plus longtemps et
leurs dégâts périodiques sont renforcés. Les surfaces du décor ou d'un autre
auteur ne sont pas modifiées.

**EFFETS**

```text
Surfaces créées / converties par le Mage :

Durée :
    +2 rounds
    maximum 6 rounds

Dégâts périodiques :
    +15 %

Surfaces du décor :
    aucune modification

Surfaces créées par un autre personnage :
    aucune modification
```

**UTILISATION**  
Aucune : bonus passif.

**ACQUISITION**

```text
Niveau requis : 14
Coût : 1 point de Talent
Prérequis : Conduction
```

**Audit source** : conforme. Les modificateurs sont snapshotés au moment de la
création ou de la conversion de la surface.

---

## 6.5 Architecte du terrain — palier V / niveau 18

**TYPE**  
SORT ACTIF

**PRINCIPE**  
Le Mage crée une grande surface maîtrisée correspondant à l'affinité de sa branche
Tisseur. Les interactions ultérieures de cette surface utilisent ensuite les
règles normales du système de surfaces.

**EFFETS**

```text
Surface de base :
    durée : 3 rounds
    zone : rayon 2 cases

Selon l'affinité Tisseur :

Feu :
    Feu

Glace :
    Glace

Air :
    Eau électrifiée

Terre :
    Huile

Les réactions de surface ultérieures suivent les règles standards.
```

Avec Surface persistante, qui est un prérequis de branche :

```text
durée créée : 5 rounds
dégâts périodiques éventuels : +15 %
```

**UTILISATION**

```text
Coût : 4 points d'action
Mana : 12
Cellule cible : portée 5 cases
Zone : rayon 2 cases
Recharge : 5 rounds
```

**ACQUISITION**

```text
Niveau requis : 18
Coût : 1 point de Talent
Prérequis : Surface persistante
```

**Audit source** : conforme à la décision Earth actuelle. Voir **MAGE-01**.

---

# 7. Chaînes de progression validées

## Évocateur

```text
Affinité élémentaire [Feu / Glace / Air / Terre]
    -> Surcharge élémentaire
        -> Explosion contrôlée
            -> Chaîne élémentaire
                -> Cataclysme
```

Toutes les variantes d'Affinité accordent l'alias logique requis par Surcharge.

## Arcaniste

```text
Bouclier arcanique
    -> Dissipation
        -> Manipulation runique
            -> Téléportation courte
                -> Maîtrise de l'Arcane
```

## Tisseur de surfaces

```text
Imprégnation [Feu / Glace / Air / Terre]
    -> Conversion élémentaire
        -> Conduction
            -> Surface persistante
                -> Architecte du terrain
```

Toutes les variantes d'Imprégnation accordent l'alias logique requis par
Conversion élémentaire.

# 8. Concordance mécanique / authoring

| Talent | RPG02 vs authoring | Décision UX |
|---|---|---|
| Affinité élémentaire | Conforme | PASSIF + VARIANTES |
| Surcharge élémentaire | Conforme | ACTIF |
| Explosion contrôlée | Conforme | PASSIF |
| Chaîne élémentaire | Conforme | SORT ACTIF |
| Cataclysme | Conforme sauf ambiguïté Earth/friendly fire dans le résumé RPG02 | SORT ACTIF |
| Bouclier arcanique | Conforme | SORT ACTIF |
| Dissipation | Conforme | SORT ACTIF |
| Manipulation runique | Conforme | PASSIF |
| Téléportation courte | Conforme | SORT ACTIF |
| Maîtrise de l'Arcane | Conforme | PASSIF |
| Imprégnation | Conforme, 4 ChoiceIds | SORT ACTIF + VARIANTES |
| Conversion élémentaire | Conforme | SORT ACTIF |
| Conduction | Conforme | PASSIF |
| Surface persistante | Conforme | PASSIF |
| Architecte du terrain | Conforme à la décision Earth Oil | SORT ACTIF |

# 9. Arbitrages validés

## MAGE-01 — sémantique de l'affinité Terre

Il n'existe volontairement aucun `EGridDamageType::Earth`.

Le runtime actuel utilise une interprétation déterministe :

```text
Évocateur Terre :
    Affinité -> école Terre
    Chaîne élémentaire -> dégâts Physiques
    Cataclysme -> dégâts Physiques + Immobilisé

Tisseur Terre :
    Imprégnation -> dégâts Physiques
    Conversion -> Eau→Poison ; cellule neutre→Huile
    Architecte du terrain -> Huile
```

RPG02 laisse parfois entendre « Physique ou Poison selon le sort » et
« Huile/Poison » sans sous-choix concret.

**Décision validée :** canoniser le comportement runtime actuel pour la v0.1.

Cela évite d'inventer une seconde sélection Terre ou un nouveau DamageType. Un
futur sort explicitement Poison pourra toujours appartenir à l'école Terre.

## MAGE-02 — deux affinités indépendantes

Les groupes d'exclusivité sont distincts :

```text
Évocateur :
    TalentGroup_Mage_Evoker_ElementalAffinity

Tisseur :
    TalentGroup_Mage_SurfaceWeaver_ImbuementAffinity
```

Un même Mage peut donc légalement posséder :

```text
Affinité élémentaire — Feu
ET
Imprégnation — Glace
```

**Décision validée :** canoniser cette indépendance.

La future UI doit parler d'**affinité Évocateur** et d'**affinité Tisseur** lorsque
le contexte pourrait être ambigu, et ne jamais présenter une « affinité globale du
Mage ».

## MAGE-03 — Cataclysme et dégâts alliés

RPG02 résume Cataclysme par « Chaque hostile », mais l'action authorée possède :

```text
bAffectsAlliesInArea = true
```

Les tests Mage valident explicitement cette propriété.

Or Explosion contrôlée est un prérequis obligatoire plus tôt dans la même branche.
Le comportement effectif d'un Mage pouvant acquérir Cataclysme est donc :

```text
ennemis dans la zone :
    dégâts normaux

Mage dans la zone :
    0 dégât direct grâce à Explosion contrôlée

autres alliés dans la zone :
    50 % des dégâts directs grâce à Explosion contrôlée

surfaces / statuts :
    non réduits par Explosion contrôlée
```

**Décision validée :** canoniser le runtime actuel et corriger ultérieurement le résumé RPG02 « Chaque hostile » pour rendre le risque allié explicite.

# 10. Skills — état actuel corrigé

Contrairement aux documents historiques RPG03.9.4, le catalogue Skills n'est plus
une dépendance manquante.

`UI-RPG06.2A` a matérialisé **25 `URPGSkillAsset` de production**, dont :

```text
Arcane
Runes
Perception
Nature
Histoire
Religion
Survie
etc.
```

Pour le Mage :

```text
Chaîne élémentaire -> rang d'Arcane
Cataclysme -> rang d'Arcane
Bouclier arcanique -> rang d'Arcane
Manipulation runique -> Runes +2
```

Ces références disposent donc maintenant de leur catalogue de production.

Conséquence pour DESC01.12 : les mentions « catalogue Skills non matérialisé »
issues des documents historiques Voleur/Rôdeur doivent être réconciliées avec
l'état actuel de UI-RPG06.2A.

# 11. Vocabulaire Mage à normaliser

À employer :

```text
Feu
Glace
Air
Terre
Foudre
école magique
Arcane
Runes
Intelligence
rang d'Arcane
armure magique
Brûlure
Ralenti
Étourdi
Immobilisé
Huile
Poison
Eau
Sang
Eau électrifiée
ligne de vue
mana
points d'action
PAM
```

À ne pas exposer :

```text
Spell.School.*
Skill_Arcana
Skill_Runes
MagicalArmor
Status_*
DamageType
OwnerVariant
RequirementId
ChoiceId
ExclusiveChoiceGroupId
TargetingPolicy
Potency
EffectId lexical
bAffectsAlliesInArea
bRelocatePartyToTargetCell
```

Le mot **Arcana** des descriptions techniques doit devenir **Arcane** dans le texte
joueur, conformément au DisplayName du Skill de production.

# 12. Contrat de données attendu plus tard

DESC01.9 ne définit pas encore la structure C++ finale.

L'audit Mage montre que le futur read-model devra pouvoir représenter :

```text
type UX ACTIF / SORT ACTIF / PASSIF
variante exclusive fixe
deux familles de variantes indépendantes
école de sort
type de dégâts dérivé d'une variante
coûts PA + mana
portée / zone / LOS
dégâts directs avec scaling attribut + Skill
multi-cible en chaîne
friendly fire + réduction self/alliés
ArmorGate magique
retrait de Buff/Debuff
restauration d'armure magique
téléportation de groupe
conversion / création de surfaces
compatibilités surface/état
propriété de surface par auteur
snapshot des modificateurs de surface
acquisition
```

La forme C++ sera définie seulement en DESC01.13 après l'audit 90/90.

# 13. Critères de validation UI-RPG-DESC01.9

Le jalon a été validé par l'utilisateur le **8 octobre 2026**. Ont été approuvés :

- les 15 fiches Mage ;
- la classification **1 ACTIF / 8 SORTS ACTIFS / 6 PASSIFS** ;
- Affinité élémentaire comme **PASSIF + VARIANTES** ;
- Imprégnation comme **SORT ACTIF + VARIANTES** ;
- les deux groupes d'affinité comme indépendants ;
- Chaine/Cataclysme/Bouclier avec le Skill **Arcane** de production ;
- les trois chaînes de progression ;
- le vocabulaire joueur ;
- les décisions MAGE-01 à MAGE-03.

Après validation :

```text
UI-RPG-DESC01.10 — audit documentaire des 15 Talents du Prêtre
```
