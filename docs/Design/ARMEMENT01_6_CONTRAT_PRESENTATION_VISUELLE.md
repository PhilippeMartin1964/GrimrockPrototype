# ARMEMENT01.6 — Contrat de présentation visuelle de l'armement

**Projet :** GrimrockPrototype — Unreal Engine 5.5.4  
**Date :** 20 octobre 2026  
**Version :** 1.0 — spécification de conception  
**Statut :** PROPOSITION À VALIDER. Document uniquement ; ARMEMENT01.6 n'est pas implémenté.  
**Référence :** TD04.2 du 20.10.2026 à 17 h 58 : 1 471 tests Grimrock réussis, 0 avertissement, 0 échec, exit 0.

> **Principe directeur :** les 82 archétypes et leurs 20 niveaux forment 1 640 profils **statistiques**, pas 1 640 assets visuels. Un jeu limité de silhouettes de base doit être réutilisable, avec variations cosmétiques et informations affichées par l'UI.

## 1. Autorités déjà actives et règles de compatibilité

- **ARMEMENT01.4 :** un catalogue de combat unique, DA_WeaponCatalogue, 82 archétypes x 20 niveaux ; le classeur Excel à 12 onglets reste la source éditoriale des statistiques.
- **ARMEMENT01.5 :** FGridItemInstance stocke ItemDefinitionId, RuntimeObjectId et ItemLevel (0 = Legacy, 1–20 = fixe) ; ses données circulent dans FGridPartyInventoryState et les sauvegardes. Le niveau est indépendant de celui du personnage.
- **Collectibles MIG10 :** UGridItemDefinitionAsset est la définition unique de l'item. Elle expose déjà Icon, WorldMesh, EquippedMesh, bEnableWorldSparkle, ainsi que les paramètres d'attaque et de lancer. Aucun world-object definition compagnon ne doit être recréé.
- **Monde :** AGridItemActor utilise actuellement WorldMesh de la définition ; UGridEditorPreviewComponent fait de même. FGridLooseItemInstance (placement direct dans le niveau) contient une ItemDefinition et une quantité, mais **ne contient pas encore ItemLevel**. Une preview de définition ne connaît pas le niveau réel d'un objet instancié.
- **Matériaux :** MATERIAL-OWNERSHIP01 conserve les **Material Slots du StaticMesh** comme autorité de rendu principal. Ne pas réintroduire d'override de matériau dans GridWorldObjectDefinitionAsset ou l'asset de niveau.
- **Inventaire :** UGridInventorySlotWidget et son GetIconTexture, GetTooltipView restent les points de lecture ; WBP_ItemTooltip est le seul tooltip.
- **Barre d'action :** le premier raccourci est **déjà** l'alias système PrimaryAttack. Si MainHand fournit une attaque, il affiche son icône et son action ; sinon Attack_Unarmed/poing. Ne pas remplacer ce binding ni supprimer la possibilité de combat à mains nues.
- **Présentation MON11.4 :** une arme non lumineuse n'est pas tenue en permanence devant la caméra ; EquippedMesh demeure disponible pour les cas explicites et les projectiles, sans changer cette règle.

**Non-objectifs :** pas de changements à FGridCombatResolver, au NdM+K, aux statistiques, à la sauvegarde du niveau, à la physique des pickups, à la génération de butin ou à la création de personnage. Pas de refonte du tooltip, pas de rareté persistante, pas de forge/runes/affixes/pénalités de niveau dans ARMEMENT01.6.

## 2. Modèle de présentation : silhouette réutilisable

### 2.1 Séparation des responsabilités

~~~text
FGridItemInstance(ItemDefinitionId, ItemLevel, RuntimeObjectId)
  |
  v
UGridItemDefinitionAsset (définition collectible unique)
  |-- WeaponArchetypeId + ItemLevel --> DA_WeaponCatalogue --> combat
  |
  |-- opt-in visuel --> DA_WeaponVisualCatalogue
  |                          |-- ArchetypeId -> VisualId (mapping)
  |                          '-- VisualId -> Icon / WorldMesh / EquippedMesh / décor
  |
  '-- Icon / WorldMesh / EquippedMesh existants (fallback Legacy)
                              |
                              v
                  Résolveur C++ en lecture seule
                     |         |           |
                  Inventaire  Pickup    Preview/hotbar
~~~

Un changement de texture, de mesh ou de décoration ne modifie jamais les profils NdM+K ; aucune ressource d'armement n'est placée dans une deuxième autorité de niveau.

