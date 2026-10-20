# ARMEMENT01.6.3 — Trois silhouettes pilotes, icônes et meshes

**Date :** 20 octobre 2026.  
**État :** sources artistiques et commande d'import publiées ; génération des .uasset,
compilation et validation visuelle dans UE5.5.4 à effectuer localement.  
**Référence de non-régression :** 1 482 tests Grimrock verts, 0 avertissement, 0 échec, TD04.2 du 20.10.2026 à 19 h 46.

## Objectifs

Ce ticket matérialise un **premier lot artistique**, pas 1 640 variantes.
Les 20 niveaux RPG d'un archétype continuent à partager la même silhouette.

Identités vérifiées dans le catalogue réel ARMEMENT01.4 :

| ArchetypeId | Nom canonique | Famille | VisualId | Icône UI 512 px | StaticMesh |
|---|---|---|---|---|---|
| ARM001 | Dague | Légères | VIS_Dagger_A | T_Weapon_Dagger_A | SM_Weapon_Dagger_A |
| ARM045 | Arc court | Distance | VIS_ShortBow_A | T_Weapon_ShortBow_A | SM_Weapon_ShortBow_A |
| ARM075 | Bouclier rond | Boucliers, passif | VIS_RoundShield_A | T_Weapon_RoundShield_A | SM_Weapon_RoundShield_A |

Les créations sont des **prototypes originaux procéduraux**, à enrichir
artistiquement ultérieurement. Elles ne sont pas des reproductions de
meshes ou textures de Legend of Grimrock 2. Direction : fantasy sombre,
bronze patiné, acier, bois, cuir, détails sobres.

## Sources versionnées

- ArtSource/ARMEMENT01_6_3/Meshes/SM_Weapon_Dagger_A.obj/.mtl
- ArtSource/ARMEMENT01_6_3/Meshes/SM_Weapon_ShortBow_A.obj/.mtl
- ArtSource/ARMEMENT01_6_3/Meshes/SM_Weapon_RoundShield_A.obj/.mtl
- Scripts/GenerateARMEMENT0163Icons.ps1 : génération locale déterministe
  des trois PNG 512×512 RGBA avec GDI+ Windows, anti-aliasing ;
  aucune dépendance à Blender, Photoshop, image distante ou font externe.
- Scripts/AuthorARMEMENT0163.ps1 : compilation, import Unreal, création
  du DataAsset et tests ciblés, **sans commit automatique**.

Les .obj sont des modèles 3D triangulés en centimètres avec axe Z
vertical et palette .mtl. Les Material Slots importés portent les
matériaux principaux ; aucun override Material dans le niveau.
Les icônes PNG sont générées sous **Saved/ARMEMENT01/ARMEMENT01_6_3/Icons**
avant import UE et ne sont pas commitées séparément : leurs .uasset
importés sont versionnés sous Git LFS.

## Asset UE attendu

    /Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue

Création par le commandlet **GridWeaponVisualPilotImport** :

- trois bindings uniquement, ARM001, ARM045 et ARM075 ;
- trois VisualProfiles contenant BaseIcon + WorldMesh et FamilyTag ;
- EquippedMesh absent volontairement : ARMEMENT01.6.2 utilise le
  WorldMesh résolu ;
- OrnamentalMesh absent : pas de décoration dynamique, de VFX automatique,
  de rareté, de générateur d'attaques ou de niveau simulé.

Les assets de rendu doivent apparaître sous :

    /Game/GrimrockPrototype/UI/Icons/Weapons/T_Weapon_*
    /Game/GrimrockPrototype/Meshes/Weapons/SM_Weapon_*

L'import des matériaux depuis les .mtl peut également produire des
Material assets sous Meshes/Weapons ; ceux-ci sont liés aux Material
Slots importés, pas à un override générique.

Les autres 79 archétypes restent **sans mapping dans ce catalogue
pilote** ; le résolveur visuel prévoit le fallback Legacy.

