# UI-RPG-DESC01.10 — Audit documentaire des 15 Talents du Prêtre

Date : **8 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
État : **DRAFT — validation utilisateur requise**  
Dépendances : **UI-RPG-DESC01.5 à .9 validés**  
Périmètre : **Prêtre / 3 branches / 15 nœuds conceptuels / 15 Choice records**

## 1. Objet

Traduire les 15 Talents du Prêtre dans le contrat UX validé par
`UI_RPG_DESC01_5_TALENT_UX_CONTRACT.md`, sans modifier le gameplay.

Sources vérifiées :

```text
docs/Rules/RPG_Talents_Mechanics_v0_1.md
docs/Rules/RPG_Class_Progression_1_20_v0_1.md
Source/GrimrockPrototypeEditor/Private/RPG/RPGPriestAuthoring.cpp
Source/GrimrockPrototypeEditor/Private/Tests/RPGRPG0395PriestAuthoringTests.cpp
Source/GrimrockPrototypeEditor/Private/Tests/RPGRPG0395PriestExorcismTests.cpp
Source/GrimrockPrototypeEditor/Private/Tests/RPGRPG0395PriestProductionTests.cpp
Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.cpp
```

Aucun C++, UMG ou DataAsset n'est modifié par ce jalon.

## 2. Synthèse de la classe

| Branche | Palier I | II | III | IV | V |
|---|---|---|---|---|---|
| **Restauration** | Soin renforcé | Régénération | Soin de groupe | Purification | Miracle |
| **Protection** | Bénédiction | Égide | Protection sacrée | Sanctuaire | Bastion divin |
| **Exorcisme** | Lumière sacrée | Repousser les morts-vivants | Dissipation sacrée | Châtiment | Exorcisme majeur |

Répartition UX proposée :

```text
SORT ACTIF              14
PASSIF                   1
TOTAL                    15
```

Le seul Talent purement passif est **Soin renforcé**.

Aucun nœud Prêtre ne possède de variantes.

## 3. Règles communes de présentation

### 3.1 Ciblage « Ally »

Dans le resolver générique actuel, `TargetingPolicy=Ally` utilise une sélection
explicite de membre vivant du groupe et **n'exclut pas le lanceur**.

La fiche joueur normalise donc ce ciblage en :

```text
soi-même ou un allié
```

pour les sorts concernés, plutôt que d'exposer le terme technique « Ally ».

### 3.2 Skills de production

Les Skills utilisés par les Talents Prêtre sont désormais matérialisés dans le
catalogue UI-RPG06.2A :

```text
Médecine
Religion
```

Ils ne constituent donc plus une dépendance manquante.

### 3.3 Soins de la branche Restauration

Soin renforcé est le palier I obligatoire de toute la branche. Son bonus +25 %
s'applique aux soins `SourcePolicy=Spell`, y compris aux soins périodiques de
Régénération : le runtime reconstruit le contexte du **Prêtre source** au moment du
tick et applique ses modificateurs de soin sortant.

Les fiches décrivent d'abord la formule propre à chaque Talent ; l'interaction avec
Soin renforcé est rappelée lorsque cela évite une ambiguïté.

---

# 4. Branche RESTAURATION

## 4.1 Soin renforcé — palier I / niveau 2

**TYPE**  
PASSIF

**PRINCIPE**  
Le Prêtre améliore tous les soins qu'il produit par l'intermédiaire d'un sort.
Le bonus est appliqué après le calcul normal de la quantité de soin.

**EFFETS**

```text
Soins issus d'un sort du Prêtre :
    +25 %

Ordre :
    calcul de la magnitude normale
    puis multiplication par 1,25

Arrondi :
    inférieur

Si le soin de base est positif :
    résultat final minimum : 1 PV
```

Le bonus s'applique notamment aux soins périodiques de Régénération lorsque le
Prêtre ayant lancé le statut possède Soin renforcé.

**UTILISATION**  
Aucune : bonus passif.

**ACQUISITION**

```text
Niveau requis : 2
Coût : 1 point de Talent
Prérequis : aucun
```

**Audit source** : conforme, y compris pour le soin périodique dont le runtime
résout le modificateur à partir du Prêtre source.

---

## 4.2 Régénération — palier II / niveau 6

**TYPE**  
SORT ACTIF

**PRINCIPE**  
Le Prêtre applique un effet de soin périodique. À la fin de chacune des trois
prochaines activations de la cible, Régénération restaure des PV puis réduit sa
durée restante.

