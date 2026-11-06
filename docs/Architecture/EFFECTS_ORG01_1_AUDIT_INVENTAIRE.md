# EFFECTS-ORG01.1 — Audit et inventaire des effets

**Date :** 6 novembre 2026. **Base :** branche master de PhilippeMartin1964/GrimrockPrototype.
**Jalon strictement en lecture seule :** aucun renommage, déplacement, effacement ou resauvegarde de package Unreal.

## Inventaire vérifié dans GitHub : 15 ressources

Le dépôt contient 9 assets dans Effects/ (2 dans Alchimie, 7 dans Teleporter) et 6 autres dans Art/FX, Art/Materials/Catalyst, Art/Materials/Surfaces.

| Emplacement sous /Game/GrimrockPrototype | Asset | Attribution / décision |
|---|---|---|
| Effects/Alchimie | NS_LiquideSparkle | Niagara alchimie, conserver |
| Effects/Alchimie | **M_Teleporter_Spark_02** | **Bulles remontantes de potion de soin ; renommer M_Potion_Spark et conserver dans Alchimie** |
| Effects/Teleporter | NS_Teleporter_Blue | Niagara du téléporteur, conserver |
| Effects/Teleporter | M_Teleporter_Spark | Effet étincelles téléporteur, conserver ; NON doublon du matériau de potion |
| Effects/Teleporter | M_Teleporter_Volume | Effet volume, conserver |
| Effects/Teleporter | M_Teleporter_Wisp | Effet filaments, conserver |
| Effects/Teleporter | MI_Teleporter_Active | Matériau actif, conserver |
| Effects/Teleporter | MI_Teleporter_Inactive | Matériau inactif, conserver |
| Effects/Teleporter | T_Teleporter_Noise_01 | Texture de bruit, conserver |
| Art/FX | NS_Flame_8_Torch | Niagara des torches, candidat à Effects/Fire/Torch |
| Art/Materials/Catalyst | M_SurfaceCatalyst_Ember | Braises catalyseur, migration à examiner |
| Art/Materials/Catalyst | M_SurfaceCatalyst_Flask | Matériau du flacon : VFX ou apparence d'objet, à distinguer |
| Art/Materials/Catalyst | M_SurfaceCatalyst_RunePulse | Onde catalyseur, migration à examiner |
| Art/Materials/Surfaces | M_Surface_Oil | Matériau huile, migration à risque |
| Art/Materials/Surfaces | MI_Surface_Oil | Instance huile, migration à risque |

**Décision explicite de l'auteur :** M_Teleporter_Spark_02 correspond aux bulles de la potion de soin, malgré son nom. Le renommer ultérieurement en **M_Potion_Spark**, dans le **même dossier Effects/Alchimie**. Ne jamais le traiter comme une ressource du téléporteur ni un doublon de M_Teleporter_Spark. Le renommage se fera dans Unreal Engine lors de EFFECTS-ORG01.2, jamais via Windows Explorer.

## Dépendances d'effets supplémentaires : 13 ressources

Ces fichiers existent également sur master, mais leur propriétaire définitif nécessite le graphe des références Unreal.

| Dossier | Ressources | Contrôle |
|---|---|---|
| Art/Materials/Instances/Flame | MI_Distortion_1, MI_Flame_4, MI_Flame_8 | Exclusifs à la torche ou partagés ? |
| Art/Materials/Masters | M_Master_Flame | Maître partagé ou propre à un effet ? |
| Art/Textures/Final/Flame | T_Flame_1, T_Flame_4, T_Flame_8, T_RefractFlowmap_1, T_RefractMask_1, T_RefractNormal_1 | Dépendances des matériaux de flammes |
| Art/Materials/Item_Materials | M_Potion_LiquidFlow, M_Item_WorldSparkle | Apparence d'item ou VFX dédié ? |
| Art/Materials/Instances/Items | MI_Potion_LiquidFlow | Usage/consommateurs du matériau liquide |

**Total statique étendu : 28 candidats (15 + 13)**. Les textures PBR ordinaires, icônes et meshes de flacons ne deviennent pas des VFX simplement parce que leur nom évoque Fire ou Potion.

## Références textuelles et risques établis

| Ressource | Utilisateurs C++ / scripts vérifiés | Risque |
|---|---|---|
| NS_LiquideSparkle | RPGStartingHealthPotionAuthoring.cpp, RPGStartingSurfaceCatalystAuthoring.cpp, GridItemVFX011Tests.cpp, GridEditorItemVFX014ProductionPolicyTests.cpp | Authoring et tests à adapter en cas de déplacement |
| NS_Flame_8_Torch | GridEditorItemVFX013AuthoringTests.cpp, GridItemVFX014BLegacyRemovalTests.cpp | Torche, tests et possibles références sérialisées dans les DataAssets |
| M_SurfaceCatalyst_Ember / M_SurfaceCatalyst_RunePulse | RPGSurfaceCatalystVisualAuthoring.cpp, GridARMEMENT011714OilSurfaceVisualTests.cpp, UEValidationAgent.ps1 | Couplage authoring, tests, présentation Catalyst |
| M_Surface_Oil / MI_Surface_Oil | GridOilSurfaceMaterialAuthoringCommandlet.cpp, GridLevelRuntimeActor.cpp, GridLevelRuntimeActorSurfaces.cpp, AuthorOilSurfaceMaterial.ps1, UEValidationAgent.ps1 | **RISQUE ÉLEVÉ** : pipeline de génération et réparation de matériaux lié aux chemins précis |
| NS_Teleporter_Blue, M_Teleporter_Spark | Pas de référence C++ textuelle trouvée dans la recherche ciblée | **Références Unreal non inspectées, pas preuve d'inutilisation** |
| M_Teleporter_Spark_02 | Fonction des bulles confirmée par l'auteur ; pas de référence textuelle C++ identifiée | Vérifier matériaux Niagara et DA_Item_Potion_Healing_01 dans Unreal |

