# BESTIAIRE DES PROFONDEURS
## Volume I — Créatures du premier donjon

**Projet :** GrimrockPrototype  
**Moteur :** Unreal Engine 5.5.4  
**Genre :** Dungeon crawler en vue subjective, déplacement case par case  
**Document :** Artbook / Bible artistique / Inventaire de production du bestiaire  
**Version :** 0.1 — Première base de travail

---

# Intention générale

Le **Bestiaire des Profondeurs** est conçu comme un ouvrage hybride :

1. **Un artbook immersif**, semblable à un carnet retrouvé dans les profondeurs d’un ancien donjon.
2. **Une bible artistique**, destinée à fixer la silhouette, les matières, les couleurs, les proportions et l’ambiance des créatures.
3. **Un document de game design**, permettant d’associer chaque monstre à un rôle clair dans un dungeon crawler case par case.
4. **Un document de production UE5**, indiquant les besoins en mesh, animation, sons, VFX, DataAssets et comportements IA.

L’objectif n’est pas de produire une encyclopédie décorative, mais un outil capable d’accompagner toute la chaîne de production :

```text
Concept visuel → Modèle 3D → Texture → Animation → Comportement IA → Intégration UE5 → Gameplay
```

---

# Ton artistique

L’univers visuel doit rester sobre, sombre et crédible.

Les monstres doivent sembler appartenir naturellement au donjon : ils sont nés des cryptes, des égouts, des anciennes salles de garde, des laboratoires abandonnés, des prisons oubliées ou des mécanismes magiques du lieu.

## Mots-clés visuels

- pierre humide ;
- torches vacillantes ;
- rouille ;
- poussière ;
- ossements ;
- moisissure ;
- bois ancien ;
- cuir usé ;
- métal oxydé ;
- chair blafarde ;
- magie ancienne ;
- obscurité lisible.

## Règle de lisibilité

Dans un dungeon crawler en vue subjective, chaque créature doit être reconnaissable immédiatement, même :

- à courte distance ;
- dans un couloir étroit ;
- sous une lumière faible ;
- partiellement cachée par une grille ou une porte ;
- en mouvement ;
- vue de face, de profil ou de trois-quarts.

La silhouette est donc prioritaire sur le détail.

---

# Format type d’une fiche de monstre

Chaque créature devrait idéalement être documentée sur une double page.

## Page gauche — Art

- Illustration principale ;
- silhouette noire ;
- vue de face ;
- vue de profil ;
- vue de dos ;
- détails anatomiques ;
- détails de texture ;
- variantes éventuelles ;
- notes manuscrites immersives.

## Page droite — Gameplay / Production

- Nom ;
- catégorie ;
- rôle tactique ;
- dangerosité ;
- taille sur la grille ;
- comportement IA ;
- attaques ;
- faiblesses ;
- résistances ;
- interaction avec les mécanismes ;
- loot ;
- animations nécessaires ;
- sons nécessaires ;
- VFX nécessaires ;
- assets UE5 associés.

---

# Classification du Volume I

Le premier volume se concentre sur les créatures utiles à un premier donjon complet.

```text
Volume I — Créatures du premier donjon
├── Vermines
│   ├── Rat géant
│   └── Araignée mineure
│
├── Matières vivantes
│   ├── Slime vert
│   └── Champignon toxique
│
├── Morts qui marchent
│   ├── Squelette guerrier
│   ├── Squelette archer
│   └── Zombie
│
├── Pièges vivants
│   ├── Mimique
│   └── Ver des cryptes
│
└── Gardiens
    ├── Gargouille
    ├── Golem de pierre
    └── Gardien de la Crypte
```

---

# Comportements IA de base

## DirectMelee

Le monstre cherche le chemin le plus court vers le joueur, avance case par case et attaque lorsqu’il est adjacent.

Utilisé pour : rat, squelette guerrier, zombie.

## FastHarasser

Le monstre se déplace rapidement, attaque, puis tente de se replacer. Il sert à stresser le joueur et à l’empêcher de rester immobile.

Utilisé pour : araignée, rat amélioré, loup futur.

## SlowPressure

Le monstre est lent mais dangereux s’il arrive au contact. Il contrôle le rythme du combat.

Utilisé pour : slime, zombie, golem.

## RangedKeeper

Le monstre tente de garder une distance minimale avec le joueur et attaque à distance.

Utilisé pour : squelette archer.

## Ambush