**EFFETS**

```text
Durée : 3 tours

À la fin de chaque activation de la cible :
    soin de base = 3 + modificateur de Sagesse du Prêtre source
    minimum : 1 PV

Si le Prêtre possède Soin renforcé :
    ce soin périodique bénéficie ensuite de +25 %
```

**UTILISATION**

```text
Coût : 2 points d'action
Mana : 5
Cible : soi-même ou un allié vivant
Portée : 3 cases
Recharge : 2 rounds
```

**ACQUISITION**

```text
Niveau requis : 6
Coût : 1 point de Talent
Prérequis : Soin renforcé
```

**Audit source** : conforme.

---

## 4.3 Soin de groupe — palier III / niveau 10

**TYPE**  
SORT ACTIF

**PRINCIPE**  
Le Prêtre soigne simultanément tous les membres vivants du groupe. Les personnages
déjà vaincus ne sont pas ciblés et le sort ne les ressuscite pas.

**EFFETS**

```text
Pour chaque membre vivant :

Soin de base :
    5
    + modificateur de Sagesse
    + rang de Médecine

Maximum :
    PV maximum de la cible

Soin renforcé :
    +25 % après calcul de la formule
```

**UTILISATION**

```text
Coût : 3 points d'action
Mana : 8
Cible : tous les membres vivants du groupe
Recharge : 3 rounds
```

**ACQUISITION**

```text
Niveau requis : 10
Coût : 1 point de Talent
Prérequis : Régénération
```

**Audit source** : conforme. Les tests valident notamment
`(5 + SAG mod 3 + Médecine 4) × 1,25 = 15`.

---

## 4.4 Purification — palier IV / niveau 14

**TYPE**  
SORT ACTIF

**PRINCIPE**  
Le Prêtre retire jusqu'à deux afflictions purifiables d'un membre vivant du groupe.
Lorsque plusieurs effets sont éligibles, le système choisit d'abord les plus
puissants puis applique un départage déterministe.

**EFFETS**

```text
Nombre maximal d'effets retirés : 2

Afflictions explicitement reconnues :
    Poison
    Brûlure
    Saignement
    Ralenti
    Silence
    Immobilisé

Sont également éligibles :
    les effets négatifs explicitement marqués comme purifiables

Priorité :
    puissance la plus élevée
    puis départage déterministe
```

L'identifiant technique utilisé pour le second départage n'est pas affiché au
joueur.

**UTILISATION**

```text
Coût : 2 points d'action
Mana : 6
Cible : soi-même ou un allié vivant
Portée : 3 cases
Recharge : 2 rounds
```

**ACQUISITION**

```text
Niveau requis : 14
Coût : 1 point de Talent
Prérequis : Soin de groupe
```

**Audit source** : conforme.

---

## 4.5 Miracle — palier V / niveau 18

**TYPE**  
SORT ACTIF

**PRINCIPE**  
Le Prêtre réalise un soin de crise sur un membre vivant du groupe. Le soin utilise
sa formule normale mais garantit au minimum la quantité nécessaire pour amener la
cible à 50 % de ses PV maximum. Miracle purifie également plusieurs afflictions et
restaure une partie de l'armure magique.

**EFFETS**

```text
Soin de base :
    12
    + 2 × modificateur de Sagesse
    + rang de Religion

Soin minimal :
    quantité nécessaire pour atteindre 50 % des PV maximum

Soin réellement retenu :
    maximum entre la formule et ce soin minimal

Soin renforcé :
    +25 % selon les règles normales de soin sortant

Purification :
    jusqu'à 3 effets négatifs purifiables

Armure magique :
    restaure 25 % du pool de référence

Résurrection :
    aucune
```

**UTILISATION**

```text
Coût : 4 points d'action
Mana : 15
Cible : soi-même ou un allié vivant
Portée : 3 cases
Recharge : 5 rounds
```

**ACQUISITION**

```text
Niveau requis : 18
Coût : 1 point de Talent
Prérequis : Purification
```

**Audit source** : conforme. Le ciblage vivant empêche Miracle de devenir une
résurrection implicite.

---

# 5. Branche PROTECTION

## 5.1 Bénédiction — palier I / niveau 2

**TYPE**  
SORT ACTIF

**PRINCIPE**  
Le Prêtre bénit un membre du groupe afin d'améliorer temporairement sa précision
et son initiative.

**EFFETS**

```text
Précision : +2
Initiative : +4
Durée : 2 rounds
Cumul : non
Réapplication : rafraîchit la durée
```

