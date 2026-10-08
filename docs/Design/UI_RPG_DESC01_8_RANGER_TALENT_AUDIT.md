# UI-RPG-DESC01.8 — Audit documentaire des 15 Talents du Rôdeur

Date : **8 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
État : **VALIDÉ PAR L'UTILISATEUR — 8 octobre 2026**  
Dépendances : **UI-RPG-DESC01.5 / .6 / .7 validés**  
Périmètre : **Rôdeur / 3 branches / 15 nœuds conceptuels / 14 + N catégories de bestiaire Choice records**

## 1. Objet

Traduire les 15 Talents conceptuels du Rôdeur dans le contrat UX validé par
`UI_RPG_DESC01_5_TALENT_UX_CONTRACT.md`, sans modifier le gameplay.

Sources vérifiées :

```text
docs/Rules/RPG_Talents_Mechanics_v0_1.md
docs/Design/RPG03_9_3_RANGER_AUTHORING.md
Source/GrimrockPrototypeEditor/Private/RPG/RPGRangerAuthoring.cpp
Source/GrimrockPrototypeEditor/Private/Tests/RPGRPG0393RangerAuthoringTests.cpp
Source/GrimrockPrototype/Private/Tests/RPGRPG0393SupportTests.cpp
Source/GrimrockPrototype/Private/Runtime/Combat/GridTurnManagerReactions.cpp
Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.cpp
Source/GrimrockPrototype/Private/Runtime/Combat/GridTurnManagerPartyMovement.cpp
Source/GrimrockPrototype/Private/Runtime/Combat/GridTurnManagerInitiative.cpp
```

Aucun C++, UMG ou DataAsset n'est modifié par ce jalon.

## 2. Synthèse de la classe

| Branche | Palier I | II | III | IV | V |
|---|---|---|---|---|---|
| **Tireur** | Tir précis | Tir perforant | Tir rapide | Volée | Œil d'aigle |
| **Chasseur** | Marque de la proie | Ennemi juré | Tir immobilisant | Frappe du prédateur | Chasseur alpha |
| **Éclaireur** | Vigilance | Piège de chasse | Repli tactique | Maître du terrain | Guide du groupe |

Répartition UX proposée :

```text
ACTIF                   9
PASSIF                  5
RÉACTION AUTOMATIQUE    1
TOTAL                   15
```

**Chasseur alpha** est classé **RÉACTION AUTOMATIQUE** dans l'UX, même si RPG02
l'appelle historiquement « Passive », car son effet est déclenché automatiquement
à la mort de la propre cible marquée du Rôdeur.

Un seul nœud conceptuel possède des variantes :

```text
Ennemi juré
    -> une variante exclusive par CategoryId réelle du bestiaire
```

Le nombre de Choice records concret est donc :

```text
14 + nombre de catégories de monstres présentes dans le bestiaire de production
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

Pour **Ennemi juré**, chaque catégorie est un Choice concret exclusif, mais toutes
les catégories doivent rester visibles dans la section VARIANTES.

---

# 4. Branche TIREUR

## 4.1 Tir précis — palier I / niveau 2

**TYPE**  
ACTIF

**PRINCIPE**  
Le Rôdeur effectue un tir puissant et précis avec une arme à distance. La portée
de la capacité est calculée à partir de la portée réelle de l'arme.

**EFFETS**

```text
Dégâts : 150 % des dégâts de l'arme
Précision : +2
Portée : portée de l'arme +2 cases
Portée maximale globale : 32 cases
```

**UTILISATION**

```text
Coût : 3 points d'action
Cible : première cible dans l'axe
Recharge : 1 round
Condition : arme à distance équipée
```

**ACQUISITION**

```text
Niveau requis : 2
Coût : 1 point de Talent
Prérequis : aucun
```

**Audit source** : conforme. Le profil utilise explicitement la portée de l'arme
comme autorité avant le clamp global.

---

## 4.2 Tir perforant — palier II / niveau 6

**TYPE**  
ACTIF

**PRINCIPE**  
Le Rôdeur tire un projectile perforant qui endommage normalement la cible tout en
infligeant un choc supplémentaire exclusivement à son armure physique.

**EFFETS**

```text
Dégâts principaux : 110 % des dégâts de l'arme
Type : physique, perforant

Dégâts supplémentaires à l'armure physique :
    50 % des dégâts bruts de l'attaque principale
    aucun débordement vers les PV
