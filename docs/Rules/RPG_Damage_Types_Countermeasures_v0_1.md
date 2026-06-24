# RPG Damage Types and Countermeasures v0.1

## Objet

Ce document complete `Docs/Rules/RPG_Core_Rules_v0_1.md` pour definir les types de degats du prototype GrimrockPrototype, leurs familles defensives et les contre-mesures possibles.

Il sert de reference aux DataAssets RPG, aux icones d'interface, aux armes, aux sorts, aux monstres, aux potions, aux resistances et aux effets d'etat.

---

## Couverture du prototype

Les types actuellement retenus couvrent le noyau d'un RPG med-fan classique :

- trois degats physiques : `Tranchant`, `Perforant`, `Contondant` ;
- quatre degats elementaires fondamentaux : `Feu`, `Glace`, `Foudre`, `Poison` ;
- quatre degats elementaires et naturels etendus : `Terre`, `Air`, `Eau`, `Psychique` ;
- trois degats mystiques : `Sacre`, `Necrotique`, `Arcane`.

Pour le prototype v0.1, cette liste est suffisante. Elle permet de couvrir les armes, les sorts, les pieges, les monstres, les surfaces, les resistances et les vulnerabilites.

Types optionnels a garder en reserve :

| Type optionnel | Interet | Recommandation v0.1 |
|---|---|---|
| `Acide` / `Corrosif` | Dissoudre armure, metal, pierre, carapace ; utile pour alchimiste et monstres. | A ajouter si l'on veut le separer clairement du poison. |
| `Sonore` | Cris, cloches, resonance, bris de concentration. | Optionnel ; peut rester dans `Air` au debut. |
| `Force` / `Energie pure` | Projectiles magiques neutres, telekinesie, impacts purs. | Optionnel ; peut rester dans `Arcane` au debut. |
| `Saignement` | Degat sur la duree lie aux plaies ouvertes. | A gerer comme un etat applique par `Tranchant` ou `Perforant`. |
| `Ombre` / `Vide` / `Chaos` | Degats exotiques de fin de campagne. | A reserver aux extensions ou boss majeurs. |

---

## Identifiants recommandes

| Nom UI | Identifiant code | Famille | Icone |
|---|---|---|---|
| Tranchant | `Slashing` | Physique | `damage_slashing.png` |
| Perforant | `Piercing` | Physique | `damage_piercing.png` |
| Contondant | `Bludgeoning` | Physique | `damage_bludgeoning.png` |
| Feu | `Fire` | Elementaire | `damage_fire.png` |
| Glace | `Ice` | Elementaire | `damage_ice.png` |
| Foudre | `Lightning` | Elementaire | `damage_lightning.png` |
| Poison | `Poison` | Toxique | `damage_poison.png` |
| Sacre | `Holy` | Mystique | `damage_holy.png` |
| Necrotique | `Necrotic` | Mystique | `damage_necrotic.png` |
| Arcane | `Arcane` | Mystique | `damage_arcane.png` |
| Terre | `Earth` | Elementaire naturel | `damage_earth.png` |
| Air | `Air` | Elementaire naturel | `damage_air.png` |
| Eau | `Water` | Elementaire naturel | `damage_water.png` |
| Psychique | `Psychic` | Mental | `damage_psychic.png` |

---

## Principes defensifs

Le systeme distingue deux protections principales :

| Protection | Role | Exemples |
|---|---|---|
| Armure physique | Encaisser les coups materiels, projectiles, griffes, morsures, chutes et impacts. | Cuirasse, casque, bouclier, armure de cuir, carapace, peau epaisse. |
| Armure magique | Encaisser les energies elementaires, les maledictions, les effets mentaux et les agressions mystiques. | Robe enchantee, sceau protecteur, barriere magique, amulette, foi, rune. |

Regle de conception :

- un degat physique cible d'abord l'armure physique ;
- un degat elementaire, toxique, mental ou mystique cible d'abord l'armure magique ;
- certains effets hybrides peuvent avoir une part physique et une part magique, mais cela doit rester explicite dans la fiche de l'objet, du sort ou du monstre.

