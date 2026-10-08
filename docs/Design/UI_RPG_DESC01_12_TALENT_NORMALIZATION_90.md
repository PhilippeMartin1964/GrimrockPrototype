# UI-RPG-DESC01.12 — Normalisation croisée des 90 Talents

Date : **8 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
État : **VALIDÉ PAR L'UTILISATEUR — 8 octobre 2026**  
Dépendances : **UI-RPG-DESC01.5 à .11 validés**  
Périmètre : **6 classes / 18 branches / 90 nœuds conceptuels**

## 1. Objet

DESC01.12 ne réécrit pas les mécaniques des six classes. Il transforme les six
audits validés en un **contrat sémantique unique** avant la conception du
read-model DESC01.13.

Sources principales :

```text
docs/Design/UI_RPG_DESC01_5_TALENT_UX_CONTRACT.md
docs/Design/UI_RPG_DESC01_6_WARRIOR_TALENT_AUDIT.md
docs/Design/UI_RPG_DESC01_7_ROGUE_TALENT_AUDIT.md
docs/Design/UI_RPG_DESC01_8_RANGER_TALENT_AUDIT.md
docs/Design/UI_RPG_DESC01_9_MAGE_TALENT_AUDIT.md
docs/Design/UI_RPG_DESC01_10_PRIEST_TALENT_AUDIT.md
docs/Design/UI_RPG_DESC01_11_ALCHEMIST_TALENT_AUDIT.md
docs/Rules/RPG_Talents_Mechanics_v0_1.md
docs/Design/UI_RPG06_1_SKILLS_AUDIT_CATALOG_AUTHORING.md
```

Ce jalon reste **strictement documentaire** :

- aucun C++ ;
- aucun UMG ;
- aucun DataAsset ;
- aucune modification de gameplay ;
- aucune compensation UI pour un bug runtime.

## 2. Invariants 90/90

Le contrat final couvre exactement :

```text
6 classes
18 branches
90 nœuds conceptuels
5 paliers par branche
niveaux canoniques : 2 / 6 / 10 / 14 / 18
coût canonique : 1 point de Talent par nœud/Choice acquis
```

Les variantes augmentent le nombre de Choice records concrets, mais **jamais**
le nombre de nœuds conceptuels affichés dans l'arbre.

## 3. Taxonomie TYPE finale

La taxonomie de DESC01.5 est conservée sans extension.

| TYPE | Nombre | Règle |
|---|---:|---|
| **ACTIF** | 32 | activation volontaire non présentée comme sort |
| **SORT ACTIF** | 22 | activation volontaire explicitement magique / sort |
| **PASSIF** | 22 | bonus sans activation volontaire |
| **RÉACTION AUTOMATIQUE** | 5 | déclenchement automatique sur événement gameplay |
| **RECETTE + OBJET RAPIDE** | 8 | droit de recette + objet consommable utilisant le pipeline QuickItem |
| **RECETTE + ACTIF** | 1 | droit de recette + action volontaire dépendant de cette recette |
| **TOTAL** | **90** | |

Le TYPE est une propriété de la **mécanique**, jamais du statut d'acquisition.

Un Mage n'est donc pas automatiquement `SORT ACTIF` : par exemple
**Surcharge élémentaire** reste `ACTIF`, car l'activation elle-même est une
capacité préparatoire, tandis que **Chaîne élémentaire** est un `SORT ACTIF`.

## 4. Matrice canonique 90/90

### 4.1 Guerrier — 15

| Branche | Niv. | Talent | TYPE | Particularité |
|---|---:|---|---|---|
| Gardien | 2 | Posture défensive | ACTIF | — |
| Gardien | 6 | Coup de bouclier | ACTIF | bouclier requis |
| Gardien | 10 | Interception | RÉACTION AUTOMATIQUE | une seule interception par attaque |
| Gardien | 14 | Rempart | PASSIF | — |
| Gardien | 18 | Forteresse | ACTIF | — |
| Brise-ligne | 2 | Coup puissant | ACTIF | — |
| Brise-ligne | 6 | Brise-armure | ACTIF | — |
| Brise-ligne | 10 | Balayage | ACTIF | zone |
| Brise-ligne | 14 | Exécution | ACTIF | — |
| Brise-ligne | 18 | Ravage | ACTIF | zone |
| Maître d'armes | 2 | Spécialisation martiale | PASSIF | **VARIANTES** Tranchant / Perforant / Contondant |
| Maître d'armes | 6 | Riposte | RÉACTION AUTOMATIQUE | mains nues autorisées |
| Maître d'armes | 10 | Second souffle | ACTIF | indisponible à PV maximum selon contrat |
| Maître d'armes | 14 | Maîtrise critique | PASSIF | — |
| Maître d'armes | 18 | Seigneur de guerre | ACTIF | — |

