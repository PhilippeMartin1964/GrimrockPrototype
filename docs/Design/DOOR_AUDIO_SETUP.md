# Audio des portes — configuration

## Objectif

Les portes utilisent des sons 3D spatialisés déclenchés au début d'un mouvement réel d'ouverture ou de fermeture.

La configuration est portée par l'asset d'archetype de la variante de porte, et non par le Blueprint runtime commun.

Exemples actuels :

~~~text
DA_Door_Wood
DA_Door_Grating
DA_Door_Secret
~~~

Cela permet à plusieurs variantes utilisant `AGridDoorActor` de conserver des signatures sonores différentes sans code spécifique.

## Convention de contenu

Dossier recommandé :

~~~text
Content/GrimrockPrototype/Audio/Environment/Doors/
├── Wood/
├── Grating/
└── Secret/
~~~

Convention :

~~~text
S_Door_<Type>_<Event>_<Variante>
~~~

Exemples :

~~~text
S_Door_Wood_Open_01
S_Door_Wood_Open_02
S_Door_Wood_Close_01

S_Door_Grating_Open_01
S_Door_Grating_Close_01

S_Door_Secret_Open_01
S_Door_Secret_Close_01
~~~

Pour une source éditée, WAV PCM est recommandé afin d'éviter une recompression destructive supplémentaire.

## Configuration d'un archetype

Ouvrir par exemple :

~~~text
Content/GrimrockPrototype/Core/DataAssets/DA_Door_Wood
~~~

Puis remplir :

~~~text
Audio
└── Door
    ├── Open Sounds
    ├── Close Sounds
    ├── Volume
    ├── Pitch Variation
    └── Attenuation
~~~

Valeurs de départ recommandées :

~~~text
Volume          = 1.0
Pitch Variation = 0.00
Attenuation     = optionnelle
~~~

`Attenuation = None` conserve les réglages d'atténuation du Sound asset.

Si un réglage commun est souhaité, créer par exemple :

~~~text
SA_Door_3D
~~~

et l'assigner au champ **Attenuation** des archetypes de porte.

## Contrat runtime

Le runtime copie les données audio de l'archetype vers `AGridDoorActor`.

Le son est créé via :

~~~cpp
UGameplayStatics::SpawnSoundAtLocation(...)
~~~

La position est celle de la porte. Le `UAudioComponent` retourné est conservé par la porte afin que la voix puisse être arrêtée dès que le mouvement change ou se termine.

Un son est demandé uniquement lorsque la porte commence effectivement un déplacement. Une commande inverse reçue dans la même frame, avant tout déplacement physique, ne joue donc pas de son dans le sens inverse.

~~~text
porte fermée + Open       -> Open Sound
porte ouverte + Close     -> Close Sound
Open répété pendant Open  -> aucun nouveau son
Close pendant Open, après début du déplacement -> Close Sound
SnapDoorOpenState         -> aucun son
chargement / restauration -> aucun son
~~~

Une liste vide est valide et signifie simplement que cette variante reste silencieuse.

## Synchronisation avec le mouvement

Une porte ne possède qu'une seule voix audio de mouvement à la fois.

~~~text
Open commence
    -> Open Sound actif

Close pendant Open
    -> Open Sound arrêté immédiatement
    -> Close Sound démarre si la porte a réellement du trajet de fermeture

fin réelle du mouvement
    -> si le sample est aligné à la durée : il termine naturellement
    -> s'il dépasse nettement une animation partielle : fade très court

SnapDoorOpenState / restauration
    -> son courant arrêté
    -> aucun nouveau son
~~~

Le fichier audio n'est donc jamais l'autorité temporelle. C'est la position et la durée réelles de la porte qui décident quand la voix doit s'arrêter.

Pour une commande `Open -> Close` reçue avant le premier Tick, la porte est encore physiquement fermée : le son Open est coupé, mais aucun son Close n'est lancé puisqu'il n'existe aucun trajet de fermeture.

La sélection des variantes est déterministe et cyclique. **Pour les sons mécaniques de porte, Pitch Variation doit normalement rester à 0.00**, car modifier le pitch modifie aussi la durée du fichier et peut désynchroniser un sample prévu pour correspondre à `MoveDuration`.

Une variation non nulle reste possible, mais le runtime calcule alors la durée effective du sample avant de décider s'il peut terminer naturellement.

## Portes secrètes

Aucun traitement audio spécial n'est codé en dur pour `Door_Secret`.

L'archetype `DA_Door_Secret` reçoit simplement ses propres sons :

~~~text
Open Sounds  = sons de mur/pierre secret
Close Sounds = sons de mur/pierre secret
~~~

Cette séparation reste indépendante du contrat acoustique de perception : une porte secrète fermée bloque toujours l'ouïe des monstres même si son animation possède un son.

## Validation

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Runtime.Doors.AudioFeedback"
~~~

Puis vérifier en PIE :

1. ouverture : un seul son 3D ;
2. fermeture : un seul son 3D ;
3. commande répétée dans le même sens : pas de doublon ;
4. inversion en cours de mouvement : nouveau son correspondant ;
5. restauration d'état : silence.


## Autorité de Move Duration

Pour une porte déjà placée, la durée runtime ne vient pas directement du DataAsset d'archetype.

Le contrat officiel est :

~~~text
Archetype.DefaultBehavior.DoorAnimation.MoveDuration
    -> valeur copiée lors du placement

FGridLevelObjectData.Behavior.DoorAnimation.MoveDuration
    -> source de vérité de l'instance placée
    -> valeur lue par AGridDoorActor
~~~

Ainsi, modifier `DA_Door_Wood > DefaultBehavior > DoorAnimation > MoveDuration` après avoir placé une porte ne modifie pas cette instance existante.

Dans **Selected Object > Door**, le champ est désormais nommé :

~~~text
Instance Move Duration (runtime)
~~~

La valeur par défaut de l'archetype est affichée séparément. En cas d'écart, **Use Archetype Duration** copie uniquement cette durée dans l'instance sans écraser les autres paramètres de comportement.

Pour une course complète :

~~~text
CurrentMoveDuration = Instance Move Duration * 1.0
~~~

Pour une inversion ou un trajet partiel :

~~~text
CurrentMoveDuration = Instance Move Duration * TravelRatio
~~~

Diagnostic runtime :

~~~text
Grid door motion start:
InstanceMoveDuration=...
TravelRatio=...
EffectiveMoveDuration=...
AudioExpectedDuration=...
PitchVariation=...
~~~