## Exécution locale — Windows PowerShell, Unreal Engine 5.5.4

Fermer l'éditeur UE si le dossier Content est déjà utilisé, puis :

    cd D:\Development\GrimrockPrototype
    git pull --ff-only origin master
    .\Scripts\AuthorARMEMENT0163.ps1 -EngineRoot D:\UE_5.5

Le script :
1. vérifie master et working tree propre ;
2. génère les trois textures PNG transparentes avec GDI+ ;
3. compile l'Editor Development via ValidateUE.ps1 -SkipAutomation ;
4. importe les six ressources graphiques via UnrealEditor-Cmd.exe ;
5. construit et sauvegarde DA_WeaponVisualCatalogue ;
6. vérifie les .uasset et exécute les trois tests sous
   Grimrock.RPG.ARMEMENT01.6.Pilot ;
7. affiche git status sans effectuer de git add/commit/push.

**Ne pas relancer aveuglément le script sur des fichiers à moitié
importés après un échec** : il protège un catalogue déjà existant
contre l'écrasement. Fournir le log pour un correctif ciblé.

## Contrôle visuel obligatoire dans Unreal

Ouvrir dans l'éditeur :

- les trois icônes : alpha transparent, silhouette lisible à **48×48**
  sans écrasement, même angle et tonalité fantasy ;
- les trois meshes : orientation Z, dimensions plausibles, silhouette
  reconnaissable, pivot, normals, Material Slots et palette ;
- aperçu d'objets : sol sombre, alcôve ou support, lumière réaliste ;
- bouclier : aucune action d'attaque produite par le simple profil
  visuel ; de même l'arc et la dague ne reçoivent pas ici de nouveaux dés ;
- un mesh de lancement/dépôt ne doit pas changer la physique avant 6.5.

Il s'agit de prototypes de silhouettes, pas de matériaux PBR finaux.
Leur validation artistique doit être exprimée explicitement par
l'utilisateur (captures recommandées).

## Automation UE

Tests de ce ticket : trois nouveaux cas **Grimrock.RPG.ARMEMENT01.6.Pilot**
vérifiant les vrais .uasset, dimensions 512×512, meshes non vides,
résolveur sur le catalogue matérialisé et bouclier sans attaque.

Si la commande locale réussit, exécuter ensuite :

    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock"

Cible attendue : **1 485 tests / 0 avertissement / 0 échec**.
Ne pas déclarer cette cible acquise sans rapport TD04.2.

## Publication des .uasset après contrôle visuel

Vérifier git status et le nouvel asset :

    git status --short
    git lfs status

Après revue des seuls packages créés dans
Core/DataAssets/Weapons, UI/Icons/Weapons et Meshes/Weapons :

    git add Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.uasset
    git add Content/GrimrockPrototype/UI/Icons/Weapons/
    git add Content/GrimrockPrototype/Meshes/Weapons/
    git diff --cached --name-only
    git commit -m "data(ARMEMENT01.6.3): materialize three pilot weapon silhouettes"
    git push origin master

Ce deuxième commit de **matérialisation binaire** est séparé du
commit atomique des **sources/outils du ticket**, comme le fut
DA_WeaponCatalogue pour ARMEMENT01.4.

## Limitations connues et prochaines étapes

- Aucun WBP ne lit encore le nouveau catalogue. Cela commence en **ARMEMENT01.6.4**.
- Le runtime monde et l'éditeur preview restent Legacy jusqu'à **ARMEMENT01.6.5**.
- Le DataAsset visuel pilote est un **asset de présentation**, distinct
  du catalogue de combat, du butin et des sauvegardes.
- La couverture des huit familles et des 82 archétypes relève de **ARMEMENT01.6.6**.
- Les matériaux PBR, finitions, ornements et icônes plus sophistiquées
  seront affinés après retour visuel ; ne pas augmenter le nombre
  d'assets avant la revue des trois silhouettes.
