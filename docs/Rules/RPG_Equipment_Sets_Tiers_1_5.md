# Sets d'équipement de classe et variantes raciales — Tiers 1 à 5

## Statut du document

- **Version** : proposition v0.2
- **Document de référence** : `RPG_Core_Rules_v0_1.md`
- **Périmètre** : six classes et six races du prototype minimal
- **Classes** : Guerrier, Voleur, Rôdeur, Mage, Prêtre, Alchimiste
- **Races** : Humain, Nain, Elfe, Halfelin, Gnome, Demi-orc

Les classes et races citées uniquement comme extensions possibles dans les règles — Paladin, Barbare, Barde, Druide, Magus, Moine, Ensorceleur, Nécromancien, Demi-elfe, Aasimar et Tieffelin — ne sont pas incluses dans cette première version. Elles devront recevoir leurs propres règles de classe ou de race avant la création de leurs sets.

---

## 1. Objectifs de conception

Les sets doivent :

- rendre immédiatement lisible le rôle de chaque personnage ;
- accompagner la progression sans remplacer les compétences, les dons et les choix de caractéristiques ;
- respecter les deux protections du système : armure physique et armure magique ;
- renforcer les spécialités de classe sans interdire les constructions atypiques ;
- soutenir la formation de deux rangs : mêlée devant, armes longues, distance, magie et soutien derrière ;
- interagir avec les types de dégâts, les états et les surfaces définis dans les règles ;
- être représentables par des `DataAssets` et des meshes modulaires dans Unreal Engine.

Un tier d'équipement représente une **qualité et une étape de campagne**, pas une obligation absolue de niveau. Pour le prototype limité aux niveaux 1 à 5, un tier peut être testé par niveau. Dans la campagne complète, les tiers couvriront des plages de niveaux plus larges.

---

## 2. Structure commune d'un set

### 2.1 Pièces portées

Chaque set complet comprend neuf pièces, en cohérence avec les emplacements déjà envisagés pour l'inventaire :

| Slot | Pièce générique | Remarque |
|---|---|---|
| Tête | Casque, capuche, coiffe ou masque | Protection ou focalisation |
| Torse | Plastron, cuirasse, tunique ou robe | Pièce principale |
| Épaules | Épaulières, spallières ou mantelet | Silhouette de classe |
| Dos | Cape, manteau ou pèlerine | Résistance ou utilité |
| Mains | Gants ou gantelets | Maniement et précision |
| Taille | Ceinture ou baudrier | Charge et consommables |
| Poignets | Brassards ou poignets | Défense ou canalisation |
| Jambes | Pantalon, braies, chausses ou bas de robe | Mobilité |
| Pieds | Bottes ou solerets | Stabilité et esquive |

Les armes, boucliers, focaliseurs, outils et bijoux sont associés au tier, mais ne comptent pas dans les bonus de set 3/6/9 pièces.

### 2.2 Paliers de bonus

| Pièces équipées | Fonction du bonus |
|---:|---|
| 3 | Identité de classe : compétence ou ressource |
| 6 | Spécialisation tactique : attaque, défense, soutien ou exploration |
| 9 | Pouvoir signature avec condition ou cooldown |

Les bonus ne se cumulent qu'au sein d'un même set et d'un même tier. Un personnage peut mélanger les pièces, mais perd les paliers supérieurs.

### 2.3 Budget indicatif par tier

| Tier | Qualité | Bonus maximal d'arme | Bonus de compétence par objet | Résistance élémentaire totale conseillée |
|---:|---|---:|---:|---:|
| 1 | Commun | +0 | +1 | 0 à 5 % |
| 2 | Peu commun | +1 | +1 | 5 à 10 % |
| 3 | Rare | +2 | +2 | 10 à 15 % |
| 4 | Épique | +3 | +2 | 15 à 20 % |
| 5 | Légendaire | +4 | +3 | 20 à 25 % |

Le bonus maximal d'arme s'applique au jet d'attaque. Les dégâts supplémentaires et les effets de set doivent rester séparés afin de faciliter l'équilibrage.

### 2.4 Code couleur des tiers