Le monstre est d’abord caché, inerte ou confondu avec le décor. Il s’active à l’approche, à l’interaction ou au déclenchement d’un événement.

Utilisé pour : mimique, gargouille, ver des cryptes.

## PuzzleLinked

Le monstre est lié à un événement du niveau. Sa mort, son activation ou sa position peut déclencher une porte, un levier, un piège ou une énigme.

Utilisé pour : golem, gardien, spectre futur, slime sur plaque.

---

# Échelle de dangerosité

| Niveau | Sens |
|---|---|
| 1 | Nuisible, très faible, idéal tutoriel |
| 2 | Faible mais dangereux en groupe |
| 3 | Ennemi standard du donjon |
| 4 | Ennemi spécialisé, demande une stratégie |
| 5 | Elite ou mini-boss |
| 6 | Boss majeur |

---

# FICHE 01 — Rat géant

## Nom technique

`MON_RatGiant`

## Catégorie

Vermine

## Description immersive

Le rat géant est l’une des premières horreurs que l’on entend avant même de la voir. Ses griffes raclent la pierre, son museau fouille les détritus et ses yeux reflètent la lumière des torches comme deux perles huileuses. Il ne représente pas une grande menace seul, mais il devient dangereux lorsqu’il surgit en nombre dans un couloir étroit.

## Intention artistique

Le rat géant doit rester crédible, sale et nerveux. Il ne doit pas ressembler à une créature fantastique exagérée, mais plutôt à un animal devenu trop gros dans un environnement malsain.

### Silhouette

- corps bas ;
- dos arqué ;
- queue longue ;
- tête triangulaire ;
- oreilles abîmées ;
- démarche rapide et saccadée.

### Matières

- pelage brun-noir humide ;
- plaques de poils manquants ;
- peau rose sale visible par endroits ;
- griffes jaunies ;
- dents trop longues ;
- yeux brillants.

## Gameplay

| Élément | Valeur |
|---|---|
| Rôle | Ennemi tutoriel / harceleur faible |
| Dangerosité | 1 |
| Taille | 1 case |
| Comportement IA | DirectMelee ou FastHarasser |
| Vitesse | Rapide |
| Attaque | Morsure |
| Effet spécial | Aucun en version de base |
| Faiblesse | Feu, armes tranchantes |
| Résistance | Aucune |
| Peut activer plaques | Oui |
| Peut ouvrir portes | Non |
| Loot | Viande de rat, dent, rien |

## Utilisation en level design

- Premier combat du jeu ;
- ennemi placé derrière une porte simple ;
- groupe de 2 ou 3 pour apprendre le recul ;
- peut être attiré par de la nourriture ;
- peut accidentellement maintenir une plaque enfoncée.

## Animations nécessaires

- Idle nerveux ;
- marche/course ;
- attaque morsure ;
- réaction aux dégâts ;
- mort ;
- optionnel : flairer le sol.

## Sons nécessaires

- couinement faible ;
- couinement agressif ;
- griffes sur pierre ;
- morsure ;
- cri de mort.

## Prompt concept art

Rat géant de donjon médiéval sombre, corps bas et nerveux, pelage brun noir humide, yeux brillants, dents jaunies, queue longue, griffes sales, lumière de torche, sol de pierre humide, style réaliste dark fantasy, silhouette lisible, vue trois-quarts.

---

# FICHE 02 — Araignée mineure

## Nom technique

`MON_MinorSpider`

## Catégorie

Vermine venimeuse

## Description immersive

L’araignée mineure tisse ses toiles dans les angles morts du donjon. Elle ne cherche pas toujours à tuer immédiatement : elle épuise, ralentit, empoisonne, puis revient lorsque la proie hésite. Dans les salles sombres, son véritable danger vient de sa mobilité et de sa capacité à forcer le joueur à gérer le poison.

## Intention artistique

Elle doit être inquiétante sans être gigantesque. C’est une créature basse, rapide, aux pattes longues, dont les mouvements sont secs et imprévisibles.

### Silhouette

- abdomen ovale ;
- pattes longues et anguleuses ;
- corps proche du sol ;
- mandibules visibles ;
- déplacement latéral possible.

### Matières

- carapace brun foncé ;
- poils fins ;
- reflets légèrement humides ;
- marques pâles sur l’abdomen ;
- crochets venimeux.

## Gameplay