### 2.2 DataAsset et structures cibles (à implémenter)

Créer UGridWeaponVisualCatalogueAsset, UPrimaryDataAsset unique :

**/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue**

Contenu :

1. **ArchetypeBindings[]**, type FGridWeaponVisualArchetypeBinding : ArchetypeId et VisualId. Une seule liaison par archétype activé ; à terme, couverture de ARM001 à ARM082.
2. **VisualProfiles[]**, type FGridWeaponVisualProfile : VisualId, FamilyTag, BaseIcon (TSoftObjectPtr<UTexture2D>), WorldMesh (TSoftObjectPtr<UStaticMesh>), EquippedMesh facultatif (TSoftObjectPtr<UStaticMesh>), WorldVisualTransform et facultativement OrnamentMesh + OrnamentRelativeTransform.
3. **FGridResolvedWeaponVisual**, structure **transiente** de projection : Icon, WorldMesh, EquippedMesh, ornement éventuel, état de résolution et **niveau UI lu de l'instance** ; elle ne doit pas être sauvegardée.
4. **EGridWeaponVisualResolutionError** : erreurs stables (catalogue absent, binding absent, VisualId inconnu, ressource manquante, contexte d'instance indisponible).

Mapping **plusieurs archétypes → une silhouette**. Une silhouette peut servir à 20 niveaux et à plusieurs archétypes ; aucune entrée VisualProfile n'est dupliquée par ItemLevel.

Dans UGridItemDefinitionAsset, ajouter seulement :
- bUseWeaponVisualCatalogue = false (opt-in Legacy sûr) ;
- TSoftObjectPtr<UGridWeaponVisualCatalogueAsset> WeaponVisualCatalogue.

Réutiliser **WeaponArchetypeId** d'ARMEMENT01.5 au lieu de multiplier les IDs sur l'instance. Aucun nouveau VisualId sérialisé dans FGridItemInstance.

### 2.3 Contrat du résolveur C++ (signature indicative)

~~~cpp
bool FGridWeaponVisualResolver::TryResolve(
    const UGridItemDefinitionAsset& Definition,
    const FGridItemInstance* OptionalInstance,
    FGridResolvedWeaponVisual& OutVisual,
    EGridWeaponVisualResolutionError& OutError);
~~~

- Avec opt-in **inactif** : restituer exactement Icon, WorldMesh et EquippedMesh existants.
- Avec opt-in **actif** : résoudre WeaponArchetypeId → VisualId → VisualProfile, sans autre calcul métier.
- L'instance réelle est **obligatoire** pour montrer son niveau exact dans inventaire, équipement et curseur ; OptionalInstance nul signifie « aperçu de définition » sans badge de niveau inventé.
- Si une référence visuelle manque : diagnostiquer clairement, puis repli explicite sur les champs visuels Legacy de la définition, et enfin sur un placeholder neutre UI si disponible. Ne jamais emprunter silencieusement la silhouette d'un autre archétype.
- **Ne jamais** transformer un échec de résolution visuelle en fallback de dégâts Legacy ; les règles ARMEMENT01.5 gardent leur autorité.
- Résolution déterministe : aucun tirage aléatoire de mesh à chaque rafraîchissement, aucune mutation d'instance, d'équipement ou de sauvegarde.
- Éviter LoadSynchronous à chaque Tick / frame UMG : charger et conserver les assets seulement aux moments de présentation appropriés. Pas d'acteur dynamique distinct pour chaque variation.
- L'ornement est purement visuel : sans collision, masse, gameplay, HitResult ou nouveau canal de lumière obligatoire.

## 3. Niveau, rareté et décoration : conventions distinctes

### 3.1 Niveau fixe (déjà effectif)

ItemLevel 1–20 choisit les **statistiques**, pas une nouvelle texture. Par défaut, les 20 niveaux d'un archétype partagent les mêmes Icon et WorldMesh.

L'UI peut afficher un badge discret **N7**, N12, etc. (texte UMG superposé), mais jamais le bake dans le PNG. Niveau 0 Legacy = badge absent. Le niveau ne doit pas colorer le contour avec une couleur de rareté.

### 3.2 Rareté (palette réservée ; système non implémenté)

La palette issue du contrat ARMEMENT01 v0.3 est maintenue **sans l'activer prématurément** :

| Rareté | Hex | Signal futur |
|---|---|---|
| Commun | #92999F | Gris |
| Rare | #42A75B | Vert |
| Épique | #368DDF | Bleu |
| Légendaire | #9B59D0 | Violet |
| Unique | #D4A23A | Doré |

**Important :** FGridItemInstance ne contient pas encore de Rarity autoritaire. Aucune valeur de rareté ne peut être dérivée de ItemLevel, des dés, de la couleur du mesh ou du nom. Le cadre reste neutre jusqu'à la mise en place d'un système de rareté approuvé.

### 3.3 Finitions et ornements

Composer éventuellement une apparence à partir de : **silhouette de base + finition cosmétique authorée + ornement facultatif + présentation UI**.

- Préférence initiale : l'icône et le mesh neutres restent stables, badge de niveau généré par UMG.
- Une finition (acier patiné, bois, cuir, gravure, gemme) n'a **pas** de bonus RPG.
- Le matériau principal appartient au mesh via ses Material Slots. Si une variante de matériau est nécessaire, produire une variante de mesh/asset authorée cohérente avec MATERIAL-OWNERSHIP01 ; ne pas modifier en douce les slots d'un collectible Legacy.
- Un décor supplémentaire peut être un StaticMesh enfant du mesh de base, avec transform local connu, non-collisionnant et sans nouvelle physique.
- Les VFX/sparkles utilisent le mécanisme d'overlay déjà présent, uniquement sur opt-in artistique explicite ; **aucun glow automatique car l'arme est niveau 20**.
- Une éventuelle finition par bandes de niveau 1–5 / 6–10 / 11–15 / 16–20 reste **une idée à valider**, pas une règle active de ce jalon, et ne constitue jamais de la rareté.

## 4. Bibliothèque visuelle : budget et tranche pilote

### 4.1 Huit familles de gameplay

Le catalogue comporte **8 familles** : légères, une main, deux mains, armes d'hast, distance, jet, focaliseurs et boucliers.

| Famille | Objectif indicatif de silhouettes |
|---|---:|
| Légères | 4–6 |
| Une main | 6–8 |
| Deux mains | 4–6 |
| Hast | 3–5 |
| Distance | 3–5 |
| Jet | 2–4 |
| Focaliseurs | 3–4 |
| Boucliers | 3–5 |
| **Total indicatif** | **28–41** |

Ces chiffres ne sont **pas un inventaire des 82 noms**. La correspondance réelle ArchetypeId → VisualId doit être auditée sur le classeur avant d'acheter, dessiner ou modéliser. Une vraie silhouette particulière mérite son propre VisualId ; il ne faut pas rendre arc et épée identiques pour respecter un quota.

**Cible :** trois silhouettes d'abord, huit silhouettes de référence ensuite, puis extension progressive vers environ 30–40 selon les besoins réels, avec leurs équivalents iconographiques. **Aucune obligation de 1 640 meshes ou icônes.**

### 4.2 Tranche verticale pilote

1. **ARM001** : archétype offensif léger. Confirmer son libellé de production avant le dessin.
2. **Une arme à distance** : choisir son ArchetypeId réel dans le catalogue, ne pas inventer un ID.
3. **Un profil passif parmi ARM070..ARM082** : confirmer dans le catalogue s'il s'agit d'un bouclier ou d'un focaliseur.

Livrables pilotes : trois icônes et trois StaticMeshes (monde), complétés seulement si nécessaire par EquippedMesh et un décor non-collisionnant. Tester mêlée, attaque distance/lancer, passif sans attaque.

### 4.3 Nommage et répertoires proposés

~~~text
Content/GrimrockPrototype/
  Core/DataAssets/Weapons/
    DA_WeaponCatalogue                   [existant]
    DA_WeaponVisualCatalogue             [nouveau]
  UI/Icons/Weapons/
    T_Weapon_...                          [silhouette RGBA]
  Meshes/Weapons/
    SM_Weapon_...                         [silhouette monde]
    Ornaments/SM_WeaponOrnament_...       [optionnel]
  Materials/Weapons/
    MI_Weapon_...                         [slots authorés, sans override générique]
~~~

Le nom technique reste en anglais, les noms et descriptifs affichés au joueur **en français**, conformément au guide de création d'items.

### 4.4 Spécification des icônes

- Master recommandé : **RGBA 512×512** ; export UI recommandé **256×256**. Une seule silhouette reconnaissable, centrée, marge constante, fond entièrement transparent.
- Éclairage et angle constants par famille, style fantasy médiéval sombre ; éviter le cartoon, les cadres ou les chiffres intégrés dans la texture.
- Vérifier dans Unreal les options d'alpha, compression UI et sRGB. Le jeu affiche actuellement des icônes **48×48** : une contrainte carrée doit préserver le ratio 1:1, sans étirement.
- Une seule source Icon pour inventaire, équipement et icône du PrimaryAttack. Les badges Nxx et futures bordures de rareté appartiennent à UMG, pas au PNG.
- Prévoir la lisibilité avec le fond sombre du jeu et sans différenciation de couleur.

### 4.5 Spécification des meshes

- Blender → StaticMesh UE5.5.4 ; conserver pivots/échelles et Material Slots ; vérifier collision et dimensions en cellule réelle de 200×200×300 cm.
- Contrôler sol, alcôve, réceptacle, preview, récupération et lancer ; aucun mesh différent uniquement parce que le niveau change.
- EquippedMesh facultatif : fallback WorldMesh ; ne pas remettre les armes non lumineuses devant la caméra en permanence.
- Les ornements ne modifient pas les collisions, les masses physiques, les transforms de placement logique ou la trajectoire des projectiles.
- Conserver AGridItemActor comme acteur runtime commun, ne pas créer BP_Weapon_001 ... BP_Weapon_1640.

## 5. Intégration : scènes et interfaces existantes

### 5.1 Donjon et objets ramassables

UGridLevelAsset reste l'**asset unique de niveau**. Ses LooseItemInstances, ItemSpawns et réceptacles utilisent la définition canonique UGridItemDefinitionAsset.

Une **limite existante** doit être traitée explicitement plus tard : FGridLooseItemInstance ne stocke pas ItemLevel, et AGridItemActor peut être construit avec la seule définition. ARMEMENT01.6 fournit un **aperçu sans niveau imaginaire** lorsque l'instance exacte n'est pas connue. La future génération de butin prendra en charge la conservation du niveau lors de spawn, drop, pickup, changement de niveau et sauvegarde.

Le résolveur visuel doit pouvoir être utilisé par GridItemActor et GridEditorPreviewComponent, mais sans casser le runtime générique, les interactions, les mécanismes, les portées, les collisions ou les règles de Material Ownership.

### 5.2 Inventaire, tooltip et hotbar

- C++ possède le read model visuel ; WBP_InventorySlot et UMG affichent seulement les références résolues et le badge si approuvé.
- WBP_ItemTooltip reste **le même** : aucune fiche complète de dégâts/affixes n'est ajoutée par ARMEMENT01.6. L'examen détaillé reste prévu dans PresentItemExamination.
- Le slot 1 PrimaryAttack est **déjà dynamique** : si une arme de départ est équipée plus tard, l'icône d'arme remplacera le poing dans les cas déjà supportés ; lorsqu'un personnage est désarmé, le poing reste un recours jouable.
- Les graphes UMG seront modifiés manuellement dans Unreal après validation C++, avec captures de contrôle. Aucune édition binaire distante supposée.

## 6. Découpage en tickets atomiques

**Politique Git : master seulement ; un ticket = un commit atomique ; aucun PR, aucune branche, aucun force-push.** Toutes les modifications des .uasset et images suivent les règles Git LFS et une revue visuelle en UE.

| Ticket | Travail | Critère de sortie |
|---|---|---|
| **ARMEMENT01.6.0** | Rédiger/publier **ce contrat** | Documentation seule ; code, UMG et assets inchangés |
| **ARMEMENT01.6.1** | UGridWeaponVisualCatalogueAsset, profils visuels et mapping unique | IDs valides et uniques ; aucune donnée gameplay dupliquée |
| **ARMEMENT01.6.2** | FGridWeaponVisualResolver et erreurs/fallback/read model | Résolution déterministe, Legacy intact, aucune mutation |
| **ARMEMENT01.6.3** | 3 icônes + 3 meshes pilotes, DataAsset visuel et validation UE | Trois familles de comportement couvertes ; icônes carrées et visuels lisibles |
| **ARMEMENT01.6.4** | Raccordement inventaire/hotbar et badge de niveau UMG (si approuvé) | Icône cohérente ; PrimaryAttack inchangé ; pas de calcul RPG en WBP |
| **ARMEMENT01.6.5** | Raccordement AGridItemActor et editor preview au résolveur | Pas de régression physique, projectile, pickup, réceptacle ou placement |
| **ARMEMENT01.6.6** | Audit des 82 ArchetypeId ; expansion vers 8 silhouettes représentatives | Mapping diffable ; erreurs de couverture signalées ; backlog des autres silhouettes |
| **ARMEMENT01.6.7** | Validation complète/documentation d'authoring/captures comparatives | Tests ciblés et suite globale verts avec journaux de l'utilisateur |

**Ordre :** 6.0 → 6.1 → 6.2 → 6.3 → 6.4 → 6.5 → 6.6 → 6.7. Ne pas commencer par produire toutes les ressources graphiques ; valider d'abord trois parcours complets.

### 6.1 Tests proposés (à créer, pas encore exécutés)

Racine : **Grimrock.RPG.ARMEMENT01.6**. Cas attendus :
- VisualCatalogueSchema : références, VisualId et ArchetypeId uniques, mappings valides.
- VisualResolverDeterminism : identité/niveau égaux donnent la même projection ; zéro mutation d'instance.
- LegacyVisualFallback : propriétés Icon/WorldMesh/EquippedMesh historiques préservées.
- MissingVisualAssetFallback : erreurs signalées et fallback neutre ; pas de substitution d'archétype.
- InventoryIconAndHotbar : présentation partagée et poing fallback.
- WorldPickupAndPreview : chemin runtime/preview et interaction inchangés.
- PassiveNoAttack : un profil visuel passif ne crée aucune attaque.
- MaterialsAndEffectIsolation : matériau de base, collisions, masse et effets inchangés.

**Référence avant ARMEMENT01.6** : 1 471 tests réussis / 0 warning / 0 échec. Le nombre final évoluera seulement après ajout des nouveaux tests ; ne jamais annoncer un test comme passé sans log utilisateur.

~~~powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.6"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock"
~~~

Validation manuelle obligatoire : capture d'icône à 48×48, objet au sol et dans un réceptacle, projectile récupéré, comportement passif, visibilité dans un donjon sombre et redimensionnement de l'UI.

## 7. Décisions proposées et clôture

À valider avant les assets graphiques : direction artistique médiévale fantasy sombre et homogène, ratio d'icône, 3 silhouettes pilotes, niveau affiché en badge textuel discret, rareté non activée, ornement sans gameplay ni collision, quantité de silhouettes déterminée par la distinction des 82 archétypes réels.

**ARMEMENT01.6 est clos** quand le schéma et le résolveur C++ sont intégrés, les trois cas pilotes sont revus dans UE5, inventaire/hotbar/monde utilisent la même autorité lorsqu'ils connaissent l'instance, les Legacy restent inchangés, les mappings manquants sont contrôlés et le filtre global Grimrock est entièrement vert selon TD04.2. **La production artistique complète de 82 archétypes n'est pas requise pour clôturer ce jalon.**

## 8. Travaux suivants (distincts)

- **ARMEMENT01.7 — Équipement de départ à la création du personnage** : ajouter après la création du personnage un écran de choix de kits selon les **six classes réelles** (Guerrier, Voleur, Rôdeur, Mage, Prêtre, Alchimiste), avec compatibilités/tags existants, équipement transactionnel, sauvegarde du choix et préservation du flux New Game et de sa barre de progression. PrimaryAttack affiche l'arme équipée ; Attack_Unarmed reste le fallback.
- **ARMEMENT01.8 — Butin et environnement de donjon** : définir et préserver ItemLevel pour LooseItemInstances, ItemSpawns, loot de monstre et réceptacles, de la génération au pickup et à la sauvegarde ; sans reroll au rechargement.
- **Plus tard** : contrat gameplay distinct pour rareté persistante, affixes, forge, runes et malus d'écart de niveau, avant toute association visuelle avec ces caractéristiques.

## 9. Références documentaires

- docs/Design/ARMEMENT01_CONTRAT_ARMEMENT_V0_3.md
- docs/Design/ARMEMENT01_4_WEAPON_CATALOGUE_AUTHORING.md
- docs/Design/ARMEMENT01_5_FIXED_LEVEL_INSTANCE_INTEGRATION.md
- docs/Architecture/MATERIAL_OWNERSHIP.md
- docs/Design/ITEM_AND_PICKUP_ASSET_CREATION_GUIDE.md (**workflow actif après MIG10, pas annexe historique**)
- docs/Design/HOTBAR01_SIMPLIFIED_GENERAL_ACTION_BAR.md
- docs/Design/UI_ITEM01_ITEM_TOOLTIP_COMPARISON.md
- docs/Design/MON11_4_PLAYER_ATTACK_PRESENTATION.md

**Ce contrat précise le nouveau jalon visuel ARMEMENT01.6 sans activer les jalons forge/alchimie/runes décrits comme propositions dans l'ancien découpage ARMEMENT01 v0.3.**
