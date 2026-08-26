# UI et flux de jeu — Fondation d’architecture

Date de référence : **26 août 2026**

## Principe

Les widgets présentent et déclenchent des contrats C++; ils ne doivent pas devenir l’autorité métier d’un système. Blueprint/UMG sert à composer, styliser et configurer les variantes concrètes.

## Flux principal

```text
Main Menu
  -> New Game / Continue / Load
  -> Character Creation ou Save restore
  -> Runtime dungeon
  -> Exploration / Inventory / In-game Menu
  -> Combat / Level Up / Spellbook / Skills
  -> Save / Transition
```

## Surfaces fonctionnelles

- `GrimrockMainMenuWidget`, LoadGame menu/slots ;
- `GridInventoryWidget` et slots/paper doll ;
- `GridPartyMemberWidget` ;
- character creation wizard ;
- recrutement Story Companion et Custom Recruit MON20 ;
- `RPGLevelUpWidget` ;
- `GridCombatHudWidget` et ActionPanel ;
- `GridSpellbookWidget` et entries ;
- `GridSkillsWidget` ;
- `GrimrockMenuWidget` pour les tabs du menu en jeu.

## Pages shell / futures

Le menu contient déjà Journal, Map, Recipes et Codex. Journal/Map/Codex ont été audités dans MON21.1 mais leur domaine métier n’est pas encore implémenté. MON21.2 n’est plus bloqué par TD04 ; sa reprise relève maintenant de la roadmap produit après les tranches de dette explicitement décidées. Recipes reste une fonctionnalité future.

L’existence de ces widgets n’est pas une dette technique.

## Recruitment UI

MON20 a fermé le recrutement : Story Companion et Custom Recruit réutilisent les services C++ de recrutement et le wizard existant. Le Blueprint ne réimplémente pas les validations de groupe, identité ou ownership.

## Combat UI

Le HUD ne décide pas initiative/coûts/résolution. Il reflète le Turn Manager et le catalogue d’actions. La hotbar est persistante dans l’état de personnage.

## Skills / Spellbook

Les pages Skills et Spellbook suivent `SelectedCharacterIndex` et projettent les autorités C++ existantes. Elles ne possèdent aucune copie gameplay indépendante.

Spellbook et SkillRanks sont persistés dans le SaveGame courant **v9**.

## Sélection de personnage / held visual

`TD-PARTY-001` est **RÉSOLU**. `UGridPartyInventoryComponent` reste l’autorité de `SelectedCharacterIndex`. Le changement de sélection émet la notification autoritaire et `AGrimrockPartyPawn` resynchronise le held visual. Les contrats `SelectionChange` et `SelectedCharacterFilter` ont été validés sous UE5.5.4.

## Dette technique UI active

Le registre autoritaire est :

```text
docs/Architecture/TECHNICAL_DEBT_REGISTER.md
```

Points UI encore actifs ou surveillés :

- `TD-UI-001` : nommage historique `Inventory` du shell global, faible priorité ;
- `TD-LOG-001` : taxonomie de logs encore partiellement `LogTemp` ;
- risque de divergence visuelle entre nombreuses surfaces UMG, à traiter par conventions/composants partagés sans déplacer la logique métier en Blueprint.

`TD-PARTY-001` ne doit plus être listé comme dette active.

## Validation

Le code UI/C++ touché par un jalon doit passer le harness local approprié :

```text
Scripts/ValidateUE.ps1
```

Les changements de bindings, widgets Blueprint ou assets nécessitent toujours une validation PIE ciblée. TD04.3 a également validé le packaging Win64 Shipping du projet via `Scripts/ValidatePackage.ps1`.

## Règle de réduction de dette

Conserver les contrats C++ existants et éviter tout graphe Blueprint métier parallèle. Une extraction UI ou un renommage transversal n’est engagé que s’il réduit un risque concret.
