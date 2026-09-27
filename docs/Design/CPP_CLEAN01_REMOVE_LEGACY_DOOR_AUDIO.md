# CPP-CLEAN01 — Remove Legacy Door Audio Migration

Date : **27 septembre 2026**  
Parent : **CPP-AUDIT01 — Audit général du code C++**

## Objectif

Supprimer la seconde représentation historique de l'audio des portes.

Avant ce ticket, `UGridWorldObjectDefinitionAsset` acceptait à la fois :

```text
AudioEvents + DefaultAudioAttenuation
```

et les anciennes propriétés :

```text
DoorOpenSounds
DoorCloseSounds
DoorAudioVolume
DoorAudioPitchVariation
DoorAudioAttenuation
```

Le runtime migrait encore ces données via `PostLoad()`, `ResolveAudioEvent()` et `AGridRuntimeObjectActor::ConfigureObjectAudio()`.

Le prototype n'a pas besoin de cette compatibilité ascendante.

## Autorité après CPP-CLEAN01

Une définition de porte possède exactement le même contrat audio que tout autre Grid Object :

```text
UGridWorldObjectDefinitionAsset
└── Audio
    ├── Attenuation
    └── Audio Events
        ├── Open
        └── Close
```

Les événements contiennent :

```text
Sounds[]
Volume
PitchVariation
```

Aucun champ Door audio spécifique n'existe plus en C++.

## Modifications

### UGridWorldObjectDefinitionAsset

Supprimés :

```text
DoorOpenSounds
DoorCloseSounds
DoorAudioVolume
DoorAudioPitchVariation
DoorAudioAttenuation
PostLoad() de migration audio
ResolveAudioEvent()
```

### AGridRuntimeObjectActor

`ConfigureObjectAudio()` devient une copie directe du schéma canonique :

```cpp
ObjectAudioEvents = Definition->AudioEvents;
DefaultObjectAudioAttenuation = Definition->DefaultAudioAttenuation;
```

Aucun fallback Door n'est reconstruit.

### Test

`Grimrock.Runtime.Objects.GenericAudioContract` ne protège plus la backward compatibility.

Il vérifie désormais :

- absence réfléchie des cinq propriétés legacy ;
- Button utilisant le contrat générique ;
- Door utilisant le même contrat générique ;
- atténuation unique ;
- `Open` explicitement configuré ;
- aucune invention automatique de `Close`.

## Assets

Aucun `.uasset` n'est modifié par ce ticket.

Les assets binaires/LFS ne sont pas interprétés à l'aveugle. La validation manuelle doit donc confirmer que les définitions de porte réellement utilisées possèdent leurs données courantes dans :

```text
Audio > Attenuation
Audio Events > Open
Audio Events > Close
```

Un asset qui ne contient encore que les anciennes propriétés n'est plus supporté par le runtime et doit être corrigé dans Unreal Editor.

## Validation demandée

Validation ciblée :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Runtime.Objects.GenericAudioContract"
```

Puis régression portes :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Runtime.Doors"
```

Enfin, en PIE :

1. ouvrir et fermer une porte normale ;
2. ouvrir et fermer une porte secrète ;
3. vérifier les variantes et l'atténuation ;
4. tester une inversion de mouvement ;
5. vérifier la reprise partielle de la timeline audio.

Le ticket n'est déclaré validé qu'après fourniture des sorties UE locales et validation PIE.
