# MON17.8.5 — Dissolution obligatoire des cadavres de monstres

Statut : **RÈGLE GLOBALE DU RUNTIME**

## 1. Invariant

Dans GrimrockPrototype, un cadavre de monstre n'est jamais un état persistant visible.

Toute mort suit obligatoirement :

```text
mort gameplay immédiate
    -> animation de mort éventuelle
    -> pose finale
    -> maintien visuel 2,0 s
    -> dissolution 1,5 s
    -> SkeletalMesh caché
    -> Actor mort conservé pour l'état runtime / SaveGame
```

La dissolution n'est ni une capacité, ni une option, ni un réglage de bestiaire.

## 2. Aucune configuration par monstre

`UGridMonsterDefinitionAsset` n'expose plus :

```text
bEnableDeathDissolve
DeathDissolveDelay
DeathDissolveDuration
DeathDissolveParameterName
```

Ces anciennes propriétés ont été retirées du schéma courant. Aucun monstre ne peut désactiver ou personnaliser sa politique de disparition.

## 3. Contrat runtime unique

`UGridMonsterDeathComponent` possède la politique commune :

```text
CorpseHoldDelaySeconds        = 2.0
CorpseDissolveDurationSeconds = 1.5
CorpseDissolveParameterName   = DissolveAmount
```

Ces valeurs sont internes au runtime et ne sont pas exposées dans les DataAssets.

`DeathExpectedDuration` reste propre à la définition parce qu'il décrit la durée de l'animation de mort elle-même. Après cette animation, la disparition est identique pour tous.

## 4. Contrat matériau obligatoire

Tous les matériaux de monstres doivent implémenter le paramètre scalaire commun `DissolveAmount` au moyen de :

```text
/Game/GrimrockPrototype/Art/Materials/Functions/MF_MonsterDeathDissolveMask
```

Le GoblinThrower et le RatGiant utilisent ce même contrat. Tout nouveau monstre doit l'utiliser.

Le runtime anime `DissolveAmount` de 0 à 1 via des `UMaterialInstanceDynamic`, puis cache le SkeletalMesh pour garantir la disparition complète.

## 5. Mort logique

La mort gameplay reste immédiate : occupation libérée, collision désactivée, loot généré, XP attribuée, `MonsterDied` émis et encounter mis à jour. La dissolution est uniquement la présentation finale obligatoire.

## 6. Save / Restore

Un monstre déjà mort restauré depuis une sauvegarde est considéré comme ayant terminé sa présentation.

`RestoreCommittedDeathState()` ne possède plus de paramètre permettant de restaurer un cadavre visible. Le SkeletalMesh reste systématiquement caché.

## 7. Tests

`Grimrock.Monsters.MON17.8` protège l'absence de réglages de dissolve dans `UGridMonsterDefinitionAsset`, l'état runtime de dissolution, l'absence de Tick permanent et la restauration sans cadavre visible.

## 8. Règle d'authoring

Pour tout nouveau monstre : animation de mort, matériaux raccordés à `MF_MonsterDeathDissolveMask`, paramètre `DissolveAmount`, puis validation PIE.

Il n'existe aucune étape « activer la dissolution ».

**Tout monstre mort est recyclé visuellement.**