| Élément | Valeur |
|---|---|
| Rôle | Poison / harcèlement |
| Dangerosité | 2 |
| Taille | 1 case |
| Comportement IA | FastHarasser |
| Vitesse | Rapide |
| Attaque | Morsure venimeuse |
| Effet spécial | Poison léger |
| Faiblesse | Feu |
| Résistance | Poison |
| Peut activer plaques | Oui |
| Peut ouvrir portes | Non |
| Loot | Glande à venin, soie, rien |

## Utilisation en level design

- Première introduction au poison ;
- salle avec antidote visible mais gardé ;
- couloir avec toiles ralentissantes ;
- nid déclenché par une plaque ou un levier.

## Animations nécessaires

- Idle avec mouvement de pattes ;
- marche rapide ;
- attaque morsure ;
- esquive latérale ;
- mort recroquevillée.

## Sons nécessaires

- petits frottements ;
- clics de mandibules ;
- attaque sèche ;
- cri aigu discret ;
- bruit de carapace.

## Prompt concept art

Araignée de donjon dark fantasy, taille d’un chien, pattes longues anguleuses, carapace brun foncé, crochets venimeux, abdomen marqué de taches pâles, ambiance de crypte humide, lumière de torche, style réaliste, silhouette basse et menaçante, vue trois-quarts.

---

# FICHE 03 — Slime vert

## Nom technique

`MON_GreenSlime`

## Catégorie

Matière vivante

## Description immersive

Le slime vert rampe dans les égouts anciens, les laboratoires oubliés et les salles où l’eau ne s’écoule plus. Il n’a ni yeux, ni os, ni volonté apparente. Pourtant, il avance. Lentement. Toujours. Sa masse gélatineuse absorbe la poussière, les insectes, les morceaux d’os et les fragments de métal tombés dans les profondeurs.

## Intention artistique

Le slime doit donner une impression de viscosité et de poids. Il n’est pas rapide, mais il occupe l’espace et rend le couloir menaçant.

### Silhouette

- masse basse ;
- forme irrégulière ;
- contour mouvant ;
- noyau interne plus sombre ;
- morceaux d’os visibles à l’intérieur.

### Matières

- gelée verte translucide ;
- reflets humides ;
- bulles internes ;
- filaments visqueux ;
- traînée au sol.

## Gameplay

| Élément | Valeur |
|---|---|
| Rôle | Contrôle de couloir / ennemi lent |
| Dangerosité | 2 |
| Taille | 1 case |
| Comportement IA | SlowPressure |
| Vitesse | Très lente |
| Attaque | Contact acide |
| Effet spécial | Peut laisser une flaque acide |
| Faiblesse | Feu |
| Résistance | Armes tranchantes, poison |
| Peut activer plaques | Oui |
| Peut ouvrir portes | Non |
| Loot | Gelée acide, résidu alchimique |

## Utilisation en level design

- Bloquer temporairement un passage ;
- apprendre au joueur que certains ennemis résistent aux armes classiques ;
- maintenir une plaque de pression ;
- créer un choix entre combat, feu ou contournement.

## Animations nécessaires

- ondulation idle ;
- déplacement rampant ;
- attaque par projection courte ;
- réaction au feu ;
- dissolution à la mort.

## Sons nécessaires

- succion humide ;
- bulles ;
- glissement visqueux ;
- grésillement acide ;
- éclatement mou.

## Prompt concept art

Slime vert de donjon, masse gélatineuse translucide, noyau sombre interne, fragments d’os et de métal suspendus dans la gelée, reflets humides, sol de pierre mouillé, lumière de torche, ambiance dark fantasy réaliste, silhouette basse et lisible.

---

# FICHE 04 — Squelette guerrier

## Nom technique

`MON_SkeletonWarrior`

## Catégorie

Mort-vivant

## Description immersive

Le squelette guerrier est le reste obstiné d’un soldat oublié. Il garde encore son poste longtemps après la chute de son royaume. Ses os craquent lorsqu’il avance, son arme rouillée se lève avec une lenteur mécanique, et ses orbites vides fixent l’intrus avec une fidélité absurde à un ordre disparu.

## Intention artistique

Il doit représenter l’ennemi classique du donjon : immédiatement identifiable, sobre, lisible, utile dans de nombreux contextes.

### Silhouette

- silhouette humaine maigre ;
- épée ou hache visible ;
- bouclier abîmé optionnel ;
- posture légèrement voûtée ;
- mouvements secs.

### Matières

- os jaunis ;
- métal rouillé ;
- lambeaux de tissu ;
- vieux cuir ;
- poussière accumulée.

## Gameplay

