# BESTIAIRE DES PROFONDEURS
## Volume II - Les Salles Interdites

**Projet :** GrimrockPrototype
**Moteur :** Unreal Engine 5.5.4
**Genre :** Dungeon crawler en vue subjective, déplacement case par case
**Document :** Artbook / Bible artistique / Document de conception du bestiaire
**Version :** 0.1 - Base de travail

---

# Intention du Volume II

Le Volume I décrivait les créatures fondamentales du premier donjon : vermines, morts-vivants simples, matières vivantes, pièges et gardiens. Le Volume II fait franchir un seuil au prototype : le donjon devient plus intelligent, plus rituel et plus dangereux.

Ce volume introduit trois familles de nouveautés :

- des ennemis capables de fuir, harceler ou appeler de l’aide ;
- des créatures magiques capables d’activer des événements ;
- des monstres de puzzle-combat, dont la mort, la position ou l’activation modifie le niveau.

# Sous-titre artistique

**Les Salles Interdites** désignent les portions du donjon où les créatures ne sont plus seulement prisonnières du lieu : elles le servent, le comprennent ou le manipulent. Le joueur y rencontre des cultes, des sentinelles, des prédateurs plus rapides et des forces élémentaires.

# Mots-clés visuels

- rituels oubliés ;
- portes scellées ;
- torches vacillantes ;
- fumées magiques ;
- runes, sceaux et craies rituelles ;
- cuir sale, os peints, métal noirci ;
- cristaux de glace, cendres, braises ;
- toiles épaisses, cocons et venins.

# Nouvelles mécaniques de gameplay

| Mécanique | Description | Créatures concernées |
|---|---|---|
| Fuite et appel | Un ennemi peut reculer, ouvrir une voie ou appeler des alliés. | Gobelin pillard, gobelin lanceur |
| Magie d’événement | Un monstre active des portes, sceaux, alarmes ou invocations. | Cultiste, œil flottant, spectre |
| Patrouille et alarme | Le joueur doit gérer la détection, pas seulement le combat. | Œil flottant |
| Intangibilité | Certaines armes ordinaires deviennent peu efficaces. | Spectre mineur |
| Contrôle élémentaire | Le terrain est modifié par feu, froid ou givre. | Chien infernal, élémentaire de glace |
| Boss à phases | Le combat devient une énigme spatiale. | Reine Araignée |

# Profils IA à préparer

```text
DirectMelee
FastHarasser
RangedKeeper
GuardStationary
Ambush
SlowPressure
PatrolSentinel
Caster
FleeAndCallHelp
PuzzleLinked
BossPhases
Summoner
```

# Classification du Volume II

```text
Volume II - Les Salles Interdites
├── Humanoïdes hostiles
│   ├── Gobelin pillard
│   ├── Gobelin lanceur
│   └── Cultiste
│
├── Sentinelles magiques
│   ├── Œil flottant
│   └── Spectre mineur
│
├── Prédateurs des profondeurs
│   ├── Goule
│   ├── Serpent venimeux
│   └── Loup des cavernes
│
├── Constructs et armes vivantes
│   └── Armure animée
│
├── Créatures élémentaires
│   ├── Chien infernal mineur
│   └── Élémentaire de glace
│
└── Boss
    └── Reine Araignée
```

# Tableau synthétique

