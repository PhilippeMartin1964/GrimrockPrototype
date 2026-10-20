# ARMEMENT01.6.3 — Correction des dépendances de matériaux

**Date :** 20 octobre 2026.
**État :** code de réparation publié ; compilation, exécution et validation locale à effectuer.
**Référence préalable :** TD04-20261020-202531, 1 484 tests réussis sans avertissement, 1 test réussi avec avertissement, 0 échec.

## Diagnostic

Les OBJ initiaux ont conservé les identifiants de matériaux issus des palettes MTL, mais 14 packages de matériaux Unreal ne sont pas présents sur disque. Les avertissements LoadErrors confirment notamment :

- Dague : Steel_Dark, Steel_Bright, Bronze, Leather, Azure_Gem
- Arc court : Yew_Wood, Leather_Binding, Bronze_Tip, String
- Bouclier rond : Worn_Oak, Dark_Oak, Oxidized_Bronze, Patinated_Iron, Brass_Studs, Leather_Binding

Leather_Binding est partagé, d'où un total de 14 et non 15. Le .mtl du bouclier est harmonisé avec celui de l'arc pour garder une autorité unique.

## Correction non destructive

Le nouveau commandlet Editor-only GridWeaponPilotMaterialRepair :

1. Lit et vérifie les trois fichiers .mtl avant toute création d'asset.
2. Crée uniquement les materials UMaterial manquants, avec des graphes de BaseColor (Kd), Metallic et Roughness (Ns).
3. Conserve les matériaux existants : aucun remplacement.
4. Vérifie les vrais Material Slots des StaticMeshes déjà importés.
5. Réaffecte et sauvegarde les slots manquants, sans reconstruire les meshes.
6. Ne modifie pas DA_WeaponVisualCatalogue, les icônes, les niveaux, ni les données RPG.

Les Material Slots restent l'autorité du rendu (MATERIAL-OWNERSHIP01).
Ces matériaux pilotes ne prétendent pas être des textures PBR finales.

Le module Editor ajoute MaterialEditor comme dépendance. Le Runtime reste inchangé.

## Procédure locale

Fermer Unreal Editor. Les sept .uasset précédemment créés ne doivent être ni effacés ni réimportés.

    cd D:\Development\GrimrockPrototype
    git status --short
    git pull --ff-only origin master
    .\Scripts\RepairARMEMENT0163Materials.ps1 -EngineRoot D:\UE_5.5

Le script autorise les .uasset pilotes non suivis, refuse les changements hors périmètre, compile l'éditeur, lance le commandlet, exécute les tests ciblés et affiche git status. Aucun commit automatique.

Le script initial AuthorARMEMENT0163.ps1 lancera dorénavant cette phase après l'import lors des générations futures, depuis une arborescence propre.

## Validation

Un quatrième test Pilot.MaterialDependencies vérifie les 14 matériaux réels et leur affectation dans chaque Material Slot.

Objectifs à confirmer avec TD04.2 :
- Pilot : 4/4 sans avertissement ;
- Grimrock global : 1 486/1 486 sans avertissement ;
- zéro échec, code de sortie 0.

Si des messages de normales ou tangentes dégénérées subsistent, traiter séparément la géométrie après revue UE. Aucun .uasset ne sera publié via Git LFS avant validation visuelle.
