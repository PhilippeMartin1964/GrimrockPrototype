# ARM-CLEAN06.5H.18 — Win64 Shipping : éliminer les chemins historiques inatteignables C4702

## Erreur reproduite par le journal local (21.11.2026)

`Scripts/ValidatePackage.ps1 -EngineRoot D:\UE_5.5 -Configuration Shipping` a lancé `BuildCookRun`, qui s'est arrêté pendant la **compilation** (UBT ExitCode=6, avant Cook), avec `C4702` « impossible d'atteindre le code » dans les modèles de lecture suivants :

- `Source/GrimrockPrototype/Private/Runtime/Combat/GridEquippedGuardItemReadModel.h` : corps de repli de parade/bouclier, premières erreurs lignes 54–64 ;
- `Source/GrimrockPrototype/Private/Runtime/GridCanonicalItemInteractionReadModel.h` : capabilities de lecture/hotbar, lignes 85–107 ;
- `Source/GrimrockPrototype/Private/RPG/GridStartingKitEquipmentPolicyReadModel.h` : fallback de politique d'équipement initial, lignes 87–92 ;
- `Source/GrimrockPrototype/Private/Runtime/GridReceptacleItemMetadataReadModel.h` : fallback de réceptacle, lignes 55–76.

Les erreurs également remontées sous `Engine/Source/Runtime/CoreUObject/Public/UObject/Object.h` concernent des `IsValid()` **inlinés depuis ces modèles**, pas une régression du moteur Unreal. L'avertissement « compilateur MSVC non préféré » est distinct de cette erreur et ne doit pas être traité par une désactivation globale de `C4702`.

## Cause et correction

En configuration Shipping, `WITH_DEV_AUTOMATION_TESTS=0`. Les modèles contenaient le schéma :

```cpp
#if WITH_DEV_AUTOMATION_TESTS
    if (Registry || !GIsAutomationTesting) return View;
#else
    return View;
#endif
    // LegacyDefinition ... : compilé malgré l'arrêt inconditionnel
```

Le compilateur optimisé signale donc le code suivant comme inatteignable. Le correctif est structurel, **sans suppression fonctionnelle des tests et sans affaiblissement des diagnostics** :

```cpp
#if WITH_DEV_AUTOMATION_TESTS
    if (Registry || !GIsAutomationTesting) return View;
    // Corps legacy réservé aux fixtures Automation : jamais compilé en Shipping
#endif // WITH_DEV_AUTOMATION_TESTS
    return View;
```

Le corps du fallback, y compris tous les usages de `UGridItemDefinitionAsset`, les profils d'arme et les actions historiques, est entièrement inclus dans la garde. Aucun item inconnu n'acquiert un fallback de production. Les chemins canoniques existants restent prioritaires.

## Tests permanents

`Scripts/AssertARM_CLEAN06_5H18_ShippingGuards.ps1 -SelfTest` est maintenant intégré au préflight `Scripts/ValidateUE.ps1` de l'agent Windows. Il exige pour chacun des quatre headers un unique bloc `#if WITH_DEV_AUTOMATION_TESTS` avec sa fermeture `#endif // WITH_DEV_AUTOMATION_TESTS`, le code legacy dedans et un `return` final commun. Une fixture négative simule l'ancien `#else return` et doit être rejetée.

Les deux statuts obligatoires `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` sur le SHA H.18 vérifieront la compilation et l'Automation **Editor**. Ils ne constituent **pas** une preuve de compilation Shipping, de Cook, de packaging ni de tests PIE.

## Validation native encore requise

Après les deux statuts Windows SUCCESS sur H.18, sur le poste local et sans écraser les sauvegardes :

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master
.\Scripts\ValidatePackage.ps1 -EngineRoot D:\UE_5.5 -Configuration Shipping
```

Vérifier les résultats de **Build, Cook, Stage, Package et Archive** ainsi que la présence de l'exécutable, du `.pak` et des assets RPG cookés. En cas d'une nouvelle erreur Shipping, ne pas conclure prématurément que le problème est terminé ; transmettre le journal suivant, et poursuivre les corrections sous lots atomiques.

Aucun `.uasset`, `.umap` ou `.sav` modifié par ce lot. L'ancienne palette `DA_EquippedWeaponActionIcons` reste retirée ; les autres candidats ARM-CLEAN06 restent protégés.
