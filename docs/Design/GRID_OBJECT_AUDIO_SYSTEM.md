# Grid Object Audio System

## Statut

Contrat actif. L'audio est une capacité générique des `UGridObjectArchetypeAsset`.

## Règle simple

Un objet physique possède **une seule atténuation spatiale**, partagée par tous ses événements audio.

~~~text
UGridObjectArchetypeAsset
└── Audio
    ├── Attenuation
    └── Audio Events
        ├── EventName
        │   ├── Sounds[]
        │   ├── Volume
        │   └── PitchVariation
        └── ...
~~~

Il n'existe pas d'atténuation par événement.

## Événements

`AudioEvents` est un `TMap<FName, FGridObjectAudioEvent>`.

~~~cpp
FGridObjectAudioEvent
    Sounds[]
    Volume
    PitchVariation
~~~

Les noms sont ouverts : `Open`, `Close`, `Press`, `Release`, `Insert`, `Teleport`, `Trigger`, `Interact`, ou un nom personnalisé.

## Runtime

Chaque `AGridRuntimeObjectActor` reçoit :

~~~text
ObjectAudioEvents
DefaultObjectAudioAttenuation
~~~

Le service commun fournit :

~~~cpp
ConfigureObjectAudio(...)
HasObjectAudioEvent(...)
PlayObjectAudioEvent(...)
PlayObjectAudioEventDetailed(...)
~~~

Toutes les lectures 3D de l'objet utilisent la même atténuation de l'archetype.

## Responsabilités

~~~text
Archetype
    = une atténuation
    = quels événements/sons existent

GridRuntimeObjectActor
    = sélection et lecture 3D

Actor spécialisé
    = quand jouer l'événement
    = politique d'interruption éventuelle
~~~

## Compatibilité historique Door

Les anciens champs Door audio restent uniquement pour désérialiser les DataAssets existants.

~~~text
DoorAudioAttenuation
    -> Audio > Attenuation

DoorOpenSounds
    -> AudioEvents["Open"]

DoorCloseSounds
    -> AudioEvents["Close"]
~~~

Ils sont cachés et dépréciés pour le nouvel authoring.

## Validation

~~~text
Grimrock.Runtime.Objects.GenericAudioContract
~~~

Le test utilise volontairement un archetype Button avec un événement `Press` et vérifie que l'atténuation de l'archetype est bien l'unique profil spatial utilisé par l'objet.

## Lecture à partir d'un timestamp

Le service générique permet aux acteurs spécialisés de demander le démarrage d'un événement au milieu de sa piste :

~~~cpp
PlayObjectAudioEventDetailed(EventName, bEnableNativePlayback, StartTimeSeconds)
~~~

`StartTimeSeconds` vaut 0 par défaut et est borné à une valeur finie positive.

Cette capacité ne crée aucune donnée supplémentaire dans l'archetype. Elle sert notamment aux mécanismes continus qui doivent reprendre une piste à l'endroit correspondant à leur état physique.

La porte est le premier consommateur : elle transforme son taux d'ouverture en timestamp Open/Close. Le service audio commun se contente de transmettre ce timestamp à `SpawnSoundAtLocation`.
