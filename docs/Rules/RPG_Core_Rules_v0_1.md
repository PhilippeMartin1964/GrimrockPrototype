# RPG Core Rules v0.1

## Objet du document

Ce document définit le noyau de règles JdR du prototype **GrimrockPrototype**.

Le système vise un dungeon crawler en vue subjective, à déplacement case par case, inspiré par :

- **Legend of Grimrock 2** pour l'exploration, la grille, les énigmes et la formation du groupe ;
- **Divinity: Original Sin 2** pour les états, les surfaces, les synergies élémentaires et les interactions tactiques ;
- **les JdR de type D20 / 3.5+** pour les caractéristiques, compétences, dons, classes, jets et progression.

L'objectif n'est pas de reproduire exactement ces systèmes, mais de créer une base simple, modulaire et orientée données, compatible avec Unreal Engine 5.5.4.

---

## Principes généraux

Le joueur contrôle un groupe de **1 à 6 personnages**.

Le système doit rester jouable avec un seul personnage, mais il est conçu pour un groupe complet de six personnages.

Le groupe occupe une seule case de la grille du donjon, mais possède une formation interne composée de deux rangs de trois emplacements :

```text
[Avant gauche]   [Avant centre]   [Avant droite]
[Arrière gauche] [Arrière centre] [Arrière droite]
```

Chaque personnage possède :

- une identité ;
- une race ;
- une classe ;
- six caractéristiques ;
- des valeurs dérivées ;
- des compétences ;
- des dons ;
- des capacités actives ;
- éventuellement des sorts ;
- un équipement ;
- des états actifs.

Le système doit être entièrement extensible par DataAssets lorsque cela est possible.

---

## Formation du groupe

### Emplacements

La formation standard est composée de six emplacements :

| Slot | Nom interne recommandé | Rang | Rôle naturel |
|---|---|---|---|
| 0 | `FrontLeft` | Avant | mêlée, bouclier, guerrier |
| 1 | `FrontCenter` | Avant | tank principal, paladin, guerrier lourd |
| 2 | `FrontRight` | Avant | mêlée, armes rapides, barbare |
| 3 | `BackLeft` | Arrière | distance, voleur, rôdeur |
| 4 | `BackCenter` | Arrière | mage, prêtre, soutien |
| 5 | `BackRight` | Arrière | alchimiste, archer, lanceur secondaire |

### Règles de position

Les personnages du rang avant peuvent attaquer normalement au corps à corps.

Les personnages du rang arrière ne peuvent attaquer que si leur arme ou capacité le permet :

- arc ;
- arbalète ;
- arme de jet ;
- lance ;
- hallebarde ;
- sort ;
- bombe alchimique ;
- capacité de soutien.

Une arme courte utilisée depuis l'arrière ne doit normalement pas pouvoir toucher un ennemi situé devant le groupe.

### Groupe incomplet

Le système doit accepter un groupe de taille variable :

- 1 personnage ;
- 2 personnages ;
- 3 personnages ;
- 4 personnages ;
- 5 personnages ;
- 6 personnages.

Il ne faut donc pas coder la logique en supposant que les six emplacements sont toujours occupés.

Recommandation C++ :

```cpp
UENUM(BlueprintType)
enum class EPartySlot : uint8
{
    FrontLeft,
    FrontCenter,
    FrontRight,
    BackLeft,
    BackCenter,
    BackRight
};
```

Le groupe peut être représenté par :

```cpp
TArray<FRPGPartyMember> PartyMembers;
```

avec une limite :

```cpp
MaxPartySize = 6;
```

---

## Structure d'un personnage

Un personnage est défini par les couches suivantes :

```text
Personnage
├── Identité
│   ├── Nom
│   ├── Race
│   ├── Classe principale
│   ├── Portrait
│   └── Éventuellement mesh ou représentation visuelle
│
├── Caractéristiques
│   ├── Force
│   ├── Dextérité
│   ├── Constitution
│   ├── Intelligence
│   ├── Sagesse
│   └── Charisme
│
├── Valeurs dérivées
│   ├── Points de vie
│   ├── Mana
│   ├── Endurance éventuelle
│   ├── Armure physique
│   ├── Armure magique
│   ├── Initiative
│   ├── Précision
│   ├── Esquive
│   └── Résistances
│
├── Compétences
│   ├── Combat
│   ├── Exploration
│   ├── Technique
│   ├── Savoir
│   └── Social éventuel
│
├── Dons
│   ├── Dons de combat
│   ├── Dons magiques
│   ├── Dons raciaux
│   └── Dons utilitaires
│
└── Capacités actives
    ├── Attaques spéciales
    ├── Sorts
    ├── Pouvoirs raciaux
    └── Talents de classe
```