```

**UTILISATION**

```text
Coût : 2 points d'action
Cible : première cible dans l'axe
Portée : portée de l'arme à distance
Recharge : 2 rounds
Condition : arme à distance équipée
```

**ACQUISITION**

```text
Niveau requis : 6
Coût : 1 point de Talent
Prérequis : Tir précis
```

**Audit source** : conforme.

---

## 4.3 Tir rapide — palier III / niveau 10

**TYPE**  
ACTIF

**PRINCIPE**  
Le Rôdeur enchaîne deux tirs indépendants contre la même cible. Le second tir est
un peu moins précis. Si le premier abat la cible, le second n'est pas exécuté.

**EFFETS**

```text
Premier tir :
    65 % des dégâts de l'arme
    résolution indépendante

Second tir :
    65 % des dégâts de l'arme
    Précision : -1
    résolution indépendante

Les deux tirs peuvent infliger un coup critique.
```

**UTILISATION**

```text
Coût : 2 points d'action
Cible : première cible dans l'axe
Portée : portée de l'arme à distance
Recharge : 2 rounds
Condition : arme à distance équipée
```

**ACQUISITION**

```text
Niveau requis : 10
Coût : 1 point de Talent
Prérequis : Tir perforant
```

**Audit source** : conforme. `ResolutionCount=2` et le malus de la résolution
suivante sont authorés.

---

## 4.4 Volée — palier IV / niveau 14

**TYPE**  
ACTIF

**PRINCIPE**  
Le Rôdeur vise une cellule à distance et tire sur tous les ennemis présents dans
la petite zone autour de cette cellule. Chaque cible reçoit sa propre résolution
d'attaque.

**EFFETS**

```text
Chaque ennemi de la zone :
    80 % des dégâts de l'arme
    résolution d'attaque indépendante

Alliés : non affectés
Statut supplémentaire : aucun
```

**UTILISATION**

```text
Coût : 3 points d'action
Cellule cible : portée 5 cases
Zone : rayon 1 case
Recharge : 3 rounds
Ligne de vue : requise vers la cellule cible
Obstacles et fumée bloquante : pris en compte avant dépense
```

**ACQUISITION**

```text
Niveau requis : 14
Coût : 1 point de Talent
Prérequis : Tir rapide
```

**Audit source** : conforme.

---

## 4.5 Œil d'aigle — palier V / niveau 18

**TYPE**  
PASSIF

**PRINCIPE**  
Le Rôdeur améliore globalement ses attaques à distance et ses observations
effectuées dans un contexte à distance.

**EFFETS**

```text
Actions à distance :
    Portée : +1 case
    Précision : +1

Tests de Perception liés à un contexte à distance :
    +2

Les limites globales de portée restent applicables.
```

**UTILISATION**  
Aucune : bonus passif.

**ACQUISITION**

```text
Niveau requis : 18
Coût : 1 point de Talent
Prérequis : Volée
```

**Audit source** : projection conforme. La partie Perception dépend du catalogue
Skills de production ; voir **RANGER-02**.

---

# 5. Branche CHASSEUR

## 5.1 Marque de la proie — palier I / niveau 2

**TYPE**  
ACTIF

**PRINCIPE**  
Le Rôdeur désigne un ennemi comme sa proie personnelle. Il ne peut maintenir
qu'une seule marque personnelle à la fois : marquer une nouvelle cible retire sa
marque précédente. Plusieurs Rôdeurs peuvent en revanche marquer le même ennemi.

**EFFETS**

```text
Durée de la marque : 3 rounds
Protection requise : aucune

Contre la propre cible marquée du Rôdeur :
    Précision : +2
    Dégâts infligés : +15 %

Une nouvelle marque du même Rôdeur remplace l'ancienne.
Les marques de Rôdeurs différents peuvent coexister sur la même cible.
```

**UTILISATION**

```text
Coût : 1 point d'action
Cible : un ennemi
Portée : 5 cases
Recharge : 1 round
```

**ACQUISITION**

```text
Niveau requis : 2
Coût : 1 point de Talent
Prérequis : aucun
```

**Audit source** : conforme. Le statut est distinct par source et unique par
source à travers les monstres.

---

## 5.2 Ennemi juré — palier II / niveau 6

**TYPE**  
PASSIF

**PRINCIPE**  
Le Rôdeur choisit définitivement une catégorie de créatures comme ennemi juré.
Les catégories disponibles proviennent du bestiaire de production : elles ne sont
pas codées en dur dans le Talent.

**EFFETS communs à chaque variante**

```text
Contre la catégorie choisie :
    Dégâts infligés : +15 %

