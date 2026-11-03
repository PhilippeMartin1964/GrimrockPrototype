# ITEM-VFX01 — Effets Niagara indépendants des lumières

> ITEM-VFX01.1 — validé. ITEM-VFX01.2 et ITEM-VFX01.3 — migrés, testés et publiés. ITEM-VFX01.4A — audit des références héritées et correction de la politique LiquidSparkle en cours.

## Périmètre

Les données `UGridItemDefinitionAsset::VisualEffects` définissent zéro, un ou plusieurs
systèmes Niagara. Le premier jalon ajoute l'infrastructure générique et ses tests,
sans migrer les DataAssets de potions, de flasques ou de torches.

## Sources de vérité

- `UGridItemDefinitionAsset::VisualEffects` : tableau de `FGridItemNiagaraEffectConfig`
  avec `EffectId` unique si renseigné, `NiagaraSystem`, `RelativeTransform`
  et `ActivationMode`.
- `UGridItemDefinitionAsset::LightEmitter` : source de vérité lumineuse historique,
  conservée sans changement à ce jalon.
- `FGridItemInstance::bLightsEnabled` : état gameplay, conservé dans le circuit
  existant inventaire, réceptacle et sauvegarde.
- `UGridItemVisualEffectsComponent` : composant de présentation transitoire,
  sans persistance de `UNiagaraComponent` dans les sauvegardes.

Modes d'activation :

1. `Always` : actif tant que l'acteur est visuellement présent, même sans lumière.
2. `WhenLightEnabled` : dépend de `AGridItemActor::SetItemLightsEnabled()`.

## Hiérarchie runtime

```text
AGridItemActor
  SceneRoot
    MeshComponent                   <- simulation Chaos
      LightEmitterComponent        <- existant, inchangé
      VisualEffectsComponent       <- nouveau, aucun Tick C++
        UNiagaraComponent[0..N]    <- transform relatif défini par asset
```

La hiérarchie suit le mesh physique, pas l'actor root. Le paramètre
`Local Space` des émetteurs Niagara doit être vérifié pour que les particules
déjà émises suivent aussi la rotation du contenant : l'attachement seul ne suffit pas.

## Cycle de vie

- `InitializeFromItemDefinition()` : applique la configuration Niagara et initialise
  l'état lumineux désactivé, sans désactiver les effets `Always`.
- `SetItemLightsEnabled()` : synchronise l'ancien composant lumineux et les nouveaux
  effets conditionnels. Les effets permanents ne dépendent pas de ce booléen.
- `InitializeFromItemDefinitionId()` : retire les effets d'une définition précédente.
- `OnRemovedFromWorld()` : désactive la présentation Niagara.
- `OnPlacedInWorld()` : réactive la présentation Niagara.
- `ApplyConfig()` : réutilise les composants lorsque possible, et détruit les
  composants devenus superflus pour éviter les doublons.
- `UGridPartyIlluminationComponent` : aucune modification.

## Vérifications

Tests ajoutés :

- `Grimrock.Items.VFX01_1.Configuration` : configuration vide, Niagara sans lumière,
  asset manquant et unicité des identifiants.
- `Grimrock.Items.VFX01_1.RuntimeLifecycle` : modes `Always` et
  `WhenLightEnabled`, attachement au mesh, rotation, réinitialisation,
  suppression/recréation de présentation, absence de doublons.

Régressions prioritaires :

- `Grimrock.Items.LIGHT01.DataDrivenEmitter`
- `Grimrock.Items.LIGHT02.CanonicalItemLightState`
- `Grimrock.Items.LIGHT03.PhysicsMeshEmitterAttachment`
- `Grimrock.Party.LIGHT01.EquipmentDrivenIllumination`

Validation sur Windows :

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Items.VFX01_1"
```

## Hors périmètre de ITEM-VFX01.1

Migration des `.uasset` existants, changement de l'empilement de potions,
retrait de la partie Niagara de `FGridLightEmitterConfig`, et couverture complète
des transferts inventaire/réceptacle/lancer/sauvegarde. Ces points relèvent
des jalons ITEM-VFX01.2 à ITEM-VFX01.4.

**Statut :** modifications poussées sur GitHub à la demande du propriétaire ;
résultats de build et d'automatisation Unreal à obtenir sur Windows.

## ITEM-VFX01.2 — Liquides (potions et flasques)

Le code de migration editor-only est livré dans `UGridItemLiquidVfxMigrationCommandlet`.
Les quatre DataAssets binaires `.uasset` sont suivis par Git LFS ; leur
conversion en vrai format Unreal doit être exécutée par UE 5.5.4 sur la
machine Windows disposant des assets complets.

Assets strictement ciblés : `DA_Item_Potion_Healing_01`,
`DA_Item_Flask_Oil`, `DA_Item_Flask_Acid` et
`DA_Item_Flask_CorrosiveCloud`. L'outil crée une entrée
`VisualEffects.Effects` `LiquidSparkle` pointant vers
`/Game/GrimrockPrototype/Effects/Alchimie/NS_LiquideSparkle` avec le
mode `Always`. Si le système existait dans `LightEmitter.NiagaraSystem`,
le transform local est conservé et la référence historique retirée.
Les propriétés PointLight demeurent inchangées. Une référence Niagara
différente de celle attendue provoque un refus explicite.

L'outil vérifie les quatre assets avant toute écriture, ne touche jamais
les torches et peut être réexécuté sans dupliquer les effets ni
réenregistrer les assets déjà conformes.

### Procédure locale (Editor fermé)

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master

# Compilation et vérification préalable
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Items.VFX01_1"

# Dry-run sans aucune écriture
.\Scripts\ItemVFX012-MigrateLiquidAssets.ps1

# Sauvegarde effective des 4 .uasset via Unreal
.\Scripts\ItemVFX012-MigrateLiquidAssets.ps1 -Apply

# Validation du jalon
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Items.VFX01_2" -SkipBuild
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Items.LIGHT0" -SkipBuild
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Party.LIGHT01" -SkipBuild
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.TechnicalDebt.TD06_4" -SkipBuild

# Après réussite uniquement : publication Git LFS ciblée des .uasset
.\Scripts\ItemVFX012-MigrateLiquidAssets.ps1 -Publish
```

