# MON12.12 — Clôture et validation complète de MON12

## Référence auditée

- Branche : `master`.
- Base publiée : `a3a29a953c8bbb39c0db4b811487fa796065288c`.
- Intitulé : `Fix MON12 action transactions and cooldowns`.
- Portée du correctif de départ : 16 fichiers, 743 ajouts et 133 suppressions.

Pendant l'audit, `origin/master` a reçu le commit indépendant
`cb66481b301bf172c06d90d0290de3fef54d8ba8` (`Add global project architecture
synthesis`), limité à `docs/Architecture/PROJECT_SYNTHESIS.md`. Il a été
conservé par fast-forward avant la clôture MON12.12.

Les fichiers audités sont :

1. `GridPlayerAttackPresentationComponent.cpp` ;
2. `GridTurnManagerComponent.cpp` ;
3. `GridTurnManagerPhases.cpp` ;
4. `GridTurnManagerPlayerActionCatalog.cpp` ;
5. `GridTurnManagerPlayerActions.cpp` ;
6. `GridPartyInventoryComponent.cpp` ;
7. `GrimrockPartyPawn.cpp` ;
8. `GridMonsterMON1141ThrownWeaponTests.cpp` ;
9. `GridMonsterMON12CombatHudTests.cpp` ;
10. `GridCombatTypes.h` ;
11. `GridPlayerAttackPresentationComponent.h` ;
12. `GridTurnManagerComponent.h` ;
13. `GridPartyInventoryComponent.h` ;
14. `GrimrockPartyPawn.h` ;
15. `MON12_11_HOTBAR_VALIDATION.md` ;
16. `MON12_6_COMBAT_ACTION_CATALOG.md`.

## Corrections de clôture

Deux lacunes réelles subsistaient autour d'un DataAsset d'arme de jet mal
configuré :

- une action d'équipement `bThrowable` déclarant un coût `0` pouvait lancer
  sans retirer d'unité ;
- un shuriken d'inventaire déclarant un coût supérieur à `1` pouvait retirer
  plusieurs unités pour un seul projectile.

Le catalogue normalise maintenant les deux provenances à une unité. Le
`TurnManager` impose également cette valeur dans la transaction autoritaire,
indépendamment de l'instantané transmis par le HUD. Les tests utilisent
volontairement des coûts invalides de `0` et `2` afin de couvrir les deux
régressions.

Le contrat de MON12.8.9 reste inchangé : l'utilisation acceptée d'un
consommable désaffecte son raccourci, même si une autre unité demeure en
inventaire. Un refus conserve en revanche l'objet, les ressources et le
raccourci.

## Invariants vérifiés statiquement

| Invariant | Preuve dans le code |
|---|---|
| Un seul shuriken consommé | Le catalogue et `RequestCharacterAttackInternal()` imposent `SourceItemQuantityCost = 1` pour `bThrowable`. |
| Paiement avant dégâts | PA, source, mana et cooldown sont engagés avant `ResolveAttack()`, les delegates et `ApplyAttackResult()`. |
| Refus sans perte | Les validations précèdent toute mutation ; un échec de création restaure l'équipement et les PA. |
| Cooldown par personnage | La clé runtime associe `CharacterId` et `ActionId`. |
| Cooldown limité au combat | La table est vidée au démarrage, à la victoire/défaite et à l'abandon. |
| Raccourci conservé sur refus | Le refus n'appelle aucune mutation de hotbar ; MON12.11 vérifie l'identité stable après refus. |
| Action interdite après combat | Le catalogue renvoie `CombatInactive` et l'entrée autoritaire reconstruit ce catalogue avant exécution. |
| Présentation non autoritaire | Le projectile récupérable est préparé par le `TurnManager` ; la présentation réutilise ce pointeur et ne retire aucun item. |
| Ownership exclusif | L'équipement transfère une unité au monde ; le lancer d'inventaire crée l'unité monde puis retire exactement une unité de la source avant les dégâts. |

## UHT, includes et module

- `AGridThrownItemActor` est déclaré avant le `USTRUCT` qui contient le
  `TObjectPtr` transitoire.
- Le pointeur préparé est un `UPROPERTY(BlueprintReadOnly, Transient)` ; le
  dernier instantané d'attaque qui le contient est lui-même un `UPROPERTY`
  transitoire du `TurnManager`.
- Les déclarations et définitions des méthodes renommées correspondent ; les
  anciens symboles `CommitQuickItemResourcesAfterAttack` et
  `TryLaunchInventoryItemForAttackPresentation` ne sont plus référencés.
