# RPG — Progression des six classes, niveaux 1 à 20 — v0.1

Date : **5 octobre 2026**  
Projet : **GrimrockPrototype — UE 5.5.4**  
Statut : **spécification de design autoritaire — Talents matérialisés/validés ; économie Skill Points RPG-SKILL01 implémentée et validée**  

## 1. But

Cette spécification complète la progression des six classes déjà définies : **Guerrier, Voleur, Rôdeur, Mage, Prêtre, Alchimiste**.

Elle combine :

- le **d20 / D&D 3.5** pour caractéristiques, modificateurs, tests de compétence et prérequis ;
- **Divinity: Original Sin 2** pour actions tactiques, spécialisations, armures physique/magique, états et surfaces ;
- les contraintes propres à GrimrockPrototype : formation 2×3, grille, puzzles, catalogue d’actions unique et architecture data-driven.

Ce document ne reproduit aucun système à l’identique. Il définit les règles du projet.

## 2. Ce qui existe déjà dans le code

Le projet possède déjà :

- niveau 1→20 ;
- courbe XP ;
- `URPGClassAsset::ProgressionLevelGrants` ;
- `FRPGClassProgressionChoiceDefinition` ;
- transaction et persistance des choix de Talent ;
- `URPGSkillAsset`, Rank 0→5, Skill Checks d20 ;
- persistance des `SkillRanks` ;
- projection Skills/Talents → `RequirementIds` ;
- `FGridCombatActionDefinition::Requirements`.

**Talent = ProgressionChoice existant.** Aucune seconde monnaie de Talent ne doit apparaître.

**Économie Skill Points :** implémentée par `FRPGSkillPointService`. Le total accordé est dérivé du niveau, les points dépensés de la somme des `SkillRanks`, et l'UI joueur passe par les transactions C++ d'achat/remboursement sûr. Aucun compteur `SkillPoints` persistant distinct n'existe.

## 3. Progression universelle

### Skill Points

- Niveau 1 : **4 Skill Points**.
- Niveaux 2→20 : **+1 Skill Point/niveau**.
- Total niveau 20 : **23**.
- 1 point = +1 Rank.
- Rank absolu : 0→5.
- Points non dépensés conservables.
- Plafond par niveau : Rank 2 (niv.1–4), 3 (5–9), 4 (10–14), 5 (15–20).

### Talent Points

- +1 aux niveaux **2,4,6,8,10,12,14,16,18,20**.
- Total : **10**.
- Coût normal : 1.
- Points conservables.
- Les prérequis utilisent `PrerequisiteChoiceIds`.
- Les déblocages utilisent `GrantedRequirementIds`.

### Caractéristiques

+1 point aux niveaux **4,8,12,16,20** : total **5**. La valeur de base normale reste plafonnée à 20.

### Courbe XP

La formule existante est conservée :

`XP(N) = 1000 × (N-1) × N / 2`

| Niv. | XP cumulé | Skill Pts gagnés | Talent Pt | Carac. | Rank max |
|---:|---:|---:|---:|---:|---:|
| 1 | 0 | 4 | — | — | 2 |
| 2 | 1000 | 1 | 1 | — | 2 |
| 3 | 3000 | 1 | — | — | 2 |
| 4 | 6000 | 1 | 1 | 1 | 2 |
| 5 | 10000 | 1 | — | — | 3 |
| 6 | 15000 | 1 | 1 | — | 3 |
| 7 | 21000 | 1 | — | — | 3 |
| 8 | 28000 | 1 | 1 | 1 | 3 |
| 9 | 36000 | 1 | — | — | 3 |
| 10 | 45000 | 1 | 1 | — | 4 |
| 11 | 55000 | 1 | — | — | 4 |
| 12 | 66000 | 1 | 1 | 1 | 4 |
| 13 | 78000 | 1 | — | — | 4 |
| 14 | 91000 | 1 | 1 | — | 4 |
| 15 | 105000 | 1 | — | — | 5 |
| 16 | 120000 | 1 | 1 | 1 | 5 |
| 17 | 136000 | 1 | — | — | 5 |
| 18 | 153000 | 1 | 1 | — | 5 |
| 19 | 171000 | 1 | — | — | 5 |
| 20 | 190000 | 1 | 1 | 1 | 5 |


## 4. Skills

### Jet

`d20 + SkillRank + modificateur de caractéristique + bonus circonstanciels >= DD`

Pour un Skill Check, un 1 ou un 20 naturel n’est pas automatiquement échec/réussite sauf règle spéciale explicite.

### DD

| Difficulté | DD |
|---|---:|
| Triviale | 5 |
| Facile | 10 |
| Standard | 15 |
| Difficile | 20 |
| Très difficile | 25 |
| Héroïque | 30 |

