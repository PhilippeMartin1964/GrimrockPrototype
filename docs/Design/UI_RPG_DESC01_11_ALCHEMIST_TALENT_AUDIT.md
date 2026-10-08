# UI-RPG-DESC01.11 — Audit documentaire des 15 Talents de l'Alchimiste

Date : **8 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
État : **VALIDÉ PAR L'UTILISATEUR — 8 octobre 2026**  
Dépendances : **UI-RPG-DESC01.5 à .10 validés**  
Périmètre : **Alchimiste / 3 branches / 15 nœuds conceptuels**

## 1. Objet

Traduire les 15 Talents de l'Alchimiste dans le contrat UX validé par
`UI_RPG_DESC01_5_TALENT_UX_CONTRACT.md`, sans modifier le gameplay.

Sources vérifiées :

```text
docs/Rules/RPG_Talents_Mechanics_v0_1.md
docs/Rules/RPG_Class_Progression_1_20_v0_1.md
docs/Design/RPG03_7_ALCHEMY_QUICKITEM_MODIFIERS.md
docs/Design/RPG03_8_BATCH_TARGET_FILTERS_TARGETABILITY.md
docs/Design/RPG03_9_6A_ALCHEMIST_SUPPORT.md
docs/Design/RPG03_9_6B1_ALCHEMIST_GRENADIER_APOTHECARY.md
docs/Design/RPG03_9_6B2_ALCHEMIST_TRANSMUTER.md
docs/Design/RPG03_9_6C_ALCHEMIST_PRODUCTION_MATERIALIZATION.md
docs/Design/UI_RPG06_1_SKILLS_AUDIT_CATALOG_AUTHORING.md
Source/GrimrockPrototypeEditor/Private/RPG/RPGAlchemistAuthoring.cpp
Source/GrimrockPrototypeEditor/Private/Tests/RPGRPG0396AlchemistB1AuthoringTests.cpp
Source/GrimrockPrototypeEditor/Private/Tests/RPGRPG0396AlchemistB2AuthoringTests.cpp
Source/GrimrockPrototypeEditor/Private/Tests/RPGRPG0396AlchemistProductionTests.cpp
Source/GrimrockPrototype/Private/Tests/RPGRPG0396SupportTests.cpp
Source/GrimrockPrototype/Private/Runtime/Combat/GridQuickItemResolver.cpp
Source/GrimrockPrototype/Private/Runtime/Combat/GridTurnManagerPlayerActionCatalog.cpp
Source/GrimrockPrototype/Private/Runtime/GridLevelRuntimeActorSurfaces.cpp
```

Aucun C++, UMG ou DataAsset n'est modifié par ce jalon.

## 2. Synthèse de la classe

| Branche | Palier I | II | III | IV | V |
|---|---|---|---|---|---|
| **Grenadier** | Bombe incendiaire | Bombe toxique | Charge précise | Réaction en chaîne | Maître grenadier |
| **Apothicaire** | Potion renforcée | Antidote | Élixir défensif | Diffusion | Panacée |
| **Transmutateur** | Huile glissante | Flasque acide | Nuage corrosif | Catalyseur | Transmutation majeure |

Répartition UX proposée :

```text
RECETTE + OBJET RAPIDE   8
PASSIF                   4
RÉACTION AUTOMATIQUE     1
ACTIF                     1
RECETTE + ACTIF           1
TOTAL                    15
```

**Réaction en chaîne** est classée **RÉACTION AUTOMATIQUE** dans l'UX, même si
RPG02 la nomme historiquement « Passive » : elle répond automatiquement à un
événement de réaction de surface.

Aucun Talent Alchimiste ne possède de variantes de Choice exclusives.

## 3. Talent, recette, objet et action : quatre notions distinctes

L'Alchimiste est la classe où cette distinction est indispensable.

```text
TALENT ACQUIS
    -> accorde un ou plusieurs droits de recette Recipe_*

RECETTE
    -> décrit à terme comment fabriquer un objet
       (moteur de crafting encore absent)

OBJET FABRIQUÉ / POSSÉDÉ
    -> UGridItemDefinitionAsset + quantité d'inventaire

ACTION OBJET RAPIDE
    -> existe dans le catalogue de combat seulement si l'objet est présent
       et est consommée lors d'une utilisation acceptée
```

Donc :

> **ACQUIS** sur « Bombe incendiaire » signifie que le Talent est acquis et que le
> droit de recette est accordé. Cela ne signifie pas que le personnage possède
> actuellement une Bombe incendiaire dans son inventaire.

