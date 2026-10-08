# UI-RPG-DESC01.5 — Talent UX Contract

Date : **8 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
État : **VALIDÉ PAR L'UTILISATEUR — 8 octobre 2026**  
Périmètre : **6 classes / 18 branches / 90 nœuds conceptuels**

## 1. Objet du jalon

UI-RPG-DESC01.5 repart du besoin joueur, et non de l'implémentation actuelle.

Les versions DESC01.1 à DESC01.4 ont permis de valider la projection des données,
les variantes, les actions et les mécaniques. Les captures PIE de DESC01.4 ont
cependant montré que la fiche restait difficile à interpréter parce qu'elle
mélangeait encore :

- le **Talent actuellement consulté** dans l'arbre ;
- le **statut d'acquisition** du Talent ;
- la **nature mécanique** du Talent ;
- les détails d'une action active ;
- le choix d'une variante.

DESC01.5 définit le contrat UX avant toute nouvelle implémentation.

**Contrat validé : DESC01.6 à DESC01.12 restent documentaires. Aucun nouveau C++, UMG ou DataAsset DESC01 n'est autorisé avant le contrat de read-model DESC01.13.**

## 2. Principes non négociables

1. **Une seule structure de fiche** pour les six classes.
2. Cliquer sur un nœud signifie uniquement **consulter** ce Talent.
3. Consulter n'acquiert rien, ne sélectionne aucune variante et ne dépense aucun point.
4. Le **TYPE** décrit comment le Talent fonctionne.
5. Le **STATUT** décrit si le personnage le possède ou peut l'acquérir.
6. Les informations de gameplay sont visibles **avant acquisition** afin de permettre un choix éclairé.
7. Une mécanique identique doit être présentée avec le même vocabulaire partout.
8. Un Talent à variantes affiche **toutes ses variantes simultanément**.
9. La consultation d'une variante ne nécessite aucun sélecteur.
10. La quantité d'information affichée ne dépend pas du statut acquis/verrouillé.
11. Les valeurs numériques restent dérivées des autorités gameplay.
12. Les identifiants techniques ne sont jamais présentés comme du texte joueur.

## 3. Trois axes indépendants

### 3.1 Focus de consultation

Le Talent sur lequel le joueur a cliqué est le **Talent consulté**.

Le focus ne signifie jamais :

- acquis ;
- disponible ;
- candidat à l'acquisition ;
- variante choisie.

Le futur traitement graphique doit donc distinguer explicitement le
**focus de consultation** de l'**état d'acquisition**.

### 3.2 Nature mécanique

La nature du Talent répond uniquement à la question :

> Comment ce Talent agit-il dans le jeu ?

Elle est affichée dans **TYPE**.

### 3.3 Statut d'acquisition

Le statut répond uniquement à la question :

> Ce personnage possède-t-il ce Talent, et sinon peut-il l'acquérir ?

Il est affiché dans **STATUT**.

Le type et le statut ne doivent jamais être fusionnés dans une même formulation.

## 4. Structure canonique de la fiche

L'ordre suivant est fixe :

```text
NOM

TYPE
<nature mécanique>

STATUT
<acquis / disponible / verrouillé + raison>

PRINCIPE
<explication fonctionnelle en langage joueur>

EFFETS
<conséquences chiffrées et conditions>

[UTILISATION]
<si déclenchement volontaire>

[VARIANTES]
<si le nœud possède plusieurs Choice concrets>

ACQUISITION
<niveau / coût / prérequis / exclusivité>
<contrôles d'acquisition éventuels>
```

Les sections entre crochets sont facultatives.  
Les sections présentes gardent **toujours le même ordre et la même signification**.

Une section facultative absente est simplement masquée : les autres sections ne
remontent pas dans une hiérarchie différente et ne changent pas de vocabulaire.

## 5. TYPE — taxonomie joueur

Le TYPE ne doit contenir ni « acquis », ni « disponible », ni « débloqué ».

Taxonomie cible :

| TYPE | Définition |
|---|---|
| **ACTIF** | capacité déclenchée volontairement par le joueur |
| **SORT ACTIF** | capacité active explicitement magique |
| **PASSIF** | bonus permanent ou conditionnel sans activation volontaire |
| **RÉACTION AUTOMATIQUE** | effet déclenché automatiquement par un événement gameplay |
| **RECETTE + OBJET RAPIDE** | acquisition donnant accès à une recette et à son usage comme objet rapide |
| **RECETTE + ACTIF** | acquisition donnant accès à une recette et à une capacité active |