### Catalogue canonique proposé

| SkillId | Nom | Attribut | Untrained |
|---|---|---|---|
| `Skill_HeavyWeapons` | Armes lourdes | FOR | oui |
| `Skill_LightWeapons` | Armes légères | DEX | oui |
| `Skill_RangedWeapons` | Armes à distance | DEX | oui |
| `Skill_Throwing` | Lancer | DEX | oui |
| `Skill_Shield` | Bouclier | CON | oui |
| `Skill_Armor` | Armures | CON | oui |
| `Skill_Athletics` | Athlétisme | FOR | oui |
| `Skill_Acrobatics` | Acrobatie | DEX | oui |
| `Skill_Perception` | Perception | SAG | oui |
| `Skill_Survival` | Survie | SAG | oui |
| `Skill_Stealth` | Discrétion | DEX | oui |
| `Skill_Lockpicking` | Crochetage | DEX | non |
| `Skill_Traps` | Pièges / désamorçage | INT | non |
| `Skill_Mechanics` | Mécanique | INT | oui |
| `Skill_Crafting` | Artisanat | INT | oui |
| `Skill_Alchemy` | Alchimie | INT | non |
| `Skill_Arcana` | Arcane | INT | non |
| `Skill_Runes` | Runes | INT | non |
| `Skill_Religion` | Religion | SAG | non |
| `Skill_Nature` | Nature | SAG | oui |
| `Skill_Medicine` | Médecine | SAG | oui |
| `Skill_History` | Histoire | INT | oui |
| `Skill_Persuasion` | Persuasion | CHA | oui |
| `Skill_Intimidation` | Intimidation | CHA | oui |
| `Skill_Deception` | Tromperie | CHA | oui |

Les tests collectifs (Perception, identification, connaissance) peuvent utiliser le meilleur membre actif éligible. Une action personnelle (crochetage, désamorçage, attaque, lancer de bombe) utilise uniquement son auteur.

Un Rank positif continue à produire le `SkillId` comme RequirementId. Les seuils supplémentaires passent par `FRPGSkillRequirementGrant`.

## 5. Structure commune des arbres

Trois branches × cinq paliers par classe :

- I : niveau 2 ;
- II : niveau 6 + I ;
- III : niveau 10 + II ;
- IV : niveau 14 + III ;
- V : niveau 18 + IV.

Les points reçus aux niveaux 4,8,12,16,20 servent à diversifier la construction ou peuvent être conservés.

Une action active débloquée reste une `FGridCombatActionDefinition` : coût en PA, mana, portée, zone, cooldown et payload **ne sont pas dupliqués dans le Talent**.

Un Talent passif produit un RequirementId consommé par le système concerné. Les RequirementIds sont dérivés et ne sont jamais persistés comme autorité.

## Guerrier — `Warrior`

**Rôle :** tank, mêlée, contrôle physique.  
**Skills initiaux conseillés :** Armes lourdes 2 ; Bouclier 1 ; Athlétisme 1.  
**Skills prioritaires :** Armes lourdes, Bouclier, Athlétisme, Armures, Perception.  
**Niveau 1 :** Entraînement martial : accès aux actions martiales de base et à l’équipement lourd.  
**Niveau 20 :** Indomptable : 1 fois/combat, un coup qui devrait mettre le Guerrier hors combat le laisse à 1 PV et restaure une fraction de son armure physique.

### Arbre

| Branche | I — niv.2 | II — niv.6 | III — niv.10 | IV — niv.14 | V — niv.18 |
|---|---|---|---|---|---|
| **Gardien** | Posture défensive | Coup de bouclier | Interception | Rempart | Forteresse |
| **Brise-ligne** | Coup puissant | Brise-armure | Balayage | Exécution | Ravage |
| **Maître d’armes** | Spécialisation martiale | Riposte | Second souffle | Maîtrise critique | Seigneur de guerre |

Chaque nœud coûte **1 Talent Point**. Les nœuds II→V exigent le nœud précédent de la même branche. Les variantes (par exemple l’Affinité élémentaire Feu/Glace/Foudre/Terre) sont des `ChoiceId` distincts mais appartiennent au même palier logique.

### Niveaux 1 à 20

