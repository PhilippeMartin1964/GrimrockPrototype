# ARMEMENT01.4 — Authoring du catalogue d'armement N1–N20

**Date :** 20 octobre 2026.
**Statut :** code C++ et outils préparés ; compilation et génération Unreal locales non encore validées.

## Architecture et source de vérité

Le document éditorial reste le classeur à 12 onglets :
docs/Design/ARMEMENT01_Catalogue_Armes_Boucliers_Niveaux_1_20_ARMEMENT01_2.xlsx

Il contient 82 archétypes x 20 niveaux = 1640 profils :
1380 attaques NdM+K (d4, d6, d8, d10, d12 ou d20) et
260 équipements passifs, sans faux jet de dégâts.

L'import produit un seul asset UE5 :
/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue

Runtime : UGridWeaponCatalogueAsset (UPrimaryDataAsset) contient des
FGridWeaponLevelProfile avec ArchetypeId, ItemLevel immuable, nom,
famille, tenue, type de dégâts, sous-type, attribut, précision,
vrais dés, armures, coût PA, portée et masse.

IsValidProfile et IsValidCatalogue vérifient les dés, les 1640 couples
uniques, les 20 niveaux de ARM001..ARM082, les 260 profils passifs et
la cohérence des attributs invariants entre niveaux. FindProfile donne
un modèle immuable. TryBuildAttack construit une FGridAttackDefinition
utilisable par le résolveur multi-dés ARMEMENT01.3, sans jet doublé.

Aucune classe d'objet existante n'est automatiquement migrée.
Le lien item-instance / niveau figé / catalogue / combat est réservé
à ARMEMENT01.5, après validation de cette phase d'authoring.

## Export et import

Scripts/ExportARMEMENT014Catalogue.ps1 :
lit le XLSX directement via ZIP/XML .NET, sans Excel installé
ni dépendance tierce, et produit un CSV temporaire sous
Saved/ARMEMENT01/ARMEMENT01_4_Profiles.csv.
Il vérifie les noms des douze onglets, les 1640 lignes,
les 260 profils non offensifs et les bornes calculées.
Il lit le cache des formules : en cas de cache absent, l'export
est rejeté plutôt que de deviner les valeurs.

UGridWeaponCatalogueImportCommandlet :
lit ce CSV ; rejette toute incohérence avant sauvegarde de
DA_WeaponCatalogue.uasset dans le dossier Weapons.
Ce CSV est généré et ne constitue jamais une deuxième
source de vérité éditoriale.

Scripts/AuthorARMEMENT014.ps1 :
exporte, compile, importe l'asset unique, lance les tests.

## Exécution locale sur master (working tree propre)

    cd D:\Development\GrimrockPrototype
    git pull --ff-only origin master
    .\Scripts\AuthorARMEMENT014.ps1 -EngineRoot D:\UE_5.5

Vérifier l'annonce 82 / 1640 / 260, la réussite du
Development Editor build, l'import du .uasset et les tests
Grimrock.RPG.ARMEMENT01.4.

Après succès uniquement, versionner le binaire dans un
deuxième commit Git LFS :

    git status --short
    git add Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.uasset
    git diff --cached --name-only
    git commit -m "data(ARMEMENT01.4): materialize 1640 fixed weapon profiles"
    git push origin master

Le script ne committe jamais de façon autonome.

## Campagne de non-régression

    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.4"
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.3"
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock"

Référence validée avant ARMEMENT01.4 :
1460 tests réussis, 0 avertissement, 0 échec
(TD04.2 du 20 octobre 2026 à 16h00).

Cinq tests ARMEMENT01.4 vérifient les profils d'attaque,
les boucliers passifs, les 1640 enregistrements, les doublons
et la compatibilité avec les vrais jets du moteur.

## Hors périmètre

Rareté, pénalités de niveau, forge, runes, affixes, butin,
sérialisation des instances et modifications UI.
ARMEMENT01.5 suivra après validation de l'asset créé localement.
