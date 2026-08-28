# TD07.3.5 — Combat Data Schema Reset — Characterization

Date : **28 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD07.3 — Prototype Data Model Reset**  
Baseline : `edca373674ed2ca4fc476b9592efe93710575b84`  
Statut : **CHARACTERIZATION VALIDÉE — TD07.3.5.2 NORMALISATION IMPLÉMENTÉE / À VALIDER**

## 1. Contexte

TD07.3.4 est clos sur le SaveGame v22 exact-match.

TD07.3.5 traite maintenant les deux schémas de combat explicitement identifiés par l'audit TD07.3.1 :

```text
Item combat legacy
Monster attack presentation / range legacy
```

Cette tranche ne modifie pas encore les DataAssets binaires.

## 2. Item combat — double autorité actuelle

`UGridItemDefinitionAsset` contient simultanément :

```text
legacy
    bProvidesAttack
    OffensiveProfile

current
    CombatActions[]
```

Le commentaire de `CombatActions` confirme encore explicitement le fallback vers le schéma legacy lorsqu'il est vide.

Le runtime conserve également :

```text
FGridCombatActionCatalog::MakeLegacyEquipmentAttackDefinition()
```

et plusieurs consumers choisissent :

```text
CombatActions
    sinon
bProvidesAttack + OffensiveProfile
```

## 3. Legacy item adapter toujours fonctionnel

Une arme ne possédant que :

```text
bProvidesAttack = true
OffensiveProfile = valid
CombatActions = []
```

reste actuellement :

- valide ;
- équipable comme source d'attaque ;
- transformable en `FGridCombatActionDefinition` par l'adapter legacy.

La coexistence n'est donc pas seulement déclarative : les deux schémas sont encore réellement exécutables.

## 4. Cible item TD07.3.5

Autorité unique :

```text
UGridItemDefinitionAsset::CombatActions
```

À terme, doivent disparaître du schéma item :

```text
bProvidesAttack
OffensiveProfile
MakeLegacyEquipmentAttackDefinition
fallback CombatActions -> legacy profile
```

`FGridOffensiveEquipmentProfile` reste une structure légitime **à l'intérieur d'une CombatAction Attack** ; ce n'est pas cette structure qui est legacy, mais son stockage parallèle directement sur l'item.

## 5. Monster attack — double présentation actuelle

`FGridMonsterAttackDefinition` contient simultanément :

Audio :

```text
legacy
    AttackSound

current
    AttackAudio
    ImpactHitAudio
    ImpactMissAudio
```

VFX :

```text
legacy
    ImpactVFX

current
    AttackVFXDefinition
    ImpactHitVFXDefinition
    ImpactMissVFXDefinition
```

Le runtime utilise encore les fallbacks :

```text
AttackAudio vide
    -> AttackSound

ImpactHitVFXDefinition vide
    -> ImpactVFX
```

## 6. Monster range — nom legacy conservé

Le schéma courant expose encore :

```text
MinRangeCells
RangeCells
```

alors que la sémantique réelle est :

```text
Minimum range
Maximum range
```

Le commentaire source indique explicitement :

```text
RangeCells
    kept for serialized asset compatibility
```

Pendant le prototype, cette compatibilité sérialisée n'est plus une exigence.

Cible :

```text
MinRangeCells
MaxRangeCells
```

sans alias historique.

## 7. Contenu courant connu

L'audit TD07.3.1 a déjà signalé :

```text
DA_Weapon_Shuriken
    dépend encore uniquement de bProvidesAttack / OffensiveProfile

RatGiant
GoblinThrower
    utilisent encore AttackSound

familles monster courantes
    utilisent encore le nom RangeCells
```

Ces assets ne sont pas réparés dans la Characterization.

Leur réparation/recréation devra être coordonnée avec TD07.3.5 et TD07.3.7 afin qu'aucun état intermédiaire validé ne perde une attaque ou une présentation nécessaire.

## 8. Frontière avec TD07.3.6 / .7

TD07.3.5 :

```text
combat schema + runtime fallbacks
```

TD07.3.6 :

```text
remaining unrelated legacy API/data purge
```

TD07.3.7 :

```text
systematic current asset repair / recreation
```

Si un asset combat doit être réparé immédiatement pour maintenir une validation fonctionnelle après suppression d'un champ, cette réparation fait partie de la transaction TD07.3.5 ; TD07.3.7 garde l'audit/réparation globale résiduelle.

## 9. Séquence proposée

```text
TD07.3.5.1 Characterization
    4 tests
    aucune suppression

TD07.3.5.2 Item CombatActions Authority
    supprimer bProvidesAttack / OffensiveProfile item-level
    supprimer MakeLegacyEquipmentAttackDefinition
    supprimer tous les fallbacks item
    aligner fixtures / assets nécessaires

TD07.3.5.3 Monster Presentation Authority
    supprimer AttackSound
    supprimer ImpactVFX
    AttackAudio / Impact*Audio / *VFXDefinition seuls

TD07.3.5.4 Monster Range Schema
    RangeCells -> MaxRangeCells
    aucune compat alias
    aligner planners/executors/tests/assets

TD07.3.5.5 Regressions / Shipping / Closure
```

## 10. Tests de Characterization

Filtre :

```text
Grimrock.TechnicalDebt.TD07_3_5.Characterization
```

Tests :

```text
ItemSchemaMultiplicity
LegacyItemAdapterActive
MonsterSchemaMultiplicity
MonsterRuntimeFallbacks
```

Attendu :

```text
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
```

## 11. Stop condition du gate

- [x] double schéma item cartographié ;
- [x] adapter item legacy caractérisé ;
- [x] double présentation monster cartographiée ;
- [x] fallback audio caractérisé ;
- [x] fallback VFX caractérisé ;
- [x] RangeCells legacy caractérisé ;
- [x] cible CombatActions-only fixée ;
- [x] cible monster presentation courante fixée ;
- [x] cible MaxRangeCells fixée ;
- [x] séquence .1–.5 proposée ;
- [x] 4 tests ajoutés ;
- [x] build UE5.5.4 vert ;
- [x] 4/4 tests verts.


## 12. Validation locale

```text
Grimrock.TechnicalDebt.TD07_3_5.Characterization
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
Report                 : Saved/Automation/TD04/TD04-20260828-083606
```

TD07.3.5.1 est validé. TD07.3.5.2 peut commencer.


### Mise à jour TD07.3.5.2

La dette item caractérisée ici est maintenant supprimée :

```text
bProvidesAttack                      supprimé
OffensiveProfile [item-level]        supprimé
MakeLegacyEquipmentAttackDefinition supprimé
fallbacks runtime item              supprimés
```

`CombatActions` est l'unique autorité item/combat.

La Characterization post-refactor conserve quatre tests : les deux premiers
constatent désormais la frontière item normalisée, tandis que les deux derniers
continuent à caractériser les dettes monster encore actives pour TD07.3.5.3/.4.