| Élément | Valeur |
|---|---|
| Rôle | Combattant standard |
| Dangerosité | 3 |
| Taille | 1 case |
| Comportement IA | DirectMelee |
| Vitesse | Moyenne |
| Attaque | Arme de mêlée |
| Effet spécial | Aucun en version de base |
| Faiblesse | Armes contondantes, lumière sacrée |
| Résistance | Poison, saignement |
| Peut activer plaques | Oui |
| Peut ouvrir portes | Non |
| Loot | Os, arme rouillée, bouclier brisé, clé rare |

## Utilisation en level design

- Ennemi de base des cryptes ;
- garde devant une porte ;
- groupe de deux dans une salle ;
- peut protéger une clé ou un levier ;
- bon test pour l’équilibrage du combat.

## Animations nécessaires

- idle debout ;
- marche ;
- attaque arme ;
- parade optionnelle ;
- réaction aux dégâts ;
- effondrement à la mort.

## Sons nécessaires

- cliquetis d’os ;
- raclement d’arme ;
- pas secs ;
- impact métallique ;
- effondrement osseux.

## Prompt concept art

Squelette guerrier médiéval dark fantasy, os jaunis, épée rouillée, bouclier abîmé, lambeaux de tissu, posture menaçante, crypte humide, lumière de torche, style réaliste sombre, silhouette très lisible, vue trois-quarts.

---

# FICHE 05 — Squelette archer

## Nom technique

`MON_SkeletonArcher`

## Catégorie

Mort-vivant à distance

## Description immersive

Le squelette archer ne charge pas. Il attend, recule, vise et tire. Il force l’intrus à traverser les couloirs, à fermer les portes, à utiliser les angles morts et à comprendre que dans les profondeurs, la distance peut tuer aussi sûrement qu’une lame.

## Intention artistique

Il doit être différencié visuellement du squelette guerrier dès le premier regard.

### Silhouette

- arc visible ;
- carquois ;
- posture plus fine ;
- bras levé pour viser ;
- peu ou pas de bouclier ;
- silhouette verticale et nerveuse.

### Matières

- os secs ;
- arc ancien en bois sombre ;
- corde usée ;
- flèches rouillées ou osseuses ;
- restes de capuche.

## Gameplay

| Élément | Valeur |
|---|---|
| Rôle | Ennemi à distance |
| Dangerosité | 3 |
| Taille | 1 case |
| Comportement IA | RangedKeeper |
| Vitesse | Moyenne |
| Attaque | Flèche |
| Effet spécial | Peut viser à travers grille ouverte |
| Faiblesse | Armes contondantes, lumière sacrée |
| Résistance | Poison, saignement |
| Peut activer plaques | Oui |
| Peut ouvrir portes | Non |
| Loot | Flèches, arc usé, os |

## Utilisation en level design

- Couloir long ;
- salle avec piliers ;
- derrière une grille ;
- oblige à utiliser les portes comme couverture ;
- introduit les projectiles ennemis.

## Animations nécessaires

- idle arc baissé ;
- viser ;
- tirer ;
- reculer ;
- réaction aux dégâts ;
- mort.

## Sons nécessaires

- corde d’arc ;
- flèche sifflante ;
- cliquetis osseux ;
- impact de projectile ;
- effondrement.

## Prompt concept art

Squelette archer de crypte médiévale, arc ancien, carquois de flèches rouillées, os secs, capuche déchirée, posture de tir, couloir de pierre sombre, lumière de torche, style réaliste dark fantasy, silhouette lisible.

---

# FICHE 06 — Zombie

## Nom technique

`MON_Zombie`

## Catégorie

Mort-vivant charnel

## Description immersive

Le zombie n’est pas rapide. Il n’est pas intelligent. Il n’a pas besoin de l’être. Son danger vient de son obstination, de sa masse et de sa capacité à transformer un simple couloir en piège. Tant qu’il avance, le joueur doit choisir : reculer, combattre ou fermer la porte.

## Intention artistique

Le zombie doit être lourd, abîmé, presque silencieux, moins spectaculaire qu’un démon mais plus dérangeant qu’un squelette.

### Silhouette

- corps humain massif ;
- épaules tombantes ;
- tête penchée ;
- bras pendants ;
- démarche lente ;
- ventre ou cage thoracique abîmée.

### Matières

- peau grisâtre ;
- chair nécrosée ;
- vêtements déchirés ;
- sang sombre séché ;
- plaies anciennes.

## Gameplay

