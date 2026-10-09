# WORLDOBJ-MOVINGPARTS01 — tableau générique de parties mobiles

> **DOC-ARCH01 — HISTORIQUE / SNAPSHOT.** Ce document décrit le jalon indiqué à sa date et peut mentionner des APIs, versions Save, compteurs ou états de roadmap désormais superseded. Il ne constitue pas le schéma courant. Références actuelles : `PROJECT_SYNTHESIS.md`, `ARCHITECTURE_INDEX.md` et `Maps/GRIMROCK_PROJECT_MAP.md`.


`UGridWorldObjectDefinitionAsset` décrit sa composition visuelle avec :

- `StaticPart` : zéro ou une partie statique ;
- `MovingParts` : zéro à N entrées `FGridWorldObjectMovingPart` ;
- chaque entrée conserve exactement `Mesh`, `LocalTransform` et `Motion` (`Type`, `Axis`, `Pivot`, `Amount`, `Duration`, `ReverseDuration`) ;
- un mécanisme générique applique un même alpha normalisé 0..1 à toutes ses parties mobiles.

Il n’existe ni `MaxMovingParts`, ni groupe, ni timeline, délai, courbe, canal ou type de mouvement supplémentaire.

Les exceptions de gameplay restent explicites : Button, Lever et Pressure Plate utilisent seulement l’index 0 ; Pit utilise exclusivement les index 0 et 1 pour ses deux feuilles. Les overrides d’instance restent un tableau sparse adressé par `PartIndex`.
