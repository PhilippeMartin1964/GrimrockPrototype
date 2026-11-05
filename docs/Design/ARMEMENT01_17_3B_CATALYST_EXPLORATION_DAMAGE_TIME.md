# ARMEMENT01.17.3B — Catalyseur : exploration, dégâts et temps

Date : 5 novembre 2026.
Parent : acd9682b37d86498feb31f5a1db021e2ea6399c4.

## Dépendance 17.3A

L'agent Windows a diagnostiqué une erreur de compilation C4430 / C2143 dans
GridCatalystAreaResolver.h (FGridResolvedCombatModifiers non visible) et des
erreurs dérivées dans les tests. 17.3B ajoute le header autoritaire du
résolveur de modificateurs. Aucun commit correctif séparé n'est créé.

## Gameplay

- Exactement le QuickItem Item_SurfaceCatalyst_01 / Action_Mage_SurfaceCatalyst_01.
- Exploration hors combat : hotbar, ciblage de zone 3×3, portée 4 et obstacles
  identiques au combat, une bouteille consommée, 0 PA, 0 mana.
- Sol neutre, Water, Fire : lancer autorisé et résultat non réactif (fizzle).
- Plan calculé sur snapshot avant consommation, application atomique de
  toutes les réactions via CommitCatalystAreaPlan. Une tentative d'écriture
  invalide ne doit modifier aucune cellule ; l'inventaire est restauré
  si la validation finale échoue.
- En combat : un seul item et 2 PA, même zone et mêmes règles C6, sans
  duplications de déclenchements de surface.
- Poison / PoisonCloud + Fire : explosion immédiate de base 6 Fire
  pour les occupants des cellules qui réagissent, une fois par occupant
  et par lancer, avec FGridCombatResolver, résistances et armure.
  Les membres du groupe peuvent aussi subir le souffle s'ils occupent
  une cellule explosive.
- Oil/Poison/PoisonCloud devenus Fire : dégâts persistants de base
  2 Fire par round pour les occupants, pris en compte par le résolveur
  partagé. Aucune nouvelle surface Fire sur une case neutre.
- Ces deux magnitudes sont éditables sur GridLevelRuntimeActor :
  CatalystPoisonBurstDamage=6 et CatalystFireDamagePerRound=2.

## Durée et sauvegarde

- RemainingSurfaceSeconds (SaveGame) devient l'horloge des surfaces
  temporaires en exploration, à 6 secondes par round par défaut.
- RemainingOilSeconds est conservé pour les anciennes sauvegardes d'huile,
  avec repli si RemainingSurfaceSeconds est zéro.
- Pendant le combat : fin de round = retrait de 6 secondes équivalentes,
  y compris pour les surfaces ayant déjà vieilli en exploration.
- Hors combat : timer de 0,25 seconde déjà existant, désormais actif
  pour toutes les surfaces RPG03.6 et non seulement Oil.
- Les dégâts périodiques utilisent le même ResolveSurfaceDamageTick()
  dans les deux modes ; le timer hors combat les déclenche par tranche
  de 6 secondes sans décompter les tours de combat.
- Les VFX persistent uniquement selon l'état C6 ; le pulse provisoire
  reste central avant les travaux graphiques 17.3C.

## Vérifications

- Compiler les sources UE 5.5.4, en particulier le correctif 17.3A.
- Grimrock.RPG.ARMEMENT01.17.3A.SquareArea
- Grimrock.RPG.ARMEMENT01.17.3A.MixedSurfaceSnapshot
- Grimrock.RPG.ARMEMENT01.17.3B.ExplorationCatalyst
- Grimrock.RPG.ARMEMENT01.17.2B.CatalystCombatHotbar
- Grimrock.RPG.ARMEMENT01.17.1.6.ExplorationTime
- Grimrock.RPG.ARMEMENT01.17.1.6.SaveLoad
- Grimrock.RPG.ARMEMENT01.17.1.6.LegacySave
- Entire Grimrock suite via UE5.5.4/LocalAuto.

Régression préexistante identifiée sur le parent :
Grimrock.Items.VFX01_4A.ProductionAssetAudit (native .uasset).
Ne pas supprimer ni neutraliser ce test pour masquer une anomalie de contenu.
Aucune modification des assets artistiques approuvés d7b8fdc0.

## Localisation et présentation UI

Seul un texte dynamique C++ a été ajouté dans le flux d'événements
(GrimrockCatalyst/CatalystZoneExploration, NSLOCTEXT). Aucun label WBP,
ST_UI, UI.csv ou structure UMG n'a été modifié ou ajouté.
Chaîne UI.csv → ST_UI → WBP : sans changement, aucun nouvel asset binaire.
Cible Game/fr/en : collecte et traduction anglaise de ce NSLOCTEXT non
encore vérifiées ; le smoke Windows et la recette PIE restent à faire
avec le jalon de présentation 17.3C.