Répartition : **10 ACTIF / 3 PASSIF / 2 RÉACTIONS AUTOMATIQUES**.

### 4.2 Voleur — 15

| Branche | Niv. | Talent | TYPE | Particularité |
|---|---:|---|---|---|
| Assassin | 2 | Attaque sournoise | ACTIF | — |
| Assassin | 6 | Frappe dans le dos | PASSIF | — |
| Assassin | 10 | Hémorragie | ACTIF | — |
| Assassin | 14 | Point faible | ACTIF | — |
| Assassin | 18 | **Mise à mort** | ACTIF | nom joueur canonique ; authoring historique « Finisseur » |
| Ombre | 2 | Esquive | ACTIF | — |
| Ombre | 6 | Disparition courte | ACTIF | — |
| Ombre | 10 | Pas de l'ombre | ACTIF | portée +1 de la prochaine attaque légère de mêlée |
| Ombre | 14 | Insaisissable | RÉACTION AUTOMATIQUE | — |
| Ombre | 18 | Ombre parfaite | ACTIF | — |
| Saboteur | 2 | Désamorçage expert | PASSIF | consommateur métier d'échec sûr encore à raccorder |
| Saboteur | 6 | Piège rapide | ACTIF | — |
| Saboteur | 10 | Bombe fumigène | ACTIF | — |
| Saboteur | 14 | Maître des serrures | PASSIF | consommateur métier d'échec sûr encore à raccorder |
| Saboteur | 18 | Sabotage | ACTIF | Skill Mécanique disponible ; caller monde à raccorder |

Répartition : **11 ACTIF / 3 PASSIF / 1 RÉACTION AUTOMATIQUE**.

### 4.3 Rôdeur — 15

| Branche | Niv. | Talent | TYPE | Particularité |
|---|---:|---|---|---|
| Tireur | 2 | Tir précis | ACTIF | — |
| Tireur | 6 | Tir perforant | ACTIF | — |
| Tireur | 10 | Tir rapide | ACTIF | résolutions répétées |
| Tireur | 14 | Volée | ACTIF | zone |
| Tireur | 18 | Œil d'aigle | PASSIF | Perception |
| Chasseur | 2 | Marque de la proie | ACTIF | — |
| Chasseur | 6 | Ennemi juré | PASSIF | **VARIANTES dynamiques** depuis le bestiaire |
| Chasseur | 10 | Tir immobilisant | ACTIF | — |
| Chasseur | 14 | Frappe du prédateur | ACTIF | — |
| Chasseur | 18 | Chasseur alpha | RÉACTION AUTOMATIQUE | mort de sa propre cible marquée |
| Éclaireur | 2 | Vigilance | PASSIF | bonus de groupe |
| Éclaireur | 6 | Piège de chasse | ACTIF | — |
| Éclaireur | 10 | Repli tactique | ACTIF | — |
| Éclaireur | 14 | Maître du terrain | PASSIF | — |
| Éclaireur | 18 | Guide du groupe | PASSIF | bonus de groupe |

Répartition : **9 ACTIF / 5 PASSIF / 1 RÉACTION AUTOMATIQUE**.

### 4.4 Mage — 15

