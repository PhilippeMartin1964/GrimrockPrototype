# ARMEMENT01 — Contrat d'armement v0.3

**Date :** 20 octobre 2026  
**Projet :** GrimrockPrototype — Unreal Engine 5.5.4  
**Statut :** PROPOSITION DE CONCEPTION — à valider ; **aucune nouvelle mécanique de dés n'est déclarée implémentée**

> **Clarification ARMEMENT01.10.3.2 (23.10.2026)** — Ce document reste une proposition historique pour ses autres chantiers. Le rôle **actuel** des boucliers est désormais fixé dans [RPG_Shields_Protection_Contract_v0_1.md](../Rules/RPG_Shields_Protection_Contract_v0_1.md) : points d'armure physique consommables, sans blocage aléatoire ni `+Défense` automatique. Les bonus de Défense particuliers exigent une valeur distincte et un effet authoré. Le classeur de production actif est `docs/Design/ARMEMENT01_Catalogue_Armes_Boucliers_Niveaux_1_20_ARMEMENT01_2.xlsx`.  
**Documents liés :** `ARMEMENT01_Catalogue_Armes_Boucliers_Niveaux_1_20_v0_3.xlsx` (classeur de conception détaillé en pièce jointe), `docs/Design/ARMEMENT01_Profils_Des_N1_N20_v0_3.xlsx` (classeur de consultation versionné) et `docs/Design/ARMEMENT01_Profils_Armes_N1_N20_v0_3.csv` (source de revue diffable).

## 1. Autorités et périmètre

Ce contrat complète les autorités existantes sans les remplacer : `UGridItemDefinitionAsset`, `FGridItemInstance`, `FGridAttackDefinition`, `FGridOffensiveEquipmentProfile`, `FGridCombatWeaponAttackProfile`, `FGridCombatResolver` et le contrat validé `docs/Design/RPG03_9_0_WEAPON_BASED_TALENT_ATTACKS.md`.

- **82 archétypes**, **8 familles** conservées : légères, une main, deux mains, armes d'hast, distance, jet, focaliseurs, boucliers.
- **6 classes** : Guerrier, Voleur, Rôdeur, Mage, Prêtre, Alchimiste ; affinités par équipement plutôt qu'interdictions arbitraires. Les tags et talents canoniques restent les conditions métier.
- Une *version de butin* combine un archétype et un **niveau d'objet fixe de 1 à 20**. La rareté et les améliorations sont indépendantes du niveau.
- Ce contrat est un **travail de game design et d'authoring**. Il n'autorise pas la création de règles C++ parallèles, ni la modification implicite des comportements déjà validés.

## 2. ARM-001 — Niveau de l'objet immuable

`ItemLevel ∈ [1,20]` est fixé à l'obtention/création de l'instance et sauvegardé sans possibilité de mutation ordinaire. Une dague niveau 6 demeure niveau 6 après affûtage, trempe, reforge, rune ou alchimie. Pour obtenir la même famille niveau 7, le joueur doit récupérer/fabriquer une **nouvelle instance de niveau 7**, pas améliorer l'ancienne.

`ForgeRank` (qualité de finition éventuelle, bornée) ne représente **jamais** `ItemLevel`. Le niveau du personnage varie normalement, ce qui peut faire disparaître les malus de maniement sans que l'objet change.

## 3. ARM-002 — Dés RPG réels pour les dégâts

### 3.1 Dés autorisés et syntaxe

- **Dés de dégâts exclusivement** : `d4`, `d6`, `d8`, `d10`, `d12`, `d20`. Pour l'armement, aucun dé `d3`, `d5`, `d7`, `d15`, `d100`, etc.
- Notation canonique d'un *profil homogène* : **`NdM + K`**, avec `N ≥ 1` dés indépendants à `M` faces dans la liste autorisée et un bonus entier `K ≥ 0` dans ce catalogue. Exemple `2d8 + 4` = deux lancers de d8 distincts, additionnés, puis +4.
- Des effets distincts (rune de feu, poison) peuvent plus tard posséder leurs **propres jets identifiés**, mais ils ne doivent pas être fusionnés implicitement avec le jet physique de base ; les règles de cumul et critique de chaque effet devront être explicitées.
- La liste des dés n'implique **aucun changement aux tests de toucher** (`d20`) déjà gérés par le moteur. Le `d20` est **aussi autorisé comme dé de dégâts** dans certains profils rares, sans remplacer le d20 de précision.