| Élément | Valeur |
|---|---|
| Rôle | Sac à PV / pression lente |
| Dangerosité | 3 |
| Taille | 1 case |
| Comportement IA | SlowPressure |
| Vitesse | Lente |
| Attaque | Coup lourd / morsure |
| Effet spécial | Peut infecter en version avancée |
| Faiblesse | Feu, lumière sacrée |
| Résistance | Poison, saignement |
| Peut activer plaques | Oui |
| Peut ouvrir portes | Non |
| Loot | Tissu souillé, pièce ancienne, rien |

## Utilisation en level design

- Couloir étroit ;
- salle où le joueur doit maintenir la distance ;
- ennemi enfermé derrière une grille ;
- menace lente utilisée avec un piège ou une porte temporisée.

## Animations nécessaires

- idle lourd ;
- marche lente ;
- coup de bras ;
- morsure ;
- réaction aux dégâts ;
- chute lourde.

## Sons nécessaires

- râle bas ;
- pas traînants ;
- respiration morte ;
- impact de chair ;
- chute molle.

## Prompt concept art

Zombie de donjon médiéval dark fantasy, corps lourd, peau grisâtre nécrosée, vêtements déchirés, posture voûtée, bras pendants, expression vide, lumière de torche, couloir de pierre humide, style réaliste sombre.

---

# FICHE 07 — Mimique

## Nom technique

`MON_MimicChest`

## Catégorie

Piège vivant

## Description immersive

La mimique imite les coffres anciens, les caisses et parfois même les petits autels. Elle demeure immobile durant des années, nourrie par la poussière et les insectes, jusqu’à ce qu’une main imprudente soulève son couvercle. Alors le bois se fend, les ferrures se tordent, et l’intérieur révèle des dents qui n’auraient jamais dû exister.

## Intention artistique

Elle doit d’abord être crédible comme coffre. Le joueur ne doit comprendre qu’après coup que quelque chose n’était pas normal.

### Silhouette

- coffre médiéval ;
- couvercle entrouvert lors de l’attaque ;
- dents visibles ;
- langue ou chair interne ;
- ferrures déformées ;
- petites pattes ou racines discrètes.

### Matières

- bois ancien ;
- métal oxydé ;
- intérieur organique rouge sombre ;
- salive brillante ;
- dents irrégulières.

## Gameplay

| Élément | Valeur |
|---|---|
| Rôle | Embuscade / punition de l’avidité |
| Dangerosité | 4 |
| Taille | 1 case |
| Comportement IA | Ambush |
| Vitesse | Faible à moyenne après activation |
| Attaque | Morsure violente |
| Effet spécial | Première attaque renforcée |
| Faiblesse | Feu, armes lourdes |
| Résistance | Poison |
| Peut activer plaques | Oui après activation |
| Peut ouvrir portes | Non |
| Loot | Objet rare, or, ou rien selon intention |

## Utilisation en level design

- Salle au trésor ;
- coffre isolé trop évident ;
- plusieurs coffres dont un seul est vivant ;
- peut garder une clé importante ;
- excellent monstre pour créer la méfiance.

## Animations nécessaires

- coffre immobile ;
- réveil brutal ;
- ouverture gueule ;
- morsure ;
- déplacement sautillé ou rampant ;
- mort avec affaissement.

## Sons nécessaires

- vieux bois qui craque ;
- ferrures qui grincent ;
- claquement de mâchoire ;
- salive ;
- grognement sourd.

## Prompt concept art

Mimique de coffre médiéval dark fantasy, vieux coffre en bois sombre avec ferrures rouillées, couvercle ouvert révélant dents irrégulières, chair rouge sombre et langue épaisse, salive brillante, ambiance de salle au trésor de donjon, lumière de torche, style réaliste sombre.

---

# FICHE 08 — Ver des cryptes

## Nom technique

`MON_CryptWorm`

## Catégorie

Bête de donjon / embuscade

## Description immersive

Le ver des cryptes vit sous les dalles, derrière les murs friables et dans les fosses anciennes. Il sent les vibrations des pas bien avant que l’aventurier ne voie sa gueule. Il n’est pas un chasseur noble : il surgit, mord, disparaît parfois, puis revient lorsque la panique a fait son œuvre.

## Intention artistique

Il doit évoquer une créature aveugle, souterraine, adaptée à la pierre et à la chair morte.

### Silhouette

- corps allongé ;
- absence d’yeux ;
- gueule circulaire ;
- anneaux de chair ;
- petites pointes ou plaques dures ;
- mouvement ondulant.

