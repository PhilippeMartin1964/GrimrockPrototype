# ITEM-LIGHT01 — Data-Driven Item Light Emitter

Statut : implémenté en C++, validation UE locale requise.  
Portée : items ramassables, monde, réceptacles, équipement/visuel tenu et projectiles récupérables.

## 1. Principe

La lumière d'un item est définie une seule fois dans son `UGridItemDefinitionAsset`.

```text
DA_Item_XXX
  -> UGridItemDefinitionAsset::LightEmitter
      -> FGridLightEmitterConfig
          -> AGridItemActor
              -> UGridLightEmitterComponent
                  -> Niagara
                  -> PointLight
                  -> Flicker
```

`UGridLightEmitterComponent` est un exécuteur runtime. Il ne constitue plus une seconde source d'authoring.

## 2. Source de vérité

Le bloc `Light Emitter` de `UGridItemDefinitionAsset` contient notamment :

```text
bDefaultEnabled
NiagaraSystem
NiagaraRelativeLocation / Rotation
bUsePointLight
PointLightRelativeLocation / Rotation
LightIntensity
LightRadius
LightColor
bEnableLightFlicker
BaseLightIntensity
FlickerIntensityAmount
FlickerSpeed
FlickerSecondarySpeed
BaseAttenuationRadius
FlickerRadiusAmount
bEnableLightPositionFlicker
PointLightFlickerPositionAmplitude
PositionFlickerSpeed
PositionFlickerSecondarySpeed
bEnableLightColorFlicker
BaseLightColor
FlickerWarmColor
FlickerHotColor
ColorFlickerAmount
ColorFlickerSpeed
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
3. `SetItemLightsEnabled()` active ou désactive le Niagara et/ou le PointLight.

Aucun `BP_Item_Torch` n'est nécessaire comme autorité runtime pour une torche tenue en main.

## 5. Visuel tenu

`AGrimrockPartyPawn::EquipHeldItem()` résout désormais l'`ItemDefinition` et génère un `AGridItemActor` générique.

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

Le booléen historique `bHasTorchInHand` est encore conservé comme état public/Blueprint de présentation, mais il signifie désormais « le personnage sélectionné présente une source lumineuse tenue » ; la logique ne dépend plus de l'identifiant `Item_Torch`.

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
- un PointLight fixe est actif sans flicker.

Le tick n'est activé que si une variation runtime du PointLight est nécessaire : intensité/rayon, position ou couleur.

## 8. Migration de `DA_Item_Torch`

Le `.uasset` doit être migré dans Unreal Editor après compilation C++.

Dans `DA_Item_Torch`, renseigner le nouveau bloc `Light Emitter` avec les valeurs de présentation actuellement utilisées par la torche :

```text
bDefaultEnabled = true
NiagaraSystem = NS_Flame_8_Torch   // si c'est bien le système actuellement retenu
bUsePointLight = true
```

Puis reporter les offsets, intensité, rayon, couleur et paramètres de flicker voulus.

Ne pas conserver une seconde configuration équivalente dans `BP_Item_Torch` pour la torche item. Le DataAsset est l'autorité.

## 9. Tests

Test dédié :

```text
Grimrock.Items.LIGHT01.DataDrivenEmitter
```

Il vérifie :

- détection d'un émetteur configuré ;
- état par défaut ;
- propagation vers `FGridItemInstance` ;
- exécution par l'`AGridItemActor` générique ;
- paramètres du PointLight ;
- absence de tick pour une lumière fixe ;
- activation du tick pour le flicker ;
- extinction complète et arrêt du tick.

Régressions associées :

```text
Grimrock.TechnicalDebt.TD01_2
Grimrock.TechnicalDebt.TD06_8
Grimrock.Monsters.MON11.Presentation.ThrownWeaponLifecycle
```

## 10. Règle d'architecture

Pour ajouter une lanterne, un cristal lumineux, une gemme éclairante ou une autre source transportable :

```text
Créer/configurer DA_Item_XXX
    -> renseigner LightEmitter
    -> aucune nouvelle classe C++
    -> aucun Blueprint d'acteur lumineux spécialisé requis
```

Une nouvelle classe ne se justifie que si l'objet possède un comportement gameplay réellement différent de la primitive d'item générique, jamais uniquement pour changer sa lumière.