La future fiche Talent décrit la mécanique de l'objet déverrouillé, mais ne doit
jamais transformer le statut du Talent en « objet disponible ».

## 4. Recettes multiples ne sont pas des variantes de Talent

Deux Talents accordent plusieurs recettes en une seule acquisition :

```text
Élixir défensif
    -> Feu
    -> Glace
    -> Foudre
    -> Poison

Transmutation majeure
    -> Feu
    -> Glace
    -> Poison
    -> Huile
```

Ce ne sont **pas** des variantes de Choice comme :

```text
Spécialisation martiale
Ennemi juré
Affinité élémentaire
Imprégnation
```

Il n'y a donc :

- aucun bouton CHOISIR entre ces recettes ;
- aucune exclusivité ;
- aucune section VARIANTES au sens d'acquisition.

Les quatre sorties sont présentées dans **EFFETS / RECETTES ACCORDÉES** et sont
toutes accordées ensemble lorsque le Talent est acquis.

---

# 5. Branche GRENADIER

## 5.1 Bombe incendiaire — palier I / niveau 2

**TYPE**  
RECETTE + OBJET RAPIDE

**PRINCIPE**  
Le Talent accorde le droit de recette de la Bombe incendiaire. Une bombe réellement
présente dans l'inventaire peut ensuite être lancée sur une zone et est consommée
par une utilisation acceptée.

**EFFETS**

```text
Dégâts directs :
    6 + rang d'Alchimie
Type :
    Feu

Zone :
    rayon 1 case

Si l'armure magique est épuisée après les dégâts :
    Brûlure pendant 2 tours
    2 dégâts de Feu par tick

Surface créée :
    Feu pendant 2 rounds

Jet pour toucher :
    aucun

Coup critique :
    impossible
```

La bombe possède un **friendly fire explicite** : si le groupe se trouve dans sa
zone, ses membres vivants reçoivent également une résolution de dégâts directs et
peuvent subir Brûlure si l'ArmorGate est satisfait.

**UTILISATION**

```text
Coût : 2 points d'action
Objet consommé : 1 Bombe incendiaire
Cellule cible : portée 4 cases
Zone : rayon 1 case
Ligne de vue : requise
Recharge : aucune
```

**ACQUISITION**

```text
Niveau requis : 2
Coût : 1 point de Talent
Prérequis : aucun
Recette accordée : Bombe incendiaire
```

**Audit source** : mécanique QuickItem matérialisée et conforme, avec précision du
friendly fire runtime. Voir **ALCH-01** pour la frontière Crafting.

---

## 5.2 Bombe toxique — palier II / niveau 6

**TYPE**  
RECETTE + OBJET RAPIDE

**PRINCIPE**  
Le Talent accorde le droit de recette de la Bombe toxique. L'objet fabriqué peut
ensuite être lancé sur une zone et est consommé par l'utilisation.

**EFFETS**

```text
Dégâts directs :
    6 + rang d'Alchimie
Type :
    Poison

Zone :
    rayon 1 case

Si l'armure magique est épuisée après les dégâts :
    Poison pendant 3 tours
    2 dégâts de Poison par tick

Surface créée :
    Poison pendant 3 rounds

Jet pour toucher :
    aucun

Coup critique :
    impossible
```

Comme la Bombe incendiaire, la Bombe toxique possède un **friendly fire
explicite**. Les membres du groupe présents dans la zone peuvent subir les dégâts
et le statut Poison selon les mêmes règles d'ArmorGate.

**UTILISATION**

```text
Coût : 2 points d'action
Objet consommé : 1 Bombe toxique
Cellule cible : portée 4 cases
Zone : rayon 1 case
Ligne de vue : requise
Recharge : aucune
```

**ACQUISITION**

```text
Niveau requis : 6
Coût : 1 point de Talent
Prérequis : Bombe incendiaire
Recette accordée : Bombe toxique
```

**Audit source** : conforme, sous la même frontière Crafting que Bombe incendiaire.

---

## 5.3 Charge précise — palier III / niveau 10

**TYPE**  
PASSIF

**PRINCIPE**  
L'Alchimiste maîtrise mieux ses bombes. Il peut les lancer plus loin et réduit les
dégâts directs qu'elles infligent accidentellement aux membres de son groupe.

**EFFETS**

