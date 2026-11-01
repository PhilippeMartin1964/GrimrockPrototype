# TRAP-SEARCH01.1 — Active Trap Search Runtime

**Statut : code publié sur master ; validation compilation et tests UE5.5.4 requise.** 

## Implémentation

- `AGridLevelRuntimeActor::EvaluateActiveDungeonTrapSearch(AGrimrockPartyPawn*, FRandomStream&)` : API C++ synchrone, **pas encore accessible par un bouton ou une touche**.
- Refactorisation du parcours TRAP01.2 en `EvaluateDungeonTrapPerception` : un même contrôle des instances, cellules, sens, lumière, murs, portes, compétence et événements, sans changer la formule passive.
- Scan volontaire : axes cardinaux droits, 1 à 2 cases indépendamment de la portée liée au score passif, sous le plafond `MaxDetectionRangeCells` de chaque instance.
- Jet via `FRPGSkillCheckService::TryResolveSkillCheck` (même meilleur observateur vivant et vrai d20) ; bonus de recherche +2 **seulement après résolution**, sans modifier `ClassModifier`, `GroupModifier`, `ProgressionModifier`, la difficulté affichée ou l'historique de dés.
- Tentatives enregistrées avant le jet dans `FGridLevelRuntimeState::TrapActiveSearchAttemptedRangeMask` sous `UPROPERTY(SaveGame)`, par GUID et distance ; pas d'altération de `TrapPerceptionAttemptedRangeMask`.
- Les refus combat, chute, relocation, groupe invalide ou skill indisponible ne dépensent pas d'aléa. La future commande UI gérera les menus modaux et le ciblage.
- Une découverte appelle `TryRecordDungeonTrapDetection`, garde son acteur révélé et émet `Dungeon.Trap.Detected` via le même message localisé que TRAP01.2. Aucun événement négatif et aucune divulgation de GUID/difficulté sur un échec.
- Aucune modification de DataAsset de piège, d'UMG, de HUD, de police, de carte, de désamorçage ou de pièges temporaires de combat.
- L'ordre des tests de pièges suit l'ordre stable de placement du `LevelAsset`.

## Tests

`Grimrock.TRAP_SEARCH01.1` :
- `BonusAndSaveContract` : bonus, absence de mutation de l'état RPG, rejet, propriétés SaveGame.
- `RuntimeDetectionAntiRerollAndSave` : échec passif suivi d'une recherche active distincte, réussite, détection 3D et événement, échec silencieux, même distance, seconde distance, diagonale, zone hors portée, cap d'instance, mur, combat, relocation, fermeture du mur et Save/Continue binaire.

Validation recommandée :

```powershell
cd D:\Development\GrimrockPrototype
git status --short
git pull --ff-only origin master
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.TRAP_SEARCH01.1"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.TRAP01"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.MON20.6.Skills"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.TRAP_PIT01"
```

## Contrat UMG / Localisation

Aucun WBP ni texte joueur créé, `UI.csv` inchangé, `ST_UI` inchangé, `Game` fr/en inchangé. L'événement `Dungeon.Trap.Detected` conserve le `FText` préexistant issu de `NSLOCTEXT`. La chaîne complète `UI.csv → ST_UI → WBP → Game(fr/en)` n'a pas besoin de matérialisation nouvelle pour ce jalon.

## Étape suivante

TRAP-SEARCH01.2 : raccourci et/ou bouton d'action explicite « Examiner les alentours », garde-fous UMG, feedback neutre si nécessaire et recette PIE. **Ne pas déclarer TRAP-SEARCH01.1 clôturé sans compilation locale et tests.**