La rareté doit rester immédiatement lisible dans l'inventaire, les tooltips, les bordures d'icônes, les fiches d'objet et les effets visuels. La couleur de rareté est globale ; la couleur secondaire de classe peut être utilisée à l'intérieur de l'icône, sur les gemmes, les runes ou les particules.

| Tier | Rareté | Couleur principale | Hex principal | Bordure UI recommandée | Hex bordure | Glow recommandé |
|---:|---|---|---|---|---|---|
| 1 | Commun | Gris | `#9CA3AF` | Fer gris | `#8A8F98` | Aucun ou gris très faible `#B0B0B0` |
| 2 | Peu commun | Vert | `#22C55E` | Vert émeraude sombre | `#2E8B57` | Vert clair `#35D07F` |
| 3 | Rare | Bleu | `#3B82F6` | Bleu arcanique | `#2F6FD6` | Bleu lumineux `#4DA3FF` |
| 4 | Épique | Violet | `#A855F7` | Violet noble | `#8B5CF6` | Violet magique `#B56CFF` |
| 5 | Légendaire | Orange / or | `#F59E0B` | Or légendaire | `#D4AF37` | Or chaud `#FFD166` |

Teintes secondaires de classe recommandées :

| Classe | Teinte secondaire | Usage visuel conseillé |
|---|---|---|
| Guerrier | Rouge sombre / acier | capes, tabards, reflets d'armes, runes de défense |
| Voleur | Violet sombre | ombres, gemmes discrètes, lames enchantées |
| Mage | Bleu-violet | runes, orbes, focaliseurs, effets arcaniques |
| Rôdeur | Vert émeraude | capes, arcs, gemmes sylvestres, effets de nature |
| Prêtre | Blanc sacré / or | symboles sacrés, halos, reliquaires, lumière divine |
| Alchimiste | Teal / ambre | fioles, bombes, catalyseurs, vapeurs alchimiques |

Règle de cohérence artistique : la bordure indique toujours la rareté, tandis que les effets internes rappellent la classe. Ainsi, un objet de Rôdeur T5 possède une bordure or, mais des effets verts sylvestres ; un objet de Mage T5 possède aussi une bordure or, mais des effets bleu-violet arcaniques.

---

## 3. Variantes raciales

Tous les sets de classe existent pour les six races. La classe détermine les statistiques et le gameplay ; la race détermine la morphologie, certains matériaux et une **résonance raciale** légère activée uniquement avec neuf pièces.

| Race | Adaptation visuelle et ergonomique | Résonance raciale du set complet |
|---|---|---|
| Humain | Proportions polyvalentes, héraldique variable, pièces facilement interchangeables | **Adaptabilité** : choisir au repos `+1` dans une compétence forte de la classe jusqu'au prochain repos |
| Nain | Torse large, centre de gravité bas, plaques courtes et épaisses, bottes renforcées | **Ancrage de pierre** : `+10 %` de résistance au renversement et `+5 %` au poison |
| Elfe | Lignes longues, pièces légères, motifs sylvestres ou arcaniques, mobilité des épaules | **Grâce elfique** : `+1 Perception` et `+2 Initiative` |
| Halfelin | Pièces compactes, baudriers accessibles, aucune traîne, poids réduit | **Pas chanceux** : `+1 Esquive`; une fois par repos, relancer un jet de Discrétion ou d'Acrobatie raté |
| Gnome | Nombreuses attaches d'outils, lentilles, fioles et runes miniaturisées | **Ingéniosité** : `+1` en Mécanique, Alchimie ou Arcane, selon la classe du set |
| Demi-orc | Renforts massifs, sangles larges, protection des articulations, silhouette agressive | **Ténacité brutale** : sous `50 %` de PV, `+2 Armure physique` et `+1` aux jets contre Étourdi |

### Règles de compatibilité raciale

- Une variante raciale ne change jamais les compétences d'armure autorisées par la classe.
- Aucun set n'est réservé à une race.
- La résonance raciale ne se cumule pas avec une seconde résonance issue d'un objet exotique.
- Le poids et l'encombrement restent identiques entre variantes pour ne pas pénaliser les petites races.
- Les armes doivent disposer de variantes de prise et d'échelle, sans modifier leur portée logique sur la grille.

---

## 4. Sets du Guerrier