| Branche | Niv. | Talent | TYPE | Particularité |
|---|---:|---|---|---|
| Évocateur | 2 | Affinité élémentaire | PASSIF | **VARIANTES** Feu / Glace / Air / Terre |
| Évocateur | 6 | Surcharge élémentaire | ACTIF | capacité préparatoire, pas SORT ACTIF |
| Évocateur | 10 | Explosion contrôlée | PASSIF | réduction du friendly fire direct |
| Évocateur | 14 | Chaîne élémentaire | SORT ACTIF | chaîne de cibles |
| Évocateur | 18 | Cataclysme | SORT ACTIF | friendly fire explicite |
| Arcaniste | 2 | Bouclier arcanique | SORT ACTIF | — |
| Arcaniste | 6 | Dissipation | SORT ACTIF | allié ou ennemi |
| Arcaniste | 10 | Manipulation runique | PASSIF | Runes |
| Arcaniste | 14 | Téléportation courte | SORT ACTIF | déplacement du groupe |
| Arcaniste | 18 | Maîtrise de l'Arcane | PASSIF | — |
| Tisseur de surfaces | 2 | Imprégnation | SORT ACTIF | **VARIANTES** Feu / Glace / Air / Terre |
| Tisseur de surfaces | 6 | Conversion élémentaire | SORT ACTIF | surfaces |
| Tisseur de surfaces | 10 | Conduction | PASSIF | surfaces |
| Tisseur de surfaces | 14 | Surface persistante | PASSIF | surfaces |
| Tisseur de surfaces | 18 | Architecte du terrain | SORT ACTIF | surfaces |

Répartition : **1 ACTIF / 8 SORTS ACTIFS / 6 PASSIFS**.

### 4.5 Prêtre — 15

| Branche | Niv. | Talent | TYPE | Particularité |
|---|---:|---|---|---|
| Restauration | 2 | Soin renforcé | PASSIF | — |
| Restauration | 6 | Régénération | SORT ACTIF | — |
| Restauration | 10 | Soin de groupe | SORT ACTIF | groupe |
| Restauration | 14 | Purification | SORT ACTIF | retrait d'effets |
| Restauration | 18 | Miracle | SORT ACTIF | — |
| Protection | 2 | Bénédiction | SORT ACTIF | — |
| Protection | 6 | Égide | SORT ACTIF | — |
| Protection | 10 | Protection sacrée | SORT ACTIF | — |
| Protection | 14 | Sanctuaire | SORT ACTIF | — |
| Protection | 18 | Bastion divin | SORT ACTIF | — |
| Exorcisme | 2 | Lumière sacrée | SORT ACTIF | — |
| Exorcisme | 6 | Repousser les morts-vivants | SORT ACTIF | contrat : 4 dégâts fixes |
| Exorcisme | 10 | Dissipation sacrée | SORT ACTIF | allié / ennemi |
| Exorcisme | 14 | Châtiment | SORT ACTIF | — |
| Exorcisme | 18 | Exorcisme majeur | SORT ACTIF | Mort-vivant / Démon / Invoqué |

Répartition : **14 SORTS ACTIFS / 1 PASSIF**.

### 4.6 Alchimiste — 15

| Branche | Niv. | Talent | TYPE | Particularité |
|---|---:|---|---|---|
| Grenadier | 2 | Bombe incendiaire | RECETTE + OBJET RAPIDE | friendly fire |
| Grenadier | 6 | Bombe toxique | RECETTE + OBJET RAPIDE | friendly fire |
| Grenadier | 10 | Charge précise | PASSIF | réduit uniquement dégâts directs alliés |
| Grenadier | 14 | Réaction en chaîne | RÉACTION AUTOMATIQUE | dette runtime ALCH-02 |
| Grenadier | 18 | Maître grenadier | PASSIF | — |
| Apothicaire | 2 | Potion renforcée | PASSIF | — |
| Apothicaire | 6 | Antidote | RECETTE + OBJET RAPIDE | no-op refusé avant consommation |
| Apothicaire | 10 | Élixir défensif | RECETTE + OBJET RAPIDE | 4 recettes, **pas des variantes** |
| Apothicaire | 14 | Diffusion | PASSIF | secondaire 50 % selon ALCH-03 |
| Apothicaire | 18 | Panacée | RECETTE + OBJET RAPIDE | — |
| Transmutateur | 2 | Huile glissante | RECETTE + OBJET RAPIDE | surface |
| Transmutateur | 6 | Flasque acide | RECETTE + OBJET RAPIDE | armure physique, aucun débordement PV |
| Transmutateur | 10 | Nuage corrosif | RECETTE + OBJET RAPIDE | surface |
| Transmutateur | 14 | Catalyseur | ACTIF | interaction de surface |
| Transmutateur | 18 | Transmutation majeure | RECETTE + ACTIF | 4 recettes, **pas des variantes** ; dettes ALCH-04/05 |

Répartition :
**8 RECETTE + OBJET RAPIDE / 4 PASSIF / 1 RÉACTION AUTOMATIQUE /
1 ACTIF / 1 RECETTE + ACTIF**.