---

## Contre-mesures generales

| Contre-mesure | Fonction | Limite |
|---|---|---|
| Bouclier | Reduit ou annule une attaque venant de face ; excellent contre armes et projectiles physiques. | Peu efficace contre auras, gaz, surfaces, maledictions et attaques mentales, sauf bouclier enchante. |
| Armure physique | Reduit les degats physiques et bloque les etats physiques tant qu'elle tient. | Lourde ; sensible au contournement, a la perforation et a la corrosion si `Acide` est ajoute. |
| Armure magique | Reduit les degats elementaires, toxiques, mystiques et mentaux ; bloque les etats magiques tant qu'elle tient. | Ne remplace pas une armure contre les coups reels. |
| Esquive | Evite totalement une attaque ciblee. | Ne protege pas contre zone, surface ou aura deja active. |
| Resistance | Reduit un type de degat en pourcentage. | Specialisee ; une forte resistance Feu ne protege pas contre Foudre. |
| Potion | Protection temporaire, soin, antidote, dissipation ou resistance ciblee. | Ressource consommable ; demande preparation et emplacements rapides. |
| Sort defensif | Barriere, benediction, purification, protection elementaire, contresort. | Cout en mana, cooldown ou temps d'incantation. |
| Positionnement | Eviter une ligne de tir, sortir d'une surface, placer le guerrier devant. | Depend de la grille, de l'initiative et de l'espace disponible. |

Ordre de resolution conseille :

1. verifier l'immunite ou la vulnerabilite speciale ;
2. verifier l'esquive, la parade ou le bouclier si l'attaque le permet ;
3. appliquer l'armure ciblee, physique ou magique ;
4. appliquer la resistance du type de degat ;
5. appliquer les reductions temporaires, barrieres et sorts actifs ;
6. retirer les PV restants ;
7. tenter l'application des etats si les conditions sont remplies.

---

## Tableau des types de degats et contre-mesures

