# Grid Object Audio System

## Statut

Contrat actif. L'audio est une capacité générique des `UGridObjectArchetypeAsset`, pas une fonctionnalité spécifique aux portes.

## Données

~~~cpp
UGridObjectArchetypeAsset
    DefaultAudioAttenuation
    TMap<FName, FGridObjectAudioEvent> AudioEvents
~~~

~~~cpp
FGridObjectAudioEvent
    Sounds[]
    Volume
    PitchVariation
    AttenuationOverride
~~~

Le nom d'événement est un `FName` volontairement ouvert. Aucun enum fermé n'est imposé : les futurs objets et contenus créés par les joueurs peuvent introduire de nouveaux événements sans modifier le C++ du cœur.

## Runtime

Chaque `AGridRuntimeObjectActor` reçoit la configuration de son archetype lors du spawn :

~~~cpp
Actor->ConfigureObjectAudio(Archetype);
~~~

Le service commun fournit :

~~~cpp
HasObjectAudioEvent(EventName)
PlayObjectAudioEvent(EventName)
PlayObjectAudioEventDetailed(EventName, bEnableNativePlayback)
~~~

Le service commun assure :

- sélection déterministe des variantes ;
- volume par événement ;
- pitch déterministe indépendant du RNG gameplay ;
- atténuation 3D par événement ou fallback archetype ;
- calcul de la durée effective du sample.

Le service ne décide pas de la sémantique gameplay ni du cycle de vie complexe d'une voix. Une classe spécialisée peut conserver le `UAudioComponent` retourné et l'interrompre selon son propre état.

## Responsabilités

~~~text
Archetype
    = quels sons existent

GridRuntimeObjectActor
    = comment sélectionner et jouer un événement 3D

Actor spécialisé
    = quand l'événement se produit
    = s'il doit interrompre une voix précédente
~~~

Exemple Door :

~~~text
AGridDoorActor
    Open  -> PlayObjectAudioEventDetailed("Open")
    Close -> PlayObjectAudioEventDetailed("Close")
~~~

Exemple futur Button :

~~~text
AGridButtonActor
    Press   -> PlayObjectAudioEvent("Press")
    Release -> PlayObjectAudioEvent("Release")
~~~

## Compatibilité historique

Les champs Door audio introduits avant ce contrat restent sérialisés mais sont cachés et dépréciés. `PostLoad()` et `ResolveAudioEvent()` garantissent la migration/fallback vers `Open` et `Close`.

Ils ne doivent plus être utilisés pour de nouveaux assets.

## Validation

~~~text
Grimrock.Runtime.Objects.GenericAudioContract
~~~

Ce test utilise volontairement un archetype **Button** avec l'événement `Press` pour empêcher toute régression vers une architecture Door-only. Il vérifie également la compatibilité d'un ancien Door archetype.