```text
Toutes les bombes de l'Alchimiste :

Portée :
    +1 case

Dégâts directs infligés aux alliés / à l'Alchimiste dans la zone :
    -50 %

Ne sont pas réduits :
    surfaces créées
    statuts appliqués
    dégâts périodiques ultérieurs
```

Exemple :

```text
Bombe incendiaire :
    portée 4 -> 5 cases
```

**UTILISATION**  
Aucune : modifie automatiquement les actions portant le tag de bombe.

**ACQUISITION**

```text
Niveau requis : 10
Coût : 1 point de Talent
Prérequis : Bombe toxique
```

**Audit source** : conforme. Le runtime applique la réduction uniquement aux
dégâts directs alliés ; il n'altère pas les Status Effects ni les surfaces.

---

## 5.4 Réaction en chaîne — palier IV / niveau 14

**TYPE**  
RÉACTION AUTOMATIQUE

**PRINCIPE — CONTRAT CIBLE RPG02**  
Une fois par action de bombe, si cette bombe déclenche une réaction canonique de
surface, Réaction en chaîne renforce automatiquement cette réaction. Une réaction
générée par Réaction en chaîne ne peut pas elle-même déclencher une nouvelle
Réaction en chaîne.

**EFFETS CIBLES**

```text
Déclencheur :
    réaction de surface causée par une bombe de l'Alchimiste

Fréquence :
    1 fois maximum par action de bombe

Dégâts de la réaction :
    +25 %

Rayon de la réaction :
    +1 case
    maximum final prévu : rayon 2

Récursion :
    interdite
```

**UTILISATION**  
Aucune : réaction automatique.

**ACQUISITION**

```text
Niveau requis : 14
Coût : 1 point de Talent
Prérequis : Charge précise
```

**Audit source** : le profil de réaction est correctement authoré et testé
isolément, mais les Bombes authorées ne portent actuellement aucun
`SurfaceInteraction`. Le chemin `SurfaceEffects` applique/remplace une surface
sans produire l'événement `SurfaceReaction` attendu par ce Talent. Voir
**ALCH-02**.

---

## 5.5 Maître grenadier — palier V / niveau 18

**TYPE**  
PASSIF

**PRINCIPE**  
L'Alchimiste utilise toutes ses bombes plus efficacement : elles coûtent moins de
points d'action et leurs dégâts directs sont renforcés.

**EFFETS**

```text
Toutes les bombes de l'Alchimiste :

Coût en points d'action :
    -1
    minimum : 1

Dégâts directs :
    +20 %

Inchangés :
    quantité d'objet consommée
    dégâts périodiques déjà appliqués
    dégâts futurs des surfaces
```

**UTILISATION**  
Aucune : bonus passif sur les actions de bombe.

**ACQUISITION**

```text
Niveau requis : 18
Coût : 1 point de Talent
Prérequis : Réaction en chaîne
```

**Audit source** : conforme.

---

# 6. Branche APOTHICAIRE

## 6.1 Potion renforcée — palier I / niveau 2

**TYPE**  
PASSIF

**PRINCIPE**  
Lorsque l'Alchimiste utilise lui-même une potion positive, ses restaurations
directes sont renforcées. Le Talent n'allonge pas la durée des effets temporaires
et n'améliore pas les consommables offensifs.

**EFFETS**

```text
Potions positives utilisées par l'Alchimiste :

Restauration de PV :
    +25 %

Restauration de mana :
    +25 %

Restauration d'armure physique ou magique :
    +25 %

Durée des statuts :
    inchangée

Bombes / poisons offensifs :
    aucun bonus
```

Le bonus appartient à la **source Alchimiste** : il fonctionne également lorsqu'il
administre la potion à un autre membre du groupe.

**UTILISATION**  
Aucune : bonus passif.

**ACQUISITION**

```text
Niveau requis : 2
Coût : 1 point de Talent
Prérequis : aucun
```

**Audit source** : conforme. Le bonus positif est appliqué après le scaling de
compétence pour les PV/mana et par le resolver d'armure pour les restaurations
d'armure.

---

## 6.2 Antidote — palier II / niveau 6

**TYPE**  
RECETTE + OBJET RAPIDE

**PRINCIPE**  
Le Talent accorde la recette de l'Antidote. L'objet retire le Poison et peut
également éliminer un autre effet négatif identifié comme Toxine.

**EFFETS**

```text
Retire :
    Poison, si présent
    ET jusqu'à 1 autre effet négatif de type Toxine

Si aucun effet applicable n'est présent :
    l'action est refusée
    l'objet n'est pas consommé
    les points d'action ne sont pas dépensés
```