**UTILISATION**

```text
Coût : 2 points d'action
Mana : 5
Cible : soi-même ou un allié vivant
Portée : 3 cases
Recharge : 2 rounds
```

**ACQUISITION**

```text
Niveau requis : 2
Coût : 1 point de Talent
Prérequis : aucun
```

**Audit source** : conforme.

---

## 5.2 Égide — palier II / niveau 6

**TYPE**  
SORT ACTIF

**PRINCIPE**  
Le Prêtre restaure immédiatement l'armure magique d'un membre du groupe, sans
pouvoir dépasser son pool d'armure magique de référence.

**EFFETS**

```text
Armure magique restaurée :
    8
    + modificateur de Sagesse
    + rang de Religion

Maximum :
    pool d'armure magique de référence
```

**UTILISATION**

```text
Coût : 2 points d'action
Mana : 6
Cible : soi-même ou un allié vivant
Portée : 3 cases
Recharge : 2 rounds
```

**ACQUISITION**

```text
Niveau requis : 6
Coût : 1 point de Talent
Prérequis : Bénédiction
```

**Audit source** : conforme.

---

## 5.3 Protection sacrée — palier III / niveau 10

**TYPE**  
SORT ACTIF

**PRINCIPE**  
Le Prêtre protège un membre du groupe contre les sorts ciblés. Le bénéficiaire
devient plus difficile à toucher et plus résistant aux dégâts sacrés, nécrotiques
et arcaniques. Les sorts de zone ne bénéficient pas de cette protection.

**EFFETS**

```text
Contre les sorts ciblés :

Esquive : +2
Résistance Sacré : +25 %
Résistance Nécrotique : +25 %
Résistance Arcane : +25 %

Durée : 3 rounds

Sorts de zone :
    bonus non appliqués
```

**UTILISATION**

```text
Coût : 2 points d'action
Mana : 7
Cible : soi-même ou un allié vivant
Portée : 3 cases
Recharge : 3 rounds
```

**ACQUISITION**

```text
Niveau requis : 10
Coût : 1 point de Talent
Prérequis : Égide
```

**Audit source** : conforme.

---

## 5.4 Sanctuaire — palier IV / niveau 14

**TYPE**  
SORT ACTIF

**PRINCIPE**  
Le Prêtre place un membre du groupe sous Sanctuaire. Tant que l'effet subsiste,
les attaques hostiles ciblées ne peuvent pas le sélectionner directement.
Sanctuaire cesse au début de la prochaine activation du bénéficiaire ou après sa
première action qui inflige effectivement des dégâts.

**EFFETS**

```text
Ciblage hostile direct :
    bloqué

Ne protège pas contre :
    attaques de zone
    dégâts périodiques
    surfaces

Fin de l'effet :
    début de la prochaine activation du bénéficiaire
    OU première action du bénéficiaire infligeant réellement des dégâts

Durée de sécurité maximale :
    2 rounds
```

Une action offensive qui n'inflige finalement aucun dégât ne consomme pas
Sanctuaire.

**UTILISATION**

```text
Coût : 3 points d'action
Mana : 10
Cible : soi-même ou un allié vivant
Portée : 3 cases
Recharge : 4 rounds
```

**ACQUISITION**

```text
Niveau requis : 14
Coût : 1 point de Talent
Prérequis : Protection sacrée
```

**Audit source** : conforme. Le statut exige explicitement un événement d'attaque
réussie provenant du bénéficiaire avec dégâts effectivement appliqués avant
consommation.

---

## 5.5 Bastion divin — palier V / niveau 18

**TYPE**  
SORT ACTIF

**PRINCIPE**  
Le Prêtre restaure immédiatement l'armure magique de tout le groupe vivant puis
accorde une protection temporaire contre tous les types de dégâts actuellement
non physiques.

**EFFETS**

```text
Pour chaque membre vivant du groupe :

Armure magique restaurée :
    35 % du pool de référence

Puis pendant 2 rounds :
    dégâts non physiques reçus : -20 %

Types actuellement couverts :
    Feu
    Glace
    Foudre
    Poison
    Sacré
    Nécrotique
    Arcane

Dégâts physiques :
    aucune réduction par Bastion divin
```

**UTILISATION**

```text
Coût : 4 points d'action
Mana : 12
Cible : tous les membres vivants du groupe
Recharge : 5 rounds
```

**ACQUISITION**

```text
Niveau requis : 18
Coût : 1 point de Talent
Prérequis : Sanctuaire
```

