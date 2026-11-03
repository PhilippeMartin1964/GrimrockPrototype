# ITEM-LIGHT01 — Data-Driven Item Light Emitter

> **ITEM-VFX01.4B — architecture actuelle (3 novembre 2026).** Le système d'éclairage d'item ne possède plus de canal Niagara. Les particules sont exclusivement configurées dans `VisualEffects`.

Statut : implémenté et validé localement sous UE 5.5.4.  
Portée : items ramassables, monde, réceptacles, équipement/visuel tenu et projectiles récupérables.

## 1. Principe

La lumière d'un item est définie une seule fois dans son `UGridItemDefinitionAsset`.

```text
DA_Item_XXX
  -> UGridItemDefinitionAsset::LightEmitter
      -> FGridLightEmitterConfig
          -> AGridItemActor
              -> UGridLightEmitterComponent
                  -> PointLight
                  -> Flicker

DA_Item_XXX
  -> UGridItemDefinitionAsset::VisualEffects
      -> UGridItemVisualEffectsComponent
          -> Niagara[0..N]
```

`UGridLightEmitterComponent` est un exécuteur runtime. Il ne constitue plus une seconde source d'authoring.

## 2. Source de vérité

Le bloc `Light Emitter` de `UGridItemDefinitionAsset` contient notamment :

```text
bDefaultEnabled
bUsePointLight
PointLightRelativeLocation / Rotation
LightIntensity
LightRadius
LightColor
bEnableLightFlicker
FlickerIntensityAmount
FlickerSpeed
FlickerSecondarySpeed
FlickerRadiusAmount
bEnableLightPositionFlicker
PointLightFlickerPositionAmplitude
PositionFlickerSpeed
PositionFlickerSecondarySpeed
bEnableLightColorFlicker
FlickerWarmColor
FlickerHotColor
ColorFlickerAmount
ColorFlickerSpeed
```

Depuis `LIGHT-CONFIG02`, les trois bases du PointLight ont chacune une autorité unique :

```text
LightIntensity = intensité de base
LightRadius    = rayon de base
LightColor     = couleur de base
```

Le flicker module ces valeurs via ses amplitudes. Il n'existe plus de seconde base ni de convention `0 = fallback` / `Black = fallback`.

Supprimés :

```text
BaseLightIntensity
BaseAttenuationRadius
BaseLightColor
```

Les anciens champs de `UGridItemDefinitionAsset` ne sont plus l'autorité :

```text
bCanEmitLight
bDefaultLightEnabled
LightRadius   // ancien champ item
```

Ils ont été remplacés par `LightEmitter`.

## 3. Etat runtime

`FGridItemInstance::bLightsEnabled` reste volontairement conservé. Il ne décrit pas la présentation de la lumière : il mémorise uniquement l'état courant de l'instance.

Ainsi :

```text
ItemDefinition.LightEmitter = comment l'item éclaire
ItemInstance.bLightsEnabled  = cette instance est-elle actuellement allumée ?
```

L'état peut donc survivre aux transferts inventaire / équipement / réceptacle / monde et à la sauvegarde sans dupliquer les paramètres visuels.

## 4. Acteur générique

Tout item lumineux utilise `AGridItemActor`.

Chaque `AGridItemActor` possède nativement un :

```text
UGridLightEmitterComponent LightEmitterComponent
```

Lors de `InitializeFromItemDefinition()` :

1. la configuration `LightEmitter` est copiée dans le composant runtime ;
2. le composant reste éteint tant que l'état de l'instance n'a pas été appliqué ;
3. `SetItemLightsEnabled()` applique l'état logique de l'instance ;
4. le contexte de présentation peut masquer son PointLight sans modifier cet état logique ni les effets Niagara indépendants.

Aucun `BP_Item_Torch` n'est nécessaire comme autorité runtime pour une torche tenue en main.

## 5. Visuel tenu et éclairage du groupe

`AGrimrockPartyPawn::EquipHeldItem()` résout l'`ItemDefinition` et génère un `AGridItemActor` générique.

Le choix d'un item lumineux équipé repose sur :

```text
Item.bLightsEnabled
&& ItemDefinition->HasLightEmitter()
```

Le mesh tenu vient de :

```text
EquippedMesh
```

avec `WorldMesh` comme fallback via `LoadHeldMesh()`.

Depuis `PARTY-LIGHT01`, un item lumineux tenu ne produit plus son PointLight directement depuis le mesh de première personne :

```text
HeldItemActor
  VisualEffectsComponent = ON (flamme si item allumé)
  LightEmitterComponent.PointLight = OFF

UGridPartyIlluminationComponent
  PointLight = paramètres transmis depuis ItemDefinition.LightEmitter
```

