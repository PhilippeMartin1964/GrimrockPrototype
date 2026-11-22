# ARM-CLEAN06.5 — Tests Editor des kits de départ, sans anciennes définitions

## Constat

L'inventaire H.23 a identifié des dépendances de tests sur les anciens
`DA_Item_Potion_Healing_01`, `DA_Item_DisarmTools_01`,
`DA_Item_Arrows_01` et `DA_Item_SurfaceCatalyst_01`.
Leurs identités, règles, actions, poids, visuels et capacités sont
désormais présents sous `DA_ItemSystem`. Ces tests peuvent et doivent
examiner la vraie source de production, sans charger des packages à retirer.

## Portée du lot

- `GridEditorCanonicalStarterTestWorld.h` introduit un monde
  `EWorldType::Game` transitoire, un `GameInstance` et son véritable
  `UGridItemSubsystem`. L'inventaire est attaché à un acteur de ce monde,
  au lieu du vieux composant isolé qui autorisait une fixture historique.
- `GridEditorARMEMENT0115HealthPotionTests.cpp` vérifie l'action
  de soin/consommation N0 canonique, la création Priest / Apothecary,
  l'identité et l'ownership des potions ainsi que leur projection SaveGame.
  Le test purement synthétique de l'ancienne classe est maintenu pour
  caractériser l'authoring, sans chargement de package historique.
- `GridEditorARMEMENT01161DisarmToolTests.cpp` vérifie le Tool.Disarm
  N0 canonique et la création Saboteur dans le vrai registre (pas de
  définition héritée, pas d'action ou de stack implicite).
- `GridEditorARMEMENTLOT025MarksmanAmmunitionTests.cpp` emprunte le
  kit Ranger Marksman déjà approuvé : arc N1, dague N1 et flèches N0
  sont résolus uniquement dans ItemSystem; 12 -> 11 flèches,
  invariant GUID et persistance Save/Continue en mémoire, sans ancien
  callback `RehydrateOwnedItemDefinitions`.
- `GridEditorARMEMENT01172SurfaceCatalystTests.cpp` conserve la
  caractérisation synthétique et migre trois contrôles de production
  vers le vrai catalyseur N0, l'action détaillée et ses mesh/icon et
  règles Area1/LOS/Fire.

Toutes les suites Automation conservent leur nom. Aucun package natif,
classe de production, fichier .sav ou WBP n'est modifié par ce lot.

## Retrait et recette

Le rapport H.23 et la preuve native 5A restent les preuves de départ,
pas une autorisation de suppression. À la fin du cutover Editor, régénérer
la preuve 5A sur le SHA exact; sauvegarder les sept fichiers privés et
les binaires historiques, vérifier les deux Readables v27 et procéder
via Unreal Editor avec contre-audit. Les anciens importeurs/commandlets
et leurs tests peuvent encore recréer des assets : leur retraite
contrôlée reste nécessaire, distincte de ce lot.