- Les nouvelles classes utilisées appartiennent au module runtime existant.
  `Core`, `CoreUObject`, `Engine`, `UMG` et `Niagara` couvrent toujours les
  dépendances ; aucune modification de `GrimrockPrototype.Build.cs` n'est
  requise.
- Aucun delegate public n'a changé de signature et aucun WBP ou autre
  `.uasset` n'est modifié par MON12.12.

## Matrice de compilation Win64

À exécuter depuis une invite Visual Studio sur le poste UE 5.5.4 :

```bat
D:\UE_5.5\Engine\Build\BatchFiles\Build.bat GrimrockPrototypeEditor Win64 Development D:\Development\GrimrockPrototype\GrimrockPrototype.uproject -WaitMutex -NoHotReloadFromIDE
D:\UE_5.5\Engine\Build\BatchFiles\Build.bat GrimrockPrototype Win64 Development D:\Development\GrimrockPrototype\GrimrockPrototype.uproject -WaitMutex
D:\UE_5.5\Engine\Build\BatchFiles\Build.bat GrimrockPrototype Win64 Shipping D:\Development\GrimrockPrototype\GrimrockPrototype.uproject -WaitMutex
```

| Cible | Résultat de l'environnement d'audit |
|---|---|
| Development Editor Win64 | Non exécuté : Unreal Engine/UBT et Windows absents. |
| Development Win64 | Non exécuté : Unreal Engine/UBT et Windows absents. |
| Shipping Win64 | Non exécuté : Unreal Engine/UBT et Windows absents. |

## Tests Automation

Inventaire statique actuel : 19 tests `CharacterCreation`, 142 tests
`Monsters.MON`, dont 21 MON11 et 49 MON12.

```bat
D:\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe D:\Development\GrimrockPrototype\GrimrockPrototype.uproject -unattended -nop4 -nosplash -NullRHI -ExecCmds="Automation RunTests Grimrock.CharacterCreation." -TestExit="Automation Test Queue Empty" -ReportOutputPath="D:\Development\GrimrockPrototype\Saved\TestReports\MON12_12_CharacterCreation"
D:\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe D:\Development\GrimrockPrototype\GrimrockPrototype.uproject -unattended -nop4 -nosplash -NullRHI -ExecCmds="Automation RunTests Grimrock.Monsters.MON" -TestExit="Automation Test Queue Empty" -ReportOutputPath="D:\Development\GrimrockPrototype\Saved\TestReports\MON12_12_Monsters"
D:\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe D:\Development\GrimrockPrototype\GrimrockPrototype.uproject -unattended -nop4 -nosplash -NullRHI -ExecCmds="Automation RunTests Grimrock.Monsters.MON11.Presentation.ThrownWeaponLifecycle" -TestExit="Automation Test Queue Empty" -ReportOutputPath="D:\Development\GrimrockPrototype\Saved\TestReports\MON12_12_MON1141"
D:\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe D:\Development\GrimrockPrototype\GrimrockPrototype.uproject -unattended -nop4 -nosplash -NullRHI -ExecCmds="Automation RunTests Grimrock.Monsters.MON12" -TestExit="Automation Test Queue Empty" -ReportOutputPath="D:\Development\GrimrockPrototype\Saved\TestReports\MON12_12_MON12"
```

Ces tests n'ont pas été exécutés dans l'environnement d'audit, qui ne contient
ni Unreal Engine 5.5.4 ni `UnrealEditor-Cmd.exe`. MON12.12 n'est considéré
comme validé en exécution qu'après obtention des rapports verts correspondants.

## Checklist PIE

- [ ] démarrer une nouvelle partie ;
- [ ] ouvrir l'inventaire ;
- [ ] attribuer, déplacer, échanger et retirer des raccourcis ;
- [ ] changer de personnage actif ;
- [ ] utiliser une arme de mêlée ;
- [ ] lancer puis récupérer un shuriken ;
- [ ] utiliser un consommable ;
- [ ] vérifier un refus sans consommation ;
- [ ] vérifier le mana ;
- [ ] vérifier un cooldown pendant une manche complète ;
- [ ] entrer puis annuler un ciblage ;
- [ ] terminer un combat ;
- [ ] sauvegarder et recharger ;
- [ ] confirmer la persistance des raccourcis.

La validation PIE doit être effectuée sur un build issu du SHA de clôture et
le SHA doit être reporté avec les résultats Automation.
