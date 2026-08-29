# Audio des objets — configuration et synchronisation des portes

## Principe

Les portes utilisent le système audio générique des Grid Objects.

~~~text
UGridObjectArchetypeAsset
└── Audio
    ├── Attenuation
    └── Audio Events
        ├── Open
        │   ├── Sounds[]
        │   ├── Volume
        │   └── Pitch Variation
        └── Close
            ├── Sounds[]
            ├── Volume
            └── Pitch Variation
~~~

L'atténuation est unique pour l'archetype. Aucun paramètre audio spécifique supplémentaire n'est nécessaire pour la reprise partielle.

## Configuration recommandée

Exemple `DA_Door_Wood` :

~~~text
Audio > Attenuation = SA_Door_3D

Audio Events
├── Open
│   ├── Sounds = S_Door_Wood_Open_01
│   ├── Volume = 1.0
│   └── Pitch Variation = 0.0
└── Close
    ├── Sounds = S_Door_Wood_Close_01
    ├── Volume = 1.0
    └── Pitch Variation = 0.0
~~~

Pour des pistes montées pour suivre précisément le mécanisme, conserver `Pitch Variation = 0.0`.

## Timeline mécanique et timeline audio

`MoveDuration` définit uniquement la partie mécanique synchronisée du son.

Une piste peut être plus longue :

~~~text
0 s ---------------------- 5.0 s -------- 5.4 s
|       mécanisme             |  queue       |
|<----- MoveDuration -------->|
~~~

La queue après `MoveDuration` (claquement, vibration, résonance) finit naturellement.

## Reprise lors d'une inversion

La porte calcule son taux d'ouverture courant :

~~~text
Openness = 0.0  -> complètement fermée
Openness = 1.0  -> complètement ouverte
~~~

Lorsqu'elle repart vers Open :

~~~text
OpenStartTime = Openness * MoveDuration
~~~

Lorsqu'elle repart vers Close :

~~~text
CloseStartTime = (1 - Openness) * MoveDuration
~~~

Exemple avec `MoveDuration = 5 s` :

~~~text
porte ouverte à 40 %
Close
-> StartTime = (1 - 0.40) * 5
-> StartTime = 3.0 s
~~~

La piste Close démarre donc directement à 3.0 s, au point correspondant à la position mécanique actuelle.

Dans l'autre sens :

~~~text
porte ouverte à 70 %
Open
-> StartTime = 0.70 * 5
-> StartTime = 3.5 s
~~~

## Contrat runtime

~~~text
mouvement complet Open
    -> Open à 0.0 s

mouvement complet Close
    -> Close à 0.0 s

Open partiel -> inversion Close
    -> Open interrompu
    -> Close démarre au timestamp correspondant

Close partiel -> inversion Open
    -> Close interrompu
    -> Open démarre au timestamp correspondant

fin mécanique normale
    -> aucun Stop
    -> queue sonore naturelle

Snap / restauration
    -> silence
~~~

Le timestamp est calculé depuis la position du mesh et `MoveDuration`, pas depuis la durée totale du SoundWave. Une queue audio plus longue ne décale donc pas la synchronisation mécanique.

## API générique

Le lecteur commun accepte maintenant un timestamp de départ optionnel :

~~~cpp
PlayObjectAudioEventDetailed(
    EventName,
    bEnableNativePlayback,
    StartTimeSeconds);
~~~

Par défaut `StartTimeSeconds = 0.0`. Cette capacité reste générique ; seule la porte décide comment calculer son timestamp mécanique.

## Diagnostics

Au démarrage d'une porte, le log contient notamment :

~~~text
InstanceMoveDuration
TravelRatio
Openness
EffectiveMoveDuration
AudioStartTime
AudioExpectedDuration
Pitch
~~~

## Validation

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Runtime.Doors.PartialAudioResume"
~~~

Régressions :

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Runtime.Doors.AudioFeedback"
~~~

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Runtime.Doors.NaturalAudioTail"
~~~
