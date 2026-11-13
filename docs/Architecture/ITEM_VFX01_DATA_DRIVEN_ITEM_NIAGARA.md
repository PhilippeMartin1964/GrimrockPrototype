# ITEM-VFX01 — Effets Niagara indépendants de l'éclairage

> **ARM-CLEAN06 — NOUVELLE AUTORITÉ (13.11.2026).** [Modèle cible approuvé](ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md) · [Migration](ARM_CLEAN06_MIGRATION_PLAN.md). Les mécaniques et tests historiques restent pertinents ; les schémas DataAssets ou fallbacks anciens ne sont plus des prescriptions de conception. **Cible non encore implémentée ; aucun legacy dans le runtime final.**


> Architecture courante depuis ITEM-VFX01.4B (3 novembre 2026).
> Jalon .1 validé ; .2 et .3 migrés et publiés ;
> .4A audité (47 assets, 17 tests et validation Git LFS) ;
> .4B = retrait définitif du canal Niagara historique.

## 1. Contrat de données

`UGridItemDefinitionAsset` possède deux blocs orthogonaux :

- `VisualEffects : FGridItemVisualEffectsConfig` gère **zéro à N** effets
  `FGridItemNiagaraEffectConfig` (ID, référence `NiagaraSystem`,
  `RelativeTransform`, `ActivationMode`).
- `LightEmitter : FGridLightEmitterConfig` définit **uniquement** le
  PointLight, ses offsets, sa couleur, son rayon, son intensité, son
  flicker et `bDefaultEnabled`.

L'état gameplay `FGridItemInstance::bLightsEnabled` reste dans
l'instance, et survit aux transferts d'inventaire et à la sauvegarde.
Aucun `UNiagaraComponent` n'est enregistré dans les sauvegardes.

Les modes Niagara sont :
- `Always` : actif tant que l'acteur est présenté, indépendamment
  de `bLightsEnabled` ;
- `WhenLightEnabled` : actif seulement si l'instance est allumée
  et si le visuel de l'acteur est présenté.

## 2. Hiérarchie runtime

```text
AGridItemActor
  SceneRoot
    MeshComponent (physique / Chaos)
      UGridLightEmitterComponent  -> UPointLightComponent (0 ou 1)
      UGridItemVisualEffectsComponent
        -> UNiagaraComponent[0..N]
```

Les offsets de Niagara sont des transforms relatifs au composant
`VisualEffects` attaché au mesh. Le paramètre Niagara `Local Space`
est indépendant de la hiérarchie d'attachement UE : vérifier le rendu
des particules en mouvement pour les systèmes nécessitant de suivre
une rotation déjà émise.

`UGridLightEmitterComponent` ne possède **aucun**
`UNiagaraComponent` ni champ `NiagaraSystem` :
- `SetLightEnabled()` contrôle exclusivement le PointLight ;
- `SetPointLightPresentationEnabled()` masque le PointLight local
  sans changer le flag gameplay ni les effets visuels ;
- `SetPointLightCastShadows()` conserve la politique d'ombres ;
- le Tick C++ ne sert qu'aux variations de PointLight (flicker).

`UGridItemVisualEffectsComponent` :
- `ApplyConfig()` réutilise les effets et supprime les composants
  devenus inutiles ;
- `SetItemLightState()` met à jour les effets conditionnels ;
- `SetPresentationEnabled()` masque les effets lors de la disparition
  de l'acteur ;
- aucune Tick C++ propre, et aucune source alternative d'authoring.

## 3. Assets canoniques après migrations

| Item | VisualEffects | PointLight |
|---|---|---|
| `DA_Item_Potion_Healing_01` | `LiquidSparkle` (`NS_LiquideSparkle`, `Always`) | selon définition |
| `DA_Item_Flask_Oil` | aucun `LiquidSparkle` | selon définition |
| `DA_Item_Flask_Acid` | aucun `LiquidSparkle` | selon définition |
| `DA_Item_Flask_CorrosiveCloud` | aucun `LiquidSparkle` | selon définition |
| `DA_Item_Torch` | `TorchFlame` (`NS_Flame_8_Torch`, `WhenLightEnabled`) | actif quand `bLightsEnabled` |

Les autres effets éventuellement définis sur les flasques restent
autorisés ; seule l'affectation de `LiquidSparkle` à la potion de soin
est exclusive pour le moment.

## 4. Torche équipée et éclairage du groupe

```text
HeldItemActor
  VisualEffects = flamme conditionnelle sur mesh tenu
  LightEmitter.PointLight = masqué (SetPointLightPresentationEnabled(false))

UGridPartyIlluminationComponent
  PointLight = lumière de groupe, alimentée par item équipé
```

`UGridPartyIlluminationComponent` réutilise le seul
`FGridLightEmitterConfig` sans copier le transform physique du
PointLight de l'item. Les effets Niagara restent sur les items ;
aucune seconde flamme n'est créée par le proxy.

## 5. Historique clos

- **ITEM-VFX01.1** : infrastructure générique
  `VisualEffects` / `UGridItemVisualEffectsComponent` et tests.
- **ITEM-VFX01.2** : migration des potions et flasques, publiées via
  Git LFS au commit `b5ad10c6` ; restriction ultérieure du sparkle
  à la potion de soin.
- **ITEM-VFX01.3** : migration torche
  `WhenLightEnabled` avec offsets préservés, publiée via
  Git LFS au commit `4cb17902`.
- **ITEM-VFX01.4A** : normalisation des flasques et audit de 47
  définitions (`legacy=0, invalidSparkle=0, loadFailure=0`),
  publiée au commit `f19ab6ea`.
- **ITEM-VFX01.4B** : les migrations d'assets étant achevées,
  suppression des propriétés Niagara historiques, du composant
  Niagara de l'exécuteur lumineux et des anciens scripts/commandlets
  editor-only. Les tests de migration ponctuels sont remplacés
  par des tests permanents de données.

Les anciens scripts `ItemVFX012-MigrateLiquidAssets.ps1`,
`ItemVFX013-MigrateTorchAsset.ps1` et
`ItemVFX014-AuditAndNormalize.ps1` sont volontairement retirés :
les relancer après retrait des propriétés héritées serait invalide.

## 6. Validation de l'architecture

Sous UE 5.5.4 :

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Items.VFX01_4B"

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Items.VFX01_1" -SkipBuild
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Items.VFX01_2" -SkipBuild
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Items.VFX01_3" -SkipBuild
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Items.VFX01_4A" -SkipBuild

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Items.LIGHT0" -SkipBuild
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Items.LIGHT_CONFIG02" -SkipBuild
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Party.LIGHT01" -SkipBuild
```

Les tests de production chargent les vrais DataAssets depuis Unreal ;
une inspection des pointeurs Git LFS seuls ne suffit pas à attester
leur configuration. Vérifier aussi la flamme en PIE (torche au sol,
prise en main, extinction, allumage, réceptacle, lancer).

Voir `ITEM_VFX014B_REMOVE_LEGACY_LIGHT_NIAGARA.md` pour le périmètre
et les critères d'acceptation de la dernière phase.