**Audit source** : conforme.

---

# 6. Branche EXORCISME

## 6.1 Lumière sacrée — palier I / niveau 2

**TYPE**  
SORT ACTIF

**PRINCIPE**  
Le Prêtre projette une attaque sacrée sur un ennemi. Elle est particulièrement
efficace contre les Morts-vivants et les Démons.

**EFFETS**

```text
Dégâts :
    5
    + modificateur de Sagesse
    + rang de Religion

Type : Sacré

Contre Mort-vivant ou Démon :
    dégâts : +50 %

Jet pour toucher :
    aucun

Coup critique :
    impossible

Statut supplémentaire :
    aucun
```

**UTILISATION**

```text
Coût : 2 points d'action
Mana : 4
Cible : un ennemi
Portée : 5 cases
Ligne de vue : requise
Recharge : aucune
```

**ACQUISITION**

```text
Niveau requis : 2
Coût : 1 point de Talent
Prérequis : aucun
```

**Audit source** : conforme.

---

## 6.2 Repousser les morts-vivants — palier II / niveau 6

**TYPE**  
SORT ACTIF

**PRINCIPE**  
Le Prêtre libère une onde sacrée centrée sur le groupe. Seuls les Morts-vivants
dans la zone sont affectés. Si leur armure magique est épuisée après les dégâts,
ils sont repoussés si le déplacement est possible et subissent une pénalité
d'initiative.

**EFFETS CIBLES — CONTRAT RPG02**

```text
Cibles :
    Morts-vivants uniquement

Dégâts sacrés :
    4 fixes

Si armure magique = 0 après les dégâts :
    repousse de 1 case à l'opposé du groupe, si possible
    Initiative : -4 pendant 1 round

Jet pour toucher :
    aucun

Coup critique :
    impossible
```

**UTILISATION**

```text
Coût : 3 points d'action
Mana : 7
Zone : rayon 2 cases autour du groupe
Ligne de vue : non requise
Recharge : 3 rounds
```

**ACQUISITION**

```text
Niveau requis : 6
Coût : 1 point de Talent
Prérequis : Lumière sacrée
```

**Audit source** : **écart détecté**. RPG02 exige 4 dégâts fixes, mais
`MakeHolyAttack` assigne actuellement aussi `DamageScalingAttribute=Wisdom` à
cette action. Le runtime est donc susceptible de produire **4 + modificateur de
Sagesse**. Voir **PRIEST-01**.

---

## 6.3 Dissipation sacrée — palier III / niveau 10

**TYPE**  
SORT ACTIF

**PRINCIPE**  
Le Prêtre utilise le même sort de deux manières : purifier certains effets
nécrotiques ou malédictions sur le groupe, ou dissiper un effet magique positif
sur un Mort-vivant hostile.

**EFFETS**

```text
Sur soi-même ou un allié :
    retire jusqu'à 2 effets négatifs
    portant le marqueur Nécrotique ou Malédiction

Sur un Mort-vivant hostile :
    retire 1 effet positif magique amovible

Priorité :
    puissance la plus élevée
    puis départage déterministe
```

**UTILISATION**

```text
Coût : 2 points d'action
Mana : 6
Cible : soi-même / allié, ou Mort-vivant hostile
Portée : 3 cases
Ligne de vue : requise par l'action pour le ciblage hostile
Recharge : 2 rounds
```

**ACQUISITION**

```text
Niveau requis : 10
Coût : 1 point de Talent
Prérequis : Repousser les morts-vivants
```

**Audit source** : conforme. La partie hostile est explicitement filtrée sur la
catégorie Mort-vivant.

---

## 6.4 Châtiment — palier IV / niveau 14

**TYPE**  
SORT ACTIF

**PRINCIPE**  
Le Prêtre frappe un ennemi avec une attaque sacrée puissante. Elle inflige encore
davantage de dégâts aux Morts-vivants et aux Démons.

**EFFETS**

```text
Dégâts :
    10
    + 2 × modificateur de Sagesse
    + rang de Religion

Type : Sacré

Contre Mort-vivant ou Démon :
    dégâts : +50 %

Jet pour toucher :
    aucun

Coup critique :
    impossible
```

**UTILISATION**

```text
Coût : 3 points d'action
Mana : 8
Cible : un ennemi
Portée : 4 cases
Ligne de vue : requise
Recharge : 2 rounds
```

**ACQUISITION**

```text
Niveau requis : 14
Coût : 1 point de Talent
Prérequis : Dissipation sacrée
```