Le moteur pré-vérifie qu'une mutation utile est possible avant la transaction.

**UTILISATION**

```text
Coût : 1 point d'action
Objet consommé : 1 Antidote
Cible : soi-même ou un allié vivant
Portée : 1 case
Recharge : aucune
```

**ACQUISITION**

```text
Niveau requis : 6
Coût : 1 point de Talent
Prérequis : Potion renforcée
Recette accordée : Antidote
```

**Audit source** : conforme.

---

## 6.3 Élixir défensif — palier III / niveau 10

**TYPE**  
RECETTE + OBJET RAPIDE

**PRINCIPE**  
Le Talent accorde simultanément quatre recettes d'Élixir défensif. Chaque objet
restaure de l'armure magique et accorde temporairement une résistance correspondant
à sa recette.

**EFFETS**

```text
Effet commun :
    armure magique restaurée : 4
    durée de la résistance : 3 rounds

Recettes accordées :

Feu :
    résistance Feu +25 %

Glace :
    résistance Glace +25 %

Foudre :
    résistance Foudre +25 %

Poison :
    résistance Poison +25 %
```

Avec Potion renforcée, la restauration d'armure magique est augmentée de 25 % ;
la résistance +25 % et sa durée ne sont pas augmentées par Potion renforcée.

**UTILISATION**

```text
Coût : 1 point d'action
Objet consommé : 1 Élixir du type utilisé
Cible : soi-même ou un allié vivant
Portée : 1 case
Recharge : aucune
```

**ACQUISITION**

```text
Niveau requis : 10
Coût : 1 point de Talent
Prérequis : Antidote
Recettes accordées :
    Élixir défensif — Feu
    Élixir défensif — Glace
    Élixir défensif — Foudre
    Élixir défensif — Poison
```

**Audit source** : conforme. Les quatre produits sont quatre ItemDefinitions
concrets, mais un seul Talent : aucune variante exclusive.

---

## 6.4 Diffusion — palier IV / niveau 14

**TYPE**  
PASSIF

**PRINCIPE**  
Lorsqu'une potion positive est utilisée sur un membre du groupe, l'Alchimiste peut
choisir un second membre vivant. L'objet n'est consommé qu'une fois et l'effet
secondaire est réduit.

**EFFETS — COMPORTEMENT RUNTIME ACTUEL**

```text
Cible primaire :
    effet normal à 100 %

Seconde cible :
    restauration de PV / mana : 50 %
    restauration d'armure : 50 %
    durée des statuts : 50 %, arrondie au tour/round entier le plus proche
    nombre maximal d'effets retirés : 50 %, minimum 1

Objet supplémentaire consommé :
    aucun

Récursion :
    aucune troisième cible
```

La puissance interne d'un statut temporaire n'est actuellement **pas divisée par
deux**. Exemple : un Élixir défensif diffusé garde sa résistance +25 %, mais sa
durée est réduite.

**UTILISATION**  
Aucune action séparée : Diffusion modifie l'utilisation d'une potion positive et
autorise la sélection explicite d'une seconde cible.

**ACQUISITION**

```text
Niveau requis : 14
Coût : 1 point de Talent
Prérequis : Élixir défensif
```

**Audit source** : le pipeline secondaire est branché dans le batch Ally et
consomme l'objet une seule fois. Voir **ALCH-03** pour la formulation canonique.

---

## 6.5 Panacée — palier V / niveau 18

**TYPE**  
RECETTE + OBJET RAPIDE

**PRINCIPE**  
Le Talent accorde la recette de la Panacée, une potion de soin avancée qui restaure
les PV et l'armure magique tout en retirant plusieurs afflictions.

**EFFETS**

```text
PV restaurés :
    10 + 2 × rang d'Alchimie

Armure magique restaurée :
    8

Retire jusqu'à 3 effets négatifs parmi :
    Poison
    Brûlure
    Saignement
    Ralenti
    Silence
    Immobilisé
    effets de type Toxine
    effets purifiables
```

Avec Potion renforcée, les restaurations positives de PV et d'armure sont
augmentées de 25 %. Le nombre maximal de Debuffs retirés n'est pas augmenté par
Potion renforcée.

Avec Diffusion, le comportement secondaire suit les règles de Diffusion décrites
ci-dessus.

**UTILISATION**

