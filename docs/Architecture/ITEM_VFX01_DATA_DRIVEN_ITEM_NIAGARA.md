# ITEM-VFX01 — Effets Niagara indépendants des lumières

> ITEM-VFX01.1 — code C++ et tests intégrés à GitHub, **compilation et tests UE 5.5.4 locaux non exécutés**.

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
