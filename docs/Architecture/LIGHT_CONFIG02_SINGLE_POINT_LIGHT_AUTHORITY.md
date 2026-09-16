# LIGHT-CONFIG02 — Single Point-Light Base Authority

Date : 16 septembre 2026

## Objectif

`FGridLightEmitterConfig` contenait deux familles de valeurs capables de définir la même base lumineuse :

```text
LightIntensity      / BaseLightIntensity
LightRadius         / BaseAttenuationRadius
LightColor          / BaseLightColor
```

Le runtime choisissait ensuite une valeur via des conventions implicites :

```text
BaseLightIntensity > 0 ? BaseLightIntensity : LightIntensity
BaseAttenuationRadius > 0 ? BaseAttenuationRadius : LightRadius
BaseLightColor == Black ? LightColor : BaseLightColor
```

Cela créait trois doubles autorités et deux sentinelles d'authoring (`0` et `Black`).

## Décision

Les seules bases du PointLight sont désormais :

```text
LightIntensity
LightRadius
LightColor
```

Le flicker ne possède que des amplitudes et des vitesses :

```text
FlickerIntensityAmount
FlickerRadiusAmount
PointLightFlickerPositionAmplitude
FlickerWarmColor / FlickerHotColor / ColorFlickerAmount
```

Supprimés du schéma :

```text
BaseLightIntensity
BaseAttenuationRadius
BaseLightColor
```

Aucun fallback de compatibilité n'est maintenu dans `UGridLightEmitterComponent`.

## Runtime

`UGridLightEmitterComponent` applique directement :

```text
base intensity = RuntimeConfig.LightIntensity
base radius    = RuntimeConfig.LightRadius
base color     = RuntimeConfig.LightColor
```

Le flicker part toujours de cette base unique.

`UGridPartyIlluminationComponent` sélectionne également la source la plus forte uniquement avec `LightIntensity` et applique ses multiplicateurs sur `LightIntensity`, `LightRadius` et les amplitudes de flicker correspondantes.

## Production asset

Le ticket ne modifie aucun `.uasset` binaire.

Le test charge cependant :

```text
/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Torch
```

et exige que la torche de production possède déjà :

```text
bUsePointLight = true
LightIntensity > 0
LightRadius > 0
LightColor != Black
```

Cela évite de masquer une autorité encore dépendante des anciens champs supprimés.

## Test

```text
Grimrock.Items.LIGHT_CONFIG02.SinglePointLightAuthority
```

Le test vérifie par réflexion que les trois anciens champs n'existent plus et que les trois champs canoniques restent présents.

## Validation locale

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Items.LIGHT_CONFIG02"
```

Régression recommandée après ce filtre :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Party.LIGHT01"
```

Le ticket est clos uniquement après validation UE5.5.4 locale verte.