```text
Coût : 2 points d'action
Objet consommé : 1 Panacée
Cible : soi-même ou un allié vivant
Portée : 1 case
Recharge : 3 rounds
```

**ACQUISITION**

```text
Niveau requis : 18
Coût : 1 point de Talent
Prérequis : Diffusion
Recette accordée : Panacée
```

**Audit source** : conforme.

---

# 7. Branche TRANSMUTATEUR

## 7.1 Huile glissante — palier I / niveau 2

**TYPE**  
RECETTE + OBJET RAPIDE

**PRINCIPE**  
Le Talent accorde la recette d'une Flasque d'huile. L'objet crée une zone d'huile
qui ralentit la traversée et peut ensuite réagir avec le Feu.

**EFFETS**

```text
Dégâts directs :
    aucun

Surface :
    Huile
    durée : 4 rounds
    zone : rayon 1 case

Traversée d'une cellule huilée :
    coût de déplacement / PAM : +1

Interaction :
    le Feu peut transformer l'Huile en Feu
```

**UTILISATION**

```text
Coût : 2 points d'action
Objet consommé : 1 Flasque d'huile
Cellule cible : portée 4 cases
Zone : rayon 1 case
Ligne de vue : requise
Recharge : aucune
```

**ACQUISITION**

```text
Niveau requis : 2
Coût : 1 point de Talent
Prérequis : aucun
Recette accordée : Flasque d'huile
```

**Audit source** : conforme.

---

## 7.2 Flasque acide — palier II / niveau 6

**TYPE**  
RECETTE + OBJET RAPIDE

**PRINCIPE**  
Le Talent accorde la recette d'une Flasque acide. L'acide n'est pas un nouveau
type de dégâts : il attaque directement l'armure physique de la cible et la rend
plus difficile à restaurer temporairement.

**EFFETS**

```text
Armure physique détruite :
    6 + 2 × rang d'Alchimie

Débordement sur les PV :
    aucun

Corrodé pendant 2 rounds :
    restaurations d'armure physique reçues : -20 %
```

**UTILISATION**

```text
Coût : 2 points d'action
Objet consommé : 1 Flasque acide
Cible : un ennemi
Portée : 4 cases
Ligne de vue : requise
Recharge : 1 round
```

**ACQUISITION**

```text
Niveau requis : 6
Coût : 1 point de Talent
Prérequis : Huile glissante
Recette accordée : Flasque acide
```

**Audit source** : conforme. Aucun `DamageType Acide` n'est créé.

---

## 7.3 Nuage corrosif — palier III / niveau 10

**TYPE**  
RECETTE + OBJET RAPIDE

**PRINCIPE**  
Le Talent accorde la recette d'une Flasque de nuage corrosif. L'impact initial
inflige des dégâts de Poison puis crée une surface persistante qui continue
d'endommager ses occupants.

**EFFETS**

```text
Impact initial :
    4 + rang d'Alchimie dégâts de Poison

Surface :
    Nuage de poison
    durée : 3 rounds
    dégâts périodiques : 2 Poison par round

Lors des effets périodiques de la surface :
    si l'armure magique est épuisée
    -> Poison pendant 2 tours
```

Le statut Poison appartient au comportement périodique de la surface ; il n'est
pas présenté comme un second statut garanti immédiatement à l'impact.

**UTILISATION**

```text
Coût : 3 points d'action
Objet consommé : 1 Flasque de nuage corrosif
Cellule cible : portée 4 cases
Zone : rayon 1 case
Ligne de vue : requise
Recharge : 2 rounds
```

**ACQUISITION**

```text
Niveau requis : 10
Coût : 1 point de Talent
Prérequis : Flasque acide
Recette accordée : Flasque de nuage corrosif
```

**Audit source** : conforme.

---

## 7.4 Catalyseur — palier IV / niveau 14

**TYPE**  
ACTIF

**PRINCIPE**  
L'Alchimiste force immédiatement une réaction canonique sur une surface existante.
La capacité n'est disponible que si la cellule ciblée contient réellement une
réaction que le moteur de surfaces sait résoudre.

**EFFETS**

```text
Réaction :
    exactement 1 réaction canonique

Surface sans réaction valide :
    action indisponible

Durée restante :
    la réaction applique ses règles normales
    Catalyseur ne supprime pas artificiellement toute la durée avant résolution

Choix en cas de plusieurs réactions possibles :
    priorité déterministe du moteur de surfaces
```

La priorité interne exacte n'est pas un choix utilisateur et n'a pas besoin d'être
exposée dans la fiche.