| N° | Créature | Rôle | IA | Danger | Nouveauté |
|---:|---|---|---|---:|---|
| 01 | Gobelin pillard | Melee intelligent / harceleur faible | FleeAndCallHelp / DirectMelee | 2 | Première IA vraiment intelligente : fuite, appel, embuscade. |
| 02 | Gobelin lanceur | Projectile / harcèlement | RangedKeeper / FleeAndCallHelp | 3 | Ennemi à distance mobile qui fuit les coins morts. |
| 03 | Cultiste | Mage / déclencheur d’événements | Caster / PuzzleLinked | 4 | Premier ennemi réellement connecté au système d’événements. |
| 04 | Œil flottant | Détection / alarme | PatrolSentinel / PuzzleLinked | 3 | Monstre de surveillance et d’alarme. |
| 05 | Spectre mineur | Hantise / résistance aux armes normales | DirectMelee / PuzzleLinked | 4 | Introduit l’intangibilité et les solutions rituelles. |
| 06 | Goule | Mêlée rapide / maladie | FastHarasser / DirectMelee | 4 | Mort-vivant agressif qui brise le rythme lent des zombies. |
| 07 | Serpent venimeux | Poison avancé / menace basse | FastHarasser | 3 | Menace basse et rapide, difficile à lire au sol. |
| 08 | Loup des cavernes | Chargeur rapide / pression mobile | FastHarasser / DirectMelee | 3 | Prédateur mobile adapté aux salles plus ouvertes. |
| 09 | Armure animée | Duel / parade / gardien | GuardStationary / DirectMelee | 5 | Ennemi martial défensif, proche du duel. |
| 10 | Chien infernal mineur | Mêlée élémentaire / zone brûlante | FastHarasser / DirectMelee | 4 | Première créature de feu mobile. |
| 11 | Élémentaire de glace | Ralentissement / gel | SlowPressure / PuzzleLinked | 5 | Contrôle de terrain par le froid. |
| 12 | Reine Araignée | Boss poison / invocation / contrôle de zone | BossPhases / Summoner | 6 | Boss de synthèse des mécaniques poison, toiles et invocation. |

---

# FICHE 01 - Gobelin pillard

## Nom technique
`MON_GoblinRaider`

## Catégorie
Humanoide hostile

## Description immersive
Le gobelin pillard est la première créature qui donne l'impression que le donjon est habité par autre chose que des bêtes et des morts. Il observe, se cache, frappe quand il peut, puis fuit si le combat tourne mal. Il ne cherche pas toujours à tuer : il cherche à survivre et à voler.

## Intention artistique
Petit humanoide nerveux, cuir sale, os peints, sacoche, dague ou gourdin, posture voûtée, regard rusé, silhouette asymétrique et très lisible.

### Silhouette
- petite taille ;
- dos voûté ;
- bras longs ;
- arme courte visible ;
- sacoche ou trophées volés ;

### Matières
- peau gris-verte ou brunâtre ;
- cuir rapiécé ;
- os et dents en pendentifs ;
- métal bas de gamme ;
- tissus sales ;

## Gameplay
| Élément | Valeur |
|---|---|
| Rôle | Melee intelligent / harceleur faible |
| Dangerosité | 2 |
| Taille | 1 case |
| Comportement IA | FleeAndCallHelp / DirectMelee |
| Vitesse | Moyenne |
| Attaque | Dague, gourdin, attaque en groupe |
| Effet spécial | Peut fuir, appeler des renforts ou attirer le joueur dans une embuscade |
| Faiblesse | Feu, peur, isolement |
| Résistance | Aucune |
| Peut activer plaques | Oui |
| Peut ouvrir portes | Oui, si scripté |
| Loot | Pièces, dague rouillée, ration volée |

## Utilisation en level design
- placer en duo dans une salle latérale ;
- peut fuir vers un levier qui ouvre une cage ;
- peut attirer le joueur vers une plaque de piège ;
- excellent pour introduire la poursuite sur grille ;

## Animations nécessaires
- idle nerveux ;
- marche prudente ;
- attaque courte ;
- recul/fuite ;
- appel de renfort ;
- mort rapide ;

## Sons nécessaires
- gloussement ;
- pas rapides ;
- cri d’alerte ;
- rire nerveux ;
- petit impact de lame ;

## VFX nécessaires
- poussière au déplacement ;
- petits signaux d’alerte optionnels ;

## Prompt concept art
Planche de bestiaire médiéval dark fantasy, gobelin pillard de donjon, petit humanoide voûté, cuir rapiécé, dague rouillée, sacoche de voleur, posture nerveuse, couloir de pierre humide, lumière de torche, annotations françaises, style artbook parchemin.

---

# FICHE 02 - Gobelin lanceur

## Nom technique
`MON_GoblinThrower`

## Catégorie
Humanoide hostile a distance

## Description immersive
Le gobelin lanceur ne combat pas loyalement. Il se tient derrière les grilles, les tables renversées ou les angles de couloir, puis bombarde l’intrus avant de reculer. Son rôle est de forcer le joueur à bouger et à utiliser les portes comme couverture.

## Intention artistique
Gobelin maigre, bras longs, carquois de couteaux ou sac de pierres, capuche courte, posture de lancer, sourire cruel.