| Niv. | Gains | Possibilités propres à la classe |
|---:|---|---|
| 1 | 4 Skill Pts + feature de classe | Répartition conseillée : Armes lourdes 2 ; Bouclier 1 ; Athlétisme 1. |
| 2 | 1 Skill Pt + 1 Talent Pt | Nouveau palier : Posture défensive / Coup puissant / Spécialisation martiale. Prérequis de branche obligatoires à partir du palier II. |
| 3 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 4 | 1 Skill Pt + 1 Talent Pt + 1 Carac. | Acheter n’importe quel Talent déjà déverrouillé, commencer une autre branche ou conserver le point. |
| 5 | 1 Skill Pt | Le Rank max passe à 3 ; possibilité de pousser un Skill prioritaire vers la nouvelle maîtrise. |
| 6 | 1 Skill Pt + 1 Talent Pt | Nouveau palier : Coup de bouclier / Brise-armure / Riposte. Prérequis de branche obligatoires à partir du palier II. |
| 7 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 8 | 1 Skill Pt + 1 Talent Pt + 1 Carac. | Acheter n’importe quel Talent déjà déverrouillé, commencer une autre branche ou conserver le point. |
| 9 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 10 | 1 Skill Pt + 1 Talent Pt | Nouveau palier : Interception / Balayage / Second souffle. Prérequis de branche obligatoires à partir du palier II. |
| 11 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 12 | 1 Skill Pt + 1 Talent Pt + 1 Carac. | Acheter n’importe quel Talent déjà déverrouillé, commencer une autre branche ou conserver le point. |
| 13 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 14 | 1 Skill Pt + 1 Talent Pt | Nouveau palier : Rempart / Exécution / Maîtrise critique. Prérequis de branche obligatoires à partir du palier II. |
| 15 | 1 Skill Pt | Le Rank max passe à 5 ; possibilité de pousser un Skill prioritaire vers la nouvelle maîtrise. |
| 16 | 1 Skill Pt + 1 Talent Pt + 1 Carac. | Acheter n’importe quel Talent déjà déverrouillé, commencer une autre branche ou conserver le point. |
| 17 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 18 | 1 Skill Pt + 1 Talent Pt | Nouveau palier : Forteresse / Ravage / Seigneur de guerre. Prérequis de branche obligatoires à partir du palier II. |
| 19 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 20 | 1 Skill Pt + 1 Talent Pt + 1 Carac. | Acheter n’importe quel Talent déjà déverrouillé, commencer une autre branche ou conserver le point. Capstone automatique : Indomptable : 1 fois/combat, un coup qui devrait mettre le Guerrier hors combat le laisse à 1 PV et restaure une fraction de son armure physique. |

## Voleur — `Rogue`

**Rôle :** dégâts ciblés, mobilité, pièges, serrures.  
**Skills initiaux conseillés :** Armes légères 1 ; Discrétion 1 ; Crochetage 1 ; Pièges 1.  
**Skills prioritaires :** Armes légères, Discrétion, Crochetage, Pièges, Perception, Acrobatie.  
**Niveau 1 :** Opportuniste : accès aux attaques précises et aux interactions de crochetage/désamorçage si les Skills requis sont entraînés.  
**Niveau 20 :** Maître opportuniste : la première attaque du tour réalisée en situation d’avantage contre une cible sans armure physique reçoit un effet critique renforcé.

### Arbre

| Branche | I — niv.2 | II — niv.6 | III — niv.10 | IV — niv.14 | V — niv.18 |
|---|---|---|---|---|---|
| **Assassin** | Attaque sournoise | Frappe dans le dos | Hémorragie | Point faible | Mise à mort |
| **Ombre** | Esquive | Disparition courte | Pas de l’ombre | Insaisissable | Ombre parfaite |
| **Saboteur** | Désamorçage expert | Piège rapide | Bombe fumigène | Maître des serrures | Sabotage |

Chaque nœud coûte **1 Talent Point**. Les nœuds II→V exigent le nœud précédent de la même branche. Les variantes (par exemple l’Affinité élémentaire Feu/Glace/Foudre/Terre) sont des `ChoiceId` distincts mais appartiennent au même palier logique.

### Niveaux 1 à 20

