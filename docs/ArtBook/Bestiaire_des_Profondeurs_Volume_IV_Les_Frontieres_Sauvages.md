# BESTIAIRE DES PROFONDEURS
## Volume IV - Les Frontières Sauvages

**Projet :** GrimrockPrototype
**Moteur :** Unreal Engine 5.5.4
**Genre :** Dungeon crawler en vue subjective, déplacement case par case
**Document :** Artbook / Bible artistique / Document de conception du bestiaire
**Version :** 0.1 - Base de conception

---
# Intention du Volume IV

Le Volume IV quitte volontairement les couloirs souterrains. Les Frontières Sauvages montrent que le système de grille fonctionne aussi en extérieur : landes, forêts, marais, ruines à ciel ouvert, littoraux et temples noyés. Ce volume introduit des créatures dont le danger dépend moins des murs que du biome, de la lumière, de la végétation, de l’eau, du vent, des hauteurs et des passages naturels.

# Biomes et états de case

Les créatures du Volume IV exploitent des cases naturelles : eau peu profonde, eau profonde, boue, hautes herbes, brouillard, racines, végétation inflammable, courant, marée et perchoirs. Le niveau n’est plus seulement un assemblage de cellules minérales : il devient un paysage jouable.

# États de grille proposés

```text
GridState.ShallowWater
GridState.DeepWater
GridState.Mud
GridState.TallGrass
GridState.Rooted
GridState.Fog
GridState.BurningVegetation
GridState.Current
GridState.Tide
GridState.Perch
```

# Nouvelles mécaniques

Patrouille extérieure, signal d’alarme, perchoirs, charge dans la végétation, contrôle par racines, nids destructibles, leurres lumineux, camouflage amphibie, embuscade aquatique, déplacement latéral, chant de confusion, courants et boss multi-têtes.

# Profils IA et tags

Profils IA : OutdoorPatrol, SignalCaller, PerchStriker, VegetationCharger, RootController, NestDefender, LureBeacon, AmphibiousSkirmisher, WaterAmbush, SideStepTank, SongController, CurrentShaper, MultiHeadBoss. Tags : Outdoor, Forest, Marsh, Coastal, Water, Fog, Rooted, TallGrass, Perch, Airborne, Tide, Current, BossPhase.

# Classification du Volume IV

Landes et ruines : Bandit éclaireur, Harpie des falaises, Sanglier des landes. Forêts : Sylvain épineux, Guêpe géante. Marais : Feu follet, Homme-lézard des marais, Crocodile géant. Littoral : Crabe des récifs, Sirène des brumes, Élémentaire d’eau. Boss : Hydre des marais.

# Un volume pensé pour les extérieurs

Ce volume prépare l’ouverture du prototype vers des zones non souterraines. Il exige des assets de terrain lisibles sur grille : roseaux, hautes herbes, eau basse, rochers, perchoirs, souches, pontons, palissades, récifs et bassins. Chaque créature doit rester compatible avec la vue subjective et le déplacement case par case.

# Tableau synthétique

| N° | Créature | Biome | Rôle | IA | Danger |
|---:|---|---|---|---|---:|
| 01 | Bandit éclaireur | Landes / ruines extérieures | patrouille / embuscade / signal | OutdoorPatrol / SignalCaller | 2 |
| 02 | Harpie des falaises | Falaises / ruines hautes | perchoirs / attaques aériennes | PerchStriker | 3 |
| 03 | Sanglier des landes | Landes / forêts claires | charge / destruction légère | VegetationCharger | 3 |
| 04 | Sylvain épineux | Forêt ancienne | racines / blocage / camouflage | RootController / LightSensitive | 4 |
| 05 | Guêpe géante | Forêt / clairière | vol / venin / nid | NestDefender / FastHarasser | 3 |
| 06 | Feu follet | Marais / brume | leurre / brouillard / piège naturel | LureBeacon | 3 |
| 07 | Homme-lézard des marais | Marais / roseaux | combat amphibie / camouflage | AmphibiousSkirmisher | 4 |
| 08 | Crocodile géant | Marais / rivière lente | embuscade depuis l’eau | WaterAmbush | 4 |
| 09 | Crabe des récifs | Littoral / récifs | blindage frontal / déplacement latéral | SideStepTank | 3 |
| 10 | Sirène des brumes | Côte / lagune / brume | chant / confusion / brouillard | SongController | 5 |
| 11 | Élémentaire d’eau | Côte / temple noyé | courants / changement de forme | CurrentShaper | 5 |
| 12 | Hydre des marais | Marais / lac noir | boss multi-têtes / poison / eau | MultiHeadBoss | 6 |

