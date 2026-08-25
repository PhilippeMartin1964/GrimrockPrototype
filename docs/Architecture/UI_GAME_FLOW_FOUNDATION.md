# UI et flux de jeu — Fondation d’architecture

Date de référence : **25 août 2026**

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

Le menu contient déjà :

```text
Journal
Map
Recipes
Codex
```

Journal/Map/Codex ont été audités dans MON21.1 mais leur domaine métier n'est pas encore implémenté. MON21.2 est suspendu pendant la phase d'exploitation/stabilisation. Recipes relève d'une fonctionnalité future.

L'existence de ces widgets n'est donc pas une dette technique.

## Recruitment UI

MON20 a fermé le recrutement : Story Companion et Custom Recruit réutilisent les services C++ de recrutement et le wizard existant. Le Blueprint ne réimplémente pas les validations de groupe, identité ou ownership.

## Combat UI

Le HUD ne décide pas initiative/coûts/résolution. Il reflète le Turn Manager et le catalogue d’actions. La hotbar est persistante dans l’état de personnage.

## Skills / Spellbook

Les pages Skills et Spellbook suivent `SelectedCharacterIndex` et projettent les autorités C++ existantes. Elles ne possèdent pas de copie gameplay indépendante.

Spellbook et SkillRanks sont persistés dans le SaveGame courant v8.

## Dette technique UI

Le registre autoritaire est :

```text
docs/Architecture/TECHNICAL_DEBT_REGISTER.md
```

Points UI réellement actifs :

- synchronisation de certaines présentations après changement direct de personnage sélectionné (`TD-PARTY-001`) ;
- nommage historique `Inventory` du shell global (`TD-UI-001`, faible priorité) ;
- taxonomie de logs encore partiellement `LogTemp` (`TD-LOG-001`) ;
- risque de divergence visuelle entre surfaces UMG nombreuses.

La règle de réduction de dette reste : conserver les contrats C++ existants et éviter tout graphe Blueprint métier parallèle.