| Niv. | Gains | Possibilités propres à la classe |
|---:|---|---|
| 1 | 4 Skill Pts + feature de classe | Répartition conseillée : Armes légères 1 ; Discrétion 1 ; Crochetage 1 ; Pièges 1. |
| 2 | 1 Skill Pt + 1 Talent Pt | Nouveau palier : Attaque sournoise / Esquive / Désamorçage expert. Prérequis de branche obligatoires à partir du palier II. |
| 3 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 4 | 1 Skill Pt + 1 Talent Pt + 1 Carac. | Acheter n’importe quel Talent déjà déverrouillé, commencer une autre branche ou conserver le point. |
| 5 | 1 Skill Pt | Le Rank max passe à 3 ; possibilité de pousser un Skill prioritaire vers la nouvelle maîtrise. |
| 6 | 1 Skill Pt + 1 Talent Pt | Nouveau palier : Frappe dans le dos / Disparition courte / Piège rapide. Prérequis de branche obligatoires à partir du palier II. |
| 7 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 8 | 1 Skill Pt + 1 Talent Pt + 1 Carac. | Acheter n’importe quel Talent déjà déverrouillé, commencer une autre branche ou conserver le point. |
| 9 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 10 | 1 Skill Pt + 1 Talent Pt | Nouveau palier : Hémorragie / Pas de l’ombre / Bombe fumigène. Prérequis de branche obligatoires à partir du palier II. |
| 11 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 12 | 1 Skill Pt + 1 Talent Pt + 1 Carac. | Acheter n’importe quel Talent déjà déverrouillé, commencer une autre branche ou conserver le point. |
| 13 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 14 | 1 Skill Pt + 1 Talent Pt | Nouveau palier : Point faible / Insaisissable / Maître des serrures. Prérequis de branche obligatoires à partir du palier II. |
| 15 | 1 Skill Pt | Le Rank max passe à 5 ; possibilité de pousser un Skill prioritaire vers la nouvelle maîtrise. |
| 16 | 1 Skill Pt + 1 Talent Pt + 1 Carac. | Acheter n’importe quel Talent déjà déverrouillé, commencer une autre branche ou conserver le point. |
| 17 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 18 | 1 Skill Pt + 1 Talent Pt | Nouveau palier : Mise à mort / Ombre parfaite / Sabotage. Prérequis de branche obligatoires à partir du palier II. |
| 19 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 20 | 1 Skill Pt + 1 Talent Pt + 1 Carac. | Acheter n’importe quel Talent déjà déverrouillé, commencer une autre branche ou conserver le point. Capstone automatique : Maître opportuniste : la première attaque du tour réalisée en situation d’avantage contre une cible sans armure physique reçoit un effet critique renforcé. |

## Rôdeur — `Ranger`

**Rôle :** distance, survie, exploration, chasse.  
**Skills initiaux conseillés :** Armes à distance 2 ; Perception 1 ; Survie 1.  
**Skills prioritaires :** Armes à distance, Perception, Survie, Nature, Discrétion, Pièges.  
**Niveau 1 :** Tireur entraîné : accès aux attaques à distance depuis le rang arrière et aux outils d’exploration.  
**Niveau 20 :** Prédateur suprême : la Marque de la proie peut être transférée plus efficacement après une élimination et renforce le focus sans devenir un bonus global permanent.

### Arbre

| Branche | I — niv.2 | II — niv.6 | III — niv.10 | IV — niv.14 | V — niv.18 |
|---|---|---|---|---|---|
| **Tireur** | Tir précis | Tir perforant | Tir rapide | Volée | Œil d’aigle |
| **Chasseur** | Marque de la proie | Ennemi juré | Tir immobilisant | Frappe du prédateur | Chasseur alpha |
| **Éclaireur** | Vigilance | Piège de chasse | Repli tactique | Maître du terrain | Guide du groupe |

Chaque nœud coûte **1 Talent Point**. Les nœuds II→V exigent le nœud précédent de la même branche. Les variantes (par exemple l’Affinité élémentaire Feu/Glace/Foudre/Terre) sont des `ChoiceId` distincts mais appartiennent au même palier logique.

### Niveaux 1 à 20

| Niv. | Gains | Possibilités propres à la classe |
|---:|---|---|
| 1 | 4 Skill Pts + feature de classe | Répartition conseillée : Armes à distance 2 ; Perception 1 ; Survie 1. |
| 2 | 1 Skill Pt + 1 Talent Pt | Nouveau palier : Tir précis / Marque de la proie / Vigilance. Prérequis de branche obligatoires à partir du palier II. |
| 3 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 4 | 1 Skill Pt + 1 Talent Pt + 1 Carac. | Acheter n’importe quel Talent déjà déverrouillé, commencer une autre branche ou conserver le point. |
| 5 | 1 Skill Pt | Le Rank max passe à 3 ; possibilité de pousser un Skill prioritaire vers la nouvelle maîtrise. |
| 6 | 1 Skill Pt + 1 Talent Pt | Nouveau palier : Tir perforant / Ennemi juré / Piège de chasse. Prérequis de branche obligatoires à partir du palier II. |
| 7 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 8 | 1 Skill Pt + 1 Talent Pt + 1 Carac. | Acheter n’importe quel Talent déjà déverrouillé, commencer une autre branche ou conserver le point. |
| 9 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 10 | 1 Skill Pt + 1 Talent Pt | Nouveau palier : Tir rapide / Tir immobilisant / Repli tactique. Prérequis de branche obligatoires à partir du palier II. |
| 11 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 12 | 1 Skill Pt + 1 Talent Pt + 1 Carac. | Acheter n’importe quel Talent déjà déverrouillé, commencer une autre branche ou conserver le point. |
| 13 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 14 | 1 Skill Pt + 1 Talent Pt | Nouveau palier : Volée / Frappe du prédateur / Maître du terrain. Prérequis de branche obligatoires à partir du palier II. |
| 15 | 1 Skill Pt | Le Rank max passe à 5 ; possibilité de pousser un Skill prioritaire vers la nouvelle maîtrise. |
| 16 | 1 Skill Pt + 1 Talent Pt + 1 Carac. | Acheter n’importe quel Talent déjà déverrouillé, commencer une autre branche ou conserver le point. |
| 17 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 18 | 1 Skill Pt + 1 Talent Pt | Nouveau palier : Œil d’aigle / Chasseur alpha / Guide du groupe. Prérequis de branche obligatoires à partir du palier II. |
| 19 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 20 | 1 Skill Pt + 1 Talent Pt + 1 Carac. | Acheter n’importe quel Talent déjà déverrouillé, commencer une autre branche ou conserver le point. Capstone automatique : Prédateur suprême : la Marque de la proie peut être transférée plus efficacement après une élimination et renforce le focus sans devenir un bonus global permanent. |

