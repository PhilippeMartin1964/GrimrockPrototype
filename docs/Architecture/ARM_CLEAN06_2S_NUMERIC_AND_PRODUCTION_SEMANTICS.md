# ARM-CLEAN06.2S — 82 progressions exactes et 22 politiques prouvées

Le nouveau constructeur Editor-only de catalogue ARM
produit **en mémoire** 82 entrées
`FGridCanonicalWeaponArchetype` à partir du
`DA_WeaponCatalogue` réel, avec les cinq progressions
compactes et la source N1. Il compare directement
**1 640 profils N1–N20**, les NdM+K, armures,
dégâts, scaling, précision, PA et portée, sans
un seul écart toléré.

La sélection des 22 `ArchetypeId` actifs vient
exclusivement de `WeaponProductionManifest.csv`.
Pour ces 22, le constructeur charge la définition
d'item réelle et mesure les deux permissions de
mains et d'attaques, puis utilise le `BuildKind`
explicite du manifeste (Weapon, Shield ou
Focus). Le champ `bHandAndKindProven` est
mis à vrai **uniquement pour ces 22**.

Les permissions des 19 attaques MainHand et deux
attaques OffHand sont mesurées dans les vrais
DataAssets, mais `AttackSourceSlots` n'est pas
encore écrit dans ce candidat : la validation du
catalogue refuse correctement une source d'attaque
sans `ItemActionBinding` réel. Aucun droit
d'attaque n'est déduit ou perdu ; les bindings
seront authorés lors de l'assemblage complet.

Les **60 autres archétypes** conservent toutes
leurs valeurs numériques ainsi que les
affinités de classes de la source historique,
mais restent `bHandAndKindProven=false`.
Le `Kind` et le `HandPolicy` présents dans
leur projection transitoire servent uniquement
à satisfaire le calcul numérique, **pas**
à qualifier leur futur gameplay. Le registre
canonique refuse tout objet concret qui tenterait
d'utiliser ces lignes sans authoring explicite.

La reprise des 82 affinités de classes et des
six préférences utilise le projecteur validé
en 06.2M, sans déduire de restriction
d'équipement d'un texte descriptif.

Un test vérifie ces compteurs, les 19 attaques
depuis MainHand, deux depuis OffHand, la
validation structurale du catalogue, la
propreté Git et le refus d'un écrasement.

## Limite de migration

Cet assemblage de catalogue **n'est pas**
un `DA_WeaponCatalogue.uasset` prêt à
remplacer l'original. Les actions, réactions
passives (Main-Gauche et bouclier), munitions
et balistique source doivent encore être
projetées sans perte avant la création de
l'asset. La présence de 82 profils numériques
ne donne jamais à elle seule l'autorisation
de migrer les assets ou de raccorder les
SaveGame. Aucun binaire n'est modifié.
