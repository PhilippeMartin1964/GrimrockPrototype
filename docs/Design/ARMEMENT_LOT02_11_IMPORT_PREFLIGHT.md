# ARMEMENT-LOT02.11 — Inventaire local des 11 imports Meshy

Le `git status --short` utilisateur du 28 octobre 2026 montre 11 meshes et 11 icônes.
Il montre 10 nouvelles Material Instances (sans `MI_HolyScepter`) et aucun dossier PBR HolyScepter.
Les neuf autres dossiers PBR ne révèlent pas individuellement leurs noms de textures dans `git status`.
Les textures et la Material Instance du couteau de jet ont le suffixe `_02` :
`MI_ThrowingKnife_02` et `T_ThrowingKnife_02_BC|N|ORM`. Le mesh/icône restent `_01`.
Ce choix évite de remplacer silencieusement le matériel historique du couteau de jet.

## Autorité de chemins d'admission

`docs/Design/ARMEMENT_LOT02_11_ART_INTAKE.csv` décrit exactement six
chemins d'assets Unreal pour chacune des onze armes.
Ces chemins sont **déclaratifs et à vérifier localement** : ils ne prouvent pas
que les `.uasset` sont chargés, ni qu'un matériau est connecté au mesh.

## Contrôle local sans mutation d'assets

En PowerShell à la racine du dépôt après `git pull --ff-only origin master` :

    .\Scripts\AuditStarterWeaponArt.ps1

Rapport : `Saved/ARMEMENT-LOT02/LOT02_11_ArtIntake.csv`.
Le script signale les fichiers manquants et retourne une erreur si au moins un set
est incomplet ; `-NoThrow` affiche uniquement les diagnostics.
Il ne fait aucun `git add`, ne committe rien, ne modifie pas le manifeste,
le XLSX, les niveaux ou les assets binaires.

## Prochaine étape, après un 66/66

Faire valider dans Unreal les contraintes réelles de `GridWeaponPipelineCommandlet::LoadArt` :
icône carrée >= 256 px ; StaticMesh avec bounds/LOD corrects ; MaterialInstance avec
parent et affectation à au moins un Material Slot du mesh ; BaseColor sRGB actif ;
Normal/ORM sans sRGB ; vraies ressources UE chargeables.
Après cet audit, autoriser explicitement les nouvelles lignes `Planned` dans
`Config/Armement/WeaponProductionManifest.csv`, puis `WeaponPipeline.ps1 -Action Plan`
et la production contrôlée du catalogue visuel, des ItemDefinitions et de la palette.

Les 18 recettes approuvées, le XLSX modifié localement et le document
`ARMEMENT_LOT02_10_PROMPTS_44_VUES_MESHY.md` non versionné ne sont pas touchés.
