# MON13.5 — Clôture du pipeline `MonsterSpawn`

## Objectif

MON13.5 ferme les régressions révélées par la validation manuelle de MON13.3 et
MON13.4 sans ajouter de nouveau comportement de production :

```text
DA_MON_RatGiant
    → BP_MON_RatGiant_C
    → MonsterMovement + MonsterBehavior + MonsterCombat
    → MonsterSpawn désactivés
    → Trigger.Activated → StartEncounter
    → vague 0 en PIE frais
    → restauration normale par Continue
```

## Lacune corrigée

Les fixtures MON13 utilisaient une définition transitoire dont
`MonsterActorClass` conservait la valeur native par défaut
`AGridMonsterActor`. Cette classe validait le spawn et la présentation, mais ne
contenait pas les composants Blueprint `MonsterMovement` et `MonsterBehavior`.

Les tests pouvaient donc réussir alors que le TurnManager refusait le Rat réel :

```text
Monster requires MonsterMovement, MonsterBehavior and MonsterCombat components.
```

La carte versionnée a également évolué de `Trigger -> Rat.Spawn` vers une
rencontre de trois Rats. L'ancien test PIE réel continuait pourtant d'exiger un
seul lien `Spawn` et un seul Actor.

## Contrats automatisés

### Classe gameplay réelle

Les fixtures runtime chargent explicitement :

```text
/Game/GrimrockPrototype/Monsters/RatGiant/Blueprints/BP_MON_RatGiant.BP_MON_RatGiant_C
```

`RuntimePipeline` vérifie ensuite sur l'Actor créé :

- la classe choisie par la définition ;
- `MonsterMovement` ;
- `MonsterBehavior` ;
- `MonsterCombat`.

`ProductionAssetContract` charge directement `DA_MON_RatGiant` et exige que
`MonsterActorClass` soit exactement `BP_MON_RatGiant_C`, jamais la classe native
de base.

### Véritable session PIE

`RealPIEIntegration` utilise `L_GrimrockEditor` et les données réellement
versionnées :

- ancre `F7319908-4F46-EDCC-7D64-ED9C42588D57`, vague 0 ;
- second Rat `E4DC825C-490F-3B73-EA57-9EB7AC3D2AEA`, vague 0 ;
- Rat `AAF0E031-45A2-838D-0B4C-CB98FC04126C`, vague 1 ;
- groupe `Encounter_Rats_01` ;
- trigger `(27,24)` relié à `StartEncounter`.

Le test prouve :

1. qu'un playtest frais ne restaure aucun des trois Rats depuis la sauvegarde ;
2. que la cellule de départ ne déclenche rien ;
3. que l'entrée sur le trigger crée exactement les deux membres de la vague 0 ;
4. que la vague 1 reste absente ;
5. que les deux Actors possèdent les trois composants nécessaires au combat ;
6. qu'une seconde notification ne crée aucun doublon ;
7. que la sauvegarde temporaire reste strictement inchangée pendant le playtest
   frais ;
8. qu'une seconde session Continue restaure les deux membres et la vague active.

Le slot `GrimrockParty` n'est jamais utilisé par ce test.

## Tests

```text
Grimrock.Monsters.MON13.2.RuntimePipeline
Grimrock.Monsters.MON13.5.ProductionAssetContract
Grimrock.Monsters.MON13.5.RealPIEIntegration
```

Commande UE 5.5.4 :

```bat
D:\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe D:\Development\GrimrockPrototype\GrimrockPrototype.uproject -unattended -nop4 -nosplash -NullRHI -DDC-ForceMemoryCache -ExecCmds="Automation RunTests Grimrock.Monsters.MON13.5" -TestExit="Automation Test Queue Empty" -ReportOutputPath="D:\Development\GrimrockPrototype\Saved\TestReports\MON13_5"
```

Après MON13.5, exécuter aussi la régression complète :

```text
Grimrock.Monsters.MON13
Grimrock.Monsters.MON8.MonsterDiedEvent
```

## Hors périmètre

- démarrage automatique du combat par `StartEncounter` ;
- téléportation inter-niveaux ;
- résolution Asset Manager depuis `MonsterDefinitionId` seul ;
- suppression définitive d'un placement ;
- nouvelles familles de monstres.
