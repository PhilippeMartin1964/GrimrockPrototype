# RPG-TALENT-FIX02 — D01 Second souffle à PV maximum

Date : **8 octobre 2026**  
État : **SOURCE / TEST PRÊT — validation locale requise**

## Contrat

```text
Second souffle
PV restaurés : 20 % des PV maximum
Condition : indisponible à PV maximum
```

## Résultat de l'audit

Aucune correction du runtime n'est nécessaire.

`Action_Warrior_SecondWind` est une action :

```text
SourcePolicy      = Ability
ResolutionProfile = Effect
TargetingPolicy   = Self
RestoreHealthMaximumPercent = 20
```

Le catalogue générique des actions de classe calcule le soin effectif pour les
actions `Self + Effect`.

À PV maximum :

```text
HealthAfter == CurrentHealth
aucun autre effet applicable
=> AvailabilityReason = NoApplicableEffect
=> bEnabled = false
```

Le même mécanisme est déjà utilisé pour empêcher de gaspiller d'autres soins.

## Correctif

RPG-TALENT-FIX02 n'ajoute **aucune branche gameplay spécifique**.

Il ajoute un test de régression utilisant la vraie définition authorée de
`Action_Warrior_SecondWind` et vérifie :

1. à 80/100 PV, l'action est disponible ;
2. à 100/100 PV, elle reste visible mais reçoit `NoApplicableEffect` ;
3. à 100/100 PV, `bEnabled == false`.

Cette approche conserve une seule autorité générique et évite :

```cpp
if (ActionId == "Action_Warrior_SecondWind")
```

## Validation attendue

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.RPG.RPG03.9.1"
```

Aucun DataAsset ne doit être rematérialisé pour ce ticket.