Le fait qu'un nœud possède plusieurs variantes n'est **pas un TYPE**.  
C'est une propriété orthogonale représentée par la section **VARIANTES**.

Exemples :

```text
Spécialisation martiale -> TYPE : PASSIF + section VARIANTES
Riposte                 -> TYPE : RÉACTION AUTOMATIQUE
Coup de bouclier        -> TYPE : ACTIF
Bouclier arcanique      -> TYPE : SORT ACTIF
Bombe incendiaire       -> TYPE : RECETTE + OBJET RAPIDE
```

## 6. STATUT — vocabulaire d'acquisition

Le statut est visible pour **tous** les Talents, y compris les passifs et réactions.

Formulations cibles :

| Situation | STATUT |
|---|---|
| Choice réellement possédé | **ACQUIS** |
| toutes les conditions sont satisfaites | **DISPONIBLE** |
| niveau insuffisant | **VERROUILLÉ — niveau X requis** |
| prérequis absent | **VERROUILLÉ — nécessite « Talent X »** |
| points insuffisants | **VERROUILLÉ — nécessite N point(s) de Talent** |
| autre variante exclusive déjà acquise | **INDISPONIBLE — autre variante déjà choisie** |

Si plusieurs conditions sont insatisfaites, **STATUT** expose la raison principale
selon la priorité gameplay existante ; **ACQUISITION** conserve la liste complète
des exigences pertinentes.

Les formulations suivantes sont interdites comme statut :

```text
ACTION DISPONIBLE
ACTION DÉBLOQUÉE
ACTION ACCORDÉE APRÈS ACQUISITION
CAPACITÉ DÉBLOQUÉE
```

Elles mélangent mécanique et acquisition.

## 7. PRINCIPE

PRINCIPE explique **quand, pourquoi et comment** le Talent fonctionne.

Règles :

- langage naturel ;
- phrase compréhensible sans connaître le code ;
- pas de duplication pure des chiffres de EFFETS ;
- pas d'identifiants internes ;
- préciser les exclusions importantes si elles changent réellement le comportement.

Exemple — Interception :

```text
PRINCIPE
Une fois par round, Interception se déclenche automatiquement lorsqu'un allié
du rang avant est touché par une attaque physique ciblée.
Les attaques de zone, dégâts périodiques et surfaces ne la déclenchent pas.
```

## 8. EFFETS

EFFETS expose les résultats mécaniques exacts et les conditions chiffrées.

Exemple — Interception :

```text
EFFETS
50 % des dégâts finaux sont redirigés vers le Guerrier.
Les dégâts redirigés sont ensuite résolus contre sa propre armure physique.
```

Exemple — Posture défensive :

```text
EFFETS
Dégâts physiques reçus : -20 %
Esquive : +2
Dégâts d'arme infligés : -10 %
Durée : 2 rounds
```

PRINCIPE et EFFETS ne doivent pas répéter la même phrase sous deux formes quasi
identiques.

## 9. UTILISATION

UTILISATION existe lorsqu'une action volontaire doit être déclenchée.

Elle décrit l'usage, **pas l'état d'acquisition**.

Contenu possible :

```text
Coût en points d'action
Coût en mana
Objet consommé
Cible
Portée
Zone
Ligne de vue
Nombre de frappes/résolutions
Recharge
Condition d'équipement
Coût de mobilité/PAM
```

Elle est affichée avant acquisition exactement comme après acquisition.

La seule différence entre un Talent actif acquis et non acquis se trouve dans
**STATUT**, pas dans UTILISATION.

Pour les PASSIFS et RÉACTIONS AUTOMATIQUES sans action volontaire, UTILISATION
est absente.

## 10. ACQUISITION

ACQUISITION répond uniquement à :

> Que faut-il pour obtenir ce Talent ?

Structure :

```text
Niveau requis : X
Coût : N point(s) de Talent
Prérequis : <Talent précédent>   [si applicable]
Exclusivité : <règle>            [si applicable]
```

Ces informations restent visibles même pour un Talent déjà acquis afin de garder
une fiche stable et de documenter sa place dans la progression.

Les boutons d'acquisition appartiennent à cette section.

## 11. VARIANTES

### 11.1 Consultation

Toutes les variantes sont visibles **simultanément dès l'ouverture du Talent**.

Aucun dropdown, ComboBox ou clic supplémentaire n'est nécessaire pour comprendre
leur fonctionnement.

Chaque variante expose au minimum :

```text
NOM DE VARIANTE
PRINCIPE / EFFETS propres
UTILISATION propre si nécessaire
```

Les autres variantes restent lisibles après acquisition d'une variante.

### 11.2 Acquisition

