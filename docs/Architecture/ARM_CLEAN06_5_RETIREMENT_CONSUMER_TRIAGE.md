# ARM-CLEAN06.5 — Analyse H.23 et conversion des recettes de départ

## Source de preuve

Le rapport local `h23-static-consumers-063e6c53.json` déclare le schéma
`ARM-CLEAN06.5H.23/private-static-retirement-blockers/v1`, SHA attendu
`063e6c535f75da2e6b5687215e01e1765a2a8b39`. Il a examiné
**1 571 fichiers** et trouvé **147 mentions textuelles** dans **43 fichiers**
distincts : 63 `game_source` (un seul fichier), 6 `game_test` (3),
20 `editor_tool` (8), 47 `editor_test` (23) et 11 `script` (8).
Les 48 définitions sont inventoriées; 11 n'ont aucune mention textuelle.

La présence d'une mention peut représenter une identité attendue, une
fixture, un commentaire ou un ancien chemin de chargement. Son absence ne
prouve ni la non-utilisation dynamique ni la sûreté d'une suppression.

## Modification de ce lot

Le registre des **18 kits de départ actifs** (`ApprovedKits`) contient
désormais uniquement des `ItemId` canoniques, niveaux, quantités et slots.
La double désignation `DA_Weapon_*/DA_Item_*` dans le tableau de production
est supprimée. Les résolutions et validations continuent à utiliser
`UGridItemSubsystem`. En Automation Editor *sans registre*, la comparaison
d'anciennes recettes synthétise le nom `DA_` + `ItemId` et est strictement
fermée hors `GIsAutomationTesting`. Les 12 kits historiques distincts,
utilisés par l'ancien audit de cohérence, ne sont **pas** présentés comme
migrés ni supprimés.

Le self-test `InspectARM_CLEAN06_5H23_HistoricalSources.ps1 -SelfTest`
empêche la réintroduction de noms de package historique directement dans
les recettes `ApprovedKits`.

## Prochain lot cohérent : outils et tests Editor

- Convertir les 23 fichiers de tests Editor et trois fichiers de tests de
  jeu pertinents vers la preuve canonique ou retirer, avec justification,
  leurs anciens tests de comparaison devenus non applicables.
- Revoir les huit outils Editor, notamment les anciens importeurs / auteurs
  de potions, de torches, d'accessoires, de munitions et les spécifications
  d'armes, sans conserver de deuxième source d'autorité.
- Revoir séparément huit scripts, les deux catalogues `DA_WeaponCatalogue`
  et `DA_WeaponVisualCatalogue`, puis le redirecteur `DA_Weapon_ThrowingKnife`.
- Protéger les Readables `DA_BookPage_Test` et `DA_Scroll_Test` référencés
  par une sauvegarde v27; ne pas effacer les deux fichiers ni leurs chemins.

## Retrait physique interdit avant contre-audit

Le rapport relève `evidence_not_bound_to_this_head=true`,
`deletion_authorized=false`. Avant d'effacer les **48 DataAssets**, il faut
rejouer l'AssetRegistry natif et la preuve des 7 SaveGames sur le SHA de
transaction, contrôler les charges dynamiques et les outils Editor encore
actifs, protéger les binaires Git LFS et passer la recette PIE, Cook,
Shipping et Save/Continue. Toutes les suppressions natives passent par UE
Editor, jamais par un effacement brut de fichiers.