La flamme reste donc physiquement sur l'item tenu, tandis que l'éclairage ergonomique du donjon appartient au groupe. `DA_Item_Torch` demeure l'unique source des paramètres lumineux de la torche. Voir `PARTY_LIGHT01_EQUIPMENT_DRIVEN_PARTY_ILLUMINATION.md`.

Le Pawn ne conserve plus aucun contrat spécifique à la torche pour la présentation tenue :

```text
DefaultInteractionItemId     // supprimé
DefaultHeldItemDefinitionId  // supprimé
HeldTorchActorClass          // supprimé
bHasTorchInHand              // supprimé
```

L'autorité runtime est directement :

```text
HeldItemActor
HeldItemDefinitionId
HeldItemActor->AreItemLightsEnabled()
```

Il n'existe aucun fallback, alias ou champ deprecated pour ces anciens membres.

## 6. Réceptacles et inventaire

Lorsqu'une nouvelle instance est construite depuis une définition, son état initial vient de :

```cpp
ItemDefinition->IsLightEnabledByDefault()
```

Un item transféré conserve ensuite son propre `bLightsEnabled`.

Un réceptacle ne possède donc aucune configuration spéciale « torche » : il héberge un item standard dont l'acteur applique la même définition lumineuse.

## 7. Performance

Le composant n'effectue aucun tick lorsque :

- il est éteint ;
- aucun émetteur n'est configuré ;
- le canal PointLight est masqué par le contexte de présentation ;
- un PointLight fixe est actif sans flicker.

Le tick n'est activé que si une variation runtime du PointLight effectivement présenté est nécessaire : intensité/rayon, position ou couleur.

## 8. Torche de production après ITEM-VFX01.4B

`DA_Item_Torch` reste le seul asset d'authoring de la torche :
- `LightEmitter` contient uniquement le PointLight, ses offsets, son
  état par défaut, son rayon, son intensité, sa couleur et son flicker ;
- `VisualEffects.Effects[TorchFlame]` référence
  `NS_Flame_8_Torch`, `WhenLightEnabled` et son transform ;
- aucun Niagara ou transform de flamme n'est stocké dans
  `FGridLightEmitterConfig`.

Les champs historiques `NiagaraSystem`,
`NiagaraRelativeLocation`, `NiagaraRelativeRotation` ont été
retirés du C++ **après l'audit de 47 DataAssets (legacy=0)**.
L'ancienne procédure de migration binaire a été clôturée et
ses outils supprimés. Ne pas recréer une configuration dans
`BP_Item_Torch`. Le DataAsset reste l'autorité.

## 9. Tests

Tests dédiés :

```text
Grimrock.Items.LIGHT01.DataDrivenEmitter
Grimrock.Items.LIGHT_CONFIG02.SinglePointLightAuthority
```

Ils vérifient notamment :

- détection d'un émetteur configuré ;
- état par défaut ;
- propagation vers `FGridItemInstance` ;
- exécution par l'`AGridItemActor` générique ;
- paramètres du PointLight ;
- absence de tick pour une lumière fixe ;
- activation du tick pour le flicker ;
- extinction complète et arrêt du tick ;
- absence des anciennes bases `BaseLightIntensity`, `BaseAttenuationRadius`, `BaseLightColor` dans la réflexion ;
- absence des champs Niagara historiques et du runtime Niagara dans `UGridLightEmitterComponent` ;
- présence des valeurs canoniques sur `DA_Item_Torch`.

Régressions associées :

```text
Grimrock.TechnicalDebt.TD01_2
Grimrock.TechnicalDebt.TD06_8
Grimrock.Monsters.MON11.Presentation.ThrownWeaponLifecycle
Grimrock.CharacterCreation.CC5
Grimrock.Party.LIGHT01
```

`TD01_2.PartySelectionHeldVisual.NoLegacyTorchFields` vérifie explicitement que les quatre anciens champs spécifiques torche ne réapparaissent pas dans la réflexion Unreal.

## 10. Règle d'architecture

Pour ajouter une lanterne, un cristal lumineux, une gemme éclairante ou une autre source transportable :

```text
Créer/configurer DA_Item_XXX
    -> renseigner LightEmitter
    -> aucune nouvelle classe C++
    -> aucun Blueprint d'acteur lumineux spécialisé requis
```

Une nouvelle classe ne se justifie que si l'objet possède un comportement gameplay réellement différent de la primitive d'item générique, jamais uniquement pour changer sa lumière.