Tests contextuellement liés à cette catégorie :
    Perception : +2
    Nature : +2
    Histoire : +2
    Religion : +2
```

**VARIANTES**

Une variante exclusive est créée pour chaque `CategoryId` réelle du bestiaire.

Exemple conceptuel :

```text
VERMINE
Dégâts contre cette catégorie : +15 %
Perception / Nature / Histoire / Religion liés à cette catégorie : +2

GOBELINS
Dégâts contre cette catégorie : +15 %
Perception / Nature / Histoire / Religion liés à cette catégorie : +2

[etc. selon le bestiaire réellement authoré]
```

Toutes les variantes sont visibles simultanément selon le contrat DESC01.5.

**UTILISATION**  
Aucune : bonus passif conditionné par la catégorie de la cible.

**ACQUISITION**

```text
Niveau requis : 6
Coût : 1 point de Talent
Prérequis : Marque de la proie
Exclusivité : une seule catégorie d'Ennemi juré
```

**Audit source** : mécanique conforme. La liste des Choice records suit
automatiquement l'Asset Registry du bestiaire. Voir **RANGER-01** pour les libellés
joueur et **RANGER-02** pour la dépendance Skills.

---

## 5.3 Tir immobilisant — palier III / niveau 10

**TYPE**  
ACTIF

**PRINCIPE**  
Le Rôdeur effectue un tir normal qui peut immobiliser une cible dont l'armure
physique est épuisée après l'impact.

**EFFETS**

```text
Dégâts : 100 % des dégâts de l'arme

Si l'armure physique est épuisée après les dégâts :
    Immobilisé pendant 1 round
    déplacements volontaires bloqués
```

**UTILISATION**

```text
Coût : 2 points d'action
Cible : première cible dans l'axe
Portée : portée de l'arme à distance
Recharge : 2 rounds
Condition : arme à distance équipée
```

**ACQUISITION**

```text
Niveau requis : 10
Coût : 1 point de Talent
Prérequis : une variante d'Ennemi juré acquise
```

**Audit source** : conforme. Le prérequis utilise l'alias logique commun accordé
par toutes les variantes d'Ennemi juré.

---

## 5.4 Frappe du prédateur — palier IV / niveau 14

**TYPE**  
ACTIF

**PRINCIPE**  
Le Rôdeur porte un tir puissant uniquement contre la cible qu'il a lui-même
marquée. La marque reste active après l'attaque.

**EFFETS**

```text
Condition :
    cible portant la propre Marque de la proie du Rôdeur

Dégâts : 170 % des dégâts de l'arme
Précision : +1
Marque consommée : non
```

**UTILISATION**

```text
Coût : 3 points d'action
Cible : première cible dans l'axe, si elle porte la propre marque
Portée : portée de l'arme à distance
Recharge : 2 rounds
Condition : arme à distance équipée
```

**ACQUISITION**

```text
Niveau requis : 14
Coût : 1 point de Talent
Prérequis : Tir immobilisant
```

**Audit source** : conforme. Le filtre exige explicitement une marque issue du
lanceur de l'action.

---

## 5.5 Chasseur alpha — palier V / niveau 18

**TYPE**  
RÉACTION AUTOMATIQUE

**PRINCIPE**  
Lorsqu'une cible portant la propre marque du Rôdeur meurt, Chasseur alpha peut
transférer automatiquement cette marque vers l'ennemi vivant le plus proche.

**EFFETS**

```text
Déclenchement : mort de la propre cible marquée
Fréquence : 1 fois par round
Coût en points d'action : 0

Nouvelle cible :
    ennemi vivant le plus proche
    distance maximale : 3 cases depuis la cible vaincue

Nouvelle durée de la marque : 2 rounds

Si aucune cible éligible n'existe dans le rayon :
    aucun transfert