---
# FICHE 01 - Bandit éclaireur

> **Planche visuelle à produire**

## Nom technique
`MON_BanditScout`

## Catégorie
Humanoïde extérieur

## Biome principal
Landes / ruines extérieures

## Description immersive
Le bandit éclaireur est le premier adversaire véritablement extérieur. Il ne garde pas un couloir, mais une zone ouverte : ruine, lande, pont effondré ou campement. Il observe, recule, alerte et tente d’attirer le groupe vers une position préparée.

## Intention artistique
Silhouette humaine légère, capuchon, manteau usé, arc court ou couteau, corne d’alerte, fanion, équipement de route et cuir poussiéreux.

## Gameplay

| Élément | Valeur |
|---|---|
| Rôle | patrouille / embuscade / signal |
| Dangerosité | 2 |
| Comportement IA | OutdoorPatrol / SignalCaller |
| Tags | Humanoid, Outdoor, Alarm, Patrol |
| Interaction de grille | ligne de vue longue, perchoir bas, signal d’alarme |
| Faiblesse | contact direct, peur, isolement |
| Résistance | aucune |
| Loot | flèche, corne d’alerte, ration sèche, pièce volée |

## Utilisation en level design
Patrouille entre plusieurs points, donne l’alarme, attire vers une embuscade, fuit vers un feu de signal ou une porte extérieure.

## Prompt concept art
Planche de bestiaire dark fantasy, bandit éclaireur dans des ruines extérieures, capuche, cuir usé, corne d’alarme, arc court, lande au crépuscule, annotations françaises, parchemin artbook.

---
# FICHE 02 - Harpie des falaises

> **Planche visuelle à produire**

## Nom technique
`MON_CliffHarpy`

## Catégorie
Hybride ailée

## Biome principal
Falaises / ruines hautes

## Description immersive
La harpie des falaises occupe les hauteurs. Elle force le joueur à penser verticalement même sur une grille : elle se pose, crie, fond sur une cible isolée puis regagne un perchoir.

## Intention artistique
Corps maigre, grandes ailes sombres, serres, visage de prédateur, plumes usées, posture perchée sur colonne ou rocher.

## Gameplay

| Élément | Valeur |
|---|---|
| Rôle | perchoirs / attaques aériennes |
| Dangerosité | 3 |
| Comportement IA | PerchStriker |
| Tags | Beast, Airborne, Perch, Fear |
| Interaction de grille | ignore certains obstacles bas, se pose sur des cases-perchoirs |
| Faiblesse | projectiles, feu, lumière vive |
| Résistance | chute, terrain difficile |
| Loot | plume sombre, serre, membrane d’aile |

## Utilisation en level design
Attaque depuis des perchoirs définis, ignore les plaques au sol, peut crier pour provoquer peur ou désorientation.

## Prompt concept art
Planche de bestiaire dark fantasy, harpie des falaises perchée sur ruines battues par le vent, ailes sombres, serres, falaise, annotations françaises, parchemin.

---
# FICHE 03 - Sanglier des landes

> **Planche visuelle à produire**

## Nom technique
`MON_MoorBoar`

## Catégorie
Bête de charge

## Biome principal
Landes / forêts claires

## Description immersive
Le sanglier des landes transforme l’espace ouvert en menace dynamique. Sa charge est lisible, puissante et exploitable : le joueur peut l’éviter, le piéger ou le pousser à briser un obstacle naturel.

## Intention artistique
Animal massif, poils noirs et bruns, défenses longues, boue sèche, cicatrices naturelles, silhouette basse et frontale.

## Gameplay

| Élément | Valeur |
|---|---|
| Rôle | charge / destruction légère |
| Dangerosité | 3 |
| Comportement IA | VegetationCharger |
| Tags | Beast, Charge, Vegetation, Breakable |
| Interaction de grille | charge rectiligne, brise clôtures ou broussailles légères |
| Faiblesse | esquive latérale, feu, terrain glissant |
| Résistance | peur légère, broussailles |
| Loot | défense, cuir épais, viande |

