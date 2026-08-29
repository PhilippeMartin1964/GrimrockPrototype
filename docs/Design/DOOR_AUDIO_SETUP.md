# Audio des objets — configuration des portes

## Principe

Les portes utilisent le système audio **générique des Grid Objects**.

L'audio n'est pas une capacité réservée aux portes. Tout `UGridObjectArchetypeAsset` peut définir des événements sonores sémantiques dans :

~~~text
Audio
├── Default Audio Attenuation
└── Audio Events
~~~

`Audio Events` est un `TMap<FName, FGridObjectAudioEvent>`. Les clés sont libres afin que le système puisse évoluer sans ajouter une propriété C++ à chaque nouveau mécanisme.

Exemples :

~~~text
Door            -> Open, Close
Button          -> Press, Release
Lever           -> Activate, Deactivate
PressurePlate   -> Press, Release
Receptacle      -> Insert, Remove, Reject
Teleporter      -> Activate, Teleport
Trap            -> Trigger, Reset
Decoration      -> Interact
Custom object   -> n'importe quel FName documenté
~~~

## Structure d'un événement

Chaque entrée contient :

~~~text
Sounds[]
Volume
Pitch Variation
Attenuation Override
~~~

Si `Attenuation Override = None`, l'événement utilise `Default Audio Attenuation` de l'archetype.

La sélection de variantes est cyclique et déterministe. La variation de pitch n'utilise pas le RNG de gameplay.

Pour les mécanismes dont le sample doit suivre une animation, conserver généralement :

~~~text
Pitch Variation = 0.00
~~~

## Configuration d'une porte

Dans `DA_Door_Wood`, `DA_Door_Grating` ou `DA_Door_Secret`, créer deux entrées :

~~~text
Audio Events
├── Open
│   ├── Sounds
│   │   ├── S_Door_Wood_Open_01
│   │   └── S_Door_Wood_Open_02
│   ├── Volume = 1.0
│   ├── Pitch Variation = 0.0
│   └── Attenuation Override = None
└── Close
    ├── Sounds
    │   └── S_Door_Wood_Close_01
    ├── Volume = 1.0
    ├── Pitch Variation = 0.0
    └── Attenuation Override = None
~~~

Puis :

~~~text
Default Audio Attenuation = SA_Door_3D
~~~

Le service générique est porté par `AGridRuntimeObjectActor` :

~~~cpp
ConfigureObjectAudio(...)
HasObjectAudioEvent(...)
PlayObjectAudioEvent(...)
PlayObjectAudioEventDetailed(...)
~~~

Les classes spécialisées décident seulement **quand** jouer un événement. La porte demande `Open` ou `Close` et conserve sa politique temporelle spécifique.

## Politique temporelle des portes

~~~text
Open commence
    -> event Open

Close pendant Open
    -> ancienne voix interrompue
    -> event Close si un trajet de fermeture existe

fin mécanique normale
    -> aucune coupure audio
    -> la queue du sample finit naturellement

Snap / restauration
    -> pas de nouveau son
~~~

La porte peut donc claquer, vibrer ou résonner après l'arrêt du mesh.

## Compatibilité des DataAssets existants

Les anciens champs sérialisés :

~~~text
DoorOpenSounds
DoorCloseSounds
DoorAudioVolume
DoorAudioPitchVariation
DoorAudioAttenuation
~~~

sont conservés uniquement comme données de migration cachées.

Au chargement, un ancien archetype Door est converti en mémoire vers :

~~~text
DoorOpenSounds  -> AudioEvents["Open"]
DoorCloseSounds -> AudioEvents["Close"]
~~~

Le runtime possède aussi un fallback transparent tant que l'asset n'a pas été resauvegardé. Les sons déjà assignés ne sont donc pas perdus.

Après ouverture puis sauvegarde de l'archetype dans l'éditeur, les nouvelles entrées génériques deviennent la source d'authoring.

## Convention de contenu

~~~text
Content/GrimrockPrototype/Audio/Environment/Doors/
├── Wood/
├── Grating/
└── Secret/
~~~

Convention recommandée :

~~~text
S_Door_<Type>_<Event>_<Variant>
~~~

## Validation

Contrat générique :

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Runtime.Objects.GenericAudioContract"
~~~

Régression portes :

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