Tests : `MigrationRecipe`, `ProductionPreflight`,
`MigratedProductionAssets` (attend que -Apply ait été exécuté) et
`StackTransferAndWorldRehydration` sous le préfixe
`Grimrock.Items.VFX01_2`.

Le comportement Niagara n'est pas stocké dans l'instance d'inventaire.
La même définition présente un effet `Always` après empilement, séparation,
dépôt, attachement à un réceptacle ou recréation d'acteur, même si la
valeur conservée de `bLightsEnabled` est `false`. Le code de sauvegarde
ne nécessite pas de changement du format de `FGridItemInstance`.

**ITEM-VFX01.2 clôturé :** validation locale réussie (10/10 tests) et les quatre
assets `.uasset` migrés poussés par Git LFS dans le commit `b5ad10c6`.
Les instructions précédentes sont conservées pour la reproductibilité.

## ITEM-VFX01.3 — Flamme conditionnelle de torche

Le seul asset de production modifié par ce jalon est :

`/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Torch`

Le Niagara de flamme existant est :

`/Game/GrimrockPrototype/Art/FX/NS_Flame_8_Torch`

L'outil `UGridItemTorchVfxMigrationCommandlet` transfère uniquement
`LightEmitter.NiagaraSystem` vers une entrée
`VisualEffects.Effects` avec :

- `EffectId = TorchFlame` ;
- `NiagaraSystem = NS_Flame_8_Torch` ;
- `ActivationMode = WhenLightEnabled` ;
- `RelativeTransform` conservant exactement les offsets de la flamme
  anciennement renseignés dans `LightEmitter`.

**Invariant essentiel :** `LightEmitter.bDefaultEnabled`,
`bUsePointLight`, les transformations du PointLight, la couleur,
l'intensité, le rayon, les flickers et
`FGridItemInstance::bLightsEnabled` restent inchangés.
La torche est initialement éteinte pendant la création de son
`AGridItemActor` et la flamme n'apparaît qu'après l'application de
l'état de l'instance. La flamme s'éteint/revient lorsque
`SetItemLightsEnabled(false/true)` est appelé. La lumière du groupe
continue d'être fournie par `UGridPartyIlluminationComponent`,
tandis que la flamme de première personne reste attachée au mesh
de la torche, sans PointLight concurrent.

L'outil est strictement limité au DataAsset de torche, effectue un
dry-run par défaut, refuse les Niagara inattendus et ne synthétise
pas de transform s'il n'existe plus de flamme héritée.
L'opération `-Apply` est idempotente. Aucun fichier de niveau,
réceptacle, Blueprint ou autre DataAsset n'est modifié.

### Procédure sur Windows (UE Editor fermé)

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master

# Construire l'éditeur + régression de base
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Items.VFX01_1"

# Préflight sans écriture
.\Scripts\ItemVFX013-MigrateTorchAsset.ps1

# Sauvegarde effective de DA_Item_Torch.uasset
.\Scripts\ItemVFX013-MigrateTorchAsset.ps1 -Apply

# Migration, données de production et runtime (4 tests)
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Items.VFX01_3" -SkipBuild

# Régressions pertinentes
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Items.LIGHT0" -SkipBuild
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Items.LIGHT_CONFIG02" -SkipBuild
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Party.LIGHT01" -SkipBuild
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Items.VFX01_2" -SkipBuild

# Seulement après toutes les validations : commit et push LFS ciblé
.\Scripts\ItemVFX013-MigrateTorchAsset.ps1 -Publish
```

Le test `Grimrock.Items.VFX01_3.MigratedProductionAsset` est prévu
**après** `-Apply` et échouera volontairement avant la migration.
Le commit des sources C++ ne prouve pas la migration du `.uasset` LFS.
Le jalon n'est clôturé qu'une fois la compilation, les tests et le
push du DataAsset réalisés.

## ITEM-VFX01.4A — Clarification des flasques et audit obligatoire

Depuis le 3 novembre 2026, `NS_LiquideSparkle` est réservé à la potion de
soin. L'utilisateur a retiré manuellement les particules de la flasque
d'huile. L'ancien outil `ITEM-VFX01.2` a donc été limité à la potion
de soin pour éviter de recréer cet effet sur les flasques.

Les flasques d'huile, d'acide et de nuage corrosif ne doivent plus
référencer `LiquidSparkle`, mais conservent leurs autres éventuels
effets Niagara et tous leurs paramètres lumineux.

La procédure de vérification globale et de normalisation ciblée
se trouve dans
[ITEM_VFX014A_LEGACY_AUDIT_AND_SPARKLE_POLICY.md](ITEM_VFX014A_LEGACY_AUDIT_AND_SPARKLE_POLICY.md).

La suppression des anciennes propriétés `LightEmitter.Niagara*`
est **reportée au jalon ITEM-VFX01.4B**, après inspection des
DataAssets binaires par Unreal et publication Git LFS de la correction.