### Matières

- peau pâle gris-rose ;
- mucus ;
- plaques cornées ;
- dents circulaires ;
- poussière collée au corps.

## Gameplay

| Élément | Valeur |
|---|---|
| Rôle | Embuscade / surprise |
| Dangerosité | 3 |
| Taille | 1 case |
| Comportement IA | Ambush |
| Vitesse | Moyenne |
| Attaque | Morsure |
| Effet spécial | Peut surgir depuis une case marquée |
| Faiblesse | Feu, froid |
| Résistance | Poison léger |
| Peut activer plaques | Non ou oui selon taille |
| Peut ouvrir portes | Non |
| Loot | Dent de ver, mucus, rien |

## Utilisation en level design

- Apparition après prise d’un objet ;
- surgissement dans un couloir silencieux ;
- salle avec dalles suspectes ;
- tutoriel pour apprendre à lire les indices visuels au sol.

## Animations nécessaires

- caché/invisible ;
- surgissement ;
- attaque morsure ;
- déplacement rampant ;
- replongée optionnelle ;
- mort.

## Sons nécessaires

- grattement sous la pierre ;
- dalle qui tremble ;
- surgissement humide ;
- morsure ;
- cri étranglé.

## Prompt concept art

Ver des cryptes dark fantasy, créature souterraine aveugle, corps annelé gris rosé, gueule circulaire remplie de dents, mucus, poussière de pierre, surgissant d’un sol de crypte fissuré, lumière de torche, style réaliste sombre.

---

# FICHE 09 — Gargouille

## Nom technique

`MON_Gargoyle`

## Catégorie

Construct / gardien dormant

## Description immersive

La gargouille est d’abord une statue. Elle patiente dans une niche, au-dessus d’une porte, près d’un autel ou à l’angle d’un couloir. Le joueur peut passer devant elle une fois, deux fois, sans incident. Puis un levier est tiré, une clé est prise, un sceau est brisé, et la pierre tourne lentement la tête.

## Intention artistique

La gargouille doit être à la frontière entre décor et créature. Elle doit fonctionner comme élément architectural avant d’être un ennemi.

### Silhouette

- posture accroupie ;
- ailes repliées ;
- griffes longues ;
- tête cornue ;
- dos voûté ;
- forme compacte.

### Matières

- pierre grise ;
- fissures ;
- mousse ;
- poussière ;
- yeux légèrement lumineux après activation ;
- éclats de pierre lors des dégâts.

## Gameplay

| Élément | Valeur |
|---|---|
| Rôle | Embuscade / gardien |
| Dangerosité | 4 |
| Taille | 1 case |
| Comportement IA | Ambush puis DirectMelee |
| Vitesse | Moyenne |
| Attaque | Griffes / coup de pierre |
| Effet spécial | Dégâts réduits tant qu’elle est immobile |
| Faiblesse | Marteaux, magie, lumière |
| Résistance | Poison, saignement, tranchant |
| Peut activer plaques | Oui |
| Peut ouvrir portes | Non |
| Loot | Fragment de pierre, œil de gargouille |

## Utilisation en level design

- Statue décorative qui s’active ;
- gardienne d’un autel ;
- piège après prise d’une clé ;
- activation par événement ;
- excellente pour réutiliser un asset décoratif en ennemi.

## Animations nécessaires

- statue immobile ;
- réveil lent ;
- rotation de tête ;
- déploiement partiel des ailes ;
- attaque griffes ;
- mort en éclats.

## Sons nécessaires

- pierre qui craque ;
- frottement minéral ;
- grognement grave ;
- impact rocheux ;
- effondrement de pierre.

## Prompt concept art

Gargouille de donjon gothique dark fantasy, statue de pierre grise accroupie, ailes repliées, griffes longues, tête cornue, fissures et mousse, yeux faiblement lumineux, lumière de torche, style réaliste sombre, mi-statue mi-créature.

---

# FICHE 10 — Golem de pierre

## Nom technique

`MON_StoneGolem`

## Catégorie

Construct lourd

## Description immersive

Le golem de pierre ne vit pas. Il obéit. Sa masse se met en mouvement lorsque le sceau qui le retient reconnaît une intrusion. Chaque pas résonne dans les murs. Chaque coup ressemble à la chute d’un bloc. Il n’est pas rapide, mais dans un couloir, il n’a pas besoin de l’être.

## Intention artistique