## Utilisation en level design
Charge en ligne, traverse hautes herbes, peut dégager un passage ou percuter un obstacle.

## Prompt concept art
Planche de bestiaire, sanglier des landes massif, charge dans hautes herbes, défenses, boue, ruines extérieures, annotations françaises.

---
# FICHE 04 - Sylvain épineux

> **Planche visuelle à produire**

## Nom technique
`MON_ThornSylvan`

## Catégorie
Végétal animé

## Biome principal
Forêt ancienne

## Description immersive
Le sylvain épineux est un gardien végétal. Il ne court pas après le joueur : il transforme la forêt en piège, ferme les passages, accroche les jambes et protège les clairières anciennes.

## Intention artistique
Corps d’écorce noueuse, branches en forme de bras, épines, mousse, feuillage sombre, yeux luisants sous le bois.

## Gameplay

| Élément | Valeur |
|---|---|
| Rôle | racines / blocage / camouflage |
| Dangerosité | 4 |
| Comportement IA | RootController / LightSensitive |
| Tags | Plant, Rooted, Forest, FireWeak |
| Interaction de grille | bloque une case, anime racines, se confond avec arbres |
| Faiblesse | feu, hache, lumière solaire directe |
| Résistance | poison, peur, froid léger |
| Loot | épine vive, fragment d’écorce, graine ancienne |

## Utilisation en level design
Crée des cases Rooted, bloque le recul, camoufle sa présence près d’un arbre, vulnérable au feu.

## Prompt concept art
Planche de bestiaire dark fantasy, sylvain épineux, gardien végétal de forêt, racines animées, écorce, mousse, annotations françaises.

---
# FICHE 05 - Guêpe géante

> **Planche visuelle à produire**

## Nom technique
`MON_GiantWasp`

## Catégorie
Insecte volant

## Biome principal
Forêt / clairière

## Description immersive
La guêpe géante est une menace rapide liée à un territoire. Tant que son nid subsiste, le joueur ne traite pas seulement un monstre, mais une source continue de pression.

## Intention artistique
Insecte allongé, abdomen rayé, ailes translucides, dard, pattes fines, nid de fibres végétales suspendu.

## Gameplay

| Élément | Valeur |
|---|---|
| Rôle | vol / venin / nid |
| Dangerosité | 3 |
| Comportement IA | NestDefender / FastHarasser |
| Tags | Beast, Flying, Poison, Nest |
| Interaction de grille | attaque depuis case adjacente, dépend d’un nid destructible |
| Faiblesse | feu, fumée, froid |
| Résistance | poison, terrain difficile |
| Loot | dard, aile translucide, venin faible |

## Utilisation en level design
Pique, empoisonne, recule, défend le nid ; destruction du nid empêche la réapparition.

## Prompt concept art
Planche de bestiaire, guêpe géante de forêt, dard, ailes translucides, nid suspendu, gameplay poison, annotations françaises.

---
# FICHE 06 - Feu follet

> **Planche visuelle à produire**

## Nom technique
`MON_WillOWisp`

## Catégorie
Esprit lumineux

## Biome principal
Marais / brume

## Description immersive
Le feu follet est moins un adversaire qu’une mauvaise direction. Il recule dans la brume, semble guider le joueur, puis l’attire vers une case instable, une eau profonde ou un autre danger.

## Intention artistique
Petite lumière bleutée ou verte, halo dans la brume, forme presque humaine à peine visible, reflets sur eau noire.

## Gameplay

| Élément | Valeur |
|---|---|
| Rôle | leurre / brouillard / piège naturel |
| Dangerosité | 3 |
| Comportement IA | LureBeacon |
| Tags | Spirit, Fog, Light, Lure |
| Interaction de grille | attire vers boue, eau profonde ou prédateur caché |
| Faiblesse | lumière sacrée, vent fort, cloche rituelle |
| Résistance | armes physiques, poison |
| Loot | étincelle éthérée, essence de brume |

## Utilisation en level design
Se déplace hors de portée, crée des leurres visuels, augmente la confusion dans les cases Fog.

## Prompt concept art
Planche de bestiaire, feu follet de marais, lumière bleue dans la brume, eau sombre, leurre, annotations françaises.

