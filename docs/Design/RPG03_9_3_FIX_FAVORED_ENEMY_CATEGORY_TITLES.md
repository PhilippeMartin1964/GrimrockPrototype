# RPG03.9.3-FIX — Synchronisation des titres Ennemi juré

**Date :** 20 octobre 2026
**Statut :** C++ prêt à être publié ; matérialisation binaire et validation locale UE5 en attente.

## Symptômes TD04.2

- ARMEMENT01.3 : 7/7 OK.
- UI.EventHUD01 : 55/55 OK.
- RPG03 : 196/197 OK. Seul échec : RPG03.9.3.Authoring.ProductionAssets,
  deux assertions des titres Favored Enemy Goblin et Vermin.

## Cause et autorité

FRPGRangerAuthoring::ConfigureClass utilise
RematerializePreservingChoiceAndActionText, qui conserve les anciens
DisplayName présents dans DA_Class_Ranger après rematérialisation des choix.
Les titres des variantes Ennemi juré peuvent donc rester périmés malgré
CategoryDefinition.DisplayName, l'autorité canonique du bestiaire.

## Correctif

1. Après la préservation des textes, ConfigureClass réapplique seulement
   les titres des variantes Ennemi juré depuis les catégories reçues.
2. Le format de titre est un FText::Format localisable : Ennemi juré — {0}.
3. Les descriptions de ces talents, les autres choix et les actions restent préservés.
4. La nouvelle opération SyncProductionFavoredEnemyTitles vérifie que chaque
   variante existe et cible la catégorie attendue, modifie uniquement
   ces titres, puis sauvegarde le seul DataAsset DA_Class_Ranger si nécessaire.
5. Le commandlet RPGRangerAuthoring accepte -FavoredEnemyTitlesOnly.
   Sans cet argument, l'authoring habituel reste inchangé.
6. Un test transient FavoredEnemyCategoryTitleAuthority simule de vieux
   titres et valide la mise à jour et la préservation des autres textes.

## Matérialisation locale (master, working tree propre)

Commande :

    cd D:\Development\GrimrockPrototype
    git pull --ff-only origin master
    .\Scripts\FixRPGRangerFavoredEnemyTitles.ps1 -EngineRoot D:\UE_5.5

Le script compile, exécute le commandlet ciblé, vérifie que seul
Content/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Ranger.uasset
a changé, puis lance le filtre Grimrock.RPG.RPG03.9.3.
Il ne publie ni ne committe le DataAsset automatiquement.

Après validation locale, committer uniquement le binaire (Git LFS) :

    git status --short
    git add Content/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Ranger.uasset
    git diff --cached --name-only
    git commit -m "fix(RPG03.9.3): materialize Favored Enemy bestiary titles"
    git push origin master

Puis exécuter la campagne complète :

    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.RPG03"

Le correctif ne sera déclaré validé qu'après retour des journaux UE5
attestant zéro échec. Aucune modification des armes, du moteur multi-dés,
des définitions de monstres ou des descriptions de catégorie.