**UTILISATION**

```text
Coût : 1 point d'action
Objet consommé : aucun
Cible : une cellule contenant une surface réactive
Portée : 4 cases
Ligne de vue : requise
Recharge : 2 rounds
```

**ACQUISITION**

```text
Niveau requis : 14
Coût : 1 point de Talent
Prérequis : Nuage corrosif
```

**Audit source** : conforme. Catalyseur est la seule action directement portée par
`URPGClassAsset::CombatActions` pour l'Alchimiste.

---

## 7.5 Transmutation majeure — palier V / niveau 18

**TYPE**  
RECETTE + ACTIF

**PRINCIPE — CONTRAT CIBLE RPG02**  
Le Talent accorde quatre recettes de Transmutation majeure. Le futur système de
crafting doit sélectionner exactement une sortie et fournir un Catalyseur rare.
L'action convertit ensuite toutes les cellules de la zone vers cette sortie.

**EFFETS CIBLES**

```text
Sorties de recette accordées :
    Feu
    Glace
    Poison
    Huile

Zone :
    rayon 2 cases

Chaque cellule convertible :
    devient la surface choisie

Durée cible RPG02 :
    4 rounds

Sortie Huile :
    coût de traversée : +1

Dégâts de réactions de surface déclenchées pendant cette conversion :
    +50 %   [contrat RPG02 à clarifier / raccorder]
```

Les quatre sorties sont accordées ensemble : elles ne constituent pas quatre
variantes exclusives du Talent.

**UTILISATION CIBLE**

```text
Coût : 4 points d'action
Objet consommé : 1 Catalyseur rare
Cellule cible : portée 4 cases
Zone : rayon 2 cases
Ligne de vue : requise
Recharge : 5 rounds
Sortie : déterminée par la recette utilisée
```

**ACQUISITION**

```text
Niveau requis : 18
Coût : 1 point de Talent
Prérequis : Catalyseur

Recettes accordées :
    Transmutation majeure — Feu
    Transmutation majeure — Glace
    Transmutation majeure — Poison
    Transmutation majeure — Huile
```

**Audit source** : le helper de contribution combat existe pour chacune des quatre
sorties, mais le moteur Crafting et l'objet Catalyseur rare ne sont pas
matérialisés comme chaîne utilisateur complète. Deux autres écarts runtime sont
documentés en **ALCH-04** et **ALCH-05**.

---

# 8. Chaînes de progression validées

## Grenadier

```text
Bombe incendiaire
    -> Bombe toxique
        -> Charge précise
            -> Réaction en chaîne
                -> Maître grenadier
```

## Apothicaire

```text
Potion renforcée
    -> Antidote
        -> Élixir défensif
            -> Diffusion
                -> Panacée
```

## Transmutateur

```text
Huile glissante
    -> Flasque acide
        -> Nuage corrosif
            -> Catalyseur
                -> Transmutation majeure
```

# 9. Concordance mécanique / authoring

| Talent | RPG02 vs authoring/runtime | Décision UX |
|---|---|---|
| Bombe incendiaire | Conforme, friendly fire explicite runtime | RECETTE + OBJET RAPIDE |
| Bombe toxique | Conforme, friendly fire explicite runtime | RECETTE + OBJET RAPIDE |
| Charge précise | Conforme | PASSIF |
| Réaction en chaîne | Profil conforme mais déclencheur non raccordé par les bombes actuelles | RÉACTION AUTOMATIQUE |
| Maître grenadier | Conforme | PASSIF |
| Potion renforcée | Conforme | PASSIF |
| Antidote | Conforme, préflight no-op présent | RECETTE + OBJET RAPIDE |
| Élixir défensif | Conforme, 4 recettes non exclusives | RECETTE + OBJET RAPIDE |
| Diffusion | Pipeline branché ; sémantique « magnitude » à préciser | PASSIF |
| Panacée | Conforme | RECETTE + OBJET RAPIDE |
| Huile glissante | Conforme | RECETTE + OBJET RAPIDE |
| Flasque acide | Conforme | RECETTE + OBJET RAPIDE |
| Nuage corrosif | Conforme ; Poison périodique via surface | RECETTE + OBJET RAPIDE |
| Catalyseur | Conforme | ACTIF |
| Transmutation majeure | Frontière Crafting + deux écarts surface | RECETTE + ACTIF |

# 10. Arbitrages et écarts validés

## ALCH-01 — aucune fausse disponibilité de recette / objet