## 5. Les quatre seuls nœuds à VARIANTES

Le mot **VARIANTE** est réservé à un ensemble de Choice records exclusifs regroupés
sous un même `TalentNodeId`.

| Classe | Nœud | Variantes | Autorité |
|---|---|---|---|
| Guerrier | Spécialisation martiale | Tranchant / Perforant / Contondant | ProgressionChoices |
| Rôdeur | Ennemi juré | une par catégorie réelle du bestiaire | bestiaire + authoring |
| Mage | Affinité élémentaire | Feu / Glace / Air / Terre | ProgressionChoices |
| Mage | Imprégnation | Feu / Glace / Air / Terre | ProgressionChoices |

Règles communes :

1. toutes les variantes sont visibles simultanément ;
2. aucun dropdown final ;
3. consulter une variante n'est pas la choisir ;
4. `CHOISIR` n'apparaît que pour une variante réellement acquérable ;
5. après confirmation, la variante possédée est `ACQUISE` ;
6. les variantes du même groupe deviennent
   `INDISPONIBLES — autre variante déjà choisie` ;
7. Affinité élémentaire et Imprégnation ont deux groupes indépendants ;
8. Ennemi juré ne duplique jamais le catalogue des catégories dans l'UI.

**Élixir défensif** et **Transmutation majeure** ne sont pas des variantes :
leurs quatre recettes sont accordées ensemble.

## 6. Structure unique de fiche

La structure de DESC01.5 devient la structure normative 90/90 :

```text
NOM

TYPE
<nature mécanique>

STATUT
<état d'acquisition du Talent>

PRINCIPE
<quand / pourquoi / comment>

EFFETS
<résultats exacts>

[UTILISATION]
<activation volontaire uniquement>

[VARIANTES]
<uniquement pour les 4 nœuds concernés>

ACQUISITION
<niveau / coût / prérequis / exclusivité / recettes accordées>
```

Aucune classe, branche ou TYPE ne change cet ordre.

Une section absente est masquée ; elle n'est jamais remplacée par une formulation
spéciale propre à la classe.

## 7. STATUT ne parle que du Talent

STATUT répond exclusivement à :

> Le personnage possède-t-il ce Talent et, sinon, peut-il l'acquérir ?

Vocabulaire :

```text
ACQUIS
DISPONIBLE
VERROUILLÉ — niveau X requis
VERROUILLÉ — nécessite « Talent X »
VERROUILLÉ — nécessite N point(s) de Talent
INDISPONIBLE — autre variante déjà choisie
```

La raison principale vient du service de progression existant.
L'UMG ne recalcule jamais cette priorité.

Sont interdits dans STATUT **et comme bandeau de mécanique** :

```text
ACTION DISPONIBLE
ACTION DÉBLOQUÉE
ACTION ACCORDÉE APRÈS ACQUISITION
CAPACITÉ DÉBLOQUÉE
OBJET DISPONIBLE
RECETTE DISPONIBLE
```

Une action peut être décrite avant acquisition sans être « disponible ».

## 8. Disponibilité d'action : hors du statut Talent

La fiche Talent décrit **ce que fera** l'action grâce à EFFETS / UTILISATION.

La disponibilité réelle d'une action en combat dépend d'autres autorités :

```text
Talent réellement acquis
+ personnage actif / vivant
+ PA
+ mana
+ cooldown
+ équipement requis
+ objet présent et quantité suffisante
+ cible valide
+ ligne de vue
+ état du combat
+ autres conditions gameplay
```

Cette disponibilité appartient au catalogue/actions de combat, pas au statut du
Talent.

Conséquence normative :

> La fiche Talent ne doit jamais déduire « action disponible » de la simple
> consultation ou acquisition d'un nœud.

## 9. UTILISATION — ordre et vocabulaire uniques

Pour toute activation volontaire, les champs présents sont ordonnés ainsi :

```text
Coût : N point(s) d'action
Mana : N
[PAM : N]
[Objet consommé : N <objet>]
Cible : <texte joueur>
[Portée : N case(s)]
[Zone : rayon N case(s)]
[Ligne de vue : requise]
[Résolutions : N]
[Recharge : N round(s)]
[Condition : ...]
```

Les champs non pertinents sont absents.

Normalisations :