Le contrat final ne prévoit **aucune ComboBox utilisateur** pour choisir une variante.

Quand le Talent est disponible, chaque variante acquérable reçoit un contrôle
explicite **CHOISIR**.

Flux cible :

```text
Consultation
    -> toutes les variantes visibles

CHOISIR sur une variante
    -> cette variante devient CHOIX EN COURS
    -> toutes les autres restent visibles
    -> CONFIRMER / ANNULER

CONFIRMER
    -> variante acquise
    -> variante choisie marquée ACQUISE
    -> variantes exclusives marquées INDISPONIBLES
```

Le terme **sélectionné** est évité dans l'interface finale autant que possible,
car il peut être confondu avec le Talent simplement consulté.

## 12. Exemples normatifs — Guerrier

### 12.1 Posture défensive

```text
POSTURE DÉFENSIVE

TYPE
ACTIF

STATUT
ACQUIS

PRINCIPE
Le Guerrier adopte une posture défensive pendant 2 rounds.
Réutiliser la capacité rafraîchit sa durée.

EFFETS
Dégâts physiques reçus : -20 %
Esquive : +2
Dégâts d'arme infligés : -10 %

UTILISATION
Coût : 1 point d'action
Cible : soi-même
Recharge : 3 tours

ACQUISITION
Niveau requis : 2
Coût : 1 point de Talent
```

### 12.2 Coup de bouclier

```text
COUP DE BOUCLIER

TYPE
ACTIF

STATUT
VERROUILLÉ — niveau 6 requis

PRINCIPE
Effectue une attaque de bouclier contre la première cible dans l'axe.
Nécessite un bouclier équipé.

EFFETS
80 % des dégâts de l'arme, de type contondant.
Si l'armure physique est épuisée après les dégâts :
Étourdi pendant 1 tour.

UTILISATION
Coût : 2 points d'action
Cible : première cible dans l'axe
Portée : 1 case
Recharge : 2 tours

ACQUISITION
Niveau requis : 6
Coût : 1 point de Talent
Prérequis : Posture défensive
```

### 12.3 Interception

```text
INTERCEPTION

TYPE
RÉACTION AUTOMATIQUE

STATUT
VERROUILLÉ — niveau 10 requis

PRINCIPE
Une fois par round, Interception se déclenche automatiquement lorsqu'un allié
du rang avant est touché par une attaque physique ciblée.
Les attaques de zone, dégâts périodiques et surfaces ne la déclenchent pas.

EFFETS
50 % des dégâts finaux sont redirigés vers le Guerrier.
Les dégâts redirigés sont ensuite résolus contre sa propre armure physique.

ACQUISITION
Niveau requis : 10
Coût : 1 point de Talent
Prérequis : Coup de bouclier
```

### 12.4 Spécialisation martiale

```text
SPÉCIALISATION MARTIALE

TYPE
PASSIF

STATUT
VERROUILLÉ — nécessite 1 point de Talent

PRINCIPE
Le Guerrier choisit une spécialisation d'arme exclusive.

VARIANTES

Tranchant
Précision : +1
Dégâts d'arme : +10 %
Condition : arme tranchante
[CHOISIR si disponible]

PERFORANT
Précision : +1
Dégâts d'arme : +10 %
Condition : arme perforante
[CHOISIR si disponible]

CONTONDANT
Précision : +1
Dégâts d'arme : +10 %
Condition : arme contondante
[CHOISIR si disponible]

ACQUISITION
Niveau requis : 2
Coût : 1 point de Talent
Choix exclusif : une seule spécialisation
```

La casse « Tranchant » dans cet exemple n'a aucune valeur normative ; la charte
graphique déterminera la casse finale des titres.

## 13. Couverture fonctionnelle

La source mécanique canonique reste :

```text
docs/Rules/RPG_Talents_Mechanics_v0_1.md
```

Elle décrit actuellement :

| Classe | Branches | Nœuds conceptuels |
|---|---|---:|
| Guerrier | Gardien / Brise-ligne / Maître d'armes | 15 |
| Voleur | Assassin / Ombre / Saboteur | 15 |
| Rôdeur | Tireur / Chasseur / Éclaireur | 15 |
| Mage | Évocateur / Arcaniste / Tisseur de surfaces | 15 |
| Prêtre | Restauration / Protection / Exorcisme | 15 |
| Alchimiste | Grenadier / Apothicaire / Transmutateur | 15 |
| **TOTAL** | **18 branches** | **90** |

Les audits DESC01.6 à DESC01.11 doivent traduire ces 90 nœuds dans le présent
contrat sans modifier leurs mécaniques.

