# ARMEMENT01.17.3A — Zone carrée et résolution C6 en combat

Date : 5 novembre 2026.
Base : d7b8fdc0 (assets artistiques approuvés). Aucun asset binaire modifié.

## Contrat mécanique — premier jalon

- Item_SurfaceCatalyst_01 reste le seul item projeté sur le nouveau
  contrat. Le DataAsset natif de l'artiste est préservé. Un pont de
  normalisation limité à cet ItemDefinitionId et son ActionId fait
  évoluer BuildQuickItemCombatActionDefinition() ; il sera retiré
  quand les champs binaires de gameplay auront été réauthorés.
- TargetingPolicy=Area ; AreaRadiusCells=1 ; AreaShape=Square.
  Zone carrée jusqu'à 3 × 3 = 9 cases accessibles, sans modifier
  les zones Manhattan des autres actions.
- bAllowNoSurfaceReaction=true : un ciblage valide sur sol neutre,
  Water ou Fire consomme normalement la fiole et ses 2 PA, sans
  inventer une surface nouvelle.
- Portée 4, ligne de vue, obstacles, identité et progression N1 conservés.
- Propagation via CanMove : murs et portes bloquées arrêtent la réaction.
- FGridCatalystAreaResolver classe toutes les cellules depuis un snapshot
  unique avant dépense. Parcours déterministe borné, sans récursion.
  Oil→Fire, Poison/PoisonCloud→Fire (flag explosif), Ice→Water ;
  Water/None/Fire restent inchangés.
- Une action de zone dépense une seule fiole et deux PA en combat.
  Les réactions ne sont pas redéclenchées par des mutations précédentes.
- VFX provisoire : un seul pulse central par fiole, fizzle inclus.
  Projectile, visuels par cellule, dégâts immédiats et exploration
  sont explicitement hors périmètre.

## Prochaines étapes

- 17.3B : consommation autoritaire hors combat, dégâts et durée commune.
  Le flag bUsableDuringExploration ne donne aucune autorisation de
  déclencher l'action depuis le HUD tant que 17.3B n'est pas fait.
- 17.3C : projectile, impact et VFX locaux simultanés.
- 17.3D : réauthoring du vrai DataAsset et retrait du pont C++, nettoyage
  de l'ancien art procédural et vérification du packaging.
- Les assets SM_Catalyst_01, T_Catalyst_Icon, instances et textures PBR
  approuvés d7b8fdc0 ne sont ni recréés ni remplacés.

## Tests

- Grimrock.RPG.ARMEMENT01.17.3A.SquareArea
- Grimrock.RPG.ARMEMENT01.17.3A.MixedSurfaceSnapshot
- Grimrock.RPG.ARMEMENT01.17.2B.CatalystCombatHotbar (zone mixte / Fizzle)
- Grimrock.RPG.ARMEMENT01.17.2.ProductionCatalyst
- Grimrock.RPG.ARMEMENT01.17.2C.ProductionVisualAssets (nouveaux assets)

Validation : UE5.5.4/LocalAuto sur le SHA exact, suite complète Grimrock.
Le statut headless -NullRHI ne valide pas le rendu graphique en PIE.

## ARMEMENT01.17.3A.1 — Correction d'assertion du test (5 novembre 2026)

- Base : `4f62792a`, incident de validation UE5.5.4 [#43](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/43).
- Le test `MixedSurfaceSnapshot` ajoutait `Source[0]` directement à `Source`,
  ce qui viole la règle `TArray::CheckAddress` et peut interrompre l'éditeur.
- Copie du snapshot dans une variable locale avant `Add` ; le test conserve
  sa vérification de rejet des cellules dupliquées et de sortie vide.
- Aucun changement aux règles C6, au runtime, aux assets ni aux sauvegardes.
- Validation requise : `UE5.5.4/LocalAuto` sur le nouveau SHA puis recette PIE/RHI séparée.
