# ARM-CLEAN06.4X.4 — Préflight réel des niveaux hérités, sans écriture

**Statut : contrôle Editor en lecture seule. Aucun .uasset ni SaveGame converti par ce jalon.**

## Base et besoin

Le commit final 06.4W
`a56f4b5c22a7d883b9e9daad432e876676f2ce82` a enregistré et
fait relire 95 `CanonicalItemId` réels dans quatre niveaux
`DA_GridLevel_*` et `DA_ObjectPalette_Default`. La règle C++ 06.4X.3
`FGridArmClean064X3AuthoredLevelMaterializer::TryResolve` sait
transformer un N0 d'ARM en niveau effectif uniquement à partir du
`DefaultWeaponItemLevel` **de sa véritable définition historique** ;
elle n'invente pas N1. Les non-ARM conservent N0.

**Problème restant :** les 95 `ItemLevel` actuellement enregistrés
peuvent encore refléter les valeurs d'authoring, pas nécessairement
les niveaux physiques qu'une sauvegarde ou un inventaire doit utiliser.

## Preuve réalisée par le nouveau test

Test dédié :
`Grimrock.RPG.ARM.CLEAN06_4X4.RealAuthoredLevelPreflight`.

Sur les vrais assets UE déjà migrés, le test :

1. Charge l'unique `DA_ItemSystem` à 34 définitions et le configure
   dans un `UGridItemSubsystem` transitoire.
2. Ouvre les quatre niveaux et la palette, et exige exactement
   **44 objets libres, 0 spawns, 18 contenus initiaux de réceptacles
   et 33 entrées directes de palette**, soit 95 références.
3. Pour chaque entrée, prouve que `CanonicalItemId` égale
   `ItemDefinitionId` de sa **source historique réelle** ; contrôle
   aussi la quantité positive, les GUID d'objets libres et
   l'unicité de ces GUID.
4. Applique la règle 06.4X.3 à chaque référence, en donnant la
   valeur de source `DefaultWeaponItemLevel` ; rejette les ARM N0
   sans source N1–N20, les niveaux explicites hors plage et les
   non-ARM qui ne restent pas N0.
5. Résout **le niveau effectif obtenu** par
   `UGridItemSubsystem::ResolveItem` et exige la même identité,
   la même famille et le même `ArchetypeId` pour chaque ARM.
6. Vérifie que les packages des quatre niveaux et de la palette
   n'ont pas changé d'état `Dirty`. Aucune instruction `SavePackage`,
   aucun `.sav` et aucune mutation de propriétés ne figurent
   dans le test.

La palette n'a actuellement **aucun champ de niveau par défaut**
propre à chaque entrée. Le test vérifie que les anciennes sources
permettent encore de déterminer ce niveau pour un **futur** placement
ARM. La conversion devra ensuite le stocker explicitement, pour ne
plus consulter la définition historique au runtime ou dans l'éditeur.

## Interdictions et suite

Un test `-NullRHI` ne prouve pas le rendu, Chaos, Niagara, l'interface
Slate/UMG ni Save/Continue. Ce jalon n'autorise ni la suppression des
pointeurs historiques ni un fallback de définition.

**Étape suivante**, uniquement après les contrôles Windows du SHA
de cette étape : convertir réellement les niveaux effectifs et le
niveau par défaut des entrées de palette dans un commandlet UE Editor
transactionnel, avec liste blanche, sauvegarde privée des cinq assets,
relecture dans un deuxième processus et parité des 95 références,
quantités, GUID, positions, règles d'énigme et lumières. Puis basculer
les lecteurs runtime/éditeur, migrer les `SaveGames` sur copies,
retirer les champs legacy et valider en PIE/Chaos/Cook.
