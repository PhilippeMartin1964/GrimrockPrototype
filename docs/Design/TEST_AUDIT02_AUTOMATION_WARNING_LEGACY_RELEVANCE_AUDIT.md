# TEST-AUDIT02 — Automation Warning & Legacy Relevance Audit

Date : **27 septembre 2026**

## Objectif

Partir du run complet `Grimrock` (962 tests, 0 fail, 14 tests avec warnings) et :

1. supprimer les warnings parasites produits par des fixtures incomplètes ;
2. marquer explicitement comme attendus les warnings qui constituent le comportement négatif testé ;
3. vérifier que les tests contenant `Legacy`, `Compatibility` ou `Characterization` protègent encore un contrat utile ;
4. ne pas réintroduire de compatibilité de sauvegarde obsolète et ne pas affaiblir le runtime.

Aucun code gameplay/runtime n'est modifié par ce ticket.

## 1. Les 14 tests avec warnings

### Warnings intentionnels — conservés comme assertions négatives

Ces warnings décrivent précisément le garde-fou exercé par le test. Ils sont
désormais déclarés avec `AddExpectedError`, afin qu'un run sain soit propre
sans masquer un warning inattendu.

- `Grimrock.MON19.2.Runtime.Logic.EventCommandChain`
  - rejet d'une récursion cyclique `LogicExecute` ;
- `Grimrock.MON19.4.LuaBridge.SharedActionBudget`
  - épuisement volontaire du budget partagé Event/Command/Lua ;
- `Grimrock.MON19.5.LuaPersistence.InvalidCurrentSourceDropsStaleVm`
  - source Lua invalide, compilation puis callback volontairement rejetés ;
- `Grimrock.Pit.PIT01.RuntimeFallLifecycle`
  - fallback de destination volontaire lorsque la cellule verticale est vide ;
- `Grimrock.Pit.PIT02.WorldItemsFallingThroughPit`
  - même contrat de fallback pour un World Item ;
- `Grimrock.TechnicalDebt.TD06_4.InventoryStackMerge`
  - split volontairement refusé lorsque l'inventaire est plein.

### Warnings parasites — fixtures complétées

- `Grimrock.Monsters.MON10.IdleVariations.IdleVariationSchedulingLifecycle`
  - ajout d'un vrai `AGrimrockPartyPawn` pour la réactivation combat ;
- `Grimrock.Monsters.MON11.Presentation.TargetReactionExclusivity`
  - ajout du composant Movement attendu par le chemin de mort ;
- `Grimrock.Monsters.MON11.PlayerResolutionDeathVictory`
  - la fixture utilise désormais le vrai Movement/Occupancy path ;
- `Grimrock.Monsters.MON13.3.DeferredSpawnLinks`
- `Grimrock.Monsters.MON13.3.LifecyclePersistence`
- `Grimrock.Monsters.MON13.4.EncounterWaves`
  - ajout d'un vrai Party Pawn afin que les Behavior Components soient initialisés comme en runtime ;
- `Grimrock.Pit.PIT03.ControlledStateAndLinks`
  - définition synthétique de Pit complétée avec StaticPart + HideCellFloor ;
- `Grimrock.WorldObjects.CEILING_OVERRIDE01.HideBaseCeiling`
  - Decoration synthétique complétée avec un StaticPart.

Les définitions PIT01 synthétiques ont également été complétées
(`StaticPart`, `RuntimeActorClass`, `bHideCellFloor`) afin que le test de
fallback ne transporte plus d'erreurs de validation sans rapport avec son sujet.

## 2. Audit Legacy / Compatibility

### Tests anti-legacy à conserver

Les tests suivants ne maintiennent pas une ancienne API ; ils garantissent au
contraire que le projet reste débarrassé de ces chemins :

- `RejectLegacySaveWithoutHotbar` : une sauvegarde antérieure au schéma de
  hotbar courant doit être rejetée, sans migration implicite ;
- `NoLegacy*`, `LegacySymbolsAbsent`, `LegacyInfrastructureRemoved`,
  `*LegacyBlueprintReferences` : garde-fous d'absence ;
- les caractérisations TD07.3.6 `LegacyPlacementMirrors` et
  `LegacyRebuildMode` vérifient explicitement que les miroirs et
  `ObjectsOnly` ont disparu.

Ils restent donc pertinents pour la politique du prototype : **pas de fallback
legacy réintroduit par inadvertance**.

### Test hotbar mal nommé

`Grimrock.Monsters.MON12.8.9.LegacyBindingsAreSanitized` ne teste aucune
migration de schéma. Il construit un état de hotbar courant contenant un doublon
d'équipement et un quick-item épuisé, puis vérifie la sanitisation.

Il devient :

`Grimrock.Monsters.MON12.8.9.InvalidBindingsAreSanitized`

Les variables internes `LegacyState` / `LegacyCharacter` sont renommées
`StateToNormalize` / `CharacterToNormalize`.

### `grid.vars.*` : API encore explicitement supportée

`grid.vars.get_bool/set_bool/get_int/set_int` n'est pas du code mort caché :

- la VM l'expose encore ;
- le compilateur d'authoring la valide ;
- le highlighter la connaît ;
- la référence Lua actuelle la documente ;
- les scripts Lua sont stockés dans les `UGridLevelAsset` binaires.

Le test anciennement nommé
`Grimrock.MON19.7.1.LuaAuthoring.LegacyGridVarsCompatibility` devient donc
`Grimrock.MON19.7.1.LuaAuthoring.GridVarsCompatibility`.

Le renommage reflète son statut réel : **API de compatibilité actuellement
supportée**, et non shim oublié.

La suppression de `grid.vars.*` n'est pas incluse dans TEST-AUDIT02 : elle
nécessiterait un ticket de rupture de contrat avec audit/migration explicite de
tous les scripts contenus dans les LevelAssets réels. Elle ne doit pas être
faite en aveugle sur des `.uasset`.

### Autres tests contenant “Compatibility”

- `RatGiantAssetCompatibility` vérifie que le DataAsset de production reste
  valide après normalisation : régression d'asset courante ;
- `NoProgressionChoicesCompatibility` vérifie qu'une classe sans choix de
  progression reste un contrat valide : compatibilité de configuration courante ;
- `TD07_2.EngineCompatibility.ReflectionContract` protège un nom réfléchi UE
  unique : compatibilité moteur, pas compatibilité de vieux gameplay.

Ils restent utiles.

### “Characterization”

Le mot `Characterization` n'est pas un indicateur de code mort. Les tests
TD07 caractérisent principalement des frontières d'autorité, l'absence
d'anciens symboles et les contrats de données actuels. Ils sont conservés tant
qu'ils protègent un invariant concret ; aucune suppression en masse n'est
justifiée par leur nom.

## 3. Validation demandée

D'abord les zones modifiées :

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.MON19.2.Runtime.Logic"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.MON19.4.LuaBridge"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.MON19.5.LuaPersistence"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Monsters.MON10.IdleVariations"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Monsters.MON11"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Monsters.MON13.3"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Monsters.MON13.4"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Pit"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.TechnicalDebt.TD06_4"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.WorldObjects.CEILING_OVERRIDE01"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.MON19.7.1.LuaAuthoring"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Monsters.MON12.8.9"
```

Puis le run global :

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock"
```

Critère cible : **962 tests exécutés, 0 fail, 0 not run, 0 succeeded-with-warnings**.
