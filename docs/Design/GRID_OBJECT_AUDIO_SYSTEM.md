# Grid Object Audio System

## Statut

Contrat actif. L'audio est une capacité générique des `UGridWorldObjectDefinitionAsset`.

## Règle simple

Un objet physique possède **une seule atténuation spatiale**, partagée par tous ses événements audio.

~~~text
UGridWorldObjectDefinitionAsset
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

Les noms restent ouverts, mais un acteur spécialisé doit réutiliser le vocabulaire gameplay existant lorsqu'il existe afin d'éviter deux autorités sémantiques.

~~~text
Button         -> Activated
Lever          -> Activated / Deactivated
PressurePlate  -> Activated / Deactivated
Door           -> Open / Close
Door chain     -> Pull
~~~

Des noms purement physiques ou spécifiques (`Pull`, `Release`, `Insert`, `Teleport`, etc.) restent possibles lorsqu'aucun événement gameplay équivalent n'existe. La chaîne de porte utilise ainsi `Pull` avant que le mouvement de porte n'émette son propre `Open` ou `Close`.

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

Toutes les lectures 3D de l'objet utilisent la même atténuation de la définition.

## Responsabilités

~~~text
Definition
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

Le test utilise volontairement une définition Button avec l’événement canonique `Activated`, vérifie que `TriggerPress()` le consomme réellement et confirme que l’atténuation de la définition reste l’unique profil spatial utilisé par l’objet.

## Lecture à partir d'un timestamp

Le service générique permet aux acteurs spécialisés de demander le démarrage d'un événement au milieu de sa piste :

~~~cpp
PlayObjectAudioEventDetailed(EventName, bEnableNativePlayback, StartTimeSeconds)
~~~

`StartTimeSeconds` vaut 0 par défaut et est borné à une valeur finie positive.

Cette capacité ne crée aucune donnée supplémentaire dans la définition. Elle sert notamment aux mécanismes continus qui doivent reprendre une piste à l'endroit correspondant à leur état physique.

La porte est le premier consommateur : elle transforme son taux d'ouverture en timestamp Open/Close. Le service audio commun se contente de transmettre ce timestamp à `SpawnSoundAtLocation`.