### Silhouette
- petit corps élancé ;
- bras levé en lancer ;
- sac de projectiles ;
- jambes prêtes à fuir ;

### Matières
- cuir sec ;
- cordes et sangles ;
- métal émoussé ;
- pierres taillées ;
- tissus poussiéreux ;

## Gameplay
| Élément | Valeur |
|---|---|
| Rôle | Projectile / harcèlement |
| Dangerosité | 3 |
| Taille | 1 case |
| Comportement IA | RangedKeeper / FleeAndCallHelp |
| Vitesse | Moyenne à rapide |
| Attaque | Couteaux, pierres, fioles acides |
| Effet spécial | Maintient la distance et force le joueur à avancer |
| Faiblesse | Contact direct, feu |
| Résistance | Aucune |
| Peut activer plaques | Oui |
| Peut ouvrir portes | Oui, si scripté |
| Loot | Couteaux, pierres taillées, fiole vide |

## Utilisation en level design
- couloirs longs ;
- derrière une grille ouverte ;
- salle avec piliers ;
- force à gérer la ligne de vue ;

## Animations nécessaires
- idle avec projectile en main ;
- viser ;
- lancer ;
- reculer ;
- fuite ;
- mort ;

## Sons nécessaires
- ricanement ;
- projectile sifflant ;
- pierre au sol ;
- pas rapides ;
- cri de panique ;

## VFX nécessaires
- traînée de projectile ;
- éclat acide optionnel ;

## Prompt concept art
Planche de bestiaire dark fantasy, gobelin lanceur, petit humanoide maigre avec sac de pierres et couteaux, posture de lancer, couloir de donjon, grilles et torches, annotations françaises, parchemin ancien, concept art RPG.

---

# FICHE 03 - Cultiste

## Nom technique
`MON_Cultist`

## Catégorie
Humanoide magique

## Description immersive
Le cultiste est un serviteur vivant d’un pouvoir enfoui. Il connaît des passages, des mots, des sceaux et des mécanismes que les autres créatures ignorent. Sa menace vient autant de ce qu’il active que de ce qu’il lance.

## Intention artistique
Silhouette humaine encapuchonnée, robe sombre, masque osseux ou visage à moitié caché, symboles rituels, petite lanterne ou tome.

### Silhouette
- capuche haute ;
- robe longue ;
- bras levé ;
- tome ou dague ;
- forme humaine lisible ;

### Matières
- tissu noir ou brun ;
- craie rituelle ;
- os gravés ;
- cuir de reliure ;
- lueur magique faible ;

## Gameplay
| Élément | Valeur |
|---|---|
| Rôle | Mage / déclencheur d’événements |
| Dangerosité | 4 |
| Taille | 1 case |
| Comportement IA | Caster / PuzzleLinked |
| Vitesse | Moyenne |
| Attaque | Sort faible, dague rituelle, invocation |
| Effet spécial | Peut ouvrir des portes, invoquer, activer un sceau |
| Faiblesse | Contact direct, interruption, lumière sacrée |
| Résistance | Peur, poison léger |
| Peut activer plaques | Oui |
| Peut ouvrir portes | Oui, scripté |
| Loot | Rune mineure, dague rituelle, parchemin |

## Utilisation en level design
- active une grille à distance ;
- invoque deux squelettes ;
- protège un autel ;
- sert de tutoriel au scripting d’événements ;

## Animations nécessaires
- incantation ;
- recul ;
- attaque magique ;
- activation de sceau ;
- mort rituelle ;

## Sons nécessaires
- chuchotement ;
- incantation ;
- souffle magique ;
- tissu froissé ;
- cri humain étouffé ;

## VFX nécessaires
- cercle runique ;
- lueur de main ;
- fumée noire ;
- étincelle de sceau ;

## Prompt concept art
Planche de bestiaire médiéval dark fantasy, cultiste encapuchonné de donjon, robe sombre, dague rituelle, tome, runes, lueur magique faible, autel de pierre, torches, annotations françaises, parchemin artbook.

---

# FICHE 04 - Œil flottant

## Nom technique
`MON_FloatingEye`

## Catégorie
Sentinelle magique