## Mage — `Mage`

**Rôle :** magie élémentaire, surfaces, contrôle, utilité.  
**Skills initiaux conseillés :** Arcane 2 ; Runes 1 ; Perception 1.  
**Skills prioritaires :** Arcane, Runes, Perception, Nature, Histoire.  
**Niveau 1 :** Magie arcane : Projectile magique et accès au mana ; la spécialisation élémentaire commence par Talent.  
**Niveau 20 :** Archimage : la première interaction élémentaire réussie de chaque tour bénéficie d’une amplification contrôlée, sans ignorer PA, mana ni cooldown.

### Arbre

| Branche | I — niv.2 | II — niv.6 | III — niv.10 | IV — niv.14 | V — niv.18 |
|---|---|---|---|---|---|
| **Évocateur** | Affinité élémentaire | Surcharge élémentaire | Explosion contrôlée | Chaîne élémentaire | Cataclysme |
| **Arcaniste** | Bouclier arcanique | Dissipation | Manipulation runique | Téléportation courte | Maîtrise de l’Arcane |
| **Tisseur de surfaces** | Imprégnation | Conversion élémentaire | Conduction | Surface persistante | Architecte du terrain |

Chaque nœud coûte **1 Talent Point**. Les nœuds II→V exigent le nœud précédent de la même branche. Les variantes (par exemple l’Affinité élémentaire Feu/Glace/Foudre/Terre) sont des `ChoiceId` distincts mais appartiennent au même palier logique.

### Niveaux 1 à 20

| Niv. | Gains | Possibilités propres à la classe |
|---:|---|---|
| 1 | 4 Skill Pts + feature de classe | Répartition conseillée : Arcane 2 ; Runes 1 ; Perception 1. |
| 2 | 1 Skill Pt + 1 Talent Pt | Nouveau palier : Affinité élémentaire / Bouclier arcanique / Imprégnation. Prérequis de branche obligatoires à partir du palier II. |
| 3 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 4 | 1 Skill Pt + 1 Talent Pt + 1 Carac. | Acheter n’importe quel Talent déjà déverrouillé, commencer une autre branche ou conserver le point. |
| 5 | 1 Skill Pt | Le Rank max passe à 3 ; possibilité de pousser un Skill prioritaire vers la nouvelle maîtrise. |
| 6 | 1 Skill Pt + 1 Talent Pt | Nouveau palier : Surcharge élémentaire / Dissipation / Conversion élémentaire. Prérequis de branche obligatoires à partir du palier II. |
| 7 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 8 | 1 Skill Pt + 1 Talent Pt + 1 Carac. | Acheter n’importe quel Talent déjà déverrouillé, commencer une autre branche ou conserver le point. |
| 9 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 10 | 1 Skill Pt + 1 Talent Pt | Nouveau palier : Explosion contrôlée / Manipulation runique / Conduction. Prérequis de branche obligatoires à partir du palier II. |
| 11 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 12 | 1 Skill Pt + 1 Talent Pt + 1 Carac. | Acheter n’importe quel Talent déjà déverrouillé, commencer une autre branche ou conserver le point. |
| 13 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 14 | 1 Skill Pt + 1 Talent Pt | Nouveau palier : Chaîne élémentaire / Téléportation courte / Surface persistante. Prérequis de branche obligatoires à partir du palier II. |
| 15 | 1 Skill Pt | Le Rank max passe à 5 ; possibilité de pousser un Skill prioritaire vers la nouvelle maîtrise. |
| 16 | 1 Skill Pt + 1 Talent Pt + 1 Carac. | Acheter n’importe quel Talent déjà déverrouillé, commencer une autre branche ou conserver le point. |
| 17 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 18 | 1 Skill Pt + 1 Talent Pt | Nouveau palier : Cataclysme / Maîtrise de l’Arcane / Architecte du terrain. Prérequis de branche obligatoires à partir du palier II. |
| 19 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 20 | 1 Skill Pt + 1 Talent Pt + 1 Carac. | Acheter n’importe quel Talent déjà déverrouillé, commencer une autre branche ou conserver le point. Capstone automatique : Archimage : la première interaction élémentaire réussie de chaque tour bénéficie d’une amplification contrôlée, sans ignorer PA, mana ni cooldown. |