### 3.2 Calculs et distribution

Pour `N dM + K`, poser `r_i ∈ {1,...,M}` et calculer :

```
JetDégâtsBase = K + Σ(i=1..N) r_i
Minimum = N + K
Maximum = N*M + K
Moyenne = N*(M+1)/2 + K
```

**Exemple** : `2d8 + 4` => minimum **6**, maximum **20**, moyenne **13**. Le résultat 13 est plus probable que 6 ou 20 : ce n'est **pas** `RandRange(6,20)`.

**Exemple de vérification** : si les deux dés font 3 et 7, le jet de base vaut `3 + 7 + 4 = 14`. Les dégâts effectifs restent soumis aux mécaniques de précision, WD, attributs, talents, critiques, résistances et armures.

**Important** : les exemples de l'ancienne v0.2 `1d5 + 2` et `1d15 + 2` étaient mathématiquement valides comme *notation d'une plage uniforme*, mais **ne sont plus des profils de dégâts autorisés**. L'ancien `3–7` ne peut pas être reproduit à l'identique par un seul dé canonique sans concession sur le minimum, le maximum ou la distribution. Les valeurs v0.3 sont donc **réauthorées** plutôt que renommées.

### 3.3 Ordre de résolution et traces

Le profil de dégâts de l'arme doit produire **un unique jet de dégâts** (somme de dés). Les étapes validées RPG03.9.0 subsistent :

1. Jet d'attaque / réussite (`d20` historique), puis jet des dés de dégâts si applicable ;
2. Bonus plat de l'arme, modificateur d'attribut ;
3. Malus de maîtrise (si niveau objet supérieur ; proposition, cf. §5), avant coefficient WD ;
4. Coefficient WD du talent (`WeaponDamagePercent`), arrondi entier selon contrat existant ;
5. Critiques, modificateurs contextuels, résistances, armures et PV selon les autorités existantes.

Le `K` des dés est un **bonus de base inclus dans la formule** ; il ne doit **pas** être réappliqué dans `FlatDamageBonus`. Les améliorations de forge et d'autres bonus plats s'ajoutent distinctement à la résolution, à un emplacement documenté unique. Une trace de combat doit conserver `DiceCount`, `DiceSides`, `IndividualRolls`, `DiceFlatBonus`, `RollTotal` et les étapes modifiantes pour affichage et tests reproductibles.

La **règle actuelle** du moteur `FGridCombatResolver::ResolveAttack` tire `RandRange(MinDamage,MaxDamage)` (plage uniforme). Elle n'implémente **pas** une somme de plusieurs dés. `MinDamage` et `MaxDamage` restent utiles comme projections dérivées pour validation, bornes, HUD et compatibilité, mais ne deviennent pas une deuxième autorité de tirage. **Il faudra implémenter un profil de dés data-driven, puis une migration contrôlée des profils**.

### 3.4 Profils spéciaux

- **Boucliers** : armor physique/magique **fixe** ; jamais de dés de blocage automatiques. Un **coup de bouclier** est une **action d'attaque distincte**, avec son propre profil de dés si autorisée par un talent et les exigences d'équipement.
- **Orbes et grimoires non offensifs** : pas de dégâts d'arme, pas de `0d0`, pas de dés inventés ; les sorts sont un autre profil de combat.
- **Armes de jet/récupérables** : dé de dégâts de l'attaque lancé une fois ; le projectile récupérable ne fait pas relancer les dégâts par le pickup.

## 4. ARM-003 — Construction des versions niveau 1–20

Le niveau d'objet sélectionne **un profil de dés fixé à la création** de l'objet. Le catalogue v0.3 fournit **82 × 20 = 1 640 versions**, incluant les focaliseurs et boucliers sans jet offensif ; **1 380 profils offensifs** sont dotés d'une expression valide en dés. Le niveau ne progresse pas sur une instance.

La courbe indicative historique de DoS2 (facteur `F(L)=1+0,11(L-1)+0,016(L-1)^2`) reste une **cible de moyenne / équilibre**, pas une formule qui doit être appliquée aux dés en runtime. Lors de l'authoring, un ensemble `(N,M,K)` autorisé est choisi pour rapprocher les extrêmes et la moyenne de la cible, tout en conservant l'identité de la famille. Modifier les coefficients ne recalculera donc pas automatiquement le type de dé ; il faut **réauthorer explicitement** les profils.