## Description immersive
L’œil flottant dérive dans les couloirs sans bruit. Il n’est pas toujours fait pour tuer ; il est fait pour voir. Lorsqu’il repère un intrus, le donjon semble soudain se souvenir qu’il possède des portes, des grilles et des gardiens.

## Intention artistique
Sphère oculaire suspendue, veines ou tentacules courts, iris lumineux, membrane humide, aura magique discrète.

### Silhouette
- forme ronde très lisible ;
- petits appendices ;
- iris central ;
- flottement stable ;

### Matières
- cornée brillante ;
- veines sombres ;
- iris lumineux ;
- sécrétions translucides ;
- fragments cristallins ;

## Gameplay
| Élément | Valeur |
|---|---|
| Rôle | Détection / alarme |
| Dangerosité | 3 |
| Taille | 1 case |
| Comportement IA | PatrolSentinel / PuzzleLinked |
| Vitesse | Lente à moyenne |
| Attaque | Rayon faible ou cri d’alarme |
| Effet spécial | Déclenche alarme, réveille gardiens, révèle portes secrètes |
| Faiblesse | Projectiles, lumière vive |
| Résistance | Poison, saignement |
| Peut activer plaques | Non |
| Peut ouvrir portes | Non, mais peut les commander par événement |
| Loot | Cristal oculaire, nerf arcanique |

## Utilisation en level design
- patrouille d’un couloir ;
- réveille une gargouille ;
- déclenche une grille ;
- peut ouvrir une porte secrète à sa mort ;

## Animations nécessaires
- flottement ;
- rotation lente ;
- détection ;
- cri silencieux/alarme ;
- rayon ;
- éclatement ;

## Sons nécessaires
- bourdonnement ;
- pulsation magique ;
- cri aigu ;
- verre qui se fissure ;

## VFX nécessaires
- cône de vision discret ;
- flash d’alarme ;
- rayon oculaire ;
- particules arcanes ;

## Prompt concept art
Planche de bestiaire dark fantasy, œil flottant magique, sentinelle de donjon, globe oculaire suspendu avec veines et iris lumineux, petits appendices, couloir sombre, annotations françaises, parchemin ancien.

---

# FICHE 05 - Spectre mineur

## Nom technique
`MON_MinorWraith`

## Catégorie
Mort-vivant intangible

## Description immersive
Le spectre mineur est une présence plus qu’un corps. Il se forme là où une mort n’a pas trouvé de repos. Le combattre frontalement est souvent inefficace : il faut comprendre ce qui le retient au lieu.

## Intention artistique
Silhouette humaine brumeuse, lambeaux flottants, visage à peine visible, traînée de fumée froide, transparence partielle.

### Silhouette
- forme humaine allongée ;
- bas du corps diffus ;
- bras flottants ;
- contour irrégulier ;

### Matières
- brume bleutée ;
- cendre froide ;
- lambeaux translucides ;
- lueur pâle ;
- poussière funéraire ;

## Gameplay
| Élément | Valeur |
|---|---|
| Rôle | Hantise / résistance aux armes normales |
| Dangerosité | 4 |
| Taille | 1 case |
| Comportement IA | DirectMelee / PuzzleLinked |
| Vitesse | Moyenne |
| Attaque | Contact spectral, drain léger |
| Effet spécial | Traverse certaines grilles, résiste aux armes non magiques |
| Faiblesse | Lumière sacrée, armes enchantées, reliques |
| Résistance | Poison, saignement, tranchant normal |
| Peut activer plaques | Non |
| Peut ouvrir portes | Peut traverser certaines portes scriptées |
| Loot | Essence spectrale, cendre froide |

## Utilisation en level design
- apparaît après lecture d’une inscription ;
- disparaît si des os sont replacés ;
- traverse une grille sacrée ;
- protège une tombe ;

## Animations nécessaires
- apparition ;
- flottement ;
- attaque spectrale ;
- dissolution ;
- réaction à la lumière ;

## Sons nécessaires
- souffle froid ;
- chuchotement inversé ;
- gémissement lointain ;
- silence brutal ;

## VFX nécessaires
- transparence ;
- fumée froide ;
- halo spectral ;
- dissipation lumineuse ;

## Prompt concept art
Planche de bestiaire dark fantasy, spectre mineur de crypte, silhouette humaine translucide, brume bleutée, lambeaux flottants, lumière sacrée, tombe ancienne, annotations françaises, parchemin artbook.