```text
AP / PA / ActionPointCost -> point(s) d'action
ManaCost                  -> mana
MobilityActionPoints      -> PAM
Cooldown                  -> recharge
Range / R4                -> portée : 4 cases
Area1                     -> zone : rayon 1 case
LOS                       -> ligne de vue
SourceItemQuantityCost    -> objet consommé
```

Aucun identifiant technique n'est imprimé.

## 10. Tour et round ne sont pas synonymes

Le runtime distingue deux unités de durée.

### TOUR

`DurationUnit=Turns` progresse lorsque **le combattant concerné termine son
activation**.

Formulation joueur :

```text
pendant N tours
à la fin de chacun de ses tours
```

### ROUND

`DurationUnit=Rounds` progresse à la **frontière du round global de combat**.

Formulation joueur :

```text
pendant N rounds
à chaque nouveau round
```

Les recharges `CooldownRounds` sont toujours exprimées en **rounds**.

Le terme technique **tick** disparaît des fiches finales. Une périodicité est
décrite par l'événement réel :

```text
2 dégâts de Feu à la fin de chacun des tours de la cible
2 dégâts de Poison à chaque nouveau round
```

selon l'unité autoritaire du Status/Surface concerné.

## 11. Ciblage joueur normalisé

Formulations cibles :

```text
Self                 -> soi-même
Ally                 -> soi-même ou un allié vivant
Hostile              -> un ennemi
FirstAxialTarget     -> première cible dans l'axe
Cell                 -> une cellule
Area                 -> une zone autour de la cellule ciblée
Party                -> tous les membres vivants du groupe
FrontRowParty        -> membres vivants du rang avant
AllyOrHostile        -> allié ou ennemi selon l'effet
```

Pour `Ally`, la formulation « soi-même ou un allié vivant » est utilisée lorsque
la définition n'ajoute pas de filtre excluant le lanceur.

La portée, la zone et la ligne de vue restent des champs séparés.

## 12. Dégâts, armures et scalings

Formes joueur :

```text
WD / WeaponDamagePercent -> % des dégâts de l'arme
Accuracy                 -> Précision
Evasion                  -> Esquive
PhysicalArmor            -> armure physique
MagicalArmor             -> armure magique
MaxHP                    -> PV maximum
RestoreHealth            -> PV restaurés / soin
RestoreMana              -> mana restauré
Resistance               -> résistance
Crit                      -> coup critique
```

Scaling :

```text
+ STR mod -> + modificateur de Force
+ DEX mod -> + modificateur de Dextérité
+ INT mod -> + modificateur d'Intelligence
+ WIS mod -> + modificateur de Sagesse
+ CHA mod -> + modificateur de Charisme

+ Skill rank -> + rang de <nom joueur de la compétence>
```

Les noms de Skills viennent des `URPGSkillAsset::DisplayName` de production.

## 13. ArmorGate

Une condition de statut dépendant de l'armure est toujours formulée avec le moment
de résolution :

```text
Si l'armure physique est épuisée après les dégâts : ...
Si l'armure magique est épuisée après les dégâts : ...
```

On évite :

```text
si armure = 0
si PhysicalArmor=0
si MagicalArmor=0
```

lorsqu'une formulation joueur est possible.

## 14. Friendly fire

Lorsqu'une action possède réellement `bAffectsAlliesInArea`, la fiche ne doit
jamais résumer l'effet par « chaque ennemi » sans préciser le risque pour le groupe.

Cas actuellement normalisés :

### Mage — Cataclysme

Avec le prérequis obligatoire Explosion contrôlée :

```text
ennemis : dégâts directs normaux
Mage : 0 % de dégâts directs alliés
autres alliés : 50 % des dégâts directs
surfaces / statuts : inchangés
```

### Alchimiste — Bombes

Sans Charge précise :

```text
alliés dans la zone : 100 % des dégâts directs
statuts / surfaces : normaux
```

Avec Charge précise :

```text
alliés dans la zone : 50 % des dégâts directs
statuts / surfaces : normaux
```

La réduction de dégâts directs n'implique jamais automatiquement une réduction de
Status Effects ou de surfaces.

## 15. PASSIF et RÉACTION AUTOMATIQUE

### PASSIF

Un PASSIF modifie une règle sans posséder d'action volontaire propre.

La fiche contient :

```text
TYPE
PASSIF

PRINCIPE
EFFETS
ACQUISITION
```

UTILISATION est absente.

