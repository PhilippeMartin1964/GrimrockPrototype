# ARM-CLEAN06.4H — Inventaire des références d'items physiques, sans conversion

**Prérequis prouvé :** ARM-CLEAN06.4G sur
`e0f877d17f39d913c585b55ba18110745c5385b6`.
Les trois statuts Windows sont SUCCESS. Les 84 binaires
(56 historiques et 28 canoniques) sont sauvegardés et les
84 copies ont passé une restauration simulée (issue 140).

## Pourquoi ce contrôle vient après les présentations Sword

Les huit changements de présentations audio des quatre armes
Sword ont été préservés et vérifiés sur le catalogue canonique.
Ils ne prouvent toutefois pas que les objets de niveau, les
plaques/receptacles, les générateurs et les inventaires
persistants possèdent une identité migrable sans perte.

Le runtime comporte encore des `TObjectPtr<UGridItemDefinitionAsset>`
dans des structures de niveau et de sauvegarde. Effacer les
anciens `DA_Item_*` serait donc prématuré.

## Audit réel en UE5.5.4

Un nouveau commandlet Editor-only,
`UGridArmClean064HReferenceAuditCommandlet`, et
`Scripts/AuditARM_CLEAN06_4H_WorldReferences.ps1`
s'exécutent uniquement après Build + suite `Grimrock`
réussis sur le sujet de commit :

`chore(ARM-CLEAN06.4H): audit real world item identities and save schema read-only`

Le script exige l'attestation privée 06.4G et ses
trois listes d'écritures vides, vérifie les 84 hash de
la **source actuelle** et de la **copie préservée**,
puis lance `UnrealEditor-Cmd.exe` en `-NullRHI`.
Aucune valeur d'asset n'est écrite.

Le commandlet inspecte explicitement :

| Source | Information inspectée |
|---|---|
| Quatre `DA_GridLevel_*` | Objets physiques `LooseItemInstances`, objets générés `ItemSpawns`, IDs et niveaux N0/N1–N20, quantités, GUID |
| `WorldObjectInstances[].InstanceConfig` | `ReceptacleInitialContent` de chaque placement |
| `DA_ObjectPalette_Default` | `DefaultItemDefinition` sur les entrées physiques directes |
| Définitions `UGridWorldObjectDefinitionAsset` sous `Core/DataAssets` | `DefaultBehavior.Receptacle.InitialContent` réels |
| `DA_ItemSystem.ItemDefinitions` | Résolution des 23 IDs canoniques et distinction des archétypes |
| `FGridRuntimeItemState`, `FGridPendingWorldItemState` | Réflexion sur les deux champs SaveGame encore de type ancien pointeur d'asset |

Pour chaque référence chargée, le rapport privé
`world-reference-private.json` enregistre l'objet propriétaire,
le chemin de l'ancien DataAsset, l'`ItemId`, la quantité,
l'`ItemLevel`, le GUID lorsqu'il existe et le résultat
de la recherche dans le vrai registre canonique. Le classement
isole `ID_CANDIDATE_ONLY`, `NO_CANONICAL_ID`,
`EMPTY_LEGACY_ID`, `NO_LEGACY_DEFINITION`,
`INVALID_LEVEL_OR_QUANTITY` et
`ARCHETYPE_MISMATCH`.

**Un `ID_CANDIDATE_ONLY` n'est qu'un lien nominal.**
Il ne prouve ni l'égalité de nom/masse/icône/maillage,
ni la politique de lancer, ni la conservation des états
d'énigme ou des sauvegardes. Les GUID invalides ou
dupliqués sont comptabilisés, mais leur signification
nécessite une analyse du type de placement.

## Périmètre conscient de ses limites

- Le scan des définitions de monde est limité aux assets de
  classe `UGridWorldObjectDefinitionAsset` répertoriés sous
  `/Game/GrimrockPrototype/Core/DataAssets`. Si des
  définitions résident ailleurs, un prochain contrôle devra
  étendre explicitement la liste, sans les inférer.
- Les réceptacles instanciés par des WBP, presets ou classes
  natives à la volée ne sont **pas couverts en intégralité**
  par les quatre niveaux DataAsset.
- `ProjectileDefinitionAsset` et `ItemDefinitionAsset`
  sont vérifiés par réflexion du **schéma SaveGame**, non
  par ouverture des fichiers personnels : ni version,
  ni compatibilité des `.sav` réels n'est prétendue.
- La copie privée 06.4G protège les **84** binaires, pas
  tous les éventuels WBP, presets et autres contenus
  référencés par les objets du niveau.
- L'audit est en `-NullRHI`, donc pas une validation PIE,
  Chaos, son 3D ou rendu UMG.

## Conditions de clôture 06.4H

Obtenir sur le **SHA 06.4H** :
`UE5.5.4/SilentTask=success`,
`UE5.5.4/LocalAuto=success` et
`UE5.5.4/ARM-CLEAN06.4H=success`, puis consulter
l'issue `[ARM-CLEAN06.4H]` avec compteurs uniquement.
SUCCESS prouve que l'inventaire a été exécuté, **pas
que toutes les références ont été migrées**.

Le verrou privé garde :
`migration_authorized=false`,
`allowed_binary_writes=[]`,
`allowed_package_deletions=[]` et
`allowed_savegame_writes=[]`.
La suite devra qualifier les références non résolues,
les niveaux N0 et objets réceptacles par cas concret,
puis la politique physique des lancers et le schéma
des vraies sauvegardes avant une transaction UE
avec sauvegarde/rollback. Aucun héritage n'est supprimé.

Aucun texte joueur ou WBP modifié :
`UI.csv → ST_UI → WBP → Game/fr/en` inchangé.