**Rôle** : tank, armes lourdes, contrôle physique.  
**Protection dominante** : armure physique.  
**Pièces** : casque, cuirasse, spallières, cape, gantelets, ceinturon, brassards, chausses, solerets.  
**Armes associées** : épée et bouclier, arme à deux mains, marteau ; lance possible depuis l'arrière.

| Tier | Set et apparence | Armure P/M | Armement associé | Bonus 3 pièces | Bonus 6 pièces | Bonus 9 pièces |
|---:|---|---:|---|---|---|---|
| 1 | **Garde de Fer** : fer noirci, cuir brun, cape courte, aucune gemme | 18 / 4 | Épée de garnison `1d8`, bouclier de bois renforcé `+1 Défense` | `+1 Athlétisme` | `+2 Armure physique` avec un bouclier | **Garde fermée** : Défense accorde `+2 Défense` jusqu'à la prochaine action |
| 2 | **Vétéran des Remparts** : acier rivé, tabard usé, premières finitions métalliques | 26 / 7 | Épée d'armes `1d8+1` ou vouge `1d10+1` | `+1 Armes de mêlée` | `+1` aux jets contre Renversement | **Riposte** : après une parade ou une esquive, prochaine attaque `+2 Précision` |
| 3 | **Bastion Runique** : plaques gravées, saphirs sobres, lueurs défensives | 35 / 11 | Marteau runique `1d10+2`, pavois `+2 Défense` | `+2 Bouclier` | `+10 %` résistance Foudre et Arcane | **Onde de bouclier** : Coup de bouclier inflige `+1d6` contondant ; cooldown 3 actions |
| 4 | **Champion de l'Avant-Garde** : acier bleui, cape héraldique, dorures visibles, rubis | 44 / 14 | Lame de champion `2d6+3` ou hallebarde `1d12+3` | `+2 Armes lourdes` | Provocation accorde `+4 Armure physique` pendant sa durée | **Inamovible** : le premier Renversement ou Étourdissement physique de chaque combat est annulé |
| 5 | **Citadelle Vivante** : alliage ancien, or royal, gemmes majeures, runes flamboyantes | 54 / 18 | Épée du rempart `2d8+4`, égide `+3 Défense` | `+3 Armures` | Tant que le guerrier est au rang avant, alliés adjacents `+2 Armure physique` | **Dernier rempart** : sous 30 % de PV, gagne 12 Armure physique et Provocation ; une fois par repos |

### Répartition des propriétés

- Casque et cuirasse : armure physique principale.
- Spallières et brassards : résistance à Renversement et Étourdi.
- Gantelets : précision de mêlée.
- Cape : armure magique secondaire.
- Ceinturon : port de charge.
- Chausses et solerets : stabilité et défense.

---

## 5. Sets du Voleur

**Rôle** : dégâts ciblés, coups critiques, serrures et pièges.  
**Protection dominante** : esquive et armure légère équilibrée.  
**Pièces** : capuche, plastron de cuir, épaulières souples, cape courte, gants, ceinture à outils, poignets, pantalon, bottes.  
**Armes associées** : dagues, couteaux de lancer, shurikens, arbalète de poing.

| Tier | Set et apparence | Armure P/M | Armement associé | Bonus 3 pièces | Bonus 6 pièces | Bonus 9 pièces |
|---:|---|---:|---|---|---|---|
| 1 | **Maraudeur des Égouts** : cuir usé, toile sombre, boucles de cuivre | 10 / 8 | Dague `1d6`, 3 couteaux `1d4` | `+1 Discrétion` | `+1 Esquive` | **Main sûre** : `+2` au premier jet de Crochetage ou Désamorçage après un repos |
| 2 | **Ombre des Serrures** : cuir bouilli, capuche profonde, équipement plus net | 14 / 12 | Paire de dagues `1d6+1`, outils fins | `+1 Crochetage` | Attaques de dos `+1d4` perforant | **Pas latéral** : après une attaque sournoise, `+2 Esquive` jusqu'à la prochaine action |
| 3 | **Lame du Crépuscule** : cuir noir huilé, renforts d'argent terni, premières améthystes | 19 / 16 | Stylet `1d6+2`, shurikens `1d4+2` | `+2 Désamorçage` | `+2 Précision` avec armes légères ou de jet | **Fumée évasive** : à 50 % de PV, Invisible pendant une action ; une fois par combat |
| 4 | **Maître des Passages** : cuir d'ombre, outils intégrés, dorures violettes | 24 / 21 | Lames jumelles `1d8+3`, arbalète `1d8+3` | `+2 Perception` des pièges et passages secrets | Les critiques réduisent de 2 l'armure physique supplémentaire | **Ouverture parfaite** : la première Attaque sournoise du combat ignore 25 % de l'armure physique |
| 5 | **Voile de la Main Invisible** : cuir enchanté mat, cape sans bruit, gemmes occultes | 30 / 26 | Dague du silence `2d6+4`, étoiles `1d6+4` | `+3 Discrétion` | `+10 %` critique contre une cible sans armure physique | **Disparition** : devient Invisible pendant deux actions et dissipe Entrave ; une fois par repos |

---

## 6. Sets du Rôdeur

**Rôle** : distance, survie, exploration et anti-monstres.  
**Protection dominante** : armure légère physique, résistances naturelles.  
**Pièces** : capuchon, brigandine, épaulières, manteau, gants d'archer, baudrier, brassards, braies, bottes de piste.  
**Armes associées** : arc, arbalète, lance, deux armes légères.

| Tier | Set et apparence | Armure P/M | Armement associé | Bonus 3 pièces | Bonus 6 pièces | Bonus 9 pièces |
|---:|---|---:|---|---|---|---|
| 1 | **Traqueur des Fourrés** : cuir fauve, laine verte grisée, arc simple | 12 / 7 | Arc court `1d8`, lance `1d8` | `+1 Survie` | `+1 Perception` | **Visée calme** : sans attaque subie depuis une action, prochain tir `+1 Précision` |
| 2 | **Éclaireur des Frontières** : cuir écaillé, manteau terreux, arc recourbé | 17 / 10 | Arc recourbé `1d8+1`, lames `1d6+1` | `+1 Armes à distance` | Marque de la proie dure une action de plus | **Trait entravant** : Tir précis peut appliquer Ralenti si l'armure physique est brisée |
| 3 | **Veilleur Runique** : mailles légères, feuilles gravées, premières émeraudes | 23 / 14 | Arc long `1d10+2`, lance-feuille `1d10+2` | `+2 Nature` | `+10 %` résistance Poison et Glace | **Chasseur patient** : `+1d6` sur la première attaque contre une créature nouvellement détectée |
| 4 | **Maître des Lisières** : cuir de monstre, dorures sylvestres, énergie verte | 30 / 18 | Arc composite `1d12+3`, épieu `1d12+3` | `+2 Monstres` | `+2 Précision` contre la cible marquée | **Perce-écaille** : Tir perforant ignore 25 % de l'armure physique ; cooldown 3 actions |
| 5 | **Seigneur des Étoiles Sylvestres** : écailles anciennes, fil d'or, constellations vertes | 36 / 22 | Arc des six voies `2d8+4`, lance astrale `2d6+4` | `+3 Perception` | La Marque révèle résistances et vulnérabilités | **Flèche inexorable** : prochain tir ne peut être esquivé et inflige `+2d6` perforant ; une fois par repos |

---

## 7. Sets du Mage

**Rôle** : dégâts élémentaires, contrôle, surfaces et utilité arcane.  
**Protection dominante** : armure magique.  
**Pièces** : coiffe, robe, mantelet, cape, gants, ceinture runique, poignets, bas de robe ou pantalon, bottes souples.  
**Armes associées** : bâton, baguette, grimoire, orbe.

| Tier | Set et apparence | Armure P/M | Focaliseur associé | Bonus 3 pièces | Bonus 6 pièces | Bonus 9 pièces |
|---:|---|---:|---|---|---|---|
| 1 | **Apprenti des Cendres** : laine gris sombre, bâton simple, aucune gemme | 4 / 16 | Bâton d'apprenti `1d4 Arcane`, `+1 Mana` | `+1 Arcane` | `+2 Mana maximal` | **Canalisation prudente** : Projectile magique coûte 1 Mana de moins une fois par combat |
| 2 | **Érudit des Sceaux** : robe doublée, baguette et grimoire, fines broderies | 6 / 23 | Baguette élémentaire `1d6+1` | `+1` à une école élémentaire choisie | `+5 %` résistance à l'élément choisi | **Accord élémentaire** : le premier sort de cet élément après repos inflige `+1d4` |
| 3 | **Adepte Runique** : soie runique, premiers saphirs, lueurs froides | 9 / 31 | Bâton bifide `1d6+2`, orbe de contrôle | `+2 Runes` | Les surfaces créées durent une action de plus | **Réaction savante** : une interaction de surfaces déclenchée par le mage inflige `+1d6` |
| 4 | **Archimage de la Veille** : robe structurée, orbe lumineux, dorures arcaniques | 11 / 39 | Sceptre `1d8+3 Arcane`, grimoire majeur | `+2 Identification` | `+2` aux jets d'application des états magiques si l'armure magique cible est à 0 | **Contresort** : annule le premier effet magique hostile de chaque combat |
| 5 | **Souverain des Arcanes** : tissu impossible, or royal, grands cristaux et flammes violettes | 14 / 48 | Bâton du Nexus `2d6+4 Arcane` | `+3 Arcane` | `+8 Mana maximal` et `+10 % Armure magique` | **Convergence** : prochain sort de zone gagne `+50 %` de dégâts ou durée ; une fois par repos |

Au tier 2 et au-delà, chaque set de Mage reçoit une rune d'affinité interchangeable : Feu, Glace/Eau, Foudre/Air, Terre/Poison, Arcane ou Illusion. La rune modifie les bonus élémentaires, la couleur des détails et certains effets visuels, sans nécessiter six meshes complets.

---

## 8. Sets du Prêtre

**Rôle** : soin, protection, Sacré et anti-morts-vivants.  
**Protection dominante** : armures physique et magique équilibrées.  
**Pièces** : coiffe ou casque ouvert, haubert ou robe renforcée, épaulières, pèlerine, gants, ceinture liturgique, brassards, chausses, bottes.  
**Armes associées** : masse, marteau, bouclier, symbole sacré.

| Tier | Set et apparence | Armure P/M | Armement associé | Bonus 3 pièces | Bonus 6 pièces | Bonus 9 pièces |
|---:|---|---:|---|---|---|---|
| 1 | **Officiant des Cryptes** : laine écrue, cuir, symbole de bronze | 14 / 12 | Masse `1d6`, symbole sacré | `+1 Religion` | `+2 Mana maximal` | **Prière brève** : Soin mineur rend `+1d4 PV` une fois par combat |
| 2 | **Gardien des Reliques** : mailles fines, pèlerine claire, bouclier sacré sobre | 20 / 17 | Marteau `1d8+1`, rondache `+1 Défense` | `+1 Médecine` | Bénédiction accorde aussi `+2 Armure magique` | **Lumière protectrice** : un soin sur une cible sans armure magique lui en rend 3 |
| 3 | **Exorciste du Seuil** : argent terni, sceaux protecteurs, premières pierres sacrées | 27 / 23 | Masse consacrée `1d8+2 Sacré` | `+2 Foi` | `+10 %` résistance Nécrotique et Maudit | **Répulsion** : Repousser les morts-vivants inflige `+1d6 Sacré` et peut Terroriser |
| 4 | **Hiérophante du Sanctuaire** : plaques claires, étole runique, lumière dorée | 35 / 30 | Marteau solaire `1d10+3`, égide bénie | `+2 Volonté` | Les soins dissipent Brûlé ou Empoisonné une fois par cible et par combat | **Sanctuaire** : l'allié le plus blessé gagne 8 Armure magique ; cooldown 4 actions |
| 5 | **Parure de l'Aube Éternelle** : or pâle, acier blanc, halo et gemmes majeures | 42 / 36 | Masse de l'aube `2d6+4 Sacré` | `+3 Foi` | Alliés adjacents `+10 %` résistance Nécrotique | **Intercession** : une fois par repos, empêche la mort d'un allié, le maintient à 1 PV et lui rend 12 Armure magique |

---

## 9. Sets de l'Alchimiste