### RÉACTION AUTOMATIQUE

Les cinq réactions sont :

```text
Guerrier    Interception
Guerrier    Riposte
Voleur      Insaisissable
Rôdeur      Chasseur alpha
Alchimiste  Réaction en chaîne
```

La fiche doit exposer systématiquement :

```text
Déclencheur
Fréquence / limite
Effet
Exclusions importantes
```

Elle ne doit jamais utiliser « action disponible », car le joueur ne déclenche pas
la réaction manuellement.

## 16. Recettes et objets rapides

Pour les Talents Alchimiste :

```text
Talent acquis
    != recette fabriquée
    != objet possédé
    != action QuickItem actuellement disponible
```

ACQUISITION peut afficher :

```text
Recette accordée : ...
Recettes accordées : ...
```

UTILISATION peut documenter la mécanique de l'objet :

```text
Objet consommé : 1 ...
```

mais la fiche Talent ne prétend jamais que l'objet est actuellement présent dans
l'inventaire.

Le moteur de Crafting reste une fonctionnalité future.

## 17. Skills — dépendances normalisées

UI-RPG06.2A a matérialisé **25 Skill assets de production**.

Les anciennes mentions des audits disant que le catalogue de production n'existe
pas sont donc obsolètes.

Dépendances désormais satisfaites notamment :

```text
Voleur :
    Crochetage
    Pièges / désamorçage
    Mécanique

Rôdeur :
    Perception
    Nature
    Histoire
    Religion
    Survie

Mage :
    Arcane
    Runes

Prêtre :
    Médecine
    Religion

Alchimiste :
    Alchimie
```

Ce qui reste éventuellement manquant n'est **pas le Skill**, mais un consommateur
métier particulier, par exemple la conséquence d'un échec sûr sur une serrure ou
un piège.

Aucun catalogue Skill parallèle n'est autorisé dans DESC01.

## 18. Noms et identités

### Nom joueur

La fiche utilise le `DisplayName` canonique, jamais le ChoiceId.

Décision déjà validée :

```text
Voleur palier V Assassin :
    nom joueur = Mise à mort
    authoring historique = Finisseur
```

L'authoring sera harmonisé plus tard ; l'UMG ne doit pas contenir un alias manuel
spécifique au Voleur.

### Catégories d'Ennemi juré

Les variantes restent dérivées du bestiaire réel.

La présentation finale doit utiliser une autorité de libellé de catégorie. Un
`CategoryId` brut ne devient pas le nom joueur par simple suppression
d'underscores.

## 19. Écoles / types de dégâts Mage

Les quatre noms d'affinité restent :

```text
Feu
Glace
Air
Terre
```

Ils décrivent des **écoles**, pas nécessairement des `DamageType` de même nom.

Décision MAGE-01 conservée :

```text
Air   -> peut produire des dégâts de Foudre
Terre -> peut produire des dégâts Physiques ou des surfaces Huile/Poison
        selon la capacité authorée
```

Il n'existe pas de `DamageType Terre` à inventer.

Affinité Évocateur et Affinité Tisseur restent deux choix exclusifs indépendants.

## 20. Dettes mécaniques après normalisation

La normalisation ne transforme pas une promesse documentaire en mécanique réelle.

### D01 — Guerrier / Second souffle — CLOS

Contrat canonique :

```text
indisponible à PV maximum
```

**RPG-TALENT-FIX02** confirme que le garde-fou existe déjà dans le catalogue
générique des actions de classe : une action `Self + Effect` dont le soin ne
peut augmenter les PV reçoit `NoApplicableEffect`.

Un test de régression utilisant le vrai `Action_Warrior_SecondWind` vérifie que
l'action est active lorsque le Guerrier est blessé et désactivée à PV maximum.

Validation finale : **12/12 `Grimrock.RPG.RPG03.9.1`, 0 warning, 0 échec**.

Aucun `if SecondWind` runtime n'est ajouté.

### D02 — Voleur / échecs sûrs — DÉPENDANCE LOCK/TRAP

Contrats cibles :

```text
Désamorçage expert -> le piège reste armé mais ne se déclenche pas
Maître des serrures -> la serrure ne se bloque/jamme pas sur un échec sûr
```

Les primitives `FRPGSkillProgressionModifier::SafeFailureMargin` et
`FRPGSkillCheckResult::bSafeFailure` sont déjà opérationnelles et testées.

L'audit du 8 octobre 2026 confirme en revanche que le runtime courant ne possède
pas encore le consommateur métier correspondant :

- aucune API `TryDisarmTrap` ;
- aucun état runtime de piège de serrure ;
- aucun caller de crochetage/désamorçage ;
- `AGridWallLockActor` ne gère aujourd'hui que les clés ;
- le futur domaine est déjà spécifié dans
  `docs/Design/GRIMROCK_LOCK_SYSTEM.md`.

**Décision :** ne pas créer un sous-système ad hoc dans la vague Talent.
D02a/D02b seront implémentés avec le vrai système Lock/Trap, qui devra consommer
directement le `bSafeFailure` existant.

### D03 — Voleur / Sabotage monde — DÉPENDANCE ACTIONS HORS COMBAT

Le Skill **Mécanique** existe en production. Le runtime possède déjà les deux
extrémités génériques du contrat :

```text
UGridWorldObjectDefinitionAsset
    bCanBeSabotaged
    SabotageDifficulty

AGridLevelRuntimeActor
    GetRuntimeObjectSabotageDifficulty()
    ExecuteRuntimeObjectSabotage() -> EGridObjectEvent::Sabotaged
```

L'audit du 8 octobre 2026 confirme cependant qu'aucun caller n'exécute encore le
Skill Check `Skill_Mechanics` entre ces deux extrémités.

**Décision :** ne pas ajouter un `if Talent_Rogue_Saboteur_Sabotage` dans un
acteur de monde. D03 sera raccordé lorsque le projet disposera du vrai flux
générique « aptitude hors combat -> objet du monde -> Skill Check -> événement ».
Le combat Sabotage reste déjà opérationnel et indépendant de cette dette.

### D04 — Rôdeur / Ennemi juré

Les catégories sont dynamiques et fonctionnelles, mais une autorité de
`DisplayName` de catégorie reste nécessaire pour une présentation de production
sans identifiant technique.

### D05 — Prêtre / Repousser les morts-vivants — SOURCE CORRIGÉE

Contrat canonique validé :

```text
4 dégâts sacrés fixes
```

**RPG-TALENT-FIX01** neutralise explicitement le scaling Sagesse sur
`Action_Priest_TurnUndead` tout en conservant le helper sacré commun pour les
autres sorts.

État : **VALIDÉ / CLOS** — source corrigée, assets Prêtre rematérialisés et
`Grimrock.RPG.RPG03.9.5` validé 21/21 sans warning ni échec.

### D06 — Alchimiste / Réaction en chaîne — SOURCE CORRIGÉE

Le profil automatique existe déjà :

```text
SurfaceReaction
QuickItem.Bomb
OncePerAction
dégâts de réaction +25 %
rayon +1
anti-récursion
```

**RPG-TALENT-FIX04** raccorde désormais les `SurfaceEffects` élémentaires au
même pipeline générique `SurfaceReaction` que les interactions explicites.

Une surface entrante Feu utilise l'interaction canonique Fire ; une surface
entrante Glace utilise Ice. Aucune interaction Poison/Oil/etc. n'est inventée.

État : **source + test de pont ajoutés ; validation locale requise avant clôture
D06.**

### D07 — Alchimiste / Transmutation majeure — durée

Contrat canonique :

```text
toute cellule convertie -> durée finale 4 rounds
```

Le resolver actuel conserve la durée restante d'une surface préexistante.

### D08 — Alchimiste / Transmutation majeure — bonus de réaction

Le +50 % de dégâts de réaction est authoré mais n'a pas de chemin d'exécution
complet avec les conversions actuelles.

Décision DESC01.11 conservée :

- ne pas le présenter comme garantie actuelle ;
- décider ultérieurement entre une vraie règle générique conversion→réaction ou
  la suppression de ce bonus mort.

### D09 — Alchimiste / Crafting

Les `Recipe_*` sont accordés comme droits, les QuickItems existent pour plusieurs
produits, mais aucun moteur d'ingrédients/recettes autoritaire n'existe encore.

La fiche peut documenter la recette accordée ; elle ne peut pas prétendre qu'un
objet a été fabriqué.

## 21. Règle de projection face aux dettes

DESC01.13 devra respecter trois catégories :

### A — mécanique runtime autoritaire et conforme

Projection normale depuis les données.

### B — contrat v0.1 validé mais runtime à corriger