| Type | Protection principale | Bouclier | Armure physique | Armure magique | Potions | Sorts et capacites | Resistances et remarques |
|---|---|---|---|---|---|---|---|
| Tranchant | Armure physique | Tres efficace de face contre epees, haches et griffes. | Tres efficace ; mailles, cuir epais et plaques limitent les plaies. | Faible, sauf effet enchante. | Soin, bandages, potion de regeneration. | Parade, peau de pierre, protection contre les armes. | Resistance Tranchant ; peut appliquer Saignement si l'armure physique est brisee. |
| Perforant | Armure physique | Efficace contre lances et fleches de face, moins contre tirs precis ou attaques de flanc. | Moyenne a forte ; plaques et mailles aident, mais certaines pointes peuvent ignorer une partie de l'armure. | Faible, sauf projectile magique. | Soin, antidouleur, potion de vigueur. | Deflection de projectiles, brouillard, peau de pierre. | Resistance Perforant ; bon contre cibles legeres, faible contre boucliers lourds. |
| Contondant | Armure physique | Efficace mais peut etre secoue par les masses lourdes. | Forte contre petits impacts, moyenne contre marteaux, masses et chutes. | Faible, sauf onde magique. | Soin, potion de vigueur, potion anti-etourdissement. | Stabilite, ancrage, peau de pierre. | Resistance Contondant ; peut appliquer Etourdi, Renverse ou Brise-garde. |
| Feu | Armure magique | Faible contre flammes et surfaces ; possible contre projectile de feu enchante. | Faible, sauf armure ignifuge. | Tres efficace. | Potion de resistance au feu, potion de soin, onguent anti-brulure. | Barriere de feu, pluie, purification, protection elementaire. | Resistance Feu ; peut appliquer Brule et enflammer huile, poison ou objets combustibles. |
| Glace | Armure magique | Faible, sauf projectile de glace direct. | Faible a moyenne contre eclats physiques de glace. | Tres efficace. | Potion de resistance au froid, potion de chaleur. | Barriere de froid, rechauffement, dissipation. | Resistance Glace ; peut appliquer Ralenti, Gele ou Fragile. |
| Foudre | Armure magique | Faible ; bouclier metallique peut meme etre dangereux si la regle de conduction est activee. | Faible, sauf isolation speciale. | Tres efficace. | Potion de resistance a la foudre, potion de concentration. | Isolation, mise a la terre, contresort, protection elementaire. | Resistance Foudre ; interagit avec eau, metal et machines ; peut appliquer Choc ou Etourdi. |
| Poison | Armure magique | Inefficace contre gaz, nuage et contact ; utile contre fleche ou lame empoisonnee pour la part physique. | Faible contre toxines ; utile seulement contre la blessure d'entree. | Moyenne a forte si le poison est magique ou alchimique. | Antidote, potion de resistance au poison, potion de purification. | Purification, neutralisation des toxines, soin continu. | Resistance Poison ; peut appliquer Empoisonne et affaiblir Constitution. |
| Sacre | Armure magique | Faible, sauf bouclier beni. | Faible. | Tres efficace. | Potion de protection sacree, eau benite, potion de soin. | Benediction, sanctuaire, aura de foi. | Resistance Sacre ; souvent plus fort contre morts-vivants, demons et creatures maudites. |
| Necrotique | Armure magique | Faible, sauf bouclier consacre. | Faible. | Tres efficace. | Potion de purification, elixir de vitalite, antidote maudit. | Benediction, protection contre la mort, dissipation de malediction. | Resistance Necrotique ; peut drainer PV, reduire soins ou appliquer Maudit. |
| Arcane | Armure magique | Faible, sauf bouclier runique. | Faible. | Tres efficace. | Potion de resistance magique, potion de mana, potion de dissipation. | Contresort, barriere runique, dissipation. | Resistance Arcane ; type neutre pour magie pure, runes, force magique et artefacts. |
| Terre | Armure magique par defaut ; armure physique pour rochers reels. | Moyenne contre projectile de pierre, faible contre secousse ou piege de sol. | Moyenne si l'effet est un impact materiel. | Forte si l'effet est magique ou tellurique. | Potion de resistance a la terre, potion de stabilite. | Leviation courte, ancrage, peau de pierre, protection elementaire. | Resistance Terre ; peut appliquer Entrave, Renverse ou Immobilise. |
| Air | Armure magique | Faible contre bourrasques ; moyenne contre lame d'air ciblee. | Faible, sauf debris projetes. | Tres efficace. | Potion de resistance a l'air, potion d'equilibre. | Ancrage, silence du vent, barriere elementaire. | Resistance Air ; peut pousser, desorienter, interrompre ou affecter les creatures volantes. |
| Eau | Armure magique | Faible contre pression, vague ou surface ; moyenne contre projectile liquide cible. | Faible, sauf impact physique d'eau ou de glace. | Tres efficace. | Potion de resistance a l'eau, potion de respiration, potion de chaleur. | Dessiccation, barriere elementaire, marche sur l'eau, purification. | Resistance Eau ; interagit avec Foudre, Feu, Glace, surfaces humides et noyade. |
| Psychique | Armure magique | Inefficace, sauf bouclier mental ou relique specifique. | Inefficace. | Tres efficace. | Potion de lucidite, potion de volonte, remede contre confusion. | Protection mentale, silence interieur, dissipation, aura de courage. | Resistance Psychique ; peut appliquer Confusion, Peur, Charme, Sommeil ou perte de concentration. |

---

## Etats associes aux familles de degats

| Famille | Etats typiques | Protection qui bloque l'etat |
|---|---|---|
| Physique | Saignement, Renverse, Etourdi, Brise-garde, Entrave physique. | Armure physique. |
| Elementaire | Brule, Gele, Ralenti, Choc, Mouille, Entrave naturelle. | Armure magique, sauf impact physique explicite. |
| Toxique | Empoisonne, Affaibli, Nausee, Maladie. | Armure magique ou resistance Poison. |
| Mystique | Maudit, Silence, Drain, Vulnerable, Dissipation. | Armure magique. |
| Mental | Confusion, Peur, Charme, Sommeil, Perte de concentration. | Armure magique et Volonte. |