**Diversité tactique** : deux armes de même moyenne peuvent différer en dispersion. Une arme à `1d12` est plus erratique que `2d6`, tandis qu'une arme à `2d8 +4` privilégie le centre de son intervalle. Cette dispersion fait partie de l'identité du type d'arme, avec les coûts en PA, portée, précision, critique, dégâts physiques et talents.

**Exemples illustratifs, non niveaux prédéfinis** :

| Notation | Minimum | Maximum | Moyenne | Distribution |
|---|---:|---:|---:|---|
| `1d4 + 1` | 2 | 5 | 3,5 | uniforme |
| `1d8 + 2` | 3 | 10 | 6,5 | uniforme |
| `2d6 + 3` | 5 | 15 | 10 | centrale |
| `2d8 + 4` | 6 | 20 | 13 | centrale |
| `1d20 + 5` | 6 | 25 | 15,5 | uniforme, très dispersée |

## 5. ARM-004 — Maniement d'un équipement de niveau supérieur (proposition à valider)

`écart = max(0, ItemLevel − CharacterLevel)`.

| Écart | Précision | Dégâts de l'arme / protection du bouclier | État proposé |
|---:|---:|---:|---|
| 0 | 0 | 100 % | normal |
| 1 | −2 | 90 % | maniement difficile |
| 2 | −4 | 80 % | maniement difficile |
| 3 | −6 | 70 % | maniement difficile |
| >3 | — | — | non équipable (**à valider**) |

La pénalité de dégâts doit agir **après le jet complet `NdM+K` et les bonus de l'arme**, avant le coefficient WD et le critique, sans changer la formule visible ni le niveau de l'objet. La pénalité défensive affecte la contribution du bouclier, **pas toute l'armure du personnage**. On recalcule au changement de niveau du personnage. Les affixes/runes ne doivent pas contourner ces pénalités.

**Ces valeurs ne sont pas approuvées définitivement.** Le plafond `écart=3` et les pourcentages doivent être confirmés avant implémentation.

## 6. ARM-005 — Raretés, palette et affixes

| Ordre | Rareté | Couleur | Hex | Affixes indicatifs | Chasses indicatives |
|---:|---|---|---|---:|---:|
| 1 | Commun | gris | `#92999F` | 0 | 0 |
| 2 | Rare | vert | `#42A75B` | 1 | 1 |
| 3 | Épique | bleu | `#368DDF` | 2 | 1 |
| 4 | Légendaire | violet | `#9B59D0` | 3 | 2 |
| 5 | Unique | doré | `#D4A23A` | propriétés fixes | par objet |

**Pas de multiplicateur automatique de dégâts fondé seulement sur la rareté.** La rareté gouverne les affixes/chasses/propriétés : elle est **indépendante** du niveau et du profil de dés. `Unique` signifie identité et propriété narrative fixes, pas obligatoirement plus puissant qu'un Légendaire de niveau supérieur. UMG utilise ces couleurs pour texte/bordure, sans coder la logique de rareté dans le widget.

## 7. ARM-006 — Forge, alchimie et runes

- **Forge :** bonus plats, précision, renforcement ou chasses explicitement autorisées, avec plafonds et exclusivités ; **jamais de changement de `ItemLevel`**. `FOR02` (+1 dégâts plats) et `FOR03` (+2 remplaçant la trempe simple) ne donnent pas un dé supplémentaire et **ne changent pas K d'origine** : ce sont des modificateurs séparés et traçables.
- **Alchimie :** enduit temporaire (huile tranchante, perforante, poison, feu, givre), durée ou nombre de charges ; une huile active à la fois ; pas de montée de niveau. Effet de statut/élément clairement séparé du jet initial.
- **Runes :** chasses limitées selon la rareté ; effets persistants, résistance, statut ou dégâts supplémentaires identifiés. Une rune ne peut pas silently remplacer `NdM+K` ni donner un nouveau profil de combat caché.
- **Objets uniques :** recettes spécifiques possibles, niveau et identité immuables.

## 8. ARM-007 — Modèle de données proposé (non implémenté)