**Rôle** : potions, bombes, surfaces et altérations.  
**Protection dominante** : armure magique et résistances aux éléments/toxines.  
**Pièces** : masque ou lunettes, tablier renforcé, épaulières, manteau court, gants, ceinture à fioles, poignets, pantalon, bottes imperméables.  
**Armes associées** : bombes, flasques, arbalète légère, couteau utilitaire.

| Tier | Set et apparence | Armure P/M | Outils associés | Bonus 3 pièces | Bonus 6 pièces | Bonus 9 pièces |
|---:|---|---:|---|---|---|---|
| 1 | **Apothicaire de Campagne** : toile cirée, cuir, cuivre, fioles simples | 8 / 12 | Bombes `1d6`, trousse d'alchimie | `+1 Alchimie` | `+5 %` résistance Poison | **Dosage propre** : la première potion utilisée après repos gagne `+25 %` d'effet |
| 2 | **Artificier des Galeries** : tablier épais, fioles protégées, harnais organisé | 11 / 18 | Bombes feu/huile `1d6+1` | `+1 Lancer` | `+1 Précision` avec bombes et flasques | **Mèche rapide** : la première bombe du combat ne déclenche pas son cooldown normal |
| 3 | **Distillateur Toxique** : masque-filtre, verre vert sombre, premières gemmes teal | 15 / 25 | Bombes poison/acide `2d6+2` | `+2 Poison` | `+15 %` résistance Poison et Acide | **Nuage persistant** : surfaces de poison durent une action de plus |
| 4 | **Maître des Réactions** : plaques de cuivre, conduits, valves, dorures nettes | 19 / 31 | Bombes élémentaires `2d6+3` | `+2 Mécanique` | Réactions Feu+Huile, Feu+Poison ou Eau+Foudre infligent `+1d6` | **Catalyse instantanée** : transforme immédiatement une surface cible ; cooldown 4 actions |
| 5 | **Laboratoire Ambulant** : alliage alchimique, verre runique, catalyseurs flamboyants | 24 / 38 | Bombes magistrales `3d6+4` | `+3 Alchimie` | Deux emplacements rapides supplémentaires pour potions ou bombes | **Formule parfaite** : prochaine bombe crée une surface renforcée et ignore 25 % de l'armure correspondante ; une fois par repos |

---

## 10. Matrice classe-race recommandée

Toutes les combinaisons sont permises. Le tableau indique seulement les affinités naturelles à mettre en avant dans les personnages préconstruits et dans la direction artistique.

| Race | Guerrier | Voleur | Rôdeur | Mage | Prêtre | Alchimiste |
|---|---|---|---|---|---|---|
| Humain | Très forte | Forte | Forte | Forte | Forte | Forte |
| Nain | Très forte | Possible | Forte | Possible | Très forte | Forte |
| Elfe | Possible | Forte | Très forte | Très forte | Forte | Possible |
| Halfelin | Possible | Très forte | Forte | Possible | Possible | Forte |
| Gnome | Possible | Forte | Possible | Très forte | Possible | Très forte |
| Demi-orc | Très forte | Possible | Forte | Possible | Forte | Possible |

`Possible` ne signifie aucune pénalité systémique. Cela signale seulement une association moins archétypale qui peut devenir intéressante par les caractéristiques, les dons et l'équipement choisis.

---

## 11. Règles de butin et d'acquisition

- **Tier 1** : équipement de départ, arsenaux abandonnés, premiers coffres. Le groupe doit pouvoir compléter au moins deux sets T1 durant le premier niveau de donjon.
- **Tier 2** : récompenses d'énigmes, coffres verrouillés, artisans ou recettes simples. Introduction des premiers bonus qui modifient une capacité de classe.
- **Tier 3** : récompenses de mini-boss, zones secrètes et chaînes d'énigmes. Première apparition recommandée de pierres précieuses visibles.
- **Tier 4** : récompenses de boss de zone et quêtes de classe. Dorures et effets lumineux doivent être clairement visibles.
- **Tier 5** : sets légendaires fragmentés entre plusieurs régions ou donjons. Le set complet doit être une récompense de fin de progression, jamais un simple achat.

Protection contre le hasard frustrant :