## 14. Nœuds à variantes connus

Le contrat doit couvrir au minimum :

- **Guerrier — Spécialisation martiale** ;
- **Rôdeur — Ennemi juré** ;
- **Mage — Affinité élémentaire** ;
- **Mage — Imprégnation**.

Les variantes peuvent être fixes ou dérivées d'un catalogue autoritaire, mais leur
présentation respecte toujours le même modèle.

Pour les listes longues ou dynamiques, la fiche complète reste visible via une
**zone de détail verticalement scrollable**. Il ne doit pas y avoir un scroll
indépendant par variante.

## 15. Langage joueur

Sont interdits dans le texte final lorsqu'une formulation joueur existe :

```text
Status_*
Skill_*
Action_*
Recipe_*
Item_*
Surface_*
Accuracy
PhysicalArmor
MagicalArmor
RawDamage
MaxHP
bSkipActivation
```

Exemples :

```text
Accuracy       -> Précision
PhysicalArmor  -> armure physique
MagicalArmor   -> armure magique
RawDamage      -> dégâts bruts
MaxHP          -> PV maximum
Status_Stunned -> Étourdi
```

L'audit des 90 Talents doit identifier les termes restant à normaliser plutôt que
les masquer par une liste de remplacements ad hoc dans l'UMG.

## 16. Contrat avec le thread UI-RPG-VISUAL01

DESC01.5 définit la **sémantique** et l'ordre des informations.

Le thread graphique parallèle peut définir :

- typographie ;
- tailles ;
- graisses ;
- espacements ;
- séparateurs ;
- couleurs ;
- accent de branche ;
- apparence du focus de consultation ;
- apparence des statuts ACQUIS / DISPONIBLE / VERROUILLÉ ;
- apparence des variantes et boutons CHOISIR ;
- comportement visuel du ScrollBox.

Contraintes transmises à VISUAL01 :

1. le focus de consultation et le statut ACQUIS doivent être visuellement distincts ;
2. aucune section ne change de style en fonction du type mécanique ;
3. la couleur ne doit jamais être l'unique vecteur d'un état ;
4. la hiérarchie sémantique de DESC01.5 ne peut pas être réordonnée ;
5. aucune icône définitive n'est requise avant UI-RPG-VISUAL01.1/.2.

## 17. Autorités et limites architecturales

DESC01.5 ne change aucune autorité.

```text
URPGClassAsset / ProgressionChoices
    -> mécanique et identité

FRPGClassProgressionService
    -> disponibilité / acquisition

CombatActions / modifiers / reactions / skills / party modifiers
    -> valeurs gameplay

FGridSkillsPageService
    -> future projection read-only

UGridTalentDetailWidget
    -> future présentation

WBP_RPGTalentDetail
    -> future composition visuelle
```

Le futur read-model doit être conçu **après** l'audit des 90 Talents
(DESC01.6 → DESC01.12), pas avant.

## 18. Roadmap associée

```text
RPG-SKILL01      CLOS
RPG-LEVELUX01    CLOS
UI-RPG-DESC01    EN COURS
UI-RPG-VISUAL01  PARALLÈLE — charte graphique uniquement à ce stade
```

Découpage DESC01 après validation de ce contrat :

```text
DESC01.6   Guerrier — 15 Talents
DESC01.7   Voleur — 15 Talents
DESC01.8   Rôdeur — 15 Talents
DESC01.9   Mage — 15 Talents
DESC01.10  Prêtre — 15 Talents
DESC01.11  Alchimiste — 15 Talents
DESC01.12  Normalisation croisée 90/90
DESC01.13  Contrat du read-model
DESC01.14  Projection C++
DESC01.15  Rebuild UMG
DESC01.16  QA six classes
DESC01.17  Documentation / clôture
```

## 19. Critères de validation de DESC01.5

DESC01.5 est validé uniquement lorsque l'utilisateur approuve explicitement :

- les trois axes **consultation / type / statut** ;
- l'ordre canonique des sections ;
- la taxonomie TYPE ;
- le vocabulaire STATUT ;
- la séparation PRINCIPE / EFFETS ;
- le rôle de UTILISATION ;
- le rôle de ACQUISITION ;
- l'affichage simultané de toutes les variantes ;
- l'abandon du dropdown comme outil final de sélection des variantes ;
- la séparation des responsabilités entre DESC01 et VISUAL01.

Validation utilisateur reçue le **8 octobre 2026**.

La phase suivante est l'audit documentaire des 90 Talents, classe par classe. Aucune nouvelle implémentation C++ / UMG / DataAsset DESC01 n'est autorisée avant DESC01.13.