---

## Caractéristiques

Le système utilise six caractéristiques classiques.

| Caractéristique | Rôle principal |
|---|---|
| Force | dégâts de mêlée, port de charge, armes lourdes |
| Dextérité | précision, esquive, initiative, armes légères, distance |
| Constitution | points de vie, résistance aux poisons, endurance |
| Intelligence | magie arcane, mémoire, identification, mécanique |
| Sagesse | perception, volonté, magie divine ou naturelle |
| Charisme | commandement, dialogue, intimidation, magie innée |

### Échelle des valeurs

Pour le prototype, les caractéristiques peuvent commencer entre 6 et 20.

| Valeur | Signification |
|---|---|
| 6 | très faible |
| 8 | faible |
| 10 | moyenne humaine |
| 12 | correcte |
| 14 | bonne |
| 16 | très bonne |
| 18 | exceptionnelle |
| 20+ | héroïque ou surnaturelle |

### Modificateur de caractéristique

Le modificateur est calculé ainsi :

```text
Modificateur = floor((Caractéristique - 10) / 2)
```

Exemples :

| Valeur | Modificateur |
|---:|---:|
| 8 | -1 |
| 10 | 0 |
| 12 | +1 |
| 14 | +2 |
| 16 | +3 |
| 18 | +4 |

---

## Valeurs dérivées

### Points de vie

Formule recommandée :

```text
PV = BaseClasse + Niveau × (GainClasse + Modificateur de Constitution)
```

Exemple de valeurs de classe :

| Classe | PV niveau 1 | Gain par niveau |
|---|---:|---:|
| Guerrier | 18 | 8 |
| Rôdeur | 14 | 6 |
| Voleur | 12 | 5 |
| Mage | 8 | 4 |
| Prêtre | 14 | 6 |
| Alchimiste | 12 | 5 |

### Mana et endurance

Pour le premier prototype, la ressource principale doit être la **mana**.

L'endurance peut être ajoutée plus tard pour les techniques physiques, mais elle n'est pas indispensable en v0.1.

Règle v0.1 :

```text
Mana = ressource utilisée par les sorts, pouvoirs spirituels et capacités spéciales magiques.
Cooldown = limitation principale des capacités actives.
```

### Armure physique et armure magique

Chaque personnage possède :

- une armure physique ;
- une armure magique ;
- des points de vie.

Les dégâts physiques réduisent d'abord l'armure physique.

Les dégâts magiques réduisent d'abord l'armure magique.

Lorsque l'armure correspondante est à 0, certains états dangereux peuvent s'appliquer.

| État | Protection concernée |
|---|---|
| Étourdi | armure physique |
| Renversé | armure physique |
| Saignement | armure physique |
| Gelé | armure magique |
| Terrorisé | armure magique |
| Brûlé | armure magique ou résistance au feu |
| Empoisonné | selon le type de poison |
| Maudit | armure magique |

---

## Types de dégâts

### Types physiques

- Tranchant ;
- Perforant ;
- Contondant.

### Types élémentaires ou magiques

Pour la v0.1 :

- Feu ;
- Glace ;
- Foudre ;
- Poison ;
- Sacré ;
- Nécrotique ;
- Arcane.

Extensions possibles :

- Terre ;
- Air ;
- Eau ;
- Psychique.

---

## Races

### Races de base recommandées

Pour le prototype initial, les races recommandées sont :

- Humain ;
- Nain ;
- Elfe ;
- Halfelin ;
- Gnome ;
- Demi-orc.

Extensions possibles :

- Demi-elfe ;
- Aasimar ;
- Tieffelin ;
- autres races propres à l'univers.

### Humain

Rôle : polyvalent.

Bonus proposés :

```text
+1 à toutes les caractéristiques
ou +2 à une caractéristique au choix
+1 don au niveau 1
+1 compétence au choix
```

### Nain

Rôle : robuste, défensif, bon au corps à corps.

Bonus proposés :