- Le premier exemplaire d'une pièce de set non acquise doit être favorisé.
- Les doublons peuvent être démontés en matériaux de classe.
- Trois doublons d'un même tier permettent de fabriquer une pièce manquante de ce tier.
- Les coffres secrets peuvent utiliser la meilleure Perception du groupe ; les coffres verrouillés, le meilleur Crochetage avec un set d'outils disponible.

---

## 12. Recommandations d'équilibrage

1. Les bonus de set ne doivent pas augmenter simultanément Précision, dégâts et critique au même palier.
2. L'armure physique et magique accordée par les sets doit suivre le rôle, sans rendre une protection inexistante.
3. Les états dangereux restent bloqués par l'armure correspondante, conformément aux règles v0.1.
4. Une capacité de set ne doit pas supprimer l'utilité d'une compétence ou d'un don.
5. Les pouvoirs T5 doivent être spectaculaires mais rares : une fois par repos est la norme.
6. Les bonus de groupe sont limités aux personnages adjacents dans la formation et ne se cumulent pas avec eux-mêmes.
7. Les effets de surface renforcés ne doivent pas s'étendre au-delà d'une case supplémentaire sans test spécifique.
8. Les valeurs `1dX` et les totaux d'armure proposés constituent un point de départ de prototypage, à ajuster après mesure du temps moyen pour tuer et du taux de survie.
9. Les couleurs de rareté ne modifient pas les valeurs mécaniques : elles servent à la lisibilité, à la direction artistique et à l'interface.
10. Les effets lumineux T4/T5 ne doivent pas masquer la silhouette ni gêner la lecture des pièces d'équipement.

---

## 13. Modèle de données UE5 conseillé

```cpp
UENUM(BlueprintType)
enum class ERPGEquipmentTier : uint8
{
    Tier1_Common      UMETA(DisplayName="Commun"),
    Tier2_Uncommon    UMETA(DisplayName="Peu commun"),
    Tier3_Rare        UMETA(DisplayName="Rare"),
    Tier4_Epic        UMETA(DisplayName="Épique"),
    Tier5_Legendary   UMETA(DisplayName="Légendaire")
};

UENUM(BlueprintType)
enum class ERPGEquipmentSlot : uint8
{
    Head,
    Chest,
    Shoulders,
    Back,
    Hands,
    Waist,
    Wrists,
    Legs,
    Feet,
    MainHand,
    OffHand,
    Accessory
};
```

Palette de tiers conseillée pour un `DataAsset`, une `DataTable` ou un réglage UI centralisé :

```text
Common      Border=#8A8F98  Main=#9CA3AF  Glow=#B0B0B0
Uncommon    Border=#2E8B57  Main=#22C55E  Glow=#35D07F
Rare        Border=#2F6FD6  Main=#3B82F6  Glow=#4DA3FF
Epic        Border=#8B5CF6  Main=#A855F7  Glow=#B56CFF
Legendary   Border=#D4AF37  Main=#F59E0B  Glow=#FFD166
```

Un `URPGEquipmentSetAsset` devrait contenir au minimum :

```text
SetId
DisplayName
ClassAffinity
Tier
TierColorProfile
ClassAccentColor
ArmorProfile
PieceIds[9]
AssociatedWeaponIds
ThreePieceEffect
SixPieceEffect
NinePieceEffect
RaceVariantData[6]
LootSources
CraftingRecipe
ReferenceImagePath
```

Chaque pièce reste un objet indépendant. Le gestionnaire d'équipement compte les pièces portant le même `SetId`, puis applique ou retire les effets de palier sous forme de `URPGStatusEffectAsset` passif.

---

## 14. Références visuelles produites

Les images générées dans le fil de conception servent de références artistiques initiales. Elles ne sont pas encore intégrées comme assets Unreal et doivent être déposées dans le dépôt avant que les liens Markdown directs puissent être activés.

Chemin recommandé dans le dépôt :

```text
docs/Art/EquipmentSets/<Classe>/Tier_<N>.png
```