Le golem doit être massif, ancien, presque architectural. Il doit sembler sculpté dans la même pierre que le donjon.

### Silhouette

- épaules très larges ;
- bras lourds ;
- tête petite ou encastrée ;
- jambes épaisses ;
- démarche pesante ;
- centre de gravité bas.

### Matières

- pierre grise ;
- gravures anciennes ;
- fissures lumineuses optionnelles ;
- poussière ;
- mousse ;
- éclats sur les articulations.

## Gameplay

| Élément | Valeur |
|---|---|
| Rôle | Tank / puzzle de combat |
| Dangerosité | 5 |
| Taille | 1 case ou 2 cases selon version |
| Comportement IA | SlowPressure / PuzzleLinked |
| Vitesse | Très lente |
| Attaque | Coup lourd |
| Effet spécial | Peut repousser le joueur d’une case |
| Faiblesse | Marteaux, magie de foudre, désactivation par sceau |
| Résistance | Poison, saignement, tranchant, feu |
| Peut activer plaques | Oui |
| Peut ouvrir portes | Non, sauf version spéciale |
| Loot | Fragment de golem, rune, cristal |

## Utilisation en level design

- Mini-boss ;
- gardien d’une porte scellée ;
- ennemi à enfermer plutôt qu’à tuer ;
- peut être désactivé par levier ;
- peut être attiré sur une plaque lourde.

## Animations nécessaires

- idle presque immobile ;
- réveil ;
- marche lourde ;
- attaque coup de poing ;
- attaque écrasement ;
- réaction minérale ;
- effondrement.

## Sons nécessaires

- pierre qui bouge ;
- pas lourds ;
- grondement grave ;
- impact rocheux ;
- fissure ;
- effondrement massif.

## Prompt concept art

Golem de pierre dark fantasy, construct massif sculpté dans la pierre du donjon, épaules larges, bras lourds, gravures anciennes, fissures lumineuses discrètes, mousse et poussière, couloir de pierre, lumière de torche, style réaliste sombre, silhouette imposante.

---

# FICHE 11 — Gardien de la Crypte

## Nom technique

`MON_CryptGuardian`

## Catégorie

Boss / mort-vivant elite

## Description immersive

Le Gardien de la Crypte fut peut-être un chevalier, un geôlier ou un prêtre de guerre. Il ne reste de lui qu’une armure funéraire, un serment et une force qui refuse de mourir. Il ne protège pas un trésor par cupidité, mais parce qu’une loi ancienne lui interdit de laisser passer les vivants.

## Intention artistique

Il doit être plus noble et plus menaçant qu’un simple squelette. Il représente le premier vrai boss du donjon.

### Silhouette

- humanoïde grand ;
- armure lourde abîmée ;
- arme massive ;
- casque ou couronne funéraire ;
- cape déchirée ;
- aura froide ou poussiéreuse.

### Matières

- os anciens ;
- métal noirci ;
- bronze oxydé ;
- tissu funéraire ;
- lumière bleutée ou verdâtre dans les orbites ;
- poussière sépulcrale.

## Gameplay

| Élément | Valeur |
|---|---|
| Rôle | Boss de fin de premier donjon |
| Dangerosité | 6 |
| Taille | 1 case, éventuellement 2 pour boss |
| Comportement IA | DirectMelee + phases |
| Vitesse | Moyenne à lente |
| Attaque | Arme lourde |
| Effet spécial | Peut invoquer des squelettes ou verrouiller des portes |
| Faiblesse | Lumière sacrée, masses, mécanisme de salle |
| Résistance | Poison, saignement, froid |
| Peut activer plaques | Oui |
| Peut ouvrir portes | Oui si scripté |
| Loot | Clé majeure, fragment de sceau, arme ancienne |

## Utilisation en level design

- Boss final du premier niveau ;
- salle circulaire ou carrée avec piliers ;
- leviers permettant d’affaiblir ses protections ;
- urnes funéraires à détruire ;
- portes qui se ferment au début du combat ;
- récompense : accès au niveau suivant.

## Phases de combat proposées

### Phase 1 — Le serment

Le gardien avance lentement et attaque au contact.

### Phase 2 — Les morts répondent

À 60 % de vie, il invoque deux squelettes faibles ou active deux alcôves.

### Phase 3 — La crypte se referme

À 30 % de vie, certaines portes se ferment ou des pièges s’activent dans la salle.

## Animations nécessaires