```text
+2 Constitution
+1 Force
Résistance au poison
Bonus contre renversement
Vision dans le noir
```

### Elfe

Rôle : agile, perceptif, magie ou distance.

Bonus proposés :

```text
+2 Dextérité
+1 Intelligence
Bonus Perception
Résistance au sommeil magique
Affinité arcane ou arc
```

### Halfelin

Rôle : discret, chanceux, mobile.

Bonus proposés :

```text
+2 Dextérité
+1 Charisme
Petite taille
Bonus Discrétion
Chance : relance occasionnelle
```

### Gnome

Rôle : intellectuel, illusion, mécanique, alchimie.

Bonus proposés :

```text
+2 Intelligence
+1 Constitution
Bonus Mécanique ou Savoir
Affinité illusion
Petite taille
```

### Demi-orc

Rôle : brutal, puissant, intimidant.

Bonus proposés :

```text
+2 Force
+1 Constitution
Fureur : bonus temporaire quand blessé
Vision dans le noir
Bonus Intimidation
```

---

## Classes

### Classes de base recommandées

Pour un groupe complet de six personnages, les classes de base recommandées sont :

- Guerrier ;
- Voleur ;
- Rôdeur ;
- Mage ;
- Prêtre ;
- Alchimiste.

Extensions possibles :

- Paladin ;
- Barbare ;
- Barde ;
- Druide ;
- Magus ;
- Moine ;
- Ensorceleur ;
- Nécromancien.

---

## Guerrier

Rôle : tank, armes lourdes, contrôle physique.

Caractéristiques principales :

```text
Force
Constitution
Dextérité secondaire
```

Compétences fortes :

```text
Armes de mêlée
Armures lourdes
Bouclier
Athlétisme
```

Capacités possibles :

```text
Coup puissant
Provocation
Garde
Brise-armure
Coup de bouclier
Posture défensive
```

---

## Voleur

Rôle : dégâts ciblés, pièges, serrures, coups critiques.

Caractéristiques principales :

```text
Dextérité
Intelligence
Charisme ou Constitution secondaire
```

Compétences fortes :

```text
Discrétion
Crochetage
Pièges
Perception
Armes légères
```

Capacités possibles :

```text
Attaque sournoise
Désamorçage
Esquive
Jet de dague
Frappe dans le dos
Disparition courte
```

Le voleur doit être utile hors combat :

- portes verrouillées ;
- coffres ;
- pièges ;
- plaques suspectes ;
- passages secrets ;
- mécanismes.

---

## Rôdeur

Rôle : distance, survie, exploration, anti-monstres.

Caractéristiques principales :

```text
Dextérité
Sagesse
Constitution
```

Compétences fortes :

```text
Arc
Survie
Perception
Nature
Armes légères
```

Capacités possibles :

```text
Tir précis
Tir perforant
Marque de la proie
Détection de créatures
Piège rudimentaire
Double attaque légère
```

---

## Mage

Rôle : dégâts élémentaires, surfaces, contrôle, utilité.

Caractéristiques principales :

```text
Intelligence
Dextérité secondaire
Constitution faible
```

Compétences fortes :

```text
Arcane
Éléments
Identification
Runes
Savoirs
```

Écoles possibles :

```text
Feu
Glace
Foudre
Terre
Arcane
Illusion
```

Capacités possibles :

```text
Projectile magique
Boule de feu
Éclair
Mur de glace
Télékinésie mineure
Détection magique
Ouverture magique
```

---

## Prêtre

Rôle : soin, protection, sacré, anti-morts-vivants.

Caractéristiques principales :

```text
Sagesse
Charisme
Constitution
```

Compétences fortes :

```text
Foi
Médecine
Volonté
Masse ou marteau
Armure moyenne
```

Capacités possibles :

```text
Soin mineur
Bénédiction
Protection sacrée
Repousser les morts-vivants
Lumière
Dissipation
Sanctuaire
```

---

## Alchimiste

Rôle : potions, bombes, surfaces, altérations.

Caractéristiques principales :

```text
Intelligence
Dextérité
Constitution
```

Compétences fortes :

```text
Alchimie
Poison
Artisanat
Mécanique
Lancer
```

Capacités possibles :

```text
Bombe incendiaire
Bombe toxique
Potion renforcée
Nuage de poison
Huile glissante
Antidote
Acide
```

L'alchimiste relie plusieurs systèmes importants :

