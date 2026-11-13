# ARM-CLEAN06.2L — Identités et masses des 22 armes ARM

La définition canonique d'arme `UGridArmamentDefinitionAsset`
contient uniquement `ItemId`, `DisplayName`,
`Description`, `WeightKg`, `Tags` et `ArchetypeId`.
Elle ne contient aucun pointeur vers les catalogues,
action, SFX, VFX, icône ou mesh, ni copie N1.

Le helper Editor-only projette les vrais `DA_Weapon_*`
présents **dans le manifeste de production uniquement** :
`ItemDefinitionId → ItemId`, `WeaponArchetypeId →
ArchetypeId`, `DisplayName`, `Description`,
`Weight → WeightKg`, `ItemTags → Tags`.
Les champs sont ensuite comparés directement, y compris
l'ordre de tags et le contenu `FText`.

La projection refuse les masses invalides et l'écrasement
d'une fiche canonique existante. En revanche une liste historique
de tags est copiée intégralement, même si le registre final la
considère invalide (doublon ou tag vide) : cette éventuelle
incompatibilité est comptée séparément et **bloque le cutover**,
sans suppression ou correction silencieuse de tags. Le test
`ProductionItemIdentityProjection` confirme les
22 ItemId/ArchetypeId uniques, les données et la propreté
des packages après exécution. Le test négatif
`ItemIdentityProjectionSafety` prouve le refus des
masses négatives et des écritures sur une destination
déjà renseignée.

L'ancien `DefaultWeaponItemLevel` reste un réglage
hérité distinct du `ItemLevel` d'instance. Les
valeurs non standard sont comptées et doivent être
converties en politique de création d'instances,
**pas** copiées dans le DataAsset d'arme de six champs.

Le premier commit 06.2L compilait, mais son test de projection
était en échec (issue #105) ; le rapport public ne permet pas
d'identifier le champ en cause. La correction 06.2M sépare donc
la parité de copie et la validité du schéma canonique, et
conserve explicitement les refus du validateur pour examen
avant conversion des vrais fichiers.

Aucun asset n'est créé sous /Game, aucune migration binaire,
aucun lecteur SaveGame/PIE n'est modifié. La sortie attendue
est la validation du code + suite `Grimrock` sur le
SHA exact, puis l'authoring canonique lorsque toutes les
dépendances seront prouvées.

## ARM-CLEAN06.2O — Réparation de la fixture d'instances transitoires

Les SHA 06.2L et 06.2M ont compilé, mais leur test
`ProductionItemIdentityProjection` a échoué avec un
`ENSURE_CONDITION_FAILED` (rapports GitHub #105 et #106).
Ce symptôme **ne prouve pas** un défaut dans les
définitions d'armes. La fixture de test créait
`NewObject<UObject>()` comme Outer ; cette construction
n'était pas alignée sur les fixtures `CLEAN06.1` validées.

La fixture utilise maintenant un
`UGridItemSystemAsset` transitoire concret, conservé
en `TStrongObjectPtr` et propriétaire des
`UGridArmamentDefinitionAsset` de test. C'est
le même modèle de durée de vie que les tests
`SingleResolver` validés.

Aucun `UPackage` de production, aucun calcul de
poids, `ItemId`, tag ou texte d'arme n'est changé.
La cause sera considérée comme démontrée seulement
après une nouvelle validation `LocalAuto` réussie.
