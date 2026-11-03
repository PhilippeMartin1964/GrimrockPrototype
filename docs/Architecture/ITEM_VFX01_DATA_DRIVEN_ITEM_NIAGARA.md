# ITEM-VFX01 — Effets Niagara indépendants des lumières

> ITEM-VFX01.1 — validé sur UE 5.5.4 (6/6 tests). ITEM-VFX01.2 — outil de migration des assets Git LFS et tests à valider sur Windows.

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

La validation locale et le commit Git LFS des assets sont requis pour
considérer ITEM-VFX01.2 totalement clôturé. La livraison initiale de
sources n'est pas une preuve de migration binaire.
