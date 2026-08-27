# TD07.2 — UE deprecation cleanup / compiler & cook warning audit

Date : **27 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD07.1 — Build / Dependency Reproducibility**  
Baseline GitHub : `cd11b743d9450454698211be360bc088a65a220c`  
Statut : **IMPLÉMENTÉ — VALIDATION UE REQUISE**

## 1. Objet

TD07.2 traite uniquement les warnings **first-party** observés dans les builds/cooks réels. Les warnings Engine, plugins Epic et toolchain sont classés mais ne déclenchent pas de modifications opportunistes du projet.

## 2. Warnings first-party identifiés

### 2.1 `USkeleton::IsCompatible()` déprécié

Deux tests MON17 utilisent encore :

```cpp
USkeleton::IsCompatible(...)
```

Fichiers :

```text
GridMonsterMON172PresentationTests.cpp
GridMonsterMON178PresentationTests.cpp
```

UE5.5.4 émet C4996 et indique que la compatibilité est désormais une préoccupation Editor et qu'il faut utiliser `IsCompatibleForEditor()`.

Correction TD07.2 :

```cpp
IsCompatibleForEditor(...)
```

Les deux tests sont déjà `EditorContext`, donc cette API correspond au contrat réellement testé : compatibilité du Skeleton de mesh avec le Skeleton ciblé par l'Animation Blueprint.

### 2.2 Collision de nom Python Item Transfer

Le cook Shipping du 27 août 2026 émet :

```text
EGridItemTransferResult
FGridItemTransferResult

-> même nom GridItemTransferResult lors de l'exposition Python
```

Le C++ et les Blueprints utilisent déjà ces noms. Les renommer serait une modification de surface publique inutile.

Correction TD07.2 :

```cpp
UENUM(BlueprintType, meta = (ScriptName = "GridItemTransferResultCode"))
enum class EGridItemTransferResult : uint8
```

Le type C++ reste `EGridItemTransferResult`. Le struct reste `FGridItemTransferResult`. Seul le nom d'export vers les langages de script devient distinct.

## 3. Nouveau contrat TD07.2

Fichier :

```text
Source/GrimrockPrototype/Private/Tests/GridTD072EngineCompatibilityTests.cpp
```

Filtre :

```text
Grimrock.TechnicalDebt.TD07_2.EngineCompatibility.ReflectionContract
```

Le test verrouille :

- l'existence du `UEnum` ;
- `ScriptName = GridItemTransferResultCode` ;
- la conservation du nom réfléchi du `FGridItemTransferResult`.

La compatibilité Skeleton continue d'être couverte par les suites MON17.2 et MON17.8 existantes.

## 4. Warnings classés hors correction first-party

### MSVC non préféré

```text
Visual Studio 2022 / MSVC 14.44.35227
UE5.5.4 preferred : 14.38.33130
```

Classification : `TD-BUILD-002` surveillé. Editor et Shipping sont verts ; aucun downgrade préventif.

### FabLauncher / GLTFImporter

Le cook peut signaler qu'un plugin optionnel référencé par FabLauncher n'est pas construit/activé.

Classification : plugin Epic / hors dépendance GrimrockPrototype. Aucun changement first-party.

### Engine virtual texture / EditorDomain / AudioCapture

Les messages concernant les textures virtuelles Engine, cycles de classes RigVM/LiveLink et l'absence d'implémentation AudioCapture proviennent de l'environnement UE/plugin et n'empêchent pas le cook.

Classification : non actionnable côté projet tant qu'aucun symptôme first-party n'est observé.

## 5. Fichiers modifiés

```text
Source/GrimrockPrototype/Public/Runtime/GridItemTransferService.h
Source/GrimrockPrototype/Private/Tests/GridMonsterMON172PresentationTests.cpp
Source/GrimrockPrototype/Private/Tests/GridMonsterMON178PresentationTests.cpp
Source/GrimrockPrototype/Private/Tests/GridTD072EngineCompatibilityTests.cpp
docs/Design/TD07_2_UE_DEPRECATION_WARNING_AUDIT.md
docs/Architecture/TECHNICAL_DEBT_REGISTER.md
```

Aucun asset, SaveGame, Blueprint ou logique gameplay n'est modifié.

## 6. Validation requise

### Build + contrat TD07.2

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.TechnicalDebt.TD07_2"
```

Critère :

```text
1 Success / 0 warning / 0 Failed
```

Le build Editor ne doit plus émettre C4996 sur `USkeleton::IsCompatible`.

### Régression MON17.2

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.Monsters.MON17.2"
```

Critère :

```text
2 Success / 0 Failed
```

### Régression MON17.8

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.Monsters.MON17.8"
```

Critère :

```text
2 Success / 0 Failed
```

### Cook / Shipping

```powershell
.\Scripts\ValidatePackage.ps1 -EngineRoot D:\UE_5.5
```

Critère :

- package Shipping réussi ;
- le warning Python `GridItemTransferResult` ne doit plus apparaître.

## 7. Stop condition

TD07.2 est clos lorsque :

1. le build Editor est vert sans C4996 first-party MON17 ;
2. TD07.2, MON17.2 et MON17.8 sont verts ;
3. le package Shipping est vert sans collision Python ItemTransfer.

La prochaine tranche devient ensuite :

```text
TD07.3 — Save compatibility / legacy model audit
```
