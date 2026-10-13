# UI-LOCALIZATION01.8A — Interaction prompts migration

> **Statut : CLOSED.** Migration : `ce871d5c6f19db3ad57b1f44b384d511426017e9`. Matérialisation 01.8B : `429b0acec51edf2d5c50c9663478d747ab723612`.


Date : **13 octobre 2026**

## Objet

Migrer le vocabulaire joueur générique des interactions monde vers
`Localization/Source/UI.csv`, sans déplacer les règles gameplay ni les
contenus authorés des World Object Data Assets.

## Périmètre

Les implémentations `GetInteractionText_Implementation()` suivantes utilisent
désormais des clés `Interaction.*` :

- chaîne de porte : ouvrir / fermer ;
- levier : tirer / pousser ;
- bouton : appuyer ;
- item monde : prendre ;
- réceptacle : prendre / placer ;
- objet lisible : lire.

Les fallbacks génériques de `AGridWallLockActor` sont également migrés :

- verrouillé ;
- déverrouillage réussi ;
- déjà déverrouillé ;
- clé adéquate manquante.

Les éventuels `Behavior.Lock.*Message` authorés restent prioritaires et ne
sont pas remplacés par la String Table.

## Non-objectifs

Le ticket ne modifie pas les règles de portée/collision, le routage souris,
les curseurs, les événements/liens, les diagnostics uniquement journalisés ni
les Data Assets de mécanismes.

## Tests

`Game.UI.Localization.LOCALIZATION01.InteractionPromptCsvCoverage` vérifie
les 12 clés attendues, leur référencement par les sept sources migrées, la
disparition des anciens `FText::FromString` joueur et la couverture du
fixture anglais / smoke runtime.

## 01.8B

Après validation de 01.8A : régénérer `ST_UI.uasset`, exécuter Gather /
Export / Import / Compile, valider le smoke fr/en, exécuter les régressions
d'interaction existantes, puis versionner les projections générées séparément.
