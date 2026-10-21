# ARMEMENT01.6.3-FINAL — Consolidation des trois armements réalistes

**Date :** 21 octobre 2026. **Statut :** code prêt, validation UE5.5.4 et publication des packages locaux à confirmer. **Baseline :** TD04-20261020-204725, 1486 tests réussis, zéro avertissement, zéro échec.

## 1. Identifiants canoniques et nomenclature

| Armement | ArchetypeId | VisualId | WorldMesh | BaseIcon | Matériau |
|---|---|---|---|---|---|
| Dague | ARM001 | VIS_Dagger_01 | SM_Weapon_Dagger_01 | T_Weapon_Dagger_01 | MI_Dagger |
| Arc court | ARM045 | VIS_ShortBow_01 | SM_Weapon_ShortBow_01 | T_Weapon_ShortBow_01 | MI_Bow |
| Bouclier rond | ARM075 | VIS_RoundShield_01 | SM_Weapon_RoundShield_01 | T_Weapon_RoundShield_01 | MI_Shield |

La nomenclature finale utilise _01, _02, etc. Les valeurs ARM001..ARM082 restent inchangées. Une silhouette sert aux 20 niveaux d'instance. Aucun changement de combat ou SaveGame.

Le lot de 19 packages listé dans le status du 21 octobre inclut :
- 3 meshes sous Content/GrimrockPrototype/Meshes/Weapons ;
- 3 icônes sous Content/GrimrockPrototype/UI/Icons/Weapons ;
- 3 instances sous Content/GrimrockPrototype/Art/Materials/Instances/Weapons ;
- 9 maps T_{Bow,Dagger,Shield}_01_{BC,N,ORM}, sous Art/Textures/Final/Weapons/{Bow,Dagger,Shield} ;
- 1 DA_WeaponVisualCatalogue dans Core/DataAssets/Weapons.

**Exclusion absolue :** ne modifier, restaurer, effacer, stage ou publier aucun des 2 DA_Weapon_Shuriken/ThrowingKnife, 2 anciennes icônes déplacées UI/Weapons, ou 2 nouvelles icônes UI/Icons/Weapons correspondantes. Leur état Git n'appartient pas à cette opération.

## 2. Références et nettoyage

L'ancien pipeline procédural (6 sources OBJ/MTL, 3 scripts et 2 commandlets C++ avec leurs headers) n'est plus la voie de production ; ses sources sont retirées de l'arbre actif et **demeurent récupérables dans l'historique Git**. Les textes de conception historiques restent accessibles comme archives.

Aucune suppression d'asset Unreal local n'est automatique. Les anciens packages _A et les matériaux procéduraux ne figurent plus dans le status local transmis ; nous ne lançons ni git clean, ni reset --hard.

## 3. Finalisation contrôlée du catalogue

Le commandlet editor-only GridWeaponVisualFinalize, invoqué par Scripts/FinalizeARMEMENT0163.ps1, gère :
- préflight des 3 icônes, 3 meshes, 3 MI, et 9 textures PBR ;
- vérification des dimensions d'import positives, icônes carrées, parents MI assignés, Material Slots valides ;
- vérification sRGB désactivé sur les maps N et ORM ;
- conservation des transforms et éléments équipés/ornements préexistants ;
- conservation des 3 ArchetypeId réels, mise à jour des VisualId numériques, Icon et WorldMesh ;
- refus de supprimer des profils supplémentaires, sans toucher aux objets de combat.

Sans -Apply, le commandlet vérifie et **ne sauvegarde rien**. Avec -Apply, il écrit uniquement le DA_WeaponVisualCatalogue ; le script lance ensuite les 4 tests Pilot modernisés.

Les tests ciblés remplacent les noms anciens _A et les 14 matériaux provisoires par les 3 meshes _01, les 3 MI et leurs 9 textures PBR. Ils continuent à vérifier le bouclier passif et le résolveur read-only.

## 4. Procédure Windows PowerShell

Fermer Unreal Editor avant les commandlets. Ne plus utiliser AuthorARMEMENT0163.ps1 ni RepairARMEMENT0163Materials.ps1.

    cd D:\Development\GrimrockPrototype
    git status --short --untracked-files=all
    git pull --ff-only origin master

    .\Scripts\FinalizeARMEMENT0163.ps1 -EngineRoot D:\UE_5.5

Après réussite du contrôle en lecture seule :

    .\Scripts\FinalizeARMEMENT0163.ps1 -EngineRoot D:\UE_5.5 -Apply

Puis valider la campagne globale :

    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock"

**Cibles, pas résultats attestés :** Pilot 4/4 et Grimrock 1486/1486, aucun avertissement, aucun échec. Soumettre les logs TD04.2.

Si un matériau parent, un slot MI ou un réglage sRGB est incorrect, corriger manuellement dans UE avec capture : ne pas forcer le script. Les règles MATERIAL-OWNERSHIP01 s'appliquent : matériau principal dans les slots du StaticMesh.

## 5. Validation visuelle avant Git LFS

Inspecter :
- dague longueur cible 30 cm, détails de la lame et poignée ;
- arc court hauteur cible 115 cm, corde et courbure ;
- bouclier diamètre cible 65 cm, renforts, relief, prise arrière ;
- icônes carrées lisibles à 48 x 48 ;
- BC en sRGB, N en normal map linéaire, ORM en linéaire avec canaux reliés correctement au parent ;
- matériaux bien affectés dans les trois meshes ; pas de références manquantes ni de géométrie dégénérée.

Les dimensions sont des **objectifs de design non mesurés automatiquement** dans les fichiers .uasset locaux.

## 6. Publication sélective ultérieure

Les 19 .uasset sont uniquement sur le poste utilisateur. Après validation UE et revue des dépendances, un commit art/data distinct pourra inclure exactement ces 19 fichiers, au moyen de git add sur les **chemins complets exacts**, suivi de git diff --cached --name-only et git lfs status. Aucun git add du répertoire Content ou de son parent.

Les six chemins Shuriken / ThrowingKnife ne doivent apparaître dans **aucun** commit de ce ticket. Si une MI utilise un Parent non versionné, arrêter avant le commit et fournir le Reference Viewer pour ajuster la liste.

**Ne pas annoncer la clôture tant que le code n'a pas été compilé, les tests n'ont pas été passés et le rendu réaliste n'a pas été examiné.**

Après le jalon : ARMEMENT01.6.4 inventaire/tooltip/hotbar ; 6.5 ramassage au sol et preview Editor.
