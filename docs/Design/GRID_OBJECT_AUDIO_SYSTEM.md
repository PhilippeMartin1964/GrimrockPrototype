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