- inventaire ;
- objets ramassables ;
- surfaces ;
- effets ;
- énigmes ;
- combat.

---

## Compétences

Les compétences sont divisées en catégories.

### Combat

- Armes de mêlée ;
- Armes à distance ;
- Armes légères ;
- Armes lourdes ;
- Bouclier ;
- Armures.

### Exploration

- Perception ;
- Survie ;
- Discrétion ;
- Athlétisme ;
- Acrobatie.

### Technique

- Crochetage ;
- Désamorçage ;
- Mécanique ;
- Artisanat ;
- Alchimie.

### Savoir

- Arcane ;
- Religion ;
- Nature ;
- Histoire ;
- Monstres ;
- Runes.

### Social, optionnel en v0.1

- Persuasion ;
- Intimidation ;
- Tromperie ;
- Commandement.

### Compétences de groupe

Certaines actions peuvent utiliser le meilleur score disponible parmi les membres du groupe.

Exemples :

```text
Perception du groupe = meilleure Perception des personnages actifs
Identification magique = meilleur score Arcane ou Runes
Détection de pièges = meilleur score Perception ou Désamorçage
```

---

## Dons

Les dons sont des avantages simples, lisibles et faciles à programmer.

### Types de dons

- Dons de combat ;
- Dons défensifs ;
- Dons magiques ;
- Dons raciaux ;
- Dons d'exploration ;
- Dons d'alchimie ;
- Dons de spécialisation.

### Exemples de dons de combat

- Maîtrise de l'épée ;
- Maîtrise de la hache ;
- Maîtrise du bouclier ;
- Coup puissant ;
- Attaque précise ;
- Combat à deux armes ;
- Tir à bout portant ;
- Tir perforant ;
- Armure lourde.

### Exemples de dons défensifs

- Robustesse ;
- Réflexes rapides ;
- Volonté de fer ;
- Résistance au poison ;
- Résistance au feu ;
- Second souffle.

### Exemples de dons magiques

- École renforcée : Feu ;
- École renforcée : Glace ;
- Sort étendu ;
- Sort rapide ;
- Concentration ;
- Réserve de mana ;
- Affinité élémentaire.

### Exemples de dons d'exploration

- Œil attentif ;
- Maître des pièges ;
- Crocheteur ;
- Explorateur souterrain ;
- Détection des passages secrets ;
- Port de charge amélioré.

### Exemples de dons d'alchimie

- Bombes renforcées ;
- Potions efficaces ;
- Maître des poisons ;
- Résistance aux toxines ;
- Nuages persistants.

---

## Magie

La magie est organisée par écoles.

### Écoles recommandées pour la v0.1

- Feu ;
- Glace / Eau ;
- Foudre / Air ;
- Terre / Poison ;
- Arcane ;
- Sacré ;
- Nécrotique.

### Feu

Sorts possibles :

- Étincelle ;
- Projectile de feu ;
- Boule de feu ;
- Mur de flammes ;
- Enflammer une surface.

Effets associés :

- brûlé ;
- surface de feu ;
- explosion avec poison ou huile.

### Glace / Eau

Sorts possibles :

- Jet d'eau ;
- Gel ;
- Armure de givre ;
- Plaque de glace ;
- Extinction de feu.

Effets associés :

- mouillé ;
- gelé ;
- sol glissant ;
- extinction du feu.

### Foudre / Air

Sorts possibles :

- Éclair ;
- Chaîne d'éclairs ;
- Vent violent ;
- Stase ;
- Téléportation courte éventuelle.

Effets associés :

- électrifié ;
- étourdi ;
- eau électrifiée ;
- activation de mécanismes électriques éventuels.

### Terre / Poison

Sorts possibles :

- Rocher ;
- Nuage toxique ;
- Armure de pierre ;
- Flasque acide ;
- Entrave de ronces.

Effets associés :

- empoisonné ;
- ralenti ;
- surface toxique ;
- explosion si feu.

### Arcane

Sorts possibles :

- Projectile magique ;
- Détection magique ;
- Ouverture magique ;
- Bouclier arcanique ;
- Dissipation.

Effets associés :

- force pure ;
- interaction avec runes ;
- résolution d'énigmes magiques.

### Sacré

Sorts possibles :

- Soin ;
- Bénédiction ;
- Lumière ;
- Repousser mort-vivant ;
- Protection.

Effets associés :