```

En cas d'égalité de distance, le runtime applique un départage déterministe
interne ; ce détail n'a pas à être exposé au joueur.

**UTILISATION**  
Aucune : réaction automatique.

**ACQUISITION**

```text
Niveau requis : 18
Coût : 1 point de Talent
Prérequis : Frappe du prédateur
```

**Audit source** : conforme.

---

# 6. Branche ÉCLAIREUR

## 6.1 Vigilance — palier I / niveau 2

**TYPE**  
PASSIF

**PRINCIPE**  
Tant que le Rôdeur est vivant, sa vigilance améliore les tests de Perception du
groupe. Lui-même commence également chaque combat avec un bonus d'initiative.

**EFFETS**

```text
Meilleur test de groupe de Perception :
    +2

Initiative du propriétaire :
    +2 au premier round de chaque combat

Plusieurs Vigilances :
    ne se cumulent pas entre elles
```

**UTILISATION**  
Aucune : bonus passif de groupe / propriétaire.

**ACQUISITION**

```text
Niveau requis : 2
Coût : 1 point de Talent
Prérequis : aucun
```

**Audit source** : conforme. Voir **RANGER-02** pour Perception et **RANGER-03**
pour le cumul avec Guide du groupe.

---

## 6.2 Piège de chasse — palier II / niveau 6

**TYPE**  
ACTIF

**PRINCIPE**  
Le Rôdeur pose un piège temporaire dans une cellule adjacente. Le premier ennemi
qui y entre le déclenche, puis le piège disparaît.

**EFFETS**

```text
Durée du piège : 4 rounds
Déclenchement : premier ennemi entrant

Dégâts :
    5 + modificateur de Sagesse
    type : physique, perforant

Si l'armure physique est épuisée après les dégâts :
    Immobilisé pendant 1 round
    déplacements volontaires bloqués

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
Prérequis : Vigilance
```

**Audit source** : conforme.

---

## 6.3 Repli tactique — palier III / niveau 10

**TYPE**  
ACTIF

**PRINCIPE**  
Le Rôdeur ordonne au groupe entier de reculer immédiatement d'une cellule. Le
déplacement n'est possible que si la translation normale vers l'arrière est
légale.

**EFFETS**

```text
Déplacement : groupe entier
Direction : 1 cellule en arrière par rapport à l'orientation
Franchissement d'obstacle : impossible
Déplacement forcé : non
```

**UTILISATION**

```text
Coût : 1 point d'action
Coût de mobilité : 1 PAM
Cible : groupe
Recharge : 3 rounds

Le coût personnel normal d'une translation n'est pas payé en plus.
```

**ACQUISITION**

```text
Niveau requis : 10
Coût : 1 point de Talent
Prérequis : Piège de chasse
```

**Audit source** : conforme.

---

## 6.4 Maître du terrain — palier IV / niveau 14

**TYPE**  
PASSIF

**PRINCIPE**  
Le Rôdeur tire mieux lorsque le groupe est resté immobile depuis sa précédente
activation. Une translation du groupe invalide immédiatement le bonus pour la
fenêtre d'activation concernée.

**EFFETS**

```text
Condition :
    aucune translation du groupe depuis la précédente activation du Rôdeur

Attaques à distance :
    Dégâts infligés : +10 %
    Précision : +1
```

Le runtime mémorise le compteur de translations à chaque activation du Rôdeur.
Une translation normale ou provoquée par une action modifie ce compteur et rend
la condition fausse.

**UTILISATION**  
Aucune : bonus passif conditionnel.

**ACQUISITION**

```text
Niveau requis : 14
Coût : 1 point de Talent
Prérequis : Repli tactique
```

**Audit source** : conforme.

---

## 6.5 Guide du groupe — palier V / niveau 18

**TYPE**  
PASSIF

**PRINCIPE**  
Tant que le Rôdeur est vivant, il améliore les capacités d'observation et de
survie du groupe et augmente sa réserve de mobilité disponible à chaque round.

**EFFETS**

```text
Meilleur test de groupe de Perception :
    +2

Meilleur test de groupe de Survie :
    +2

Maximum de PAM du groupe :
    +1 par round

Plusieurs Guides du groupe :
    ne se cumulent pas entre eux
```

**UTILISATION**  
Aucune : bonus passif de groupe.

**ACQUISITION**

```text
Niveau requis : 18
Coût : 1 point de Talent
Prérequis : Maître du terrain
```

**Audit source** : conforme. Voir **RANGER-02** pour les Skills et **RANGER-03**
pour son interaction avec Vigilance.

---

# 7. Chaînes de progression validées

## Tireur

```text
Tir précis
    -> Tir perforant
        -> Tir rapide
            -> Volée
                -> Œil d'aigle
