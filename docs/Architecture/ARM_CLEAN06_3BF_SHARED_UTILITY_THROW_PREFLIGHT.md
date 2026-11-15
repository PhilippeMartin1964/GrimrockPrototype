# ARM-CLEAN06.3BF — Précontrôle partagé des jets utilitaires

**Parent :** 06.3BE, commit `4d56651`, statuts Windows réussis.

Le menu contextuel `Throw` validait seulement la Force, le poids et
la main ; le Pawn demandait aussi vitesse et arc. Cela créait un faux
bouton « Lancer » disponible lorsque le jet ne pouvait pas être exécuté.

## Correction

- `FGridPhysicalThrowPreflightReadModel` compose les deux modèles
  existants sans ajouter de données, vitesse, arc ni coût.
- `GridItemContextActionLibrary` et
  `GridPartyPawnResolveThrowProfile` consomment le même résultat.
  Absence de balistique valide : bouton désactivé et réutilisation du
  `NSLOCTEXT ThrowMissingDefinition` existant, sans nouvelle chaîne.
- `FGridCanonicalThrowBallisticsReadModel` refuse le legacy
  `ThrowSpeed` non fini ou nul et `ThrowArc` non fini. Les
  valeurs finies signées restent intactes (l'aim applique déjà
  `Max(0, Arc)` au moment du lancer).
- ARM de jet N1–N20 : `Weapon.Projectile` canonique ; une ancienne
  définition même contradictoire ne prime jamais. Un ARM sans
  politique spécialisée continue d'exiger sa définition historique
  **exactement assortie** jusqu'à migration de cette physique
  via Unreal Editor (06.4).
- Les ItemId inconnus conservent les paramètres historiques valides
  et leurs contrôles de Force/poids, sans conversion de contenus.

## Couverture UE

`Grimrock.RPG.ARM.CLEAN06_3.PhysicalThrowPreflight` :
`CanonicalAndMenu` (N1/N20 de trois armes et vrai menu contextuel),
`GenericFallback` (jet d'épée, arc historique non fini et refus du
menu réel), `LegacyAndInvalid` (item non enregistré et ARM N0/N21).

Aucun `.uasset`, `.umap`, SaveGame, WBP ou texte localisé nouveau.
La chaîne `UI.csv → ST_UI.uasset → WBP → Game/fr/en` reste
**inchangée**. Le rendu de trajectoire GPU/Chaos exige le PIE 06.6.

**Dette ouverte :** conversion des paramètres de jet générique
non authorés en canonique (06.4), suppression des pointeurs
historiques (06.5), recette intégrale (06.6).
