# ARMEMENT-LOT02.14 — Remplacement de Throwing Knife historique

**État :** migration des deux kits C++ publiée ; suppression des assets Unreal
**expressément différée** jusqu'au relevé local des références binaires et
validation PIE. Travail effectué en un lot, sans micro-tickets.

## Inventaire autoritaire au 29 octobre 2026

| Ressource | Ancien | Canonique |
| --- | --- | --- |
| DataAsset | `DA_Weapon_ThrowingKnife` | `DA_Weapon_ThrowingKnife_01` |
| Archetype | hors manifeste | `ARM054`, `ThrowingOneHand`, actuellement `Planned` |
| Mesh | `SM_ThrowingKnife` | `SM_Weapon_ThrowingKnife_01` |
| Icône | `T_Weapon_ThrowingKnife` | `T_Weapon_ThrowingKnife_01` |
| MI | `MI_ThrowingKnife` | `MI_ThrowingKnife_02` |
| PBR | `T_ThrowingKnife_01_{BC,N,ORM}` | `T_ThrowingKnife_02_{BC,N,ORM}` |

Les sept packages legacy sont conservés jusqu'à la preuve que leurs références
sont nulles (ou correctement remplaçables), dans chaque projet/carte/Blueprint.
Le dépôt GitHub conserve les .uasset sous Git LFS ; leurs liens binaires internes
ne sont pas audités par simple recherche texte. Un nom de fichier seul ne
prouve donc ni usage, ni possibilité de suppression. Les animations
`GoblinThrower` qui contiennent le nom ThrowKnife ne sont PAS à supprimer
par analogie de nom. Le shuriken historique est indépendant.

**Références C++ exactes connues avant migration :**
- `RPGStartingEquipmentPresetCatalog.cpp` : `Warrior_Skirmisher` et
  `Rogue_Shadow` donnaient `DA_Weapon_ThrowingKnife` dans le sac.
- `GridEditorARMEMENTINDUS05MatrixTests.cpp` :
  `LegacyThrowingItemsPreserved` exigeait le vieux couteau et le shuriken.
- Scripts/notes historiques d'art qui mentionnent l'ancien nom : ne
  constituent pas à eux seuls des dépendances runtime ; auditer leurs
  usages avant tout retrait du dépôt.

## Migration C++ de ce lot (sans toucher aux .uasset)

Les deux kits utilisent désormais `DA_Weapon_ThrowingKnife_01` (ARM054).
Leur dague en MainHand, destination bag, quantité 1 et kit par défaut
restent inchangés. L'activation `Planned -> Active` du manifeste n'est
pas effectuée implicitement, pas plus que la migration des 18 kits
candidats (un autre circuit distinct).

Le test industriel existant (7 tests au total) vérifie :
- DataAsset ARM054, identité, capacité de lancer et résolution SaveGame
  par identifiant **exact**, sans remappage d'ancienne identité ;
- kits transitoires Warrior_Skirmisher et Rogue_Shadow : objet ARM054
  canonique dans le sac, dague en MainHand, quantité 1 ;
- immutabilité des vrais assets de classe, du nouveau DataAsset et des
  assets legacy ;
- scan AssetRegistry en lecture seule de sept anciens packages, sortie
  `Saved/ARMEMENT-LOT02/LOT02_14_LegacyThrowingKnifeReferences.csv`.

L'inventaire `Saved/` contient `Exists`, `RegistryQuery`, les
références par package et un verdict. Si `ReferencerCount > 0` :
NE RIEN SUPPRIMER, migrer la dépendance Unreal d'abord. Même en absence
de références, confirmer dans **Reference Viewer** avant suppression.

## Sauvegardes : stratégie explicite, sans réécriture silencieuse

Le SaveGame prototype utilise une validation de schéma exact-match.
Les objets sont identifiés par `ItemDefinitionId` et `RuntimeObjectId`.
La migration des kits influe sur les **nouveaux personnages seulement**.
Une ancienne sauvegarde portant l'ancien ID n'est pas automatiquement
convertie en ARM054 : aucun faux GUID, changement de statistiques ou
substitution silencieuse. Avant suppression du DataAsset legacy, décider
explicitement si les anciennes sauvegardes de test seront abandonnées
ou si une migration validée est requise. Le test de réhydratation du kit
vérifie le nouveau `ItemDefinitionId` littéralement.

## Recette rapide après `git pull --ff-only origin master`

Fermer UE Editor puis :

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT.Industrial"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.ARMEMENT01.9"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.ARMEMENT.LOT02"
Import-Csv .\Saved\ARMEMENT-LOT02\LOT02_14_LegacyThrowingKnifeReferences.csv | Format-Table -AutoSize
```

Sur PIE tester les deux kits : le couteau ARM054 est obtenu
une seule fois au départ, dans le sac ; lancement, chute, récupération
et Save/Continue conservent son GUID et son identifiant. Ne pas supprimer
d'asset binaire sur la seule base d'un test compilé.

**Interdictions :** aucune suppression/redirector aveugle, aucune
modification de `L_Dungeon`, `DA_GridLevel_00`, palette, niveaux,
classeur Excel local, ni des assets de flèches non commités.