- idle boss ;
- réveil depuis position agenouillée ou sarcophage ;
- marche lourde ;
- attaque horizontale ;
- attaque verticale ;
- invocation ;
- réaction aux dégâts ;
- mort dramatique.

## Sons nécessaires

- souffle spectral ;
- armure lourde ;
- voix grave ancienne ;
- arme raclant le sol ;
- impact massif ;
- explosion de poussière à la mort.

## Prompt concept art

Gardien de la Crypte dark fantasy, boss mort-vivant en armure funéraire lourde, os anciens, métal noirci et bronze oxydé, casque menaçant, cape déchirée, arme massive, orbites lumineuses froides, salle de crypte avec torches, style réaliste sombre, silhouette noble et terrifiante.

---

# Tableau synthétique du Volume I

| Monstre | Rôle | IA | Danger | Mécanique principale |
|---|---|---|---:|---|
| Rat géant | Tutoriel | DirectMelee | 1 | Mouvement rapide simple |
| Araignée mineure | Poison | FastHarasser | 2 | Poison léger |
| Slime vert | Contrôle | SlowPressure | 2 | Résistance / plaque |
| Squelette guerrier | Standard | DirectMelee | 3 | Combat de base |
| Squelette archer | Distance | RangedKeeper | 3 | Projectile |
| Zombie | Pression lente | SlowPressure | 3 | Sac à PV |
| Mimique | Embuscade | Ambush | 4 | Faux coffre |
| Ver des cryptes | Surprise | Ambush | 3 | Surgissement |
| Gargouille | Gardien dormant | Ambush | 4 | Statue animée |
| Golem de pierre | Tank | PuzzleLinked | 5 | Ennemi lourd / plaque |
| Gardien de la Crypte | Boss | Phases | 6 | Combat scénarisé |

---

# Proposition de nomenclature UE5

## Classes C++ futures possibles

```text
AGridMonsterActor
AGridMonsterSpawnActor
UGridMonsterDataAsset
UGridMonsterBehaviorComponent
UGridMonsterCombatComponent
UGridMonsterPerceptionComponent
```

## DataAssets

```text
DA_MON_RatGiant
DA_MON_MinorSpider
DA_MON_GreenSlime
DA_MON_SkeletonWarrior
DA_MON_SkeletonArcher
DA_MON_Zombie
DA_MON_MimicChest
DA_MON_CryptWorm
DA_MON_Gargoyle
DA_MON_StoneGolem
DA_MON_CryptGuardian
```

## Blueprints

```text
BP_MON_RatGiant
BP_MON_MinorSpider
BP_MON_GreenSlime
BP_MON_SkeletonWarrior
BP_MON_SkeletonArcher
BP_MON_Zombie
BP_MON_MimicChest
BP_MON_CryptWorm
BP_MON_Gargoyle
BP_MON_StoneGolem
BP_MON_CryptGuardian
```

## Meshes

```text
SK_RatGiant
SK_MinorSpider
SK_SkeletonWarrior
SK_SkeletonArcher
SK_Zombie
SK_MimicChest
SK_CryptWorm
SK_Gargoyle
SK_StoneGolem
SK_CryptGuardian
```

Pour le slime, un Static Mesh animé par shader ou morph target peut suffire au départ :

```text
SM_GreenSlime
```

---

# Tags gameplay proposés

```text
Monster.Vermin
Monster.Undead
Monster.Construct
Monster.Beast
Monster.Ambush
Monster.Boss

Damage.Fire
Damage.Cold
Damage.Poison
Damage.Blunt
Damage.Slashing
Damage.Piercing
Damage.Holy
Damage.Acid

State.Poisoned
State.Burning
State.Stunned
State.Rooted
State.Dormant

Interaction.CanTriggerPressurePlate
Interaction.CanOpenDoor
Interaction.CanBeLuredByFood
Interaction.LinkedToPuzzle
```

---

# Conclusion de version 0.1

Ce premier volume donne une base cohérente pour créer le bestiaire initial de GrimrockPrototype.

Il définit :

- une direction artistique sombre et lisible ;
- une logique de classification ;
- onze créatures adaptées au premier donjon ;
- des rôles tactiques distincts ;
- des comportements IA simples mais réutilisables ;
- des pistes concrètes pour les assets UE5 ;
- des prompts de concept art pour produire les premières images.

La prochaine étape naturelle consiste à produire les premières planches visuelles, en commençant par :

1. Rat géant ;
2. Squelette guerrier ;
3. Slime vert ;
4. Mimique ;
5. Gardien de la Crypte.