```

## Chasseur

```text
Marque de la proie
    -> Ennemi juré [une catégorie exclusive]
        -> Tir immobilisant
            -> Frappe du prédateur
                -> Chasseur alpha
```

Toutes les variantes d'Ennemi juré accordent le même alias logique, qui satisfait
le prérequis de Tir immobilisant.

## Éclaireur

```text
Vigilance
    -> Piège de chasse
        -> Repli tactique
            -> Maître du terrain
                -> Guide du groupe
```

# 8. Concordance mécanique / authoring

| Talent | RPG02 vs authoring | Décision UX |
|---|---|---|
| Tir précis | Conforme | ACTIF |
| Tir perforant | Conforme | ACTIF |
| Tir rapide | Conforme | ACTIF |
| Volée | Conforme | ACTIF |
| Œil d'aigle | Conforme ; dépendance Perception | PASSIF |
| Marque de la proie | Conforme | ACTIF |
| Ennemi juré | Conforme, variantes dynamiques | PASSIF + VARIANTES |
| Tir immobilisant | Conforme | ACTIF |
| Frappe du prédateur | Conforme | ACTIF |
| Chasseur alpha | Conforme ; RPG02 dit Passive | RÉACTION AUTOMATIQUE |
| Vigilance | Conforme ; dépendance Perception | PASSIF |
| Piège de chasse | Conforme | ACTIF |
| Repli tactique | Conforme | ACTIF |
| Maître du terrain | Conforme | PASSIF |
| Guide du groupe | Conforme ; dépendances Perception/Survie | PASSIF |

Aucune nouvelle mécanique n'est inventée dans les fiches ci-dessus.

# 9. Arbitrages et dépendances validés / à normaliser

## RANGER-01 — libellés joueur d'Ennemi juré

L'authoring génère les variantes à partir des `CategoryId` réelles du bestiaire
et construit actuellement leur nom à partir de l'identifiant brut :

```text
Ennemi juré — <CategoryId>
```

C'est correct comme identité technique, mais DESC01.5 interdit d'exposer un
identifiant interne comme libellé joueur si une présentation dédiée est nécessaire.

**Décision validée :** conserver la génération entièrement dynamique depuis le bestiaire, sans liste de catégories dupliquée dans l'UI.

**RPG-TALENT-FIX07 / D04** introduit `UGridMonsterCategoryAsset` comme autorité
bestiaire unique de présentation :

```text
CategoryId   = identité gameplay
DisplayName  = libellé joueur
```

Les monstres de production référencent leur catégorie canonique. L'authoring
`Ennemi juré` découvre toujours dynamiquement les CategoryId réellement présents,
mais construit les titres depuis `CategoryAsset.DisplayName`.

Le read-model utilise la même autorité pour les lignes EFFETS. `HumanizeId`
reste uniquement un fallback diagnostic si une catégorie de développement n'a
pas encore d'asset de présentation.

Catégories de production actuelles :

```text
Goblin -> Gobelins
Vermin -> Vermine
```

État : **source prête ; matérialisation et validation locale requises avant
clôture D04.**

La zone VARIANTES doit rester verticalement scrollable si le bestiaire contient
beaucoup de catégories.

## RANGER-02 — dépendance au catalogue Skills

Les Talents suivants dépendent de Skills canoniques :

```text
Œil d'aigle
    -> Perception

Ennemi juré
    -> Perception
    -> Nature
    -> Histoire
    -> Religion

Vigilance
    -> Perception

Guide du groupe
    -> Perception
    -> Survie
```

Le document RPG03.9.3 précise que le catalogue complet des `URPGSkillAsset` de
production n'est pas encore matérialisé ; les contrats sont couverts avec des
définitions transitoires.

**Décision initialement validée :** ne jamais créer de catalogue Skills parallèle et conserver les effets canoniques. **Mise à jour découverte pendant DESC01.9 :** `UI-RPG06.2A` a déjà matérialisé les 25 `URPGSkillAsset` de production ; cette dépendance est donc désormais satisfaite et sera normalisée en DESC01.12.

## RANGER-03 — cumul Vigilance + Guide du groupe

Les deux Talents utilisent des groupes de stacking distincts :

```text
Vigilance       -> Party.Ranger.Vigilance
Guide du groupe -> Party.Ranger.GroupGuide
```

Conséquence runtime actuelle :

- plusieurs Vigilances ne se cumulent pas ;
- plusieurs Guides ne se cumulent pas ;
- **une Vigilance et un Guide se cumulent entre eux**.

Un groupe possédant les deux bénéficie donc de :

```text
Perception de groupe : +4
    +2 Vigilance
    +2 Guide du groupe