- Définition (ou profil d'archétype) : `ArchetypeId`, familles, tags, affinités de classe, coût PA, portée, attribut de scaling, profils de dégâts par niveau, identité visuelle.
- Instance persistante : `RuntimeObjectId`, `ItemDefinitionId`, `ItemLevel`, `Rarity`, `AffixIds`, `ForgeRank`, `RuneIds`, huile et charges. Toutes les mutations restent sous l'autorité C++ et sont restaurées sans reroll à la sauvegarde.
- Profil offensif : **`DiceCount` (>=1), `DiceSides` (dans {4,6,8,10,12,20}), `DiceFlatBonus`**, `DamageType`, `PhysicalSubtype`, `AccuracyBonus`, attribut, portée. `MinDamage`/`MaxDamage` sont calculés/dérivés et peuvent être projetés pour compatibilité.
- Résolveur unique : sommer les résultats indépendants des dés, conserver la trace RNG, puis réutiliser l'ordre RPG03.9.0. Ne pas ajouter un résolveur de dégâts dans les Blueprints, tooltips, actions de talent, ou objets.
- UI : tooltip et examen affichent `2d8 + 4 (6–20)`, rareté, `ItemLevel`, écarts de niveau, amélioration et détails ; combat log peut exposer `[3,7]+4=14`.

## 9. ARM-008 — Critères d'acceptation et jalons

1. 82 familles d'archétypes, 20 versions immuables, aucune mutation de niveau par forge/alchimie/rune/sauvegarde.
2. Toutes les **formules non nulles** ont des faces dans `{4,6,8,10,12,20}`, `N ≥ 1`, `K ≥ 0`, `Min=N+K`, `Max=N*M+K` ; les boucliers et focaliseurs non offensifs restent sans dés.
3. `2d8 + 4` avec lancers `[3,7]` produit 14, sans nouveau tirage ; moyenne 13, bornes 6/20.
4. Le dé multiple donne une distribution non uniforme (ex. le milieu de `2d8` est plus fréquent que les bornes), à RNG fixé pour tests déterministes.
5. Sur un personnage N4, objet N6/N7 : malus proposés (précision −4/−6, dégâts 80/70 %). Aucun malus au niveau requis ; rejets au-delà de l'écart maximal *si cette règle est approuvée*.
6. L'affichage du tooltip, la trace du flux combat et l'examen reposent sur la même formule canonique, sans double bonus de `K`.
7. Les talents WD, la précision d20, le critique, les résistances, l'armure et les dégâts des monstres déjà validés restent testés en régression.
8. Les cinq couleurs, les six classes, les propriétés des runes et la sauvegarde sont testées ; les collisions de propriétés sont bornées.
9. Aucun profil d'attaque de bouclier n'apparaît sans action explicite ; pas de jets de protection aléatoires.
10. Build et automation **uniquement validés après logs utilisateur** via `Scripts/ValidateUE.ps1` ; proposer `Grimrock.RPG.ARMEMENT01` comme nouveau filtre, pas le déclarer passant d'avance.

**Découpage proposé :** ARMEMENT01.1 contrat / jeu de données ; ARMEMENT01.2 schéma de dés et trace ; ARMEMENT01.3 résolution multi-dés C++ et tests ; ARMEMENT01.4 raretés et instances persistées ; ARMEMENT01.5 équilibrage/tooltip et malus ; ARMEMENT01.6 forge/alchimie/runes. Une modification atomique par jalon sur `master`, sans PR ni force-push.

## 10. Références et provenance

- GrimrockPrototype : `docs/Design/RPG03_9_0_WEAPON_BASED_TALENT_ATTACKS.md` ; `Source/GrimrockPrototype/Public/Runtime/Combat/GridCombatTypes.h` ; `Source/GrimrockPrototype/Private/Runtime/Combat/GridCombatResolver.cpp` ; `Source/GrimrockPrototype/Public/Runtime/GridInventoryTypes.h` ; `Source/GrimrockPrototype/Public/Runtime/GridItemDefinitionAsset.h`.
- Inspiration règles de dés : [Les règles de Donjons & Dragons — personnages](https://jackalghul-dnd-system.netlify.app/page2), [Dégâts](https://jackalghul-dnd-system.netlify.app/page1532). Référence d'inspiration uniquement ; les paramètres ARMEMENT01 sont originaux et ne reproduisent pas toutes les règles de D&D.
- Inspiration de progression/loot : *Divinity: Original Sin II* (niveau fixe de butin, diversité du matériel), **pas reprise de ses tables numériques**.

**Fin du contrat proposé v0.3.**