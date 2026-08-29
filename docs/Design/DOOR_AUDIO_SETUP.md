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
Pitch Variation = 0.04
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

Le son est joué via :

~~~cpp
UGameplayStatics::PlaySoundAtLocation(...)
~~~

La position est celle de la porte.

Un son est demandé uniquement lorsque la porte commence effectivement un déplacement.

~~~text
porte fermée + Open       -> Open Sound
porte ouverte + Close     -> Close Sound
Open répété pendant Open  -> aucun nouveau son
Close pendant Open        -> Close Sound
SnapDoorOpenState         -> aucun son
chargement / restauration -> aucun son
~~~

Une liste vide est valide et signifie simplement que cette variante reste silencieuse.

La sélection des variantes est déterministe et cyclique. Une variation de pitch de présentation est appliquée sans consommer le RNG de gameplay.

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
