# UI-LOCALIZATION01.10A — Residual P0 presentation migration

Date : **13 octobre 2026**

## Objet

Fermer le bloc P0-E identifié par `UI-LOCALIZATION01.2` sans créer de nouvelle
couche architecturale.

Le ticket migre huit textes joueur résiduels vers l'autorité UI commune
`Localization/Source/UI.csv`.

## Périmètre

### Inventory

`GridInventoryWidget.cpp` :

- `Poids : %.1f / %.1f`
  devient `Inventory.Bag.Weight`.

La précision à une décimale reste une règle de présentation C++ via
`FNumberFormattingOptions`; seul le format linguistique est externalisé.

### Save / Load

`GrimrockGameInstance.cpp` :

- `Sauvegarde principale`
  devient `SaveLoad.Slot.Primary` ;
- `Sauvegarde %d`
  devient `SaveLoad.Slot.Numbered` avec `{Index}`.

`GrimrockLoadGameSlotWidget.cpp` :

- `Disponible`
  devient `SaveLoad.Slot.Available` ;
- `Indisponible`
  devient `SaveLoad.Slot.Unavailable`.

Les noms techniques de slots comme `GrimrockParty_2` restent des identités
runtime et ne sont pas localisés.

### Spellbook

`GridSpellbookUI.cpp` :

- `Définition de sort indisponible.`
  devient `Spellbook.Fallback.DefinitionUnavailable`.

Les noms et descriptions des sorts résolus restent authorés par leurs
définitions existantes.

### Skills

`GridSkillsWidget.cpp` :

- `Attribution de compétence impossible.`
  devient `Progression.Skill.AllocationFailed` ;
- `Annulation de compétence impossible.`
  devient `Progression.Skill.CancellationFailed`.

Les raisons détaillées déjà fournies par `RPGProgressionFeedbackService` en
`LOCTEXT` restent inchangées.

## Garde-fou

Le test :

~~~text
Game.UI.Localization.LOCALIZATION01.ResidualPresentationCsvCoverage
~~~

vérifie :

- les huit clés attendues ;
- leur référencement exact par les cinq sources migrées ;
- la disparition des anciens littéraux P0 ;
- l'utilisation de formats nommés pour le poids et le slot numéroté ;
- la couverture du fixture anglais et du smoke runtime.

## 01.10B

Après validation source :

1. régénérer `ST_UI.uasset` ;
2. Gather / Export / Import / Compile ;
3. valider le smoke fr/en ;
4. exécuter les régressions Inventory, Spellbook et Skills ;
5. versionner uniquement les projections générées.

## Ce qui reste après 01.10

Le reliquat P0 principal de `LOCALIZATION01.2` est alors la frontière
Recruitment / services :

- Custom Recruit ;
- Story Companion registration ;
- Party Recruitment.

Ces services peuvent produire des diagnostics anglais puis les widgets les
convertissent directement en `FText`. La cible recommandée reste :

~~~text
service C++
  -> reason stable + données dynamiques
presentation
  -> texte localisable
~~~

Les nombreux `LOCTEXT/NSLOCTEXT` classés P1 sont déjà localisables et ne
constituent pas un blocage fonctionnel multilingue.