---

# FICHE 06 - Goule

## Nom technique
`MON_Ghoul`

## Catégorie
Mort-vivant prédateur

## Description immersive
La goule n’est pas un mort lent. Elle rampe, bondit et se nourrit des restes funéraires. Elle apporte une violence animale dans les cryptes, obligeant le joueur à réagir vite même dans un espace contraint.

## Intention artistique
Humanoïde maigre et courbé, bras longs, griffes sales, peau pâle, bouche trop large, posture rampante.

### Silhouette
- dos très courbé ;
- bras longs ;
- jambes fléchies ;
- tête basse ;
- griffes visibles ;

### Matières
- peau pâle tendue ;
- ongles sombres ;
- dents sales ;
- haillons ;
- poussière de tombe ;

## Gameplay
| Élément | Valeur |
|---|---|
| Rôle | Mêlée rapide / maladie |
| Dangerosité | 4 |
| Taille | 1 case |
| Comportement IA | FastHarasser / DirectMelee |
| Vitesse | Rapide |
| Attaque | Griffes, morsure |
| Effet spécial | Peut infliger maladie ou affaiblissement |
| Faiblesse | Feu, lumière sacrée |
| Résistance | Poison, peur |
| Peut activer plaques | Oui |
| Peut ouvrir portes | Non |
| Loot | Griffe de goule, tissu funéraire |

## Utilisation en level design
- surgit d’un charnier ;
- attaque en duo avec zombies ;
- force le recul rapide ;
- punit les pauses dans les couloirs ;

## Animations nécessaires
- ramper ;
- bond court ;
- griffure ;
- morsure ;
- recul nerveux ;
- mort recroquevillée ;

## Sons nécessaires
- halètement ;
- griffes sur pierre ;
- râle aigu ;
- morsure sèche ;

## VFX nécessaires
- poussière au bond ;
- léger effet de maladie ;

## Prompt concept art
Planche de bestiaire dark fantasy, goule de crypte, mort-vivant maigre et courbé, longues griffes, peau pâle, posture rampante et agressive, couloir funéraire, annotations françaises, artbook parchemin.

---

# FICHE 07 - Serpent venimeux

## Nom technique
`MON_VenomSerpent`

## Catégorie
Prédateur venimeux

## Description immersive
Le serpent venimeux exploite les angles morts du regard. Il se glisse entre les dalles, attend dans les herbes souterraines ou sous les débris, puis frappe à hauteur de jambe.

## Intention artistique
Grand serpent souterrain, écailles sombres, motifs pâles, tête triangulaire, crochets visibles, corps enroulé.

### Silhouette
- corps bas ;
- forme sinueuse ;
- tête triangulaire ;
- queue longue ;
- profil discret ;

### Matières
- écailles noires ou vertes ;
- ventre pâle ;
- venin luisant ;
- yeux fendus ;
- poussière humide ;

## Gameplay
| Élément | Valeur |
|---|---|
| Rôle | Poison avancé / menace basse |
| Dangerosité | 3 |
| Taille | 1 case |
| Comportement IA | FastHarasser |
| Vitesse | Rapide |
| Attaque | Morsure venimeuse |
| Effet spécial | Poison plus dangereux, peut se dissimuler au sol |
| Faiblesse | Feu, froid, attaques de zone |
| Résistance | Poison |
| Peut activer plaques | Oui selon taille |
| Peut ouvrir portes | Non |
| Loot | Venin, croc, peau de serpent |

## Utilisation en level design
- piège dans une salle sombre ;
- garde un coffre bas ;
- se cache près d’un cadavre ;
- introduit antidotes avancés ;

## Animations nécessaires
- ondulation ;
- attaque rapide ;
- repli ;
- enroulement ;
- mort ;

## Sons nécessaires
- sifflement ;
- glissement discret ;
- morsure sèche ;
- cri faible ;

## VFX nécessaires
- goutte de venin ;
- icône poison côté UI ;

## Prompt concept art
Planche de bestiaire dark fantasy, serpent venimeux de donjon, corps long et sombre, tête triangulaire, crochets luisants, sol de pierre humide, lumière de torche, annotations françaises, parchemin.