---
# FICHE 07 - Homme-lézard des marais

> **Planche visuelle à produire**

## Nom technique
`MON_MarshLizardman`

## Catégorie
Humanoïde amphibie

## Biome principal
Marais / roseaux

## Description immersive
L’homme-lézard des marais est un guerrier de terrain. Il connaît les roseaux, les eaux basses et les sols mous. Là où le joueur ralentit, lui accélère.

## Intention artistique
Humanoïde reptilien, peau sombre et humide, lance courte, bouclier de bois, roseaux sur l’armure, yeux latéraux.

## Gameplay

| Élément | Valeur |
|---|---|
| Rôle | combat amphibie / camouflage |
| Dangerosité | 4 |
| Comportement IA | AmphibiousSkirmisher |
| Tags | Humanoid, Amphibious, Marsh, Camouflage |
| Interaction de grille | se déplace librement dans l’eau peu profonde et les roseaux |
| Faiblesse | froid, terrain sec, lumière vive |
| Résistance | poison léger, eau, boue |
| Loot | lance courte, écaille de marais, amulette tribale |

## Utilisation en level design
Surgit d’une case marécageuse, se replie dans l’eau, utilise roseaux comme couverture.

## Prompt concept art
Planche de bestiaire, homme-lézard des marais, lance, roseaux, eau peu profonde, camouflage, annotations françaises.

---
# FICHE 08 - Crocodile géant

> **Planche visuelle à produire**

## Nom technique
`MON_GiantCrocodile`

## Catégorie
Prédateur aquatique

## Biome principal
Marais / rivière lente

## Description immersive
Le crocodile géant enseigne au joueur à lire l’eau. Remous, bulles, roseaux déplacés : tout indique une menace avant que la morsure ne surgisse.

## Intention artistique
Grand reptile bas, cuir sombre, yeux au ras de l’eau, mâchoire large, dos couvert de vase et plantes.

## Gameplay

| Élément | Valeur |
|---|---|
| Rôle | embuscade depuis l’eau |
| Dangerosité | 4 |
| Comportement IA | WaterAmbush |
| Tags | Beast, Water, Ambush, Grab |
| Interaction de grille | invisible en eau trouble, visible après attaque |
| Faiblesse | feu, attaque de flanc, hauteur |
| Résistance | eau, boue, poison léger |
| Loot | cuir épais, dent, écaille dorsale |

## Utilisation en level design
Reste caché en eau trouble, attaque une case adjacente, peut tirer vers l’eau peu profonde.

## Prompt concept art
Planche de bestiaire, crocodile géant dans marais, yeux au ras de l’eau, remous, embuscade, annotations françaises.

---
# FICHE 09 - Crabe des récifs

> **Planche visuelle à produire**

## Nom technique
`MON_ReefCrab`

## Catégorie
Crustacé cuirassé

## Biome principal
Littoral / récifs

## Description immersive
Le crabe des récifs est un défenseur naturel des rivages. Il se déplace latéralement, bloque les chemins rocheux et oblige le joueur à chercher le bon angle.

## Intention artistique
Crabe massif, carapace bleue-grise, pinces asymétriques, algues, coquillages incrustés, sable humide.

## Gameplay

| Élément | Valeur |
|---|---|
| Rôle | blindage frontal / déplacement latéral |
| Dangerosité | 3 |
| Comportement IA | SideStepTank |
| Tags | Beast, Coastal, Armor, SideStep |
| Interaction de grille | avance de côté, protège son front, contrôle passage étroit |
| Faiblesse | dos, foudre, renversement |
| Résistance | tranchant frontal, eau salée |
| Loot | pince, carapace, chair saline |

## Utilisation en level design
Blindage frontal, déplacement latéral imprévisible, pince immobilisante, faiblesse arrière.

## Prompt concept art
Planche de bestiaire, crabe des récifs, carapace, pinces, littoral rocheux, déplacement latéral, annotations françaises.

---
# FICHE 10 - Sirène des brumes

> **Planche visuelle à produire**

## Nom technique
`MON_MistSiren`

## Catégorie
Enchanteresse côtière

## Biome principal
Côte / lagune / brume

## Description immersive
La sirène des brumes n’attaque pas toujours directement. Elle chante, voile les repères, affaiblit la volonté et fait perdre la bonne direction dans les passages noyés de brouillard.