- soin ;
- protection ;
- purification ;
- anti-nécrotique ;
- activation d'autels.

### Nécrotique

Sorts possibles :

- Drain de vie ;
- Malédiction ;
- Affaiblissement ;
- Main spectrale ;
- Nuage de mort.

Effets associés :

- affaibli ;
- maudit ;
- drain ;
- peur.

---

## Jets de compétence

Le système utilise un D20 simplifié.

Formule générale :

```text
D20 + modificateur de caractéristique + compétence + bonus divers >= difficulté
```

### Difficultés recommandées

| Difficulté | Valeur |
|---|---:|
| Facile | 8 |
| Normale | 12 |
| Difficile | 16 |
| Très difficile | 20 |
| Héroïque | 25 |

### Exemples

Détecter un passage secret :

```text
D20 + Sagesse + Perception >= 16
```

Crocheter une serrure :

```text
D20 + Dextérité + Crochetage >= 14
```

Identifier une rune :

```text
D20 + Intelligence + Arcane >= 15
```

---

## Combat

Le combat doit rester compatible avec le déplacement case par case.

### Actions possibles

Chaque personnage peut disposer de plusieurs types d'actions :

- attaque principale ;
- capacité active ;
- sort ;
- utilisation d'objet rapide ;
- défense ;
- attente.

### Attaque

Formule proposée :

```text
Jet d'attaque = D20 + précision + bonus arme + modificateur de caractéristique
```

Contre :

```text
Défense = 10 + esquive + bonus d'armure légère + bouclier éventuel
```

Si l'attaque réussit :

```text
Dégâts = dégâts arme + modificateur de caractéristique + bonus
```

Les dégâts sont ensuite appliqués à :

1. armure physique ou magique ;
2. points de vie.

### Équilibrage avec six personnages

Avec six personnages, le groupe peut devenir très puissant. Il faut donc limiter la puissance par la position, les cooldowns, la portée et la nature des armes.

Règles recommandées :

- un personnage arrière ne peut pas frapper avec une arme courte ;
- les armes longues permettent d'attaquer depuis l'arrière ;
- les arcs, arbalètes, armes de jet, sorts et bombes fonctionnent depuis l'arrière ;
- certains ennemis ou obstacles peuvent limiter le nombre de personnages capables de toucher une cible ;
- les sorts de zone et surfaces deviennent un élément central du combat.

---

## États

Les états sont essentiels pour obtenir une profondeur proche de Divinity: Original Sin 2.

### États physiques

- Renversé ;
- Étourdi ;
- Saignement ;
- Entravé ;
- Aveuglé ;
- Ralenti ;
- Désarmé.

### États magiques

- Brûlé ;
- Gelé ;
- Mouillé ;
- Électrifié ;
- Empoisonné ;
- Maudit ;
- Béni ;
- Silence ;
- Terrorisé.

### États utilitaires

- Invisible ;
- Détection accrue ;
- Protection ;
- Régénération ;
- Hâte ;
- Lenteur.

Chaque état devrait être représenté par un DataAsset.

Structure recommandée :

```text
Nom
Icône
Durée
Type
Cumulable ou non
Effet au début du tour ou tick
Effet à la fin du tour ou expiration
Conditions d'application
Effets supprimés
Interactions avec surfaces
```

---

## Surfaces et environnement

Chaque case du donjon peut contenir une surface.

Surfaces proposées :

- aucune ;
- eau ;
- feu ;
- poison ;
- huile ;
- glace ;
- sang ;
- électricité ;
- bénédiction ;
- malédiction.

### Interactions de surfaces

| Surface ou état | Action | Résultat |
|---|---|---|
| Huile | Feu | feu étendu |
| Poison | Feu | explosion toxique |
| Eau | Foudre | eau électrifiée |
| Eau | Glace | glace |
| Feu | Eau | vapeur ou extinction |
| Sang | Foudre | sang électrifié |
| Malédiction | Sacré | purification |
| Feu | Poison | explosion |

### Usage en énigmes

Les surfaces ne sont pas seulement des éléments de combat. Elles peuvent servir à résoudre des énigmes.

Exemples :

```text
Une porte magique s'ouvre si trois plaques sont couvertes par feu, eau et foudre.
```

```text
Un autel maudit doit être purifié par une surface sacrée.
```

```text
Une plaque de pression gelée reste bloquée tant que la glace n'est pas fondue.
```