| Classe | Tier 1 | Tier 2 | Tier 3 | Tier 4 | Tier 5 |
|---|---|---|---|---|---|
| Guerrier | `docs/Art/EquipmentSets/Guerrier/Tier_1.png` | `docs/Art/EquipmentSets/Guerrier/Tier_2.png` | `docs/Art/EquipmentSets/Guerrier/Tier_3.png` | `docs/Art/EquipmentSets/Guerrier/Tier_4.png` | `docs/Art/EquipmentSets/Guerrier/Tier_5.png` |
| Voleur | `docs/Art/EquipmentSets/Voleur/Tier_1.png` | `docs/Art/EquipmentSets/Voleur/Tier_2.png` | `docs/Art/EquipmentSets/Voleur/Tier_3.png` | `docs/Art/EquipmentSets/Voleur/Tier_4.png` | `docs/Art/EquipmentSets/Voleur/Tier_5.png` |
| Rôdeur | `docs/Art/EquipmentSets/Rodeur/Tier_1.png` | `docs/Art/EquipmentSets/Rodeur/Tier_2.png` | `docs/Art/EquipmentSets/Rodeur/Tier_3.png` | `docs/Art/EquipmentSets/Rodeur/Tier_4.png` | `docs/Art/EquipmentSets/Rodeur/Tier_5.png` |
| Mage | `docs/Art/EquipmentSets/Mage/Tier_1.png` | `docs/Art/EquipmentSets/Mage/Tier_2.png` | `docs/Art/EquipmentSets/Mage/Tier_3.png` | `docs/Art/EquipmentSets/Mage/Tier_4.png` | `docs/Art/EquipmentSets/Mage/Tier_5.png` |
| Prêtre | `docs/Art/EquipmentSets/Pretre/Tier_1.png` | `docs/Art/EquipmentSets/Pretre/Tier_2.png` | `docs/Art/EquipmentSets/Pretre/Tier_3.png` | `docs/Art/EquipmentSets/Pretre/Tier_4.png` | `docs/Art/EquipmentSets/Pretre/Tier_5.png` |
| Alchimiste | `docs/Art/EquipmentSets/Alchimiste/Tier_1.png` | `docs/Art/EquipmentSets/Alchimiste/Tier_2.png` | `docs/Art/EquipmentSets/Alchimiste/Tier_3.png` | `docs/Art/EquipmentSets/Alchimiste/Tier_4.png` | `docs/Art/EquipmentSets/Alchimiste/Tier_5.png` |

Archive locale de travail produite pendant le fil : `RPG_Equipment_Set_Images_Tiers_1_5.zip`.

> Note : les liens ci-dessus sont des chemins cibles recommandés. Tant que les fichiers image ne sont pas ajoutés au dépôt, ils doivent rester traités comme références de production et non comme liens actifs garantis.

---

## 15. Ordre de production artistique recommandé

1. Produire les six silhouettes de classe au tier 1.
2. Décliner chaque silhouette sur les six morphologies raciales.
3. Produire les tiers 2 à 5 en réutilisant le squelette, les points d'attache et une partie des meshes modulaires.
4. Créer d'abord les vues de face, dos, profil droit et profil gauche de chaque set complet.
5. Isoler ensuite les neuf pièces pour Meshy ou Blender.
6. Terminer par les armes et focaliseurs associés.
7. Appliquer la bordure de rareté globale et la teinte secondaire de classe dans les icônes d'inventaire.

Pour limiter la production initiale, la première vague peut couvrir les six personnages du groupe de test : Guerrier humain, Prêtre nain, Rôdeur demi-orc, Voleur halfelin, Mage elfe et Alchimiste gnome.

---

## 16. Décisions à valider avant implémentation

- Confirmer si les bonus de set exigent exactement 3/6/9 pièces ou simplement au moins 3/6/9.
- Confirmer si les bijoux participent à un futur palier 10 ou restent toujours indépendants.
- Fixer les formules définitives d'armure physique et magique avant de figer les valeurs numériques.
- Décider si les objets s'usent ; l'usure n'est pas recommandée pour le prototype v0.1.
- Décider si les variantes raciales conservent une résonance mécanique ou deviennent purement visuelles en mode compétitif/équilibrage strict.
- Confirmer l'emplacement définitif des références artistiques dans le dépôt : `docs/Art/EquipmentSets/` ou `Content/Grimrock/Icons/EquipmentSets/`.