## Intention artistique
Figure mi-humaine mi-aquatique sobre, voile humide, cheveux sombres, bijoux ternis, silhouette dans brume côtière.

## Gameplay

| Élément | Valeur |
|---|---|
| Rôle | chant / confusion / brouillard |
| Dangerosité | 5 |
| Comportement IA | SongController |
| Tags | Humanoid, Song, Fog, Mind, Coastal |
| Interaction de grille | altère orientation et perception dans la brume |
| Faiblesse | silence, lumière sacrée, distance courte |
| Résistance | eau, peur, charme faible |
| Loot | perle terne, écaille fine, fragment de chant |

## Utilisation en level design
Chant de confusion, attire vers la côte, renforce les cases Fog, interruption possible par son fort ou relique.

## Prompt concept art
Planche de bestiaire, sirène des brumes, lagune, brouillard côtier, chant, fantasy sobre, annotations françaises.

---
# FICHE 11 - Élémentaire d’eau

> **Planche visuelle à produire**

## Nom technique
`MON_WaterElemental`

## Catégorie
Élémentaire

## Biome principal
Côte / temple noyé

## Description immersive
L’élémentaire d’eau est une force de circulation. Il n’est pas simplement liquide : il pousse, tire, remplit, vide et transforme les bassins en mécanismes vivants.

## Intention artistique
Silhouette humanoïde translucide faite de vagues, noyau bleu, algues et bulles, bras en courant, reflets de pierre.

## Gameplay

| Élément | Valeur |
|---|---|
| Rôle | courants / changement de forme |
| Dangerosité | 5 |
| Comportement IA | CurrentShaper |
| Tags | Elemental, Water, Current, ShapeShift |
| Interaction de grille | déplace ou bloque avec courants, traverse grilles immergées |
| Faiblesse | foudre contrôlée, gel, récipient rituel |
| Résistance | feu léger, poison, armes ordinaires |
| Loot | noyau aqueux, perle de courant, eau liée |

## Utilisation en level design
Change la direction des courants, pousse l’équipe, traverse les grilles, peut être figé temporairement.

## Prompt concept art
Planche de bestiaire, élémentaire d’eau, temple noyé, courants, vague humanoïde, annotations françaises.

---
# FICHE 12 - Hydre des marais

> **Planche visuelle à produire**

## Nom technique
`MON_MarshHydra`

## Catégorie
Boss amphibie

## Biome principal
Marais / lac noir

## Description immersive
L’hydre des marais est le boss naturel du Volume IV. Elle occupe le centre d’un bassin, surveille plusieurs directions et force le joueur à gérer position, poison, eau et régénération.

## Intention artistique
Grande créature amphibie à plusieurs têtes, peau sombre, crêtes, vase, roseaux, eau noire, présence massive et lisible.

## Gameplay

| Élément | Valeur |
|---|---|
| Rôle | boss multi-têtes / poison / eau |
| Dangerosité | 6 |
| Comportement IA | MultiHeadBoss |
| Tags | Boss, Beast, Poison, Water, Regeneration |
| Interaction de grille | plusieurs arcs d’attaque, têtes distinctes, bassin central |
| Faiblesse | feu, attaques coordonnées, terrain sec |
| Résistance | eau, poison, peur |
| Loot | écaille d’hydre, croc, glande toxique, cœur amphibie |

## Utilisation en level design
Chaque tête possède un arc d’attaque ; certaines mordent, d’autres crachent poison ou boue ; le feu empêche une repousse.

## Prompt concept art
Planche de bestiaire, hydre des marais, boss multi-têtes, lac noir, roseaux, poison, annotations françaises, artbook dark fantasy.

---
# Annexes UE5

Prévoir des DataAssets de créature contenant : identifiant `MON_*`, catégorie, biome, tags, profil IA, danger, taille en cases, vitesse, interactions de grille, faiblesses, résistances, loot, sons, VFX et liens éventuels aux objets du niveau.

# Checklist de production

- vérifier la silhouette en vue subjective ;
- vérifier la lisibilité de la créature en extérieur ;
- tester les états de case associés ;
- tester la navigation sur grille ;
- créer mesh, matériaux, animations et icône ;
- créer le DataAsset ;
- tester dans une scène extérieure simple 5x5 ou 7x7.