# RPG03 — Synthèse des six classes et des 90 Talents

Date : **6 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Source mécanique autoritaire : `docs/Rules/RPG_Talents_Mechanics_v0_1.md`  
Source progression autoritaire : `docs/Rules/RPG_Class_Progression_1_20_v0_1.md`  
État : **90/90 talents conceptuels authorés ; 6 classes matérialisées ; campagne globale `Grimrock.RPG.RPG03` validée 193/193**

## Règles communes

Chaque classe possède **3 branches × 5 paliers**, aux niveaux **2 / 6 / 10 / 14 / 18**. Chaque nœud coûte 1 Talent Point et les nœuds II→V exigent le nœud précédent de la branche. Les variantes exclusives (spécialisation martiale, affinités, ennemi juré, etc.) peuvent produire plusieurs Choice records techniques tout en comptant comme un seul talent conceptuel.

Les personnages gagnent **10 Talent Points** au total : +1 aux niveaux **2, 4, 6, 8, 10, 12, 14, 16, 18, 20**. Une branche complète coûte 5 points ; au niveau 20, un personnage peut donc compléter deux branches ou répartir ses points.

L'architecture reste data-driven : `URPGClassAsset` porte progression et actions, MON16 porte les Status Effects, le catalogue/TurnManager porte l'exécution, les QuickItems restent des `UGridItemDefinitionAsset`, et le runtime ne branche pas sur les identifiants de talents propres aux classes.

## Guerrier

**Rôle synthétique :** frontline martial : protection du groupe, rupture d'armure et maîtrise des armes.

| Branche | Identité | Niv. 2 | Niv. 6 | Niv. 10 | Niv. 14 | Niv. 18 |
|---|---|---|---|---|---|---|
| Gardien | défense, interception et armure | Posture défensive | Coup de bouclier | Interception | Rempart | Forteresse |
| Brise-ligne | dégâts lourds, brise-armure et exécution | Coup puissant | Brise-armure | Balayage | Exécution | Ravage |
| Maître d'armes | spécialisation, riposte, critique et buff de groupe | Spécialisation martiale | Riposte | Second souffle | Maîtrise critique | Seigneur de guerre |

## Voleur

**Rôle synthétique :** burst positionnel, furtivité, contrôle léger, pièges et interactions techniques.

| Branche | Identité | Niv. 2 | Niv. 6 | Niv. 10 | Niv. 14 | Niv. 18 |
|---|---|---|---|---|---|---|
| Assassin | dégâts conditionnels, arrière, saignement et finisher | Attaque sournoise | Frappe dans le dos | Hémorragie | Point faible | Mise à mort |
| Ombre | évasion, disparition et attaque opportuniste | Esquive | Disparition courte | Pas de l'ombre | Insaisissable | Ombre parfaite |
| Saboteur | pièges, fumée, crochetage et sabotage | Désamorçage expert | Piège rapide | Bombe fumigène | Maître des serrures | Sabotage |

## Rôdeur

**Rôle synthétique :** combat à distance, chasse de cible et utilité d'exploration/mobilité.

| Branche | Identité | Niv. 2 | Niv. 6 | Niv. 10 | Niv. 14 | Niv. 18 |
|---|---|---|---|---|---|---|
| Tireur | précision, perforation, multi-tirs et zone | Tir précis | Tir perforant | Tir rapide | Volée | Œil d'aigle |
| Chasseur | marque, ennemi juré et focus de cible | Marque de la proie | Ennemi juré | Tir immobilisant | Frappe du prédateur | Chasseur alpha |
| Éclaireur | initiative, piège, repli, terrain et soutien d'exploration | Vigilance | Piège de chasse | Repli tactique | Maître du terrain | Guide du groupe |

## Mage

**Rôle synthétique :** puissance élémentaire, magie arcanique et manipulation des surfaces.

| Branche | Identité | Niv. 2 | Niv. 6 | Niv. 10 | Niv. 14 | Niv. 18 |
|---|---|---|---|---|---|---|
| Évocateur | affinité élémentaire, surcharge, chaîne et cataclysme | Affinité élémentaire | Surcharge élémentaire | Explosion contrôlée | Chaîne élémentaire | Cataclysme |
| Arcaniste | bouclier, dissipation, runes, téléportation et maîtrise Arcane | Bouclier arcanique | Dissipation | Manipulation runique | Téléportation courte | Maîtrise de l'Arcane |
| Tisseur de surfaces | imprégnation, conversions et contrôle durable du terrain | Imprégnation | Conversion élémentaire | Conduction | Surface persistante | Architecte du terrain |

## Prêtre

**Rôle synthétique :** soin, protection du groupe et spécialisation anti-Undead/Demon.

| Branche | Identité | Niv. 2 | Niv. 6 | Niv. 10 | Niv. 14 | Niv. 18 |
|---|---|---|---|---|---|---|
| Restauration | soins directs/périodiques, purification et miracle | Soin renforcé | Régénération | Soin de groupe | Purification | Miracle |
| Protection | buffs, armure magique, sanctuaire et mitigation de groupe | Bénédiction | Égide | Protection sacrée | Sanctuaire | Bastion divin |
| Exorcisme | dégâts Holy, repoussement, dispel et bannissement | Lumière sacrée | Repousser les morts-vivants | Dissipation sacrée | Châtiment | Exorcisme majeur |

## Alchimiste

**Rôle synthétique :** bombes, potions et contrôle/transmutation des surfaces.

| Branche | Identité | Niv. 2 | Niv. 6 | Niv. 10 | Niv. 14 | Niv. 18 |
|---|---|---|---|---|---|---|
| Grenadier | bombes Fire/Poison, friendly-fire réduit et réactions de surface | Bombe incendiaire | Bombe toxique | Charge précise | Réaction en chaîne | Maître grenadier |
| Apothicaire | potions positives, antidote, élixirs, diffusion et panacée | Potion renforcée | Antidote | Élixir défensif | Diffusion | Panacée |
| Transmutateur | Oil, corrosion, PoisonCloud, catalyse et conversion de zone | Huile glissante | Flasque acide | Nuage corrosif | Catalyseur | Transmutation majeure |

## Lecture technique rapide

- **Guerrier / Voleur / Rôdeur** : les attaques martiales utilisent les profils d'arme génériques et les filtres data-driven ; aucune seconde résolution d'attaque n'a été créée.
- **Mage / Prêtre** : les actions magiques restent `SourcePolicy=Spell`, ce qui conserve Silence, mana et le spell/combat pipeline existants.
- **Alchimiste** : les bombes/potions/flasques restent de vrais QuickItems et l'inventaire reste l'autorité de quantité.
- **Surfaces** : C6 reste l'autorité unique ; les talents ne créent pas un second état de terrain.
- **Progression** : `FRPGClassProgressionAuthoring::ConfigureCanonicalTalentGrants()` est l'autorité Editor des 10 grants canoniques sur les six classes.

## Frontières restantes

Le volet Talents est matérialisé. Deux frontières ne doivent pas être confondues avec une absence d'authoring des 90 talents :

1. **Crafting/recettes** : la sélection concrète des quatre sorties de Transmutation majeure attend toujours le futur système Crafting ; `BuildMajorTransmutationRecipeAction()` décrit déjà les profils de combat sans inventer un moteur de recettes parallèle.
2. **Validation gameplay exhaustive** : le PIE RPG03.10 valide les six classes, leurs branches et la projection runtime ; il ne remplace pas encore une campagne manuelle/automatisée qui exécute chaque famille UI/hotbar/targeting en situation de jeu.

Pour les coûts, formules, ArmorGates, statuts, surfaces et ChoiceIds exacts, le document mécanique autoritaire reste `RPG_Talents_Mechanics_v0_1.md`.