## Prêtre — `Priest`

**Rôle :** soin, protection, purification, anti-morts-vivants.  
**Skills initiaux conseillés :** Religion 2 ; Médecine 1 ; Armures 1.  
**Skills prioritaires :** Religion, Médecine, Armures, Perception, Histoire.  
**Niveau 1 :** Foi consacrée : accès à Soin mineur et Lumière ; les contrôles restent soumis aux protections correspondantes.  
**Niveau 20 :** Avatar sacré : 1 fois/combat, une protection de crise peut sauver un allié actif sans contourner les règles normales de ciblage et de ressources.

### Arbre

| Branche | I — niv.2 | II — niv.6 | III — niv.10 | IV — niv.14 | V — niv.18 |
|---|---|---|---|---|---|
| **Restauration** | Soin renforcé | Régénération | Soin de groupe | Purification | Miracle |
| **Protection** | Bénédiction | Égide | Protection sacrée | Sanctuaire | Bastion divin |
| **Exorcisme** | Lumière sacrée | Repousser les morts-vivants | Dissipation sacrée | Châtiment | Exorcisme majeur |

Chaque nœud coûte **1 Talent Point**. Les nœuds II→V exigent le nœud précédent de la même branche. Les variantes (par exemple l’Affinité élémentaire Feu/Glace/Foudre/Terre) sont des `ChoiceId` distincts mais appartiennent au même palier logique.

### Niveaux 1 à 20

| Niv. | Gains | Possibilités propres à la classe |
|---:|---|---|
| 1 | 4 Skill Pts + feature de classe | Répartition conseillée : Religion 2 ; Médecine 1 ; Armures 1. |
| 2 | 1 Skill Pt + 1 Talent Pt | Nouveau palier : Soin renforcé / Bénédiction / Lumière sacrée. Prérequis de branche obligatoires à partir du palier II. |
| 3 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 4 | 1 Skill Pt + 1 Talent Pt + 1 Carac. | Acheter n’importe quel Talent déjà déverrouillé, commencer une autre branche ou conserver le point. |
| 5 | 1 Skill Pt | Le Rank max passe à 3 ; possibilité de pousser un Skill prioritaire vers la nouvelle maîtrise. |
| 6 | 1 Skill Pt + 1 Talent Pt | Nouveau palier : Régénération / Égide / Repousser les morts-vivants. Prérequis de branche obligatoires à partir du palier II. |
| 7 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 8 | 1 Skill Pt + 1 Talent Pt + 1 Carac. | Acheter n’importe quel Talent déjà déverrouillé, commencer une autre branche ou conserver le point. |
| 9 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 10 | 1 Skill Pt + 1 Talent Pt | Nouveau palier : Soin de groupe / Protection sacrée / Dissipation sacrée. Prérequis de branche obligatoires à partir du palier II. |
| 11 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 12 | 1 Skill Pt + 1 Talent Pt + 1 Carac. | Acheter n’importe quel Talent déjà déverrouillé, commencer une autre branche ou conserver le point. |
| 13 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 14 | 1 Skill Pt + 1 Talent Pt | Nouveau palier : Purification / Sanctuaire / Châtiment. Prérequis de branche obligatoires à partir du palier II. |
| 15 | 1 Skill Pt | Le Rank max passe à 5 ; possibilité de pousser un Skill prioritaire vers la nouvelle maîtrise. |
| 16 | 1 Skill Pt + 1 Talent Pt + 1 Carac. | Acheter n’importe quel Talent déjà déverrouillé, commencer une autre branche ou conserver le point. |
| 17 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 18 | 1 Skill Pt + 1 Talent Pt | Nouveau palier : Miracle / Bastion divin / Exorcisme majeur. Prérequis de branche obligatoires à partir du palier II. |
| 19 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 20 | 1 Skill Pt + 1 Talent Pt + 1 Carac. | Acheter n’importe quel Talent déjà déverrouillé, commencer une autre branche ou conserver le point. Capstone automatique : Avatar sacré : 1 fois/combat, une protection de crise peut sauver un allié actif sans contourner les règles normales de ciblage et de ressources. |

## Alchimiste — `Alchemist`

**Rôle :** potions, bombes, surfaces, altérations.  
**Skills initiaux conseillés :** Alchimie 2 ; Lancer 1 ; Mécanique 1.  
**Skills prioritaires :** Alchimie, Lancer, Mécanique, Artisanat, Nature, Perception.  
**Niveau 1 :** Préparation alchimique : recettes et consommables de base ; le lancer utilitaire reste distinct d’une attaque de combat.  
**Niveau 20 :** Grand œuvre : 1 fois/combat, amplifier une préparation ou une réaction de surface majeure sans créer gratuitement d’objet persistant.

### Arbre

| Branche | I — niv.2 | II — niv.6 | III — niv.10 | IV — niv.14 | V — niv.18 |
|---|---|---|---|---|---|
| **Grenadier** | Bombe incendiaire | Bombe toxique | Charge précise | Réaction en chaîne | Maître grenadier |
| **Apothicaire** | Potion renforcée | Antidote | Élixir défensif | Diffusion | Panacée |
| **Transmutateur** | Huile glissante | Flasque acide | Nuage corrosif | Catalyseur | Transmutation majeure |

Chaque nœud coûte **1 Talent Point**. Les nœuds II→V exigent le nœud précédent de la même branche. Les variantes (par exemple l’Affinité élémentaire Feu/Glace/Foudre/Terre) sont des `ChoiceId` distincts mais appartiennent au même palier logique.

### Niveaux 1 à 20

| Niv. | Gains | Possibilités propres à la classe |
|---:|---|---|
| 1 | 4 Skill Pts + feature de classe | Répartition conseillée : Alchimie 2 ; Lancer 1 ; Mécanique 1. |
| 2 | 1 Skill Pt + 1 Talent Pt | Nouveau palier : Bombe incendiaire / Potion renforcée / Huile glissante. Prérequis de branche obligatoires à partir du palier II. |
| 3 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 4 | 1 Skill Pt + 1 Talent Pt + 1 Carac. | Acheter n’importe quel Talent déjà déverrouillé, commencer une autre branche ou conserver le point. |
| 5 | 1 Skill Pt | Le Rank max passe à 3 ; possibilité de pousser un Skill prioritaire vers la nouvelle maîtrise. |
| 6 | 1 Skill Pt + 1 Talent Pt | Nouveau palier : Bombe toxique / Antidote / Flasque acide. Prérequis de branche obligatoires à partir du palier II. |
| 7 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 8 | 1 Skill Pt + 1 Talent Pt + 1 Carac. | Acheter n’importe quel Talent déjà déverrouillé, commencer une autre branche ou conserver le point. |
| 9 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 10 | 1 Skill Pt + 1 Talent Pt | Nouveau palier : Charge précise / Élixir défensif / Nuage corrosif. Prérequis de branche obligatoires à partir du palier II. |
| 11 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 12 | 1 Skill Pt + 1 Talent Pt + 1 Carac. | Acheter n’importe quel Talent déjà déverrouillé, commencer une autre branche ou conserver le point. |
| 13 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 14 | 1 Skill Pt + 1 Talent Pt | Nouveau palier : Réaction en chaîne / Diffusion / Catalyseur. Prérequis de branche obligatoires à partir du palier II. |
| 15 | 1 Skill Pt | Le Rank max passe à 5 ; possibilité de pousser un Skill prioritaire vers la nouvelle maîtrise. |
| 16 | 1 Skill Pt + 1 Talent Pt + 1 Carac. | Acheter n’importe quel Talent déjà déverrouillé, commencer une autre branche ou conserver le point. |
| 17 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 18 | 1 Skill Pt + 1 Talent Pt | Nouveau palier : Maître grenadier / Panacée / Transmutation majeure. Prérequis de branche obligatoires à partir du palier II. |
| 19 | 1 Skill Pt | Monter un Skill sous le plafond courant ou conserver le point. |
| 20 | 1 Skill Pt + 1 Talent Pt + 1 Carac. | Acheter n’importe quel Talent déjà déverrouillé, commencer une autre branche ou conserver le point. Capstone automatique : Grand œuvre : 1 fois/combat, amplifier une préparation ou une réaction de surface majeure sans créer gratuitement d’objet persistant. |

## 12. Données de classe autoritaires

| Classe | FOR | DEX | CON | INT | SAG | CHA | PV1 | PV/niv. | Mana1 | Mana/niv. |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Guerrier | 15 | 11 | 13 | 9 | 9 | 9 | 18 | 8 | 0 | 0 |
| Voleur | 9 | 15 | 10 | 13 | 9 | 10 | 14 | 6 | 0 | 0 |
| Rôdeur | 11 | 15 | 12 | 9 | 11 | 8 | 16 | 7 | 0 | 0 |
| Mage | 8 | 12 | 10 | 15 | 12 | 9 | 8 | 4 | 18 | 8 |
| Prêtre | 10 | 9 | 13 | 9 | 15 | 10 | 12 | 6 | 16 | 7 |
| Alchimiste | 9 | 13 | 12 | 15 | 9 | 8 | 12 | 5 | 10 | 5 |