Exemples :

```text
Second souffle à PV max
Repousser les morts-vivants = 4 fixes
Réaction en chaîne
Transmutation majeure = 4 rounds
```

La solution n'est **jamais** de hard-coder le texte cible dans l'UMG pour masquer
la divergence.

Le gameplay/source devra rejoindre le contrat avant la clôture DESC01.16.

### C — fonctionnalité future non garantie

Exemples :

```text
conséquence métier des échecs sûrs
moteur de Crafting
+50 % de réaction de Transmutation majeure tant que sa sémantique n'est pas définie
```

Ces éléments ne sont pas présentés comme fonctionnalités actuellement
opérationnelles.

## 22. Ce que le read-model DESC01.13 devra pouvoir exprimer

Sans encore définir de struct C++, l'audit 90/90 exige au minimum la capacité de
représenter :

```text
identité conceptuelle du nœud
nom joueur
TYPE
STATUT + raison
PRINCIPE
EFFETS structurés
UTILISATION structurée
acquisition niveau / coût / prérequis
variante exclusive
groupe d'exclusivité
toutes les variantes visibles simultanément
recettes accordées non exclusives
coût PA / mana / PAM / objet
cible / portée / zone / LOS
recharge
nombre de résolutions
condition d'équipement
dégâts / armure / restauration
scaling attribut / Skill
ArmorGate
Status Effect + durée + unité
surface + durée
friendly fire
réaction automatique + déclencheur + limite
modificateurs personnels / groupe
interaction / conversion de surface
distinction Talent / recette / objet / action
```

Le read-model doit rester **read-only** et dérivé des autorités existantes.

## 23. Interdictions pour DESC01.13+

Le futur code de présentation ne doit pas :

```text
switch(ClassId) pour construire les fiches
switch(TalentId) pour fabriquer des phrases gameplay
dupliquer les valeurs numériques des authoring/DataAssets
créer un registre parallèle de Talents
créer un registre parallèle de Skills
humaniser silencieusement des identifiants comme autorité de production
recalculer le statut d'acquisition dans UMG
inférer "action disponible" depuis le focus de consultation
utiliser une ComboBox pour comprendre les variantes
présenter une recette multiple comme choix exclusif
masquer une divergence gameplay par un texte UI mensonger
```

Des helpers génériques de formatage joueur seront autorisés si leur entrée reste
un read-model déjà autoritaire et structuré.

## 24. Contrat transmis au thread UI-RPG-VISUAL01

VISUAL01 reçoit désormais une sémantique stable :

```text
NOM
TYPE
STATUT
PRINCIPE
EFFETS
[UTILISATION]
[VARIANTES]
ACQUISITION
```

Il peut décider :

- typographie ;
- espacements ;
- couleurs ;
- séparateurs ;
- style des badges TYPE / STATUT ;
- apparence du focus ;
- apparence de CHOISIR / CONFIRMER / ANNULER ;
- composition visuelle des variantes ;
- ScrollBox.

Il ne peut pas :

- renommer les sections ;
- fusionner TYPE et STATUT ;
- transformer le focus en acquisition ;
- cacher une variante non choisie ;
- recréer un statut « action disponible » ;
- changer l'ordre sémantique.

## 25. Critères de validation UI-RPG-DESC01.12

DESC01.12 a été validé par l'utilisateur le **8 octobre 2026**. Ont été approuvés :

1. la matrice TYPE **90/90** ;
2. les **4 seuls nœuds à variantes** ;
3. la séparation variante / recette multiple ;
4. l'interdiction définitive des statuts « ACTION DISPONIBLE / DÉBLOQUÉE » ;
5. l'ordre et le vocabulaire communs de UTILISATION ;
6. la distinction **tour / round** ;
7. la normalisation globale du ciblage ;
8. le vocabulaire dégâts / armures / scalings / ArmorGate ;
9. le traitement explicite du friendly fire ;
10. la mise à jour des dépendances Skills désormais matérialisées ;
11. le registre D01 à D09 ;
12. la règle « aucune compensation UI pour une mécanique runtime divergente » ;
13. les exigences sémantiques transmises à DESC01.13 et VISUAL01.

Après validation :

```text
UI-RPG-DESC01.13 — contrat du read-model Talent
```

Aucun nouveau C++ / UMG / DataAsset DESC01 n'est autorisé avant cette validation.
