# ARMEMENT01.9.5.2 — Réintégration des affinités d’armement depuis Excel

**22 octobre 2026 — statut : implémentation C++/PowerShell publiée sous réserve de validation locale UE5.5.4 et matérialisation de l'asset.**

## Autorité unique et contrat

Le classeur existant
`docs/Design/ARMEMENT01_Catalogue_Armes_Boucliers_Niveaux_1_20_ARMEMENT01_2.xlsx`
reste **l'unique autorité éditoriale**.

- Onglet `Catalogue`, colonnes **A, P, Q** :
  `ArchetypeId`, `Classe principale`, `Classes secondaires`,
  pour les 82 archétypes.
- Onglet `Classes`, lignes des **six classes** : branches canoniques,
  armement prioritaire, exemples, alternatives, attributs et synergies.
- Onglet `Niveaux 1-20` : **1640 profils statistiques immuables**,
  non modifiés par ce ticket.

La chaîne de production est :

```text
XLSX original (Catalogue P/Q + Classes)
  → Scripts/ExportARMEMENT01952Affinities.ps1
  → Saved/ARMEMENT01/ARMEMENT01_9_5_2_Affinities.csv (82 lignes)
  → Saved/ARMEMENT01/ARMEMENT01_9_5_2_ClassPreferences.csv (6 lignes)
  → GridWeaponClassAffinityImport (commandlet)
  → DA_WeaponCatalogue.uasset (2 collections de métadonnées)
  → UGridWeaponCatalogueAsset::GetClassAffinity / FindClassPreference
```

Les fichiers CSV sous `Saved/` sont **générés à chaque import**.
Ils ne sont pas des matrices éditoriales autonomes et ne doivent pas
être committés comme une nouvelle source. Le script d'export **réutilise
le lecteur XLSX éprouvé d’ARMEMENT01.4**, sans toucher au classeur.

### API purement consultative

`EGridWeaponClassAffinity` : `Primary`, `Secondary` ou
`Unspecified`. `Unspecified` **ne signifie pas interdit**.
`UGridWeaponCatalogueAsset::GetClassAffinity(ArchetypeId, ClassId)`
classe un armement ; elle ne change ni `CanEquipToSlot`, ni inventaire,
ni talents, ni `FRPGStartingEquipmentService`.

`FindClassPreference(ClassId)` donne les orientations issues de l'onglet
`Classes`, telles quelles **pour la revue de design**. Ces champs de
conception sont de type `FString` et ne doivent **jamais** être
affichés directement dans un widget. Toute description visible par le
joueur devra passer par `Localization/Source/UI.csv → ST_UI`.

Les observations spéciales comme `Prêtre (marteaux)` sont conservées
dans `SecondarySourceLabel` exactement comme dans l'Excel.
Le classement `Secondary` est **indicatif** ; il n'encode aucun
prérequis ni aucun verrou par famille.

### Cas de contrôle documentés

| Archétype | Principal | Secondaire(s) notables |
|---|---|---|
| ARM001 Dague | Voleur | Rôdeur, Alchimiste |
| ARM045 Arc court | Rôdeur | Voleur, Alchimiste |
| ARM054 Couteau de jet | Rôdeur | Voleur, Alchimiste |
| ARM061 Baguette arcanique | Mage | Prêtre, Alchimiste |
| ARM069 Sceptre sacré | Prêtre | Mage |
| ARM075 Bouclier rond | Guerrier | Prêtre |

Le catalogue statistique demeure **inchangé** : `Profiles` garde 1640
entrées, le niveau de l'objet reste indépendant de la classe et les
dés/PA/portée/poids ne sont pas recalculés.

## Import dans Unreal — travail local contrôlé

Fermer Unreal Editor, puis sur master à arbre de travail propre :

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master

.\Scripts\AuthorARMEMENT01952Affinities.ps1 -EngineRoot D:\UE_5.5
```

Ce script :
1. exige un dépôt local propre et `DA_WeaponCatalogue.uasset` existant ;
2. génère les deux CSV **depuis le XLSX versionné** (en réutilisant
   l'exporter ARMEMENT01.4 pour les profils) ;
3. compile le `Development Editor` ;
4. importe **seulement** `ClassAffinities` et `ClassPreferences` dans le
   même `DA_WeaponCatalogue` ;
5. valide les trois tests
   `Grimrock.RPG.ARMEMENT01.9.WeaponAffinity` ;
6. vérifie la régression `Grimrock.RPG.ARMEMENT01.4` (5 tests) ;
7. refuse les modifications à des fichiers hors catalogue.

Résultat attendu : **3/3 et 5/5**, sans avertissement, à confirmer
dans les journaux TD04.2. Aucun résultat local n'est anticipé comme
déjà réussi ici.

La commande d'import peut être relancée après l'export, pour contrôler
l'idempotence :

```powershell
& "D:\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "D:\Development\GrimrockPrototype\GrimrockPrototype.uproject" `
  -run=GridWeaponClassAffinityImport -unattended -nop4 -nosplash -nullrhi
```

Premier import : `Changed=true`, deuxième : `Changed=false`.

### Publication séparée des données binaires

Après validation, **un seul asset binaire** devrait être modifié :

`Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.uasset`

Contrôler `git status --short`, `git lfs status`, puis publier ce
`.uasset` dans un **commit Git LFS atomique distinct sur master**.
Ne pas versionner les CSV sous `Saved/`.

## Ce que ce jalon n'implémente pas

- Pas de recette des 18 kits et pas de nouveau kit ajouté ;
- pas de création des 82 `UGridItemDefinitionAsset` ;
- pas de nouvelles représentations 3D, icônes ou attaques ;
- aucune restriction classe/arme, aucun bonus chiffré inventé ;
- pas de traduction implicite des champs `FString` du classeur.

La suite logique est **ARMEMENT01.9.6 — 18 kits N1 à partir
des données d'affinité matérialisées**, avec validation des
`UGridItemDefinitionAsset` réellement disponibles.