---

## Progression

Le prototype doit commencer avec une progression courte.

### Niveau maximal recommandé en v0.1

```text
Niveau 1 à 5
```

C'est suffisant pour tester :

- création de personnage ;
- combat ;
- inventaire ;
- sorts ;
- progression ;
- dons ;
- équilibrage.

### Règles de progression

À chaque niveau :

```text
+ points de vie
+ éventuellement mana
+ points de compétence
```

Tous les 2 niveaux :

```text
+ 1 don
```

Tous les 4 niveaux :

```text
+ 1 point de caractéristique
```

Extension future : progression du niveau 1 au niveau 10, puis éventuellement jusqu'au niveau 20.

---

## Groupe de test recommandé

Pour tester le système complet dès le départ :

| Slot | Personnage recommandé | Rôle |
|---|---|---|
| Avant gauche | Guerrier humain | tank secondaire / dégâts |
| Avant centre | Guerrier ou prêtre nain | tank principal |
| Avant droite | Demi-orc guerrier ou rôdeur de mêlée | dégâts physiques |
| Arrière gauche | Voleur halfelin | pièges, serrures, attaque sournoise |
| Arrière centre | Mage elfe ou gnome | magie élémentaire |
| Arrière droite | Alchimiste gnome ou prêtre | soutien, potions, surfaces |

Composition de base idéale :

```text
Guerrier
Voleur
Rôdeur
Mage
Prêtre
Alchimiste
```

---

## Prototype minimal recommandé

Pour éviter de commencer trop large, le prototype v0.1 peut être limité ainsi :

### Races

- Humain ;
- Nain ;
- Elfe ;
- Halfelin ;
- Gnome ;
- Demi-orc.

### Classes

- Guerrier ;
- Voleur ;
- Rôdeur ;
- Mage ;
- Prêtre ;
- Alchimiste.

### Compétences initiales

- Mêlée ;
- Distance ;
- Arcane ;
- Foi ;
- Perception ;
- Crochetage ;
- Alchimie ;
- Mécanique.

### Dons initiaux

- Robustesse ;
- Maîtrise de l'épée ;
- Attaque sournoise ;
- Concentration ;
- Foi renforcée ;
- Œil attentif ;
- Bombes renforcées ;
- Tir précis.

### Sorts initiaux

- Projectile magique ;
- Étincelle ;
- Boule de feu ;
- Jet d'eau ;
- Éclair ;
- Bouclier arcanique ;
- Soin mineur ;
- Bénédiction.

---

## Architecture Unreal Engine recommandée

### Dossier C++ proposé

```text
Source/GrimrockPrototype/Public/RPG/
Source/GrimrockPrototype/Private/RPG/
```

### Fichiers possibles

```text
Public/RPG/
├── RPGTypes.h
├── RPGCharacterData.h
├── RPGRaceAsset.h
├── RPGClassAsset.h
├── RPGSkillAsset.h
├── RPGFeatAsset.h
├── RPGSpellAsset.h
├── RPGStatusEffectAsset.h
├── RPGDamageTypes.h
└── RPGCombatResolver.h
```

### Composants runtime possibles

```text
Public/Runtime/
├── GrimrockPartyComponent.h
├── GrimrockCharacterComponent.h
└── GrimrockCombatComponent.h
```

---

## DataAssets recommandés

### Race

Classe suggérée :

```cpp
URPGRaceAsset
```

Contenu :

```text
Nom
Description
Bonus de caractéristiques
Compétences bonus
Dons raciaux
Résistances
Pouvoirs raciaux
```

### Classe

Classe suggérée :

```cpp
URPGClassAsset
```

Contenu :

```text
Nom
PV de base
Gain PV par niveau
Compétences de classe
Dons disponibles
Capacités par niveau
Sorts disponibles
Équipement autorisé
```

### Compétence

Classe suggérée :

```cpp
URPGSkillAsset
```

Contenu :

```text
Nom
Caractéristique associée
Catégorie
Utilisable en combat
Utilisable en exploration
```

### Don

Classe suggérée :

```cpp
URPGFeatAsset
```

Contenu :

```text
Nom
Description
Prérequis
Effets passifs
Effets actifs éventuels
```

### Sort

Classe suggérée :

```cpp
URPGSpellAsset
```

Contenu :

```text
Nom
École
Coût en mana
Cooldown
Portée
Zone d'effet
Type de dégâts
Effets appliqués
Surface créée
Conditions
```

### Effet d'état

Classe suggérée :

```cpp
URPGStatusEffectAsset
```

Contenu :

```text
Nom
Durée
Type
Icône
Effets numériques
Interactions
Suppression par autre effet
```

---

## Ordre d'implémentation conseillé

### Phase 1 : données de personnage

Objectif : avoir des personnages définis proprement.

À créer :

```text
FRPGAttributes
FRPGDerivedStats
URPGRaceAsset
URPGClassAsset
URPGCharacterDataAsset
```

Résultat attendu :

```text
Créer un personnage dans un DataAsset.
Lui attribuer race, classe et caractéristiques.
Calculer ses PV, armure et mana.
```

### Phase 2 : groupe de six personnages

Objectif : intégrer les personnages au `GrimrockPartyPawn`.

À créer :

```text
UGrimrockPartyComponent
FRPGPartyMember
EPartySlot
Positions avant/arrière
Sélection de personnage
```

Résultat attendu :

```text
Le groupe possède 1 à 6 personnages.
Chaque personnage a ses stats.
L'UI peut afficher jusqu'à 6 portraits.
```

### Phase 3 : compétences et jets

Objectif : utiliser les compétences dans l'exploration.

À créer :

```text
Jet de Perception
Jet de Crochetage
Jet de Désamorçage
Jet d'Arcane
```

Résultat attendu :

```text
Un passage secret peut demander Perception 16.
Un coffre peut demander Crochetage 14.
Une rune peut demander Arcane 15.
```

### Phase 4 : combat simple

Objectif : attaquer un monstre sur la case devant le groupe.

À créer :

```text
attaque physique
défense
dégâts
armure physique
PV
mort
```

Résultat attendu :

```text
Le guerrier peut frapper.
Le voleur peut attaquer.
Le mage peut lancer Projectile magique.
Un ennemi peut mourir.
```

### Phase 5 : magie et surfaces

Objectif : introduire les synergies élémentaires.

À créer :

```text
sort de feu
sort d'eau ou glace
sort de foudre
surface feu
surface eau
surface poison
interaction feu + poison
interaction eau + foudre
```

Résultat attendu :

```text
Une case peut brûler.
Un ennemi peut être mouillé puis électrocuté.
Un nuage de poison peut exploser.
```

### Phase 6 : dons et progression

Objectif : faire monter les personnages en niveau.

À créer :

```text
XP
niveau
gain PV
gain compétences
choix de don
choix de sort
```

Résultat attendu :

```text
Le joueur peut faire progresser son groupe.
Les choix modifient réellement le gameplay.
```

---

## Règle officielle v0.1

```text
Le joueur contrôle un groupe de 1 à 6 personnages.

Le groupe occupe une seule case du donjon, mais possède une formation interne composée de deux rangs de trois emplacements :
Avant gauche, Avant centre, Avant droite,
Arrière gauche, Arrière centre, Arrière droite.

Les personnages du rang avant sont spécialisés dans le corps à corps.
Les personnages du rang arrière utilisent prioritairement armes longues, armes à distance, magie, alchimie et soutien.

Chaque personnage possède :
race, classe, caractéristiques, compétences, dons, capacités, sorts éventuels, inventaire, équipement et états.

Les caractéristiques sont :
Force, Dextérité, Constitution, Intelligence, Sagesse, Charisme.

Les valeurs dérivées principales sont :
PV, mana, armure physique, armure magique, initiative, précision, esquive et résistances.

Les actions incertaines utilisent :
D20 + modificateur de caractéristique + compétence + bonus divers contre une difficulté.

Le combat utilise :
attaque contre défense, puis dégâts contre armure physique ou magique.

Les états dangereux sont généralement bloqués tant que l'armure correspondante existe encore.

Les cases du donjon peuvent contenir des surfaces :
feu, eau, poison, huile, glace, électricité, sang, bénédiction, malédiction.

Les sorts et objets peuvent créer, transformer ou supprimer ces surfaces.

Les personnages progressent par niveaux :
PV à chaque niveau, compétences régulièrement, dons tous les deux niveaux, caractéristique tous les quatre niveaux.

Toutes les races, classes, compétences, dons, sorts et effets doivent être définis par DataAssets lorsque cela est possible.
```