---

# FICHE 08 - Loup des cavernes

## Nom technique
`MON_CaveWolf`

## Catégorie
Bête de donjon

## Description immersive
Le loup des cavernes chasse dans les anciennes galeries et les salles effondrées. Il n’est pas aussi rusé qu’un gobelin, mais son instinct suffit : contourner, charger, mordre, reculer.

## Intention artistique
Loup robuste, pelage gris sombre, épaules hautes, yeux brillants, museau marqué, cicatrices, posture de charge.

### Silhouette
- quadrupède bas ;
- épaule forte ;
- tête allongée ;
- queue basse ;
- silhouette de prédateur ;

### Matières
- fourrure grise humide ;
- cicatrices ;
- griffes sombres ;
- dents jaunes ;
- boue ;

## Gameplay
| Élément | Valeur |
|---|---|
| Rôle | Chargeur rapide / pression mobile |
| Dangerosité | 3 |
| Taille | 1 case |
| Comportement IA | FastHarasser / DirectMelee |
| Vitesse | Rapide |
| Attaque | Morsure, charge courte |
| Effet spécial | Peut charger en ligne droite et forcer le joueur à se replacer |
| Faiblesse | Feu, pièges, murs étroits |
| Résistance | Froid léger |
| Peut activer plaques | Oui |
| Peut ouvrir portes | Non |
| Loot | Fourrure, croc, viande |

## Utilisation en level design
- salle ouverte avec piliers ;
- attaque en meute de deux ;
- poursuite dans couloir ;
- peut déclencher plaques par accident ;

## Animations nécessaires
- marche prudente ;
- course ;
- charge courte ;
- morsure ;
- recul ;
- mort ;

## Sons nécessaires
- grognement ;
- aboiement grave ;
- griffes sur pierre ;
- halètement ;

## VFX nécessaires
- poussière de charge ;
- souffle froid optionnel ;

## Prompt concept art
Planche de bestiaire dark fantasy, loup des cavernes, prédateur gris sombre, fourrure humide, yeux brillants, posture de charge, ruines souterraines, torche, annotations françaises, artbook parchemin.

---

# FICHE 09 - Armure animée

## Nom technique
`MON_AnimatedArmor`

## Catégorie
Construct martial

## Description immersive
L’armure animée n’a ni chair ni peur. Elle attend dans les salles d’armes, immobile parmi les trophées, puis s’assemble dans un claquement de métal lorsque l’intrus franchit la ligne interdite.

## Intention artistique
Armure médiévale vide, métal noirci, lueur dans le casque, cape déchirée, arme cérémonielle, posture de duel.

### Silhouette
- forme humaine blindée ;
- épaulettes larges ;
- arme longue ;
- casque vide ;
- position de garde ;

### Matières
- acier noirci ;
- cuir desséché ;
- laiton oxydé ;
- runes gravées ;
- tissu poussiéreux ;

## Gameplay
| Élément | Valeur |
|---|---|
| Rôle | Duel / parade / gardien |
| Dangerosité | 5 |
| Taille | 1 case |
| Comportement IA | GuardStationary / DirectMelee |
| Vitesse | Lente à moyenne |
| Attaque | Épée, hallebarde ou masse |
| Effet spécial | Peut parer, résiste aux armes faibles |
| Faiblesse | Foudre, marteaux, désactivation runique |
| Résistance | Poison, saignement, tranchant léger |
| Peut activer plaques | Oui |
| Peut ouvrir portes | Non, sauf script |
| Loot | Plaque d’armure, rune, arme ancienne |

## Utilisation en level design
- salle d’armes ;
- se confond avec un décor ;
- bloque une porte scellée ;
- récompense en loot d’équipement ;

## Animations nécessaires
- immobile décorative ;
- assemblage ;
- parade ;
- attaque horizontale ;
- attaque verticale ;
- effondrement en pièces ;

## Sons nécessaires
- cliquetis d’armure ;
- métal qui se verrouille ;
- pas métalliques ;
- impact de lame ;

## VFX nécessaires
- étincelles ;
- lueur de casque ;
- runes de contrôle ;

## Prompt concept art
Planche de bestiaire dark fantasy, armure animée médiévale vide, métal noirci, casque lumineux, épée ancienne, salle d’armes de donjon, annotations françaises, parchemin artbook.

---

# FICHE 10 - Chien infernal mineur

## Nom technique
`MON_LesserHellhound`

## Catégorie
Bête élémentaire de feu

## Description immersive
Le chien infernal mineur n’est pas un démon majeur, mais une bête liée à la chaleur souterraine. Il chasse dans les forges oubliées, les salles calcinées et les corridors où l’air tremble.

## Intention artistique
Chien maigre et musclé, peau sombre, fissures rouges, gueule incandescente, fumée, yeux braise.

### Silhouette
- quadrupède agressif ;
- dos arqué ;
- gueule ouverte ;
- queue basse ;
- fissures lumineuses ;

### Matières
- peau noire ;
- braises sous la peau ;
- fumée ;
- salive chaude ;
- cendres ;

## Gameplay
| Élément | Valeur |
|---|---|
| Rôle | Mêlée élémentaire / zone brûlante |
| Dangerosité | 4 |
| Taille | 1 case |
| Comportement IA | FastHarasser / DirectMelee |
| Vitesse | Rapide |
| Attaque | Morsure de feu, souffle court |
| Effet spécial | Peut laisser une case brûlante temporaire |
| Faiblesse | Froid, eau, lumière sacrée |
| Résistance | Feu, peur |
| Peut activer plaques | Oui |
| Peut ouvrir portes | Non |
| Loot | Croc brûlé, cendre chaude, glande ignée |

## Utilisation en level design
- salle avec torches explosives ;
- couloir où il force à reculer ;
- interaction avec flaque d’huile ;
- vulnérable au froid ;

## Animations nécessaires
- idle fumant ;
- course ;
- morsure ;
- souffle court ;
- secouement de flammes ;
- mort en cendres ;

## Sons nécessaires
- grondement ;
- crépitement ;
- souffle chaud ;
- aboiement grave ;

## VFX nécessaires
- flamme courte ;
- fumée ;
- trace brûlante au sol ;
- braises ;

## Prompt concept art
Planche de bestiaire dark fantasy, chien infernal mineur, bête quadrupède sombre avec fissures de braise, gueule incandescente, fumée, donjon calciné, annotations françaises, parchemin.

---

# FICHE 11 - Élémentaire de glace

## Nom technique
`MON_IceElemental`

## Catégorie
Élémentaire de contrôle

## Description immersive
L’élémentaire de glace transforme le donjon en piège lent. Il ne poursuit pas toujours le joueur : il modifie l’espace. Les portes se couvrent de givre, les torches meurent, les dalles deviennent glissantes.

## Intention artistique
Forme humanoïde de glace et de roche gelée, cristaux, vapeur froide, cœur bleu pâle, silhouette anguleuse.

### Silhouette
- corps anguleux ;
- épaules cristallines ;
- jambes lourdes ;
- cœur lumineux ;
- forme minérale ;

### Matières
- glace translucide ;
- roche givrée ;
- cristaux bleus ;
- vapeur froide ;
- neige ancienne ;

## Gameplay
| Élément | Valeur |
|---|---|
| Rôle | Ralentissement / gel |
| Dangerosité | 5 |
| Taille | 1 case ou 2 cases |
| Comportement IA | SlowPressure / PuzzleLinked |
| Vitesse | Lente |
| Attaque | Contact glacial, éclat de glace |
| Effet spécial | Ralentit, gèle portes ou cases, éteint torches |
| Faiblesse | Feu, chaleur, choc thermique |
| Résistance | Froid, poison, saignement |
| Peut activer plaques | Oui |
| Peut ouvrir portes | Peut geler par événement |
| Loot | Cristal de glace, noyau froid |

## Utilisation en level design
- éteint torches utiles ;
- gèle une porte à déverrouiller ;
- rend certaines cases glissantes ;
- peut refroidir un mécanisme brûlant ;

## Animations nécessaires
- flottement lourd ;
- marche craquante ;
- attaque glacée ;
- gel de case ;
- fonte à la mort ;

## Sons nécessaires
- craquement de glace ;
- souffle froid ;
- cristaux qui tintent ;
- gel instantané ;

## VFX nécessaires
- givre au sol ;
- nuage froid ;
- éclats de glace ;
- extinction de torche ;