Le projet ne possède pas encore de moteur de Crafting autoritaire.

Les Talents accordent aujourd'hui des `GrantedRequirementIds Recipe_*`, tandis que
les QuickItems matérialisés existent comme ItemDefinitions indépendants.

**Décision validée : canoniser la distinction suivante dans l'UX :**

```text
Talent ACQUIS
    != objet possédé
    != action QuickItem actuellement disponible
```

La fiche Talent peut dire **« Recette accordée »** et décrire l'objet, mais ne doit
jamais dire « Action disponible » sur la seule base de l'acquisition du Talent.

Le futur écran d'artisanat sera l'autorité de fabrication lorsque ce système
existera.

## ALCH-02 — Réaction en chaîne actuellement non raccordée aux bombes

Le Talent possède bien :

```text
Trigger = SurfaceReaction
Source = QuickItem.Bomb
OncePerAction
+25 % dégâts de réaction
+1 rayon
anti-récursion
```

Mais les Bombes actuelles créent des `SurfaceEffects` sans
`SurfaceInteraction`.

Le runtime :

```text
SurfaceInteraction
    -> ResolveReaction
    -> événement SurfaceReaction

SurfaceEffect
    -> ApplyCombatSurfaceAtCell
    -> aucune réaction canonique déclenchée
```

Donc le profil de Réaction en chaîne est authoré et testé isolément, mais le chemin
normal d'une Bombe incendiaire/toxique ne produit actuellement pas son déclencheur.

**Décision validée : conserver le contrat RPG02 comme comportement cible**, puis
corriger ultérieurement le pipeline générique des bombes/surfaces afin qu'une
bombe puisse réellement déclencher une réaction canonique sans
`switch(TalentId)`.

Jusqu'à cette correction, la future fiche finale ne doit pas prétendre que la
mécanique est runtime-validée.

## ALCH-03 — sens exact de « 50 % de magnitude » pour Diffusion

Le runtime générique actuel applique à la cible secondaire :

```text
PV / mana restaurés       -> 50 %
armure restaurée          -> 50 %
durée de statut           -> 50 %
nombre maximal de retraits-> 50 %, minimum 1
puissance interne du statut -> inchangée
```

Exemple Élixir défensif diffusé :

```text
armure magique restaurée -> moitié
résistance               -> reste +25 %
durée                    -> environ moitié, arrondie
```

**Décision validée : canoniser ce comportement pour la v0.1.**

Il reste générique, simple et évite d'introduire un système séparé de « statut à
potency divisée ». La fiche joueur devra dire précisément ce qui est réduit plutôt
que l'ambigu « 50 % de la magnitude et de la durée ».

## ALCH-04 — durée de Transmutation majeure

RPG02 dit :

```text
fixe leur durée à 4 rounds
```

Le helper authoré renseigne bien :

```text
EmptyCellDurationRounds = 4
```

mais le resolver de conversion fait actuellement :

```text
cellule vide
    -> 4 rounds

surface existante
    -> conserve RemainingRounds de la surface existante
```

**Décision validée : conserver RPG02 comme autorité : toute cellule convertie par
Transmutation majeure doit avoir une durée finale de 4 rounds.**

**RPG-TALENT-FIX05 / D07** corrige maintenant le contrat générique de conversion
avec un opt-in de durée finale fixe. Transmutation majeure demande explicitement
4 rounds ; les conversions ordinaires conservent leur ancien comportement.

État : **VALIDÉ / CLOS** — 10/10 RPG03.9.6A, 4/4 RPG03.9.4F1 et 7/7 RPG03.9.6B2, sans warning ni échec.

## ALCH-05 — bonus +50 % de réaction pendant Transmutation majeure

Le Choice authoré contient :

```text
SurfaceReactionDamagePercentModifier = +50 %
ActionId = Action_Alchemist_MajorTransmutation
```

Mais l'action de Transmutation majeure utilise uniquement des
`SurfaceConversions`. Le chemin de conversion actuel ne déclenche pas de réaction
de surface et n'utilise pas ce bonus pour produire des dégâts.

La phrase RPG02 :

```text
Les dégâts de réaction déclenchés pendant cette conversion +50 %
```

n'a donc pas encore de sémantique runtime complète.

**Décision validée pour la v0.1 : ne pas promettre ce +50 % dans la fiche joueur
finale tant qu'un contrat explicite « conversion -> réaction » n'a pas été validé.**