**Audit source** : conforme.

---

## 6.5 Exorcisme majeur — palier V / niveau 18

**TYPE**  
SORT ACTIF

**PRINCIPE**  
Le Prêtre consacre une vaste zone contre les créatures surnaturelles ciblées par
le Talent. Seuls les Morts-vivants, Démons et créatures invoquées sont affectés.
Une cible dont l'armure magique est épuisée peut être bannie temporairement.

**EFFETS**

```text
Cibles affectées uniquement :
    Morts-vivants
    Démons
    créatures invoquées

Dégâts :
    14
    + 2 × modificateur de Sagesse
    + rang de Religion

Type : Sacré

Si armure magique = 0 après les dégâts :
    Banni pendant 1 tour
    -> la prochaine activation est perdue

Jet pour toucher :
    aucun

Coup critique :
    impossible
```

**UTILISATION**

```text
Coût : 4 points d'action
Mana : 14
Cellule cible : portée 4 cases
Zone : rayon 2 cases
Ligne de vue : requise
Recharge : 5 rounds
```

**ACQUISITION**

```text
Niveau requis : 18
Coût : 1 point de Talent
Prérequis : Châtiment
```

**Audit source** : conforme au comportement actuellement implémenté. La mention
RPG02 d'une éventuelle future destruction des invocations faibles n'est **pas une
mécanique actuelle** ; voir **PRIEST-02**.

---

# 7. Chaînes de progression validées

## Restauration

```text
Soin renforcé
    -> Régénération
        -> Soin de groupe
            -> Purification
                -> Miracle
```

## Protection

```text
Bénédiction
    -> Égide
        -> Protection sacrée
            -> Sanctuaire
                -> Bastion divin
```

## Exorcisme

```text
Lumière sacrée
    -> Repousser les morts-vivants
        -> Dissipation sacrée
            -> Châtiment
                -> Exorcisme majeur
```

# 8. Concordance mécanique / authoring

| Talent | RPG02 vs authoring | Décision UX |
|---|---|---|
| Soin renforcé | Conforme | PASSIF |
| Régénération | Conforme | SORT ACTIF |
| Soin de groupe | Conforme | SORT ACTIF |
| Purification | Conforme | SORT ACTIF |
| Miracle | Conforme | SORT ACTIF |
| Bénédiction | Conforme | SORT ACTIF |
| Égide | Conforme | SORT ACTIF |
| Protection sacrée | Conforme | SORT ACTIF |
| Sanctuaire | Conforme | SORT ACTIF |
| Bastion divin | Conforme | SORT ACTIF |
| Lumière sacrée | Conforme | SORT ACTIF |
| Repousser les morts-vivants | **Écart : scaling SAG runtime non prévu par RPG02** | SORT ACTIF |
| Dissipation sacrée | Conforme | SORT ACTIF |
| Châtiment | Conforme | SORT ACTIF |
| Exorcisme majeur | Conforme ; future règle Summoned non implémentée | SORT ACTIF |

# 9. Arbitrages et reports proposés

## PRIEST-01 — dégâts de Repousser les morts-vivants

RPG02 est explicite :

```text
Chaque Mort-vivant dans la zone subit 4 dégâts sacrés.
```

L'authoring appelle toutefois le helper générique `MakeHolyAttack`, qui affecte
systématiquement :

```text
DamageScalingAttribute = Wisdom
```

même lorsque `AdditionalWisdomModifierScale=0`.

Conséquence : l'action peut actuellement résoudre :

```text
4 + modificateur de Sagesse
```

alors que le contrat dit 4 fixes.

**Recommandation : conserver RPG02 comme autorité et canoniser 4 dégâts fixes.**
L'authoring devra être corrigé plus tard dans la phase d'implémentation/régression,
sans modifier le contrat UX.

## PRIEST-02 — destruction future des créatures invoquées faibles

RPG02 mentionne :

```text
Une future règle d'invocation pourra détruire les Summoned faibles,
sans hard-code ici.
```

Aucune telle règle n'existe actuellement dans Exorcisme majeur.

**Recommandation : ne pas afficher cette promesse dans la fiche joueur actuelle.**

Le Talent actuel :

```text
filtre Mort-vivant / Démon / Invoqué
+ dégâts sacrés
+ Banni si armure magique épuisée
```

Si un futur système d'invocation ajoute cette mécanique, elle fera l'objet d'un
contrat distinct avant d'être projetée dans la fiche.