Les bonus de race sont appliqués ensuite.

## 13. Principes d’équilibrage

- Les builds doivent être **spécialisés**, pas omnipotents.
- Les PA, le mana, la portée, les cooldowns et la formation bornent la puissance.
- Le contrôle fort respecte les armures/protections correspondantes.
- Les surfaces doivent produire des synergies lisibles, jamais des exceptions cachées.
- Le focus d’une cible est récompensé, mais chaque classe doit aussi apporter une utilité d’exploration.
- Avec 10 Talent Points pour 15 nœuds principaux, aucun personnage ne peut compléter les trois branches.
- Avec 23 Skill Points et un Rank max 5, un niveau 20 peut maîtriser environ quatre Skills et rester compétent dans quelques autres.

## 14. Multiclassage

**Hors v0.1.** Un personnage conserve une classe principale unique du niveau 1 au niveau 20. Le multiclassage sera étudié seulement après équilibrage des six arbres purs.

## 15. Mapping Unreal cible

### Talents

Réutiliser exclusivement :

- `FRPGClassProgressionLevelGrant::ChoicePointsGranted`;
- `FRPGClassProgressionChoiceDefinition`;
- `SelectedChoiceIds`;
- `FRPGClassProgressionTransactionService`.

### Skills

Le Skill Rank et sa persistance existent. L’achat manque encore.

Implémentation minimale future :

1. exposer le nombre de Skill Points accordés par niveau dans la progression existante (ou une règle globale unique) ;
2. calculer le budget accordé depuis classe+niveau ;
3. calculer le budget dépensé depuis les Ranks achetés ;
4. appliquer plafond par niveau et `URPGSkillAsset::MaxRank` ;
5. commit atomique de l’achat ;
6. conserver uniquement les Ranks durables ; solde et RequirementIds restent reconstructibles.

Aucun tableau runtime « Skill Tree » parallèle n’est nécessaire.

## 16. Références

- D&D 3.5 / d20 : https://regles-donjons-dragons.com/
- Guide communautaire DOS2 fourni : https://steamcommunity.com/sharedfiles/filedetails/?id=1133139481
- Guide communautaire Reddit fourni : https://www.reddit.com/r/DivinityOriginalSin/comments/ko0p38/a_huge_dos2_guide_i_typed_up_for_new_players/

Ces références inspirent la structure. Les valeurs et règles autoritaires sont celles de GrimrockPrototype.

## 17. État d'implémentation

### Volet Talents — réalisé

- les six classes exposent la progression des Talent Points jusqu'au niveau 20 ;
- les 90 nœuds conceptuels sont authorés, plus leurs variantes explicites ;
- sauvegarde/chargement et projection des Talents réutilisent MON15/MON20 ;
- actions et passifs se déverrouillent via les requirements existants ;
- les six `DA_Class_*` portent les 10 grants canoniques aux niveaux pairs 2→20 ;
- la campagne `Grimrock.RPG.RPG03` est validée **193/193** ;
- un vrai PIE `L_Dungeon` valide un build de branche complet pour chacune des six classes.

### Volet Skills / écran Level Up — encore ouvert

- les `URPGSkillAsset` de production ne sont pas encore matérialisés dans `Content/` ;
- le budget et les plafonds d'achat de Skill restent à rendre transactionnels ;
- l'écran Level Up complet Skill/Talent/Carac. reste à finaliser.

Le volet Talents ne doit donc pas être réimplémenté pour terminer ces travaux : ils doivent se raccorder aux autorités de progression existantes.

## 18. RPG02 — mécanique détaillée des Talents

La progression de ce document fixe **quand** les Talents deviennent accessibles.

La spécification autoritaire suivante fixe désormais **ce qu'ils font réellement** :

[`RPG_Talents_Mechanics_v0_1.md`](RPG_Talents_Mechanics_v0_1.md)

RPG02 définit pour chacun des **90 Talents** :

- `ChoiceId` et prérequis ;
- action/passif/recette ;
- coût PA, mana et item ;
- ciblage, portée, zone et cooldown ;
- coefficients de dégâts/soins ;
- ArmorGate physique ou magique ;
- Status Effects ;
- interactions de formation, surfaces et inventaire ;
- dépendances techniques génériques nécessaires à l'implémentation.

En cas de divergence sur la mécanique précise d'un Talent, **RPG_Talents_Mechanics_v0_1.md est autoritaire** ; le présent document reste autoritaire pour la structure de progression 1→20.