```

Les tests de support valident explicitement cette composition.

**Décision validée :** canoniser ce comportement pour la v0.1. Vigilance et Guide du groupe se cumulent entre eux (+4 Perception au total) mais pas avec une seconde copie du même Talent. Réévaluer à l'usage si nécessaire.

# 10. Points runtime confirmés sans arbitrage

## Marque personnelle

- un Rôdeur : une seule cible marquée à la fois ;
- re-marquer une nouvelle cible retire l'ancienne marque du même Rôdeur ;
- plusieurs Rôdeurs peuvent marquer le même ennemi ;
- les bonus de Marque de la proie et Frappe du prédateur exigent la propre marque
  du Rôdeur concerné.

## Chasseur alpha

- transfert automatique une fois par round ;
- origine du rayon : cellule de la cible vaincue ;
- rayon : 3 cases ;
- cible : ennemi vivant le plus proche ;
- égalité : départage interne déterministe ;
- nouvelle durée : 2 rounds ;
- aucun transfert si aucune cible éligible.

## Maître du terrain

La qualification est mémorisée à l'activation du Rôdeur à partir du compteur de
translations du groupe. Toute translation modifie ce compteur et invalide le bonus
pour la fenêtre concernée.

# 11. Vocabulaire Rôdeur à normaliser

À employer :

```text
arme à distance
portée de l'arme
Précision
armure physique
dégâts bruts
PV
ligne de vue
Marque de la proie
Ennemi juré
catégorie de créature
Immobilisé
Perception
Nature
Histoire
Religion
Survie
Sagesse
PAM / points d'action de mobilité
fumée
```

À ne pas exposer :

```text
Accuracy
PhysicalArmor
RawDamage
Status_MarkedByRanger
Status_Immobilized
CategoryId
AllowedTargetMonsterCategoryIds
Skill_*
StackingGroupId
MaximumMobilityActionPointsModifier
bRequiredStatusesFromSource
bRequirePartyStationarySincePreviousActivation
ChoiceId / RequirementId
```

# 12. Lien avec l'effort Skills

DESC01.8 confirme une deuxième dépendance forte du chantier Talent envers le futur
catalogue de compétences :

```text
Rôdeur
    -> Œil d'aigle -> Perception
    -> Ennemi juré -> Perception / Nature / Histoire / Religion
    -> Vigilance -> Perception
    -> Guide du groupe -> Perception / Survie
```

Cette dépendance rejoint celle déjà identifiée sur le Voleur. Elle doit être
consolidée en DESC01.12, sans rouvrir RPG-SKILL01 qui reste clos sur l'économie et
l'allocation des points.

# 13. Contrat de données attendu plus tard

DESC01.8 ne définit toujours pas la structure C++ finale.

L'audit Rôdeur montre que le futur read-model devra pouvoir représenter :

```text
type UX
principe
effets conditionnels
portée dynamique basée sur l'arme
multi-résolution
zone / ligne de vue
statut personnel par source
variante dynamique issue d'un catalogue externe
bonus de Skills contextuels
réaction automatique de transfert
effets de groupe non cumulables
mobilité du groupe / PAM
condition « groupe resté immobile »
acquisition
```

La forme C++ sera définie seulement en DESC01.13 après l'audit 90/90.

# 14. Critères de validation UI-RPG-DESC01.8

Le jalon a été validé par l'utilisateur le **8 octobre 2026**. Ont été approuvés :

- les 15 fiches Rôdeur ;
- la classification **9 ACTIF / 5 PASSIF / 1 RÉACTION AUTOMATIQUE** ;
- Chasseur alpha comme RÉACTION AUTOMATIQUE ;
- Ennemi juré comme PASSIF + VARIANTES dynamiques du bestiaire ;
- les trois chaînes de progression ;
- le vocabulaire joueur ;
- les décisions/reports RANGER-01 à RANGER-03.

Après validation :

```text
UI-RPG-DESC01.9 — audit documentaire des 15 Talents du Mage
```