## PRIEST-03 — sémantique joueur de « Ally »

Les sources RPG02 alternent entre « Ally » et « Self/Ally », alors que le resolver
générique actuel permet à une action `Ally` de sélectionner explicitement le
lanceur lui-même.

**Recommandation : canoniser pour l'UX Prêtre :**

```text
Ally -> soi-même ou un allié vivant
```

Cela concerne :

```text
Régénération
Purification
Miracle
Bénédiction
Égide
Protection sacrée
Sanctuaire
partie alliée de Dissipation sacrée
```

Cette décision ne change aucune mécanique ; elle rend simplement le comportement
runtime existant explicite.

# 10. Interactions de branche à rendre compréhensibles

## Soin renforcé et Restauration

Toute progression complète de Restauration possède nécessairement Soin renforcé.
La fiche de chaque Talent affiche sa formule propre, mais l'UI doit être capable
d'expliquer que les soins de sorts du Prêtre sont ensuite amplifiés de 25 %.

Elle ne doit toutefois pas recalculer une valeur finale dépendante des
caractéristiques/rangs du personnage si cette valeur n'est pas fournie par le
read-model.

## Sanctuaire

Le joueur doit comprendre que Sanctuaire n'est pas une invulnérabilité :

```text
bloque le ciblage hostile direct
mais pas AoE / dégâts périodiques / surfaces
et se brise seulement si le bénéficiaire inflige réellement des dégâts
```

## Protection sacrée

Le +2 Esquive et les résistances +25 % sont conditionnés aux **sorts non-AoE**.
Ils ne sont pas des bonus défensifs universels.

# 11. Skills — état actuel

Le catalogue de production UI-RPG06.2A contient notamment :

```text
Médecine
Religion
```

Utilisation dans les Talents Prêtre :

```text
Soin de groupe
    -> rang de Médecine

Miracle
    -> rang de Religion

Égide
    -> rang de Religion

Lumière sacrée
    -> rang de Religion

Châtiment
    -> rang de Religion

Exorcisme majeur
    -> rang de Religion
```

DESC01.10 ne crée donc aucune nouvelle autorité de Skill.

# 12. Vocabulaire Prêtre à normaliser

À employer :

```text
Sagesse
Médecine
Religion
Précision
Initiative
Esquive
PV / PV maximum
armure magique
Sacré
Nécrotique
Arcane
Poison
Brûlure
Saignement
Ralenti
Silence
Immobilisé
Mort-vivant
Démon
créature invoquée
Banni
purifiable
effet positif
effet négatif
```

À ne pas exposer :

```text
WIS
Skill_Medicine
Skill_Religion
MagicalArmor
Holy
Necrotic
Status_*
Purifiable tag
Dispel.Magical
Potency
EffectId
SourcePolicy
TargetingPolicy
AllowedMonsterCategoryIds
bSkipActivation
bBlockDirectHostileTargeting
ArmorGate
```

# 13. Contrat de données attendu plus tard

DESC01.10 ne définit pas encore la structure C++ finale.

L'audit Prêtre montre que le futur read-model devra pouvoir représenter :

```text
type UX PASSIF / SORT ACTIF
soin direct et périodique
scaling attribut + Skill
bonus global de soin
seuil minimal de PV après soin
restauration d'armure magique
purification / dissipation avec filtres
buffs temporaires
résistances conditionnelles par source d'action
blocage de ciblage direct
consommation d'un statut sur dégâts réellement infligés
zone centrée sur le groupe
filtre par catégorie de monstre
déplacement forcé après ArmorGate
pénalité d'initiative
dégâts Sacrés avec bonus de catégorie
contrôle Banni / perte d'activation
acquisition
```

La forme C++ sera définie seulement en DESC01.13 après l'audit 90/90.

# 14. Critères de validation UI-RPG-DESC01.10

Le jalon est validé lorsque l'utilisateur approuve :

- les 15 fiches Prêtre ;
- la classification **14 SORTS ACTIFS / 1 PASSIF** ;
- la sémantique « Ally = soi-même ou allié vivant » ;
- les trois chaînes de progression ;
- le vocabulaire joueur ;
- PRIEST-01 : 4 dégâts fixes comme contrat de Repousser les morts-vivants ;
- PRIEST-02 : ne pas afficher la future destruction d'invocations faibles ;
- PRIEST-03 : normalisation du ciblage Ally.

Après validation :

```text
UI-RPG-DESC01.11 — audit documentaire des 15 Talents de l'Alchimiste
```
