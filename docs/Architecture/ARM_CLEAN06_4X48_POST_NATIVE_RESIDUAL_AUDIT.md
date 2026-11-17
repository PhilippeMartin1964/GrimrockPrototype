# ARM-CLEAN06.4X.48 — Contrôle natif en lecture seule des références après X.46B

**Parent :** `848d0caa2047911869e8eba3a06c79a59f7445ff` (HEAD validé SilentTask + LocalAuto).
**Recette PIE X.46B :** validée par le propriétaire après réenregistrement de
`DA_GridLevel_00.uasset` ; cet accord est distinct des tests sans GPU.
**Lot :** une seule suite Automation Editor, rapport et point de reprise.

## Objectif et limites

L'audit natif X.19 du 17 novembre comptait 107 propriétés
historiques `UGridItemDefinitionAsset*` dans neuf packages.
La transaction X.46B `cd177cd5473c8ab3777bfecae56cd0e33abd23eb`
a retiré 44 anciennes références d'objets libres, sans autres packages.
Le solde **attendu** est 63, mais ce décompte n'est valide après
conversion qu'une fois le nouveau test exécuté réellement par l'agent UE.

## Nouveau test Editor-only

`Grimrock.RPG.ARM.CLEAN06_4X48.PostNative63ReferenceBaseline`
charge les **vrais** packages UE5.5.4, sans copie de leurs propriétés
et **sans `Modify`, `SavePackage`, ni lecture/écriture SaveGame**.
Il vérifie le schéma canonique `DA_ItemSystem` (34 IDs uniques)
et refuse les changements inattendus des sous-ensembles non migrés :

| Ensemble | Nombres attendus de pointeurs historiques |
|---|---:|
| 44 `FGridLooseItemInstance` des 4 niveaux | **0** |
| Contenus initiaux réceptacles (`_00` + `_01`) | 18 |
| Override de réceptacle dormant dans `_00` | 1 |
| Overrides de serrures dormants dans `_00` | 2 |
| Palette `DA_ObjectPalette_Default` (33 entrées ID canoniques) | 33 |
| Règles partagées des 2 définitions de réceptacles | 2 |
| Règles partagées des 2 définitions de serrures | 2 |
| Loot réel des monstres GoblinThrower (3) et RatGiant (2) | 5 |
| **Total attendu après X.46B** | **63** |

Contrôles d'intégrité : identités et niveaux canoniques, GUID uniques
des 44 objets libres, égalité des IDs dans les définitions historiques
encore nécessaires, booléens d'override restés **false**,
`AcceptedKeyIds` toujours cohérents, niveau N0 pour les cinq loots,
quatre loots hors registre et Stone déjà canonique.
Il vérifie également les états `dirty` de chaque package chargé.
Une divergence doit échouer avec le groupe concerné : aucune
suppression automatique ni régénération d'identifiant.

Les **44 références aux deux catalogues** encore stockées dans
22 anciens `DA_Weapon_*`, la référence WBP à l'ancienne palette
d'icônes et le redirecteur ThrowingKnife sont des dépendances
**supplémentaires distinctes** du groupe de 63. Ils restent à
auditer et supprimer après la convergence des lecteurs/Blueprints.
Les fichiers SaveGame v27 hors dépôt restent intacts.

## Conditions de sortie

Un seul commit atomique (test + documentation) sur `master`,
puis `UE5.5.4/SilentTask=success` ET
`UE5.5.4/LocalAuto=success` sur son SHA.
Le détail de répartition est écrit dans `AddInfo` de la suite,
observable dans l'agent Windows. Après les deux réussites,
le compteur de 63 peut être annoncé comme **contrôlé nativement**
sur ces packages. Un audit AssetRegistry global post-migration
reste distinct de cette sonde bornée.

## Suite technique (sans conversion automatique)

Commencer par les deux règles de réceptacle et les deux serrures
déjà munies de leurs IDs canoniques, mais seulement après
relecture des **lecteurs runtime et éditeurs** qui utilisent les
anciennes sources. Préparer ensuite la conversion des 18
contenus initiaux, des 33 entrées de palette et des trois overrides
inactifs par des lots protégés indépendants. Les quatre loots
hors registre et les dix identités d'alchimie nécessitent une
décision de contenu avant suppression. Aucune classe/UPROPERTY
SaveGame v27 ne doit être retirée prématurément.

**Statut : X.46B PIE validé ; X.48 contrôle natif en cours ;
ARM-CLEAN06.4 partielle ; 06.5 bloquée.**