Regle recommandee : un etat dangereux ne devrait pas s'appliquer tant que la protection correspondante n'est pas brisee ou ignoree par une capacite explicite.

---

## Recommandations pour l'equipement

### Boucliers

Un bouclier doit etre excellent contre les attaques physiques venant de face :

- `Tranchant` : blocage eleve ;
- `Perforant` : blocage eleve, sauf tir precis, attaque de flanc ou capacite perce-bouclier ;
- `Contondant` : blocage moyen a eleve, mais risque de recul ou d'etourdissement ;
- degats elementaires : blocage faible, sauf bouclier enchante ;
- degats mentaux et maledictions : aucun effet sans propriete speciale.

| Bouclier | Role |
|---|---|
| Rondache | Defense legere, bonne contre attaques rapides. |
| Ecu | Defense polyvalente. |
| Pavois | Excellente defense frontale contre projectiles et lances. |
| Bouclier runique | Protection secondaire contre Arcane, Foudre ou Psychique. |
| Bouclier beni | Protection contre Necrotique et maledictions. |

### Armures

| Armure | Force | Faiblesse |
|---|---|---|
| Tissu enchante | Armure magique, mana, resistances. | Faible contre degats physiques. |
| Cuir | Mobilite, poison, perforant leger. | Moins bon contre contondant lourd. |
| Mailles | Bon contre tranchant et perforant. | Moins bon contre contondant et foudre si conduction activee. |
| Plaques | Tres bon contre tranchant, perforant et impacts moderes. | Lourde, sensible a chaleur, foudre et attaques contournant l'armure. |
| Armure runique | Equilibre physique/magique. | Rare, couteuse, potentiellement specialisee. |

### Potions

| Potion | Effet |
|---|---|
| Soin | Rend des PV. |
| Regeneration | Rend des PV sur plusieurs tours/actions. |
| Antidote | Retire ou reduit Poison. |
| Purification | Retire Poison, Brule, Maudit ou Necrotique leger selon puissance. |
| Resistance elementaire | Donne une resistance temporaire a un type : Feu, Glace, Foudre, Terre, Air, Eau. |
| Resistance magique | Reduit Arcane, Sacre, Necrotique et Psychique de facon moderee. |
| Lucidite | Protege contre Psychique, Confusion, Charme et Peur. |
| Vigueur | Aide contre Contondant, Etourdi, Renverse et fatigue. |

### Sorts defensifs

| Sort | Role |
|---|---|
| Barriere physique | Renforce armure physique temporairement. |
| Barriere magique | Renforce armure magique temporairement. |
| Protection elementaire | Augmente une resistance choisie. |
| Benediction | Protege contre Necrotique, Maudit et Peur. |
| Purification | Retire poison, malediction ou brulure. |
| Contresort | Annule ou reduit une attaque magique entrante. |
| Peau de pierre | Reduit Tranchant, Perforant, Contondant et Terre physique. |
| Protection mentale | Reduit Psychique et les etats mentaux. |

---

## Donnees recommandees

Chaque source de degat devrait exposer au minimum :

```text
DamageType
BaseDamage
DefenseTarget        // PhysicalArmor, MagicalArmor, Hybrid, DirectHP
ResistanceTag
CanBeBlockedByShield
CanBeDodged
StatusEffects
SurfaceInteraction
ArmorIgnorePercent
```

Regles d'equilibrage v0.1 :

- garder les resistances ordinaires entre `0 %` et `25 %` ;
- reserver `50 %` et plus aux monstres specialises ou boss ;
- utiliser les vulnerabilites avec parcimonie, typiquement `-25 %` ;
- eviter les immunites totales sauf pour les creatures elementaires ou morts-vivants tres typiques ;
- ne pas multiplier les types hybrides avant d'avoir teste le rythme des combats.
