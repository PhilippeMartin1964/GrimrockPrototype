# ARM-CLEAN06.3AH — Niveaux de butin et identité ItemSystem

## Point de départ

06.3X projetait déjà le nom et le poids des butins depuis le
`DA_ItemSystem` réel, mais le choix de leur niveau passait encore par
`UGridItemDefinitionAsset::TryGetWeaponCatalogueProfile`. Une arme
connue pouvait être écartée si cet ancien catalogue individuel
manquait, alors que son profil N1–N20 était disponible dans
`UGridItemSubsystem`.

## Modification bornée

`FGridMonsterLootResolver::TryResolveItemLevel` reçoit un dernier
paramètre facultatif `const UGridItemSubsystem*`. Les appels historiques
sans registre conservent leur comportement jusqu'à 06.4.

Pour un `ItemId` enregistré :

- Le type **ARM ou non-ARM** vient du seul ItemSystem, jamais d'un
  `bUseWeaponCatalogue` historique.
- Une arme avec `Entry.ItemLevel=N` conserve **N1–N20** après
  vérification par `ResolveItem`. Si l'entrée a `ItemLevel=0`,
  la création hérite **une seule fois** de
  `DefaultWeaponItemLevel` de sa définition legacy. Si cette
  définition est absente, la création par héritage est rejetée ;
  il est interdit d'inventer N1.
- Les flèches sont des objets sans progression : **N0 réel**, sans
  héritage. Un niveau authoré N1 sur une munition est refusé même
  si `ResolveItem` l'accepte temporairement pour compatibilité.
- Une identité connue mais un niveau invalide ne récupère
  jamais un ancien profil N1. Un échec efface la valeur de sortie.
- Les identités non enregistrées gardent temporairement la
  validation et les comportements MON8 historiques.

`UGridMonsterDeathComponent` passe le registre déjà présent dans le
`GameInstance`, puis utilise ce même registre pour les métadonnées.
Le sous-système est récupéré **une fois** pour la génération de loot,
plutôt que pour chaque item. Aucun tirage de plus.

## Invariants et tests

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalMonsterLootLevelResolution`
utilise le véritable `DA_ItemSystem` et compare quatre armes N1,
N20 et héritage N6, résolutions explicites sans définition ancienne,
refus N0 incorrect, flèches N0, objets hors registre et sortie remise
à zéro en cas d'échec. Un tirage `ResolveLoot` avec seed identique
confirme que modifier N1 en N20 ne change **ni la chance ni la
quantité générée**.

Les tests historiques
`Grimrock.RPG.ARMEMENT01.8.MonsterLoot.*` restent dans
la suite. Pas de modification de loot table, de save, de position
de drop, de mesh, de quantité, de cooldown, de matériau ou de
`UObject` sérialisé.

**UI.csv → ST_UI → WBP → Game/fr/en : inchangé.**
La validation de l'étape requiert les deux statuts Windows
`UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` sur son SHA.