## Prompt concept art
Planche de bestiaire dark fantasy, élémentaire de glace, corps de glace et roche givrée, cristaux bleus, cœur froid lumineux, couloir gelé, torche éteinte, annotations françaises, parchemin artbook.

---

# FICHE 12 - Reine Araignée

## Nom technique
`MON_SpiderQueen`

## Catégorie
Boss / vermine majeure

## Description immersive
La Reine Araignée n’est pas seulement une version plus grande de l’araignée mineure. Elle est le centre d’un territoire. Ses cocons, ses toiles et ses rejetons composent une salle de boss où chaque case compte.

## Intention artistique
Très grande araignée, abdomen énorme, motifs de reine, crochets imposants, pattes longues, cocons autour, aura toxique.

### Silhouette
- grande masse basse ;
- abdomen dominant ;
- pattes largement écartées ;
- tête menaçante ;
- cocons visibles ;

### Matières
- chitine sombre ;
- soie épaisse ;
- venin vert ;
- œufs translucides ;
- poussière de crypte ;

## Gameplay
| Élément | Valeur |
|---|---|
| Rôle | Boss poison / invocation / contrôle de zone |
| Dangerosité | 6 |
| Taille | 2 cases ou boss 1 case large |
| Comportement IA | BossPhases / Summoner |
| Vitesse | Moyenne |
| Attaque | Morsure, poison, toiles, invocation |
| Effet spécial | Invoque araignées, crée toiles, sature la salle de poison |
| Faiblesse | Feu, destruction des cocons |
| Résistance | Poison, immobilisation |
| Peut activer plaques | Oui selon taille |
| Peut ouvrir portes | Non |
| Loot | Glande royale, soie rare, clé organique |

## Utilisation en level design
- boss de fin du Volume II ;
- salle avec cocons destructibles ;
- cases bloquées par toiles ;
- feu comme solution tactique ;

## Animations nécessaires
- idle reine ;
- morsure ;
- projection de toile ;
- ponte/invocation ;
- rage finale ;
- mort spectaculaire ;

## Sons nécessaires
- cri strident ;
- grattement de pattes ;
- toile tendue ;
- cocons qui se rompent ;

## VFX nécessaires
- nuage poison ;
- toiles au sol ;
- éclatement de cocon ;
- brûlure de soie ;

## Prompt concept art
Planche de bestiaire dark fantasy, Reine Araignée, boss arachnide immense dans un nid de donjon, cocons, toiles, venin, pattes longues, torches, ambiance toxique, annotations françaises, parchemin artbook premium.

---

# Annexes UE5

## DataAssets proposés

```text
DA_MON_GoblinRaider
DA_MON_GoblinThrower
DA_MON_Cultist
DA_MON_FloatingEye
DA_MON_MinorWraith
DA_MON_Ghoul
DA_MON_VenomSerpent
DA_MON_CaveWolf
DA_MON_AnimatedArmor
DA_MON_LesserHellhound
DA_MON_IceElemental
DA_MON_SpiderQueen
```

## Blueprints proposés

```text
BP_MON_GoblinRaider
BP_MON_GoblinThrower
BP_MON_Cultist
BP_MON_FloatingEye
BP_MON_MinorWraith
BP_MON_Ghoul
BP_MON_VenomSerpent
BP_MON_CaveWolf
BP_MON_AnimatedArmor
BP_MON_LesserHellhound
BP_MON_IceElemental
BP_MON_SpiderQueen
```

## Tags gameplay proposés

```text
Humanoid
Beast
Undead
Construct
Elemental
Poison
Fire
Cold
Holy
Magic
Alarm
Summon
Patrol
Flee
PhaseBoss
PuzzleLinked
CanOpenDoor
CanTriggerEvent
```

# Checklist de production

- Valider les 12 fiches en design ;
- Produire les 12 planches visuelles ;
- Définir les DataAssets `UGridMonsterDataAsset` ;
- Créer les premiers comportements IA : `PatrolSentinel`, `Caster`, `FleeAndCallHelp`, `BossPhases` ;
- Tester gobelins et cultistes dans une salle de prototype ;
- Tester alarme de l’Œil flottant ;
- Tester Reine Araignée comme boss à phases ;
- Préparer l’intégration progressive dans `docs/ArtBook`.