# Sets d'equipement de classe et variantes raciales - Tiers 1 a 5

## Statut du document

- **Version** : proposition v0.1
- **Document de reference** : `RPG_Core_Rules_v0_1.md`
- **Perimetre** : six classes et six races du prototype minimal
- **Classes** : Guerrier, Voleur, Rodeur, Mage, Pretre, Alchimiste
- **Races** : Humain, Nain, Elfe, Halfelin, Gnome, Demi-orc

Les classes et races citees uniquement comme extensions possibles dans les regles (Paladin, Barbare, Barde, Druide, Magus, Moine, Ensorceleur, Necromancien, Demi-elfe, Aasimar et Tieffelin) ne sont pas incluses dans cette premiere version. Elles devront recevoir leurs propres regles de classe ou de race avant la creation de leurs sets.

---

## 1. Objectifs de conception

Les sets doivent :

- rendre immediatement lisible le role de chaque personnage ;
- accompagner la progression sans remplacer les competences, dons et choix de caracteristiques ;
- respecter les deux protections du systeme : armure physique et armure magique ;
- renforcer les specialites de classe sans interdire les constructions atypiques ;
- soutenir la formation de deux rangs : melee devant, armes longues, distance, magie et soutien derriere ;
- interagir avec les types de degats, etats et surfaces definis dans les regles ;
- etre representables par des `DataAssets` et des meshes modulaires dans Unreal Engine.

Un tier d'equipement represente une **qualite et une etape de campagne**, pas une obligation absolue de niveau. Pour le prototype limite aux niveaux 1 a 5, un tier peut etre teste par niveau. Dans la campagne complete, les tiers couvriront des plages de niveaux plus larges.

---

## 2. Structure commune d'un set

### 2.1 Pieces portees

Chaque set complet comprend neuf pieces, en coherence avec les emplacements deja envisages pour l'inventaire :

| Slot | Piece generique | Remarque |
|---|---|---|
| Tete | Casque, capuche, coiffe ou masque | Protection ou focalisation |
| Torse | Plastron, cuirasse, tunique ou robe | Piece principale |
| Epaules | Epaulieres, spallieres ou mantelet | Silhouette de classe |
| Dos | Cape, manteau ou pelerine | Resistance ou utilite |
| Mains | Gants ou gantelets | Maniement et precision |
| Taille | Ceinture ou baudrier | Charge et consommables |
| Poignets | Brassards ou poignets | Defense ou canalisation |
| Jambes | Pantalon, braies, chausses ou bas de robe | Mobilite |
| Pieds | Bottes ou solerets | Stabilite et esquive |

Les armes, boucliers, focaliseurs, outils et bijoux sont associes au tier, mais ne comptent pas dans les bonus de set 3/6/9 pieces.

### 2.2 Paliers de bonus

| Pieces equipees | Fonction du bonus |
|---:|---|
| 3 | Identite de classe : competence ou ressource |
| 6 | Specialisation tactique : attaque, defense, soutien ou exploration |
| 9 | Pouvoir signature avec condition ou cooldown |

Les bonus ne se cumulent qu'au sein d'un meme set et d'un meme tier. Un personnage peut melanger les pieces, mais perd les paliers superieurs.

### 2.3 Budget indicatif par tier

| Tier | Qualite | Bonus maximal d'arme | Bonus de competence par objet | Resistance elementaire totale conseillee |
|---:|---|---:|---:|---:|
| 1 | Commun / usage | +0 | +1 | 0 a 5 % |
| 2 | Ouvrage | +1 | +1 | 5 a 10 % |
| 3 | Rare | +2 | +2 | 10 a 15 % |
| 4 | Epique | +3 | +2 | 15 a 20 % |
| 5 | Legendaire | +4 | +3 | 20 a 25 % |

Le bonus maximal d'arme s'applique au jet d'attaque. Les degats supplementaires et les effets de set doivent rester separes afin de faciliter l'equilibrage.

---

## 3. Variantes raciales

Tous les sets de classe existent pour les six races. La classe determine les statistiques et le gameplay ; la race determine la morphologie, certains materiaux et une **resonance raciale** legere activee uniquement avec neuf pieces.

| Race | Adaptation visuelle et ergonomique | Resonance raciale du set complet |
|---|---|---|
| Humain | Proportions polyvalentes, heraldique variable, pieces facilement interchangeables | **Adaptabilite** : choisir au repos `+1` dans une competence forte de la classe jusqu'au prochain repos |
| Nain | Torse large, centre de gravite bas, plaques courtes et epaisses, bottes renforcees | **Ancrage de pierre** : `+10 %` de resistance au renversement et `+5 %` au poison |
| Elfe | Lignes longues, pieces legeres, motifs sylvestres ou arcaniques, mobilite des epaules | **Grace elfique** : `+1 Perception` et `+2 Initiative` |
| Halfelin | Pieces compactes, baudriers accessibles, aucune traine, poids reduit | **Pas chanceux** : `+1 Esquive`; une fois par repos, relancer un jet de Discretion ou d'Acrobatie rate |
| Gnome | Nombreuses attaches d'outils, lentilles, fioles et runes miniaturisees | **Ingeniosite** : `+1` en Mecanique, Alchimie ou Arcane, selon la classe du set |
| Demi-orc | Renforts massifs, sangles larges, protection des articulations, silhouette agressive | **Tenacite brutale** : sous `50 %` de PV, `+2 Armure physique` et `+1` aux jets contre Etourdi |

### Regles de compatibilite raciale

- Une variante raciale ne change jamais les competences d'armure autorisees par la classe.
- Aucun set n'est reserve a une race.
- La resonance raciale ne se cumule pas avec une seconde resonance issue d'un objet exotique.
- Le poids et l'encombrement restent identiques entre variantes pour ne pas penaliser les petites races.
- Les armes doivent disposer de variantes de prise et d'echelle, sans modifier leur portee logique sur la grille.

---

## 4. Sets du Guerrier

**Role** : tank, armes lourdes, controle physique.  
**Protection dominante** : armure physique.  
**Pieces** : casque, cuirasse, spallieres, cape, gantelets, ceinturon, brassards, chausses, solerets.  
**Armes associees** : epee et bouclier, hache a deux mains, marteau; lance possible depuis l'arriere.

| Tier | Set et apparence | Armure P/M | Armement associe | Bonus 3 pieces | Bonus 6 pieces | Bonus 9 pieces |
|---:|---|---:|---|---|---|---|
| 1 | **Garde de Fer** : fer noirci, cuir brun, cape courte | 18 / 4 | Epee de garnison `1d8`, bouclier de bois renforce `+1 Defense` | `+1 Athletisme` | `+2 Armure physique` avec un bouclier | **Garde fermee** : Defense accorde `+2 Defense` jusqu'a la prochaine action |
| 2 | **Veteran des Remparts** : acier rive, tabard use | 26 / 7 | Epee d'armes `1d8+1` ou vouge `1d10+1` | `+1 Armes de melee` | `+1` aux jets contre Renversement | **Riposte** : apres une parade ou une esquive, prochaine attaque `+2 Precision` |
| 3 | **Bastion Runique** : plaques gravees, doublure isolante | 35 / 11 | Marteau runique `1d10+2`, pavois `+2 Defense` | `+2 Bouclier` | `+10 %` resistance Foudre et Arcane | **Onde de bouclier** : Coup de bouclier inflige `+1d6` contondant; cooldown 3 actions |
| 4 | **Champion de l'Avant-Garde** : acier bleui, cape heraldique | 44 / 14 | Lame de champion `2d6+3` ou hallebarde `1d12+3` | `+2 Armes lourdes` | Provocation accorde `+4 Armure physique` pendant sa duree | **Inamovible** : le premier Renversement ou Etourdissement physique de chaque combat est annule |
| 5 | **Citadelle Vivante** : alliage ancien, runes defensives | 54 / 18 | Epee du rempart `2d8+4`, egide `+3 Defense` | `+3 Armures` | Tant que le guerrier est au rang avant, allies adjacents `+2 Armure physique` | **Dernier rempart** : sous 30 % de PV, gagne 12 Armure physique et Provocation; une fois par repos |

### Repartition des proprietes

- Casque et cuirasse : armure physique principale.
- Spallieres et brassards : resistance a Renversement et Etourdi.
- Gantelets : precision de melee.
- Cape : armure magique secondaire.
- Ceinturon : port de charge.
- Chausses et solerets : stabilite et defense.

---

## 5. Sets du Voleur

**Role** : degats cibles, coups critiques, serrures et pieges.  
**Protection dominante** : esquive et armure legere equilibree.  
**Pieces** : capuche, plastron de cuir, epaulieres souples, cape courte, gants, ceinture a outils, poignets, pantalon, bottes.  
**Armes associees** : dagues, couteaux de lancer, shurikens, arbalete de poing.

| Tier | Set et apparence | Armure P/M | Armement associe | Bonus 3 pieces | Bonus 6 pieces | Bonus 9 pieces |
|---:|---|---:|---|---|---|---|
| 1 | **Maraudeur des Egouts** : cuir use, toile sombre, boucles de cuivre | 10 / 8 | Dague `1d6`, 3 couteaux `1d4` | `+1 Discretion` | `+1 Esquive` | **Main sure** : `+2` au premier jet de Crochetage ou Desamorcage apres un repos |
| 2 | **Ombre des Serrures** : cuir bouilli, capuche profonde | 14 / 12 | Paire de dagues `1d6+1`, outils fins | `+1 Crochetage` | Attaques de dos `+1d4` perforant | **Pas lateral** : apres une attaque sournoise, `+2 Esquive` jusqu'a la prochaine action |
| 3 | **Lame du Crepuscule** : cuir noir huile, renforts d'argent terni | 19 / 16 | Stylet `1d6+2`, shurikens `1d4+2` | `+2 Desamorcage` | `+2 Precision` avec armes legeres ou de jet | **Fumee evasive** : a 50 % de PV, Invisible pendant une action; une fois par combat |
| 4 | **Maitre des Passages** : cuir d'ombre, outils integres | 24 / 21 | Lames jumelles `1d8+3`, arbalete `1d8+3` | `+2 Perception` des pieges et passages secrets | Les critiques reduisent de 2 l'armure physique supplementaire | **Ouverture parfaite** : la premiere Attaque sournoise du combat ignore 25 % de l'armure physique |
| 5 | **Voile de la Main Invisible** : cuir enchante mat, cape sans bruit | 30 / 26 | Dague du silence `2d6+4`, etoiles `1d6+4` | `+3 Discretion` | `+10 %` critique contre une cible sans armure physique | **Disparition** : devient Invisible pendant deux actions et dissipe Entrave; une fois par repos |

### Repartition des proprietes

- Capuche et cape : discretion et perception.
- Plastron et epaulieres : armure physique sans penalite de mobilite.
- Gants et poignets : precision, crochetage et desamorcage.
- Ceinture : emplacements d'outils et d'armes de jet.
- Pantalon et bottes : esquive, acrobatie et silence.

---

## 6. Sets du Rodeur

**Role** : distance, survie, exploration et anti-monstres.  
**Protection dominante** : armure legere physique, resistances naturelles.  
**Pieces** : capuchon, brigandine, epaulieres, manteau, gants d'archer, baudrier, brassards, braies, bottes de piste.  
**Armes associees** : arc, arbalete, lance, deux armes legeres.

| Tier | Set et apparence | Armure P/M | Armement associe | Bonus 3 pieces | Bonus 6 pieces | Bonus 9 pieces |
|---:|---|---:|---|---|---|---|
| 1 | **Pisteur des Galeries** : cuir fauve, laine verte grisee | 12 / 7 | Arc court `1d8`, lance `1d8` | `+1 Survie` | `+1 Perception` | **Visee calme** : sans attaque subie depuis une action, prochain tir `+1 Precision` |
| 2 | **Chasseur des Profondeurs** : cuir ecaille, manteau terreux | 17 / 10 | Arc recourbe `1d8+1`, lames `1d6+1` | `+1 Armes a distance` | Marque de la proie dure une action de plus | **Trait entravant** : Tir precis peut appliquer Ralenti si l'armure physique est brisee |
| 3 | **Veilleur Sylvestre** : mailles legeres, feuilles gravees | 23 / 14 | Arc long `1d10+2`, lance-feuille `1d10+2` | `+2 Nature` | `+10 %` resistance Poison et Glace | **Chasseur patient** : `+1d6` sur la premiere attaque contre une creature nouvellement detectee |
| 4 | **Traqueur de Chimeres** : cuir de monstre, trophees sobres | 30 / 18 | Arc composite `1d12+3`, epieu `1d12+3` | `+2 Monstres` | `+2 Precision` contre la cible marquee | **Perce-ecaille** : Tir perforant ignore 25 % de l'armure physique; cooldown 3 actions |
| 5 | **Sentinelle de l'Horizon Souterrain** : ecailles anciennes, fil d'argent | 36 / 22 | Arc des six voies `2d8+4`, lance astrale `2d6+4` | `+3 Perception` | La Marque revele resistances et vulnerabilites | **Fleche inexorable** : prochain tir ne peut etre esquive et inflige `+2d6` perforant; une fois par repos |

### Repartition des proprietes

- Capuchon et manteau : perception, survie et resistances.
- Brigandine et epaulieres : armure physique mobile.
- Gants et brassards : precision a distance.
- Baudrier : munitions et changement d'arme.
- Braies et bottes : initiative et detection environnementale.

---

## 7. Sets du Mage

**Role** : degats elementaires, controle, surfaces et utilite arcane.  
**Protection dominante** : armure magique.  
**Pieces** : coiffe, robe, mantelet, cape, gants, ceinture runique, poignets, bas de robe ou pantalon, bottes souples.  
**Armes associees** : baton, baguette, grimoire, orbe.

| Tier | Set et apparence | Armure P/M | Focaliseur associe | Bonus 3 pieces | Bonus 6 pieces | Bonus 9 pieces |
|---:|---|---:|---|---|---|---|
| 1 | **Apprenti des Runes** : laine bleu-gris, runes de craie | 4 / 16 | Baton d'apprenti `1d4 Arcane`, `+1 Mana` | `+1 Arcane` | `+2 Mana maximal` | **Canalisation prudente** : Projectile magique coute 1 Mana de moins une fois par combat |
| 2 | **Adepte des Elements** : robe doublee, broderies feu/eau/air/terre | 6 / 23 | Baguette elementaire `1d6+1` | `+1` a une ecole elementaire choisie | `+5 %` resistance a l'element choisi | **Accord elementaire** : le premier sort de cet element apres repos inflige `+1d4` |
| 3 | **Tisseur de Surfaces** : soie runique, fioles catalytiques | 9 / 31 | Baton bifide `1d6+2`, orbe de controle | `+2 Runes` | Les surfaces creees durent une action de plus | **Reaction savante** : une interaction de surfaces declenchee par le mage inflige `+1d6` |
| 4 | **Archimage des Sept Ecoles** : robe structuree, plaques arcaniques | 11 / 39 | Sceptre `1d8+3 Arcane`, grimoire majeur | `+2 Identification` | `+2` aux jets d'application des etats magiques si l'armure magique cible est a 0 | **Contresort** : annule le premier effet magique hostile de chaque combat |
| 5 | **Regalia du Nexus** : tissu impossible, runes lumineuses contenues | 14 / 48 | Baton du Nexus `2d6+4 Arcane` | `+3 Arcane` | `+8 Mana maximal` et `+10 % Armure magique` | **Convergence** : prochain sort de zone gagne `+50 %` de degats ou duree; une fois par repos |

### Specialisation elementaire

Au tier 2 et au-dela, chaque set de Mage recoit une rune d'affinite interchangeable : Feu, Glace/Eau, Foudre/Air, Terre/Poison, Arcane ou Illusion. La rune modifie les bonus elementaires, la couleur des details et certains effets visuels, sans necessiter six meshes complets.

---

## 8. Sets du Pretre

**Role** : soin, protection, Sacre et anti-morts-vivants.  
**Protection dominante** : armures physique et magique equilibrees.  
**Pieces** : coiffe ou casque ouvert, haubert ou robe renforcee, epaulieres, pelerine, gants, ceinture liturgique, brassards, chausses, bottes.  
**Armes associees** : masse, marteau, bouclier, symbole sacre.

| Tier | Set et apparence | Armure P/M | Armement associe | Bonus 3 pieces | Bonus 6 pieces | Bonus 9 pieces |
|---:|---|---:|---|---|---|---|
| 1 | **Officiant des Cryptes** : laine ecrue, cuir, symbole de bronze | 14 / 12 | Masse `1d6`, symbole sacre | `+1 Religion` | `+2 Mana maximal` | **Priere breve** : Soin mineur rend `+1d4 PV` une fois par combat |
| 2 | **Gardien des Reliques** : mailles fines, pelerine claire | 20 / 17 | Marteau `1d8+1`, rondache `+1 Defense` | `+1 Medecine` | Benediction accorde aussi `+2 Armure magique` | **Lumiere protectrice** : un soin sur une cible sans armure magique lui en rend 3 |
| 3 | **Exorciste du Seuil** : argent terni, sceaux protecteurs | 27 / 23 | Masse consacree `1d8+2 Sacre` | `+2 Foi` | `+10 %` resistance Necrotique et Maudit | **Repulsion** : Repousser les morts-vivants inflige `+1d6 Sacre` et peut Terroriser |
| 4 | **Hierophante du Sanctuaire** : plaques claires, etole runique | 35 / 30 | Marteau solaire `1d10+3`, egide benie | `+2 Volonte` | Les soins dissipent Brule ou Empoisonne une fois par cible et par combat | **Sanctuaire** : l'allie le plus blesse gagne 8 Armure magique; cooldown 4 actions |
| 5 | **Parure de l'Aube Eternelle** : or pale, acier blanc, lumiere sobre | 42 / 36 | Masse de l'aube `2d6+4 Sacre` | `+3 Foi` | Allies adjacents `+10 %` resistance Necrotique | **Intercession** : une fois par repos, empeche la mort d'un allie, le maintient a 1 PV et lui rend 12 Armure magique |

### Repartition des proprietes

- Coiffe, pelerine et symbole : foi, mana et armure magique.
- Haubert, epaulieres et chausses : armure physique.
- Gants et brassards : puissance de soin et precision a la masse.
- Ceinture : consommables de soin.
- Bottes : volonte et stabilite.

---

## 9. Sets de l'Alchimiste

**Role** : potions, bombes, surfaces et alterations.  
**Protection dominante** : armure magique et resistances aux elements/toxines.  
**Pieces** : masque ou lunettes, tablier renforce, epaulieres, manteau court, gants, ceinture a fioles, poignets, pantalon, bottes impermeables.  
**Armes associees** : bombes, flasques, arbalete legere, couteau utilitaire.

| Tier | Set et apparence | Armure P/M | Outils associes | Bonus 3 pieces | Bonus 6 pieces | Bonus 9 pieces |
|---:|---|---:|---|---|---|---|
| 1 | **Apothicaire de Campagne** : toile ciree, cuir, cuivre | 8 / 12 | Bombes `1d6`, trousse d'alchimie | `+1 Alchimie` | `+5 %` resistance Poison | **Dosage propre** : la premiere potion utilisee apres repos gagne `+25 %` d'effet |
| 2 | **Artificier des Galeries** : tablier epais, fioles protegees | 11 / 18 | Bombes feu/huile `1d6+1` | `+1 Lancer` | `+1 Precision` avec bombes et flasques | **Meche rapide** : la premiere bombe du combat ne declenche pas son cooldown normal |
| 3 | **Distillateur Toxique** : masque filtre, verre vert sombre | 15 / 25 | Bombes poison/acide `2d6+2` | `+2 Poison` | `+15 %` resistance Poison et Acide | **Nuage persistant** : surfaces de poison durent une action de plus |
| 4 | **Maitre des Reactions** : plaques de cuivre, conduits et valves | 19 / 31 | Bombes elementaires `2d6+3` | `+2 Mecanique` | Reactions Feu+Huile, Feu+Poison ou Eau+Foudre infligent `+1d6` | **Catalyse instantanee** : transforme immediatement une surface cible; cooldown 4 actions |
| 5 | **Laboratoire Ambulant** : alliage alchimique, verre runique | 24 / 38 | Bombes magistrales `3d6+4` | `+3 Alchimie` | Deux emplacements rapides supplementaires pour potions ou bombes | **Formule parfaite** : prochaine bombe cree une surface renforcee et ignore 25 % de l'armure correspondante; une fois par repos |

### Repartition des proprietes

- Masque et manteau : resistances au poison, acide et feu.
- Tablier et epaulieres : armure physique contre les accidents.
- Gants et poignets : Alchimie, Artisanat et Mecanique.
- Ceinture : emplacements rapides et capacite de fioles.
- Pantalon et bottes : protection contre les surfaces.

---

## 10. Matrice classe-race recommandee

Toutes les combinaisons sont permises. Le tableau indique seulement les affinites naturelles a mettre en avant dans les personnages preconstruits et dans la direction artistique.

| Race | Guerrier | Voleur | Rodeur | Mage | Pretre | Alchimiste |
|---|---|---|---|---|---|---|
| Humain | Tres forte | Forte | Forte | Forte | Forte | Forte |
| Nain | Tres forte | Possible | Forte | Possible | Tres forte | Forte |
| Elfe | Possible | Forte | Tres forte | Tres forte | Forte | Possible |
| Halfelin | Possible | Tres forte | Forte | Possible | Possible | Forte |
| Gnome | Possible | Forte | Possible | Tres forte | Possible | Tres forte |
| Demi-orc | Tres forte | Possible | Forte | Possible | Forte | Possible |

`Possible` ne signifie aucune penalite systemique. Cela signale seulement une association moins archetypale qui peut devenir interessante par les caracteristiques, les dons et l'equipement choisis.

---

## 11. Regles de butin et d'acquisition

### Tier 1

- Equipement de depart, arsenaux abandonnes, premiers coffres.
- Le groupe doit pouvoir completer au moins deux sets T1 durant le premier niveau de donjon.
- Les autres personnages utilisent des pieces independantes pour eviter une puissance de groupe excessive.

### Tier 2

- Recompenses d'enigmes, coffres verrouilles, artisans ou recettes simples.
- Introduction des premiers bonus qui modifient une capacite de classe.

### Tier 3

- Recompenses de mini-boss, zones secretes et chaines d'enigmes.
- Premier tier avec interactions significatives sur etats, resistances et surfaces.

### Tier 4

- Recompenses de boss de zone et quetes de classe.
- Les bonus de neuf pieces peuvent modifier une capacite signature, avec cooldown.

### Tier 5

- Sets legendaires fragmentes entre plusieurs regions ou donjons.
- Le set complet doit etre une recompense de fin de progression, jamais un simple achat.
- Chaque pouvoir majeur est limite a une utilisation par repos ou par condition forte.

### Protection contre le hasard frustrant

- Le premier exemplaire d'une piece de set non acquise doit etre favorise.
- Les doublons peuvent etre demontes en materiaux de classe.
- Trois doublons d'un meme tier permettent de fabriquer une piece manquante de ce tier.
- Les coffres secrets peuvent utiliser la meilleure Perception du groupe; les coffres verrouilles, le meilleur Crochetage avec un set d'outils disponible.

---

## 12. Recommandations d'equilibrage

1. Les bonus de set ne doivent pas augmenter simultanement Precision, degats et critique au meme palier.
2. L'armure physique et magique accordee par les sets doit suivre le role, sans rendre une protection inexistante.
3. Les etats dangereux restent bloques par l'armure correspondante, conformement aux regles v0.1.
4. Une capacite de set ne doit pas supprimer l'utilite d'une competence ou d'un don.
5. Les pouvoirs T5 doivent etre spectaculaires mais rares : une fois par repos est la norme.
6. Les bonus de groupe sont limites aux personnages adjacents dans la formation et ne se cumulent pas avec eux-memes.
7. Les effets de surface renforces ne doivent pas s'etendre au-dela d'une case supplementaire sans test specifique.
8. Les valeurs `1dX` et les totaux d'armure proposes constituent un point de depart de prototypage, a ajuster apres mesure du temps moyen pour tuer et du taux de survie.

---

## 13. Modele de donnees UE5 conseille

```cpp
UENUM(BlueprintType)
enum class ERPGEquipmentTier : uint8
{
    Tier1,
    Tier2,
    Tier3,
    Tier4,
    Tier5
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

Un `URPGEquipmentSetAsset` devrait contenir au minimum :

```text
SetId
DisplayName
ClassAffinity
Tier
ArmorProfile
PieceIds[9]
AssociatedWeaponIds
ThreePieceEffect
SixPieceEffect
NinePieceEffect
RaceVariantData[6]
LootSources
CraftingRecipe
```

Chaque piece reste un objet independant. Le gestionnaire d'equipement compte les pieces portant le meme `SetId`, puis applique ou retire les effets de palier sous forme de `URPGStatusEffectAsset` passif.

---

## 14. Ordre de production artistique recommande

1. Produire les six silhouettes de classe au tier 1.
2. Decliner chaque silhouette sur les six morphologies raciales.
3. Produire les tiers 2 a 5 en reutilisant le squelette, les points d'attache et une partie des meshes modulaires.
4. Creer d'abord les vues de face, dos, profil droit et profil gauche de chaque set complet.
5. Isoler ensuite les neuf pieces pour Meshy ou Blender.
6. Terminer par les armes et focaliseurs associes.

Pour limiter la production initiale, la premiere vague peut couvrir les six personnages du groupe de test : Guerrier humain, Pretre nain, Rodeur demi-orc, Voleur halfelin, Mage elfe et Alchimiste gnome.

---

## 15. Decisions a valider avant implementation

- Confirmer si les bonus de set exigent exactement 3/6/9 pieces ou simplement au moins 3/6/9.
- Confirmer si les bijoux participent a un futur palier 10 ou restent toujours independants.
- Fixer les formules definitives d'armure physique et magique avant de figer les valeurs numeriques.
- Decider si les objets s'usent; l'usure n'est pas recommandee pour le prototype v0.1.
- Decider si les variantes raciales conservent une resonance mecanique ou deviennent purement visuelles en mode competitif/equilibrage strict.