Consommateurs candidats à examiner : DA_Item_Torch, DA_Item_Potion_Healing_01, DA_Item_SurfaceCatalyst_01, DA_Item_Flask_Oil, DA_Teleporter, BP_GridLevelRuntimeActor.

Les assets versionnés sont des fichiers **Git LFS**. Les chemins/noms sont consultables sur GitHub, mais les références internes .uasset/.umap ne sont pas visibles depuis le connecteur GitHub. Les absences de référence textuelle C++ ne signifient donc pas qu'un asset est inutilisé.

## Structure cible proposée — pas encore appliquée

Racine unique : Content/GrimrockPrototype/Effects, organisée par **fonction d'effet**, non par extension technique :

    Effects/
      Alchimie/
        NS_LiquideSparkle
        M_Potion_Spark                  [nom futur, dans Alchimie]
        Catalyst/
          [présentation projectile / rune / braises]
      Teleporter/
        [7 assets existants : rester en place]
      Fire/
        Torch/
          [Niagara et ressources propres à la torche]
        [profils d'effets publics et réutilisables]
        Local/
          DA_GroundFire                 [profil privé, exclu Git]
      Surfaces/
        Oil/
          [matériaux huile, après mise à jour pipeline]

Les ressources visuelles **partagées** restent dans Art (maîtres
de matériaux, apparence des items, textures PBR) ; les éléments
**spécifiques à un effet** sont regroupés dans son dossier Effects.
Les niveaux GridLevelAsset décrivent la géométrie et la logique ;
ils ne récupèrent aucun réglage global Surface VFX.

Le pack commercial FireFX reste intact dans Content/FireFX/, exclu de Git.
Un futur profil local le référence. Le nouveau répertoire
Content/GrimrockPrototype/Effects/Fire/Local/ est exclu de Git.
L'exclusion précédente LocalVFX est conservée **uniquement comme mesure
anti-divulgation** tant que des assets privés peuvent encore y résider.

## Audit Asset Registry local en lecture seule

Un nouveau test Unreal fournit l'inventaire concret des référents et
dépendances sérialisées :

    Grimrock.Editor.EffectsOrg01.Inventory

Pour l'exécuter sur la machine Windows qui détient les assets :

    cd D:\Development\GrimrockPrototype
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Editor.EffectsOrg01.Inventory"

Rapports produits exclusivement sous Saved, jamais dans Content :

    Saved/Audits/EFFECTS-ORG01/EffectsAssetRegistry.csv
    Saved/Audits/EFFECTS-ORG01/EffectsSummary.txt

Le CSV exporte Package, AssetName, AssetClass, OwnerHint,
RefQueryOK, ReferencerCount, Referencers, DepQueryOK, DependencyCount,
Dependencies, Attention. Il découvre également les Niagara dont le
nom commence par NS_ même s'ils sont dans un dossier inattendu.
Le résumé distingue les assets FireFX locaux, facultatifs ; le worktree
de validation automatique n'est pas supposé disposer du pack commercial.

La mention ZERO_REGISTRY_REFERENCERS_NOT_PROOF_OF_DEAD_ASSET signifie
qu'aucun référent sérialisé n'a été trouvé, **pas** qu'il faut supprimer
l'asset : il peut être chargé dynamiquement en C++, par script ou outil.
Un graphe de dépendances introuvable est marqué
REGISTRY_QUERY_INCOMPLETE et ne justifie aucune action irréversible.

## Execution autonome de l'agent Windows

Le test d'inventaire est lance **automatiquement** par la suite Grimrock
lorsque l'agent Windows valide un commit sur master. Il ne faut pas
demander a l'utilisateur de lancer manuellement ValidateUE.ps1.

Une fois la suite complete validee, UEValidationAgent.ps1 lit le CSV
local en lecture seule et cree une issue GitHub
[EFFECTS-ORG01.1] Inventaire AssetRegistry <SHA> contenant seulement
des noms de packages **premiere partie** sous
/Game/GrimrockPrototype/ ainsi que leurs liens vers d'autres packages
de meme perimetre. Il ne publie **jamais** de chemins Windows,
d'identifiants du poste ou de noms d'assets proprietaires FireFX.
Le CSV exhaustif demeure sur la machine Windows.

La publication du rapport est non bloquante : si elle echoue, le
build et les tests UE conservent leur statut veridique, sans
modifier le depot. L'agent ne lance aucune migration d'assets.

## Portes avant EFFECTS-ORG01.2 — migration contrôlée

1. Récupérer les rapports du test sur le **vrai dossier de développement local**.
2. Auditer les dépendances et référents dans Unreal : potion/bulles, téléporteur, torche, Catalyst, surfaces.
3. Confirmer chaque propriétaire et établir la table ancien chemin → nouveau chemin.
4. Renommer M_Teleporter_Spark_02 en **M_Potion_Spark** directement dans Unreal, **sans sortir d'Alchimie**.
5. Déplacer les autres assets dans Unreal, sauvegarder les consommateurs, corriger les redirectors et contrôler les chemins C++, scripts, tests et DataAssets.
6. Protéger le pipeline de génération d'huile (AuthorOilSurfaceMaterial.ps1 et UEValidationAgent.ps1) avant toute migration de M_Surface_Oil.
7. Vérifier le rendu PIE des potions, du téléporteur, des torches, des réactions et des surfaces, puis la suite complète UE5.5.4.

**EFFECTS-ORG01.1 n'effectue ni déplacement ni suppression de contenu.**
