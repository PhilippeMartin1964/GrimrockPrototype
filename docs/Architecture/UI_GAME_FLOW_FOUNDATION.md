# UI et flux de jeu — Fondation d’architecture

## Principe

Les widgets présentent et déclenchent des contrats C++; ils ne doivent pas devenir l’autorité métier d’un système. Blueprint/UMG sert à composer, styliser et configurer les variantes concrètes.

## Flux principal

```text
Main Menu
  -> New Game / Continue / Load
  -> Character Creation ou Save restore
  -> Runtime dungeon
  -> Exploration / Inventory / In-game Menu
  -> Combat / Level Up / Spellbook
  -> Save / Transition
```

## Surfaces fonctionnelles

- `GrimrockMainMenuWidget`, LoadGame menu/slots ;
- `GridInventoryWidget` et slots/paper doll ;
- `GridPartyMemberWidget` ;
- character creation wizard ;
- `RPGLevelUpWidget` ;
- `GridCombatHudWidget` et ActionPanel ;
- `GridSpellbookWidget` et entries ;
- `GrimrockMenuWidget` pour les tabs du menu en jeu.

## Pages partielles

L’existence de `Skills`, `Journal`, `Map`, `Recipes` ou `Codex` dans le menu ne signifie pas que leur domaine métier est achevé. Leur contenu fonctionnel appartient à MON20/MON21.

## Recruitment UI

MON20.4 doit présenter le compagnon (`URPGStoryCompanionAsset`) et déclencher les services MON20.3/MON20.2. Les décisions « Recruter / Refuser / Voir la fiche » doivent rester des appels vers des contrats C++ testables ; le Blueprint ne doit pas réimplémenter les validations de groupe, identité ou ownership.

## Combat UI

Le HUD ne décide pas initiative/coûts/résolution. Il reflète le Turn Manager et le catalogue d’actions. La hotbar est persistante dans l’état de personnage.

## Dette

Les surfaces UI sont nombreuses et peuvent diverger visuellement. Toute extension doit conserver les contrats C++ existants et éviter les graphes Blueprint métier parallèles.