Le champ reste une dette de mécanique à examiner en DESC01.12 / futur chantier
surfaces. Deux options pourront alors être décidées :

```text
A — définir génériquement les réactions provoquées par conversion et conserver +50 %
B — simplifier Transmutation majeure en conversion pure et retirer ce bonus mort
```

Aucune des deux options ne doit être implémentée pendant l'audit documentaire.

# 11. Friendly fire des bombes

RPG02 dit historiquement « chaque hostile », mais les deux Bombes authorées ont :

```text
bAffectsAlliesInArea = true
```

et Charge précise n'aurait autrement aucune raison d'avoir :

```text
FriendlyDirectDamagePercentModifier = -50
```

Le runtime confirme qu'un groupe couvert par la zone reçoit une résolution
séparée, y compris les Status Applications derrière ArmorGate.

Le contrat cohérent est donc :

```text
sans Charge précise :
    alliés dans la zone -> 100 % des dégâts directs
    statuts/surfaces -> normaux

avec Charge précise :
    alliés dans la zone -> 50 % des dégâts directs
    statuts/surfaces -> toujours normaux
```

Cette règle doit être normalisée dans RPG02 en DESC01.12.

# 12. Skills — état actuel

Le catalogue UI-RPG06.2A contient le Skill de production :

```text
Alchimie
```

Il intervient dans :

```text
Bombe incendiaire       -> + rang d'Alchimie dégâts
Bombe toxique           -> + rang d'Alchimie dégâts
Panacée                  -> +2 × rang d'Alchimie PV
Flasque acide            -> +2 × rang d'Alchimie armure physique détruite
Nuage corrosif           -> + rang d'Alchimie dégâts initiaux
```

DESC01.11 ne crée aucune nouvelle autorité de Skill.

# 13. Vocabulaire Alchimiste à normaliser

À employer :

```text
Alchimie
objet rapide
recette accordée
objet consommé
Bombe incendiaire
Bombe toxique
Brûlure
Poison
armure physique
armure magique
résistance
Toxine
effet purifiable
Huile
Nuage de poison
Corrodé
réaction de surface
Catalyseur rare
points d'action
PAM
```

À ne pas exposer :

```text
Skill_Alchemy
QuickItem.*
Recipe_*
Item_*
Status_*
Surface_*
PhysicalArmor
MagicalArmor
DamageType
SourceItemQuantityCost
PositiveEffectPercentModifier
QuickItemSecondary*
SurfaceReactionDamagePercentModifier
SurfaceInteraction
GrantedRequirementIds
```

# 14. Contrat de données attendu plus tard

DESC01.11 ne définit pas encore la structure C++ finale.

L'audit Alchimiste montre que le futur read-model devra pouvoir représenter :

```text
type UX RECETTE + OBJET RAPIDE / PASSIF / RÉACTION / ACTIF / RECETTE + ACTIF
droits de recette accordés
distinction Talent acquis / objet possédé
description d'une action issue d'un ItemDefinition
coût en objet
portée / zone / LOS / cooldown
scaling par Skill Alchimie
friendly fire direct
statut derrière ArmorGate
surface créée et durée
modificateurs conditionnés par tags de source
réaction automatique de surface
restauration positive amplifiée
retrait conditionnel de statuts
secondaire Diffusion
dégâts directs d'armure sans débordement PV
coût de traversée de surface
interaction canonique de surface
conversion de surface
recettes multiples non exclusives
frontière Crafting
acquisition
```

La forme C++ sera définie seulement en DESC01.13 après la normalisation croisée
DESC01.12.

# 15. Critères de validation UI-RPG-DESC01.11

Le jalon a été validé par l'utilisateur le **8 octobre 2026**. Ont été approuvés :

- les 15 fiches Alchimiste ;
- la classification
  **8 RECETTE + OBJET RAPIDE / 4 PASSIF / 1 RÉACTION AUTOMATIQUE /
  1 ACTIF / 1 RECETTE + ACTIF** ;
- Réaction en chaîne comme RÉACTION AUTOMATIQUE ;
- la distinction stricte Talent / recette / objet / action ;
- Élixir défensif et Transmutation majeure comme ensembles de recettes,
  pas comme variantes de Talent ;
- le friendly fire canonique des bombes ;
- les trois chaînes de progression ;
- le vocabulaire joueur ;
- ALCH-01 à ALCH-05.

Après validation :

```text
UI-RPG-DESC01.12 — normalisation croisée des 90 Talents
```
