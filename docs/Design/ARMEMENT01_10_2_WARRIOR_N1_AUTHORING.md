# ARMEMENT01.10.2 — Premier lot Guerrier : préflight et authoring sécurisé N1

Date : 22 octobre 2026. **Statut : code d'authoring publié ; compilation/test UE et fabrication des assets non encore validés.**

## Point de départ et autorité

ARMEMENT01.9.7 : **6/6**, zéro avertissement/échec, code 0 (TD04-20261022-132200).
ARMEMENT01.10.1 : **1/1**, zéro avertissement/échec, code 0 (TD04-20261022-134725), d'après le journal utilisateur. Le contenu des 22 lignes CSV n'a pas été transmis : ne pas inventer la disponibilité des ressources visuelles.

La source éditoriale est le classeur ARMEMENT01_Catalogue_Armes_Boucliers_Niveaux_1_20_ARMEMENT01_2.xlsx, matérialisé en DA_WeaponCatalogue, et les trois kits Warrior sont dans le **seul** manifeste ARMEMENT01_9_6_STARTER_KIT_RECIPES.csv. Le présent ticket ne crée pas de nouvelles statistiques.

| ArchetypeId | Désignation du manifeste | ItemDefinitionId réservé | DataAsset à authorer |
|---|---|---|---|
| ARM014 | Épée droite | Weapon_StraightSword_01 | DA_Weapon_StraightSword_01 |
| ARM015 | Épée longue | Weapon_LongSword_01 | DA_Weapon_LongSword_01 |
| ARM025 | Pic de guerre | Weapon_WarPick_01 | DA_Weapon_WarPick_01 |
| ARM031 | Grande hache | Weapon_GreatAxe_01 | DA_Weapon_GreatAxe_01 |

ARM075 (bouclier rond du Gardien) **existe déjà**, ne doit pas être recréé.
Le Maître d'armes reçoit ARM015 en MainHand et ARM025 **dans le sac**, pas en double équipement forcé.
La grande hache ARM031 conserve l'usage **deux mains** du catalogue ; aucun talent Briseur n'est accordé au N1.

## Ce que le code ajoute

- Commandlet GridWarriorN1ItemAuthoring (module Editor) et spécifications des quatre identités.
- Mode par défaut **préflight seul** : inspecte réellement le profil N1, l'affinité Warrior et les ressources visuelles. Signale BLOCKED pour absence de mapping, d'icône ou de mesh. **Aucun package écrit**.
- Mode -Apply -Archetype=ARMxxx : **un seul package** au maximum par invocation ; refuse un chemin déjà occupé ou un archetype déjà matérialisé sous un autre nom. Pas d'écrasement silencieux ni de modification de DA_WeaponCatalogue / DA_WeaponVisualCatalogue.
- Statistiques, nom et poids récupérés du profil N1 existant. Attaque de mêlée avec dés, PA, portée, scaling dérivés de FGridWeaponLevelProfile. Pas de coûts ou dés inventés ; action source Equipment.
- bUseWeaponCatalogue=true, bUseWeaponVisualCatalogue=true, DefaultWeaponItemLevel=1, compatibilité MainHand explicite. La représentation Legacy Icon/WorldMesh/EquippedMesh référence exactement les vrais visuels du catalogue.
- Script PowerShell Scripts/AuthorARMEMENT01102WarriorN1.ps1 : compilation, préflight ou création sélective, tests après -Apply et contrôle des changements Git, sans commit/push automatique.
- Deux nouveaux tests sous Grimrock.RPG.ARMEMENT01.10.Warrior : conformité des recettes/profils/affinités et contrôle de toutes les définitions réellement présentes. Les absences restent signalées, **jamais déclarées jouables**.

## Blocage artistique normal

Le dépôt Git versionne pour l'instant des visuels complets uniquement pour les **trois pilotes** (ARM001/ARM045/ARM075). On ne déduit donc pas que les quatre nouvelles armes possèdent déjà un mesh et une icône. Elles doivent être créées ou approuvées **dans UE5/Blender**, puis liées dans **le même** DA_WeaponVisualCatalogue.

Noms conseillés : VIS_StraightSword_01, VIS_LongSword_01, VIS_WarPick_01, VIS_GreatAxe_01, avec icônes T_Weapon_*_01, meshes SM_Weapon_*_01. Ces noms sont des **conventions de production**, pas de faux chemins déclarés existants. Une correspondance visuelle correcte peut être partagée, mais jamais une substitution silencieuse.

Avant de publier de nouveaux fichiers .uasset :
1. Revoir silhouettes (épée droite ≠ épée longue), pivot, dimensions physiques, normales, matériau(x) PBR dans les Material Slots (pas d'override parallèle).
2. Vérifier icônes lisibles à **48×48** sans distorsion et meshes world/held corrects.
3. Contrôler que chaque profil visuel est présent et que ses soft references se chargent.
4. Revalider les ItemDefinitionId, la création de FGridItemInstance au niveau 1, attaque depuis MainHand, 2 mains et les tests PIE.
5. Revue visuelle humaine obligatoire ; aucun test statique ne remplace celle-ci.

**Ne pas employer -Apply tant que le préflight signale BLOCKED.**

## Procédure immédiate : compilation, tests et inventaire

Fermer Unreal Editor puis, depuis **master**, vérifier l'absence de travaux locaux conflictuels :

~~~powershell
cd D:\Development\GrimrockPrototype
git status --short
git pull --ff-only origin master

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.10"

.\Scripts\AuthorARMEMENT01102WarriorN1.ps1 -EngineRoot D:\UE_5.5
~~~

Tests attendus : **3/3** (1 readiness existant + 2 Warrior nouveaux), zéro avertissement. Le préflight peut légitimement rapporter BLOCKED sur quatre armes sans visuels : il ne doit modifier aucun asset et son code retour est 0 en mode audit. Les résultats locaux restent à confirmer par le journal utilisateur.

## Production ultérieure (après visuels réels validés)

Exemple pour **une seule arme**, après un préflight avec READY :

~~~powershell
.\Scripts\AuthorARMEMENT01102WarriorN1.ps1 -EngineRoot D:\UE_5.5 -Archetype ARM014 -Apply
~~~

Le script refuse les conflits, valide localement puis ne publie rien. Répéter avec chaque ARMxxx **uniquement après contrôle des dépendances** ; ne pas utiliser git add . Vérifier le contrôle Git LFS, les références du catalogue visuel, la visibilité dans l'inventaire, les actions et les sauvegardes. Versionner sélectivement les quatre DataAssets et leurs propres ressources validées, selon des commits atomiques appropriés.

## Exclusions formelles

Aucune révision du XLSX, des 1 640 profils N1–N20, de l'API CanEquipToSlot, de la physique, des deux objets Shuriken/ThrowingKnife, des 12 kits legacy, de l'inventaire existant, de l'UMG ou des SaveGame. Pas d'activation des 18 kits dans le wizard. Aucune création d'asset sans ressources réelles.
