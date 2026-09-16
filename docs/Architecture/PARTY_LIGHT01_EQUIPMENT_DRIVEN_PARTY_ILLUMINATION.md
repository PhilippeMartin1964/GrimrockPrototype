# PARTY-LIGHT01 — Equipment-Driven Party Illumination

Statut : implémenté en C++, validation UE locale requise.

## 1. Principe

La définition d'un item lumineux reste inchangée : `UGridItemDefinitionAsset::LightEmitter` reste l'unique source d'authoring des paramètres lumineux de l'objet.

`PARTY-LIGHT01` sépare désormais deux présentations runtime :

```text
Item dans le monde / réceptacle
  -> AGridItemActor
      -> Niagara de l'item
      -> PointLight local de l'item

Item lumineux équipé en MainHand / OffHand
  -> AGridItemActor tenu
      -> Niagara de l'item uniquement
  -> UGridPartyIlluminationComponent
      -> PointLight ergonomique du groupe
```

Le `DA_Item_Torch` n'est donc pas enrichi d'un second profil "held". Ses paramètres existants sont transmis au proxy lumineux du groupe.

## 2. Autorité des données

L'instance conserve :

```text
FGridItemInstance::bLightsEnabled
```

La définition conserve :

```text
FGridLightEmitterConfig
```

Le composant du groupe copie uniquement les paramètres utiles au PointLight : intensité, rayon, couleur et flicker.

Il ne copie volontairement pas :

```text
NiagaraSystem
NiagaraRelativeLocation / Rotation
PointLightRelativeLocation / Rotation
```

Ces transforms appartiennent à la présentation physique de l'item. Le proxy du groupe possède son propre transform relativement à la caméra.

## 3. Composant du groupe

`UGridPartyIlluminationComponent` dérive de `UGridLightEmitterComponent` et fournit :

```text
IntensityMultiplier
RadiusMultiplier
bCastShadows
ActiveSourceId
```

Valeurs par défaut :

```text
IntensityMultiplier = 1.0
RadiusMultiplier    = 1.0
bCastShadows        = false
```

Le mode sans ombres est volontaire pour éviter les grandes ombres de première personne produites par les objets tenus.

Le composant peut être ajouté directement à `BP_GrimrockPartyPawn` afin de régler son transform sous la caméra et ses multiplicateurs. Si aucun composant n'est authoré dans le Blueprint, le Pawn crée un fallback runtime attaché à la caméra, décalé de `X=25 cm`.

## 4. Sélection de la source d'équipement

L'éclairage est une propriété du groupe et non du personnage sélectionné.

Le runtime inspecte les `MainHand` et `OffHand` de tous les personnages actifs. Une source est admissible si :

```text
ItemInstance.bLightsEnabled == true
&& ItemDefinition.LightEmitter.bUsePointLight == true
```

Si plusieurs sources sont présentes, la source possédant la plus forte intensité effective (`BaseLightIntensity` si renseignée, sinon `LightIntensity`) est retenue. En cas d'égalité, l'ordre est déterministe : index de personnage croissant, puis `MainHand`, puis `OffHand`.

Cette règle évite d'additionner brutalement plusieurs torches.

## 5. Item tenu

Le `HeldItemActor` reste logiquement allumé, mais son exécuteur lumineux reçoit un masque de présentation :

```text
Niagara   = ON
PointLight = OFF
```

La flamme reste donc visuellement attachée à la torche tenue, tandis que l'éclairage du donjon provient du groupe.

Les items dans le monde, les réceptacles et les projectiles conservent les deux canaux par défaut.

## 6. Extension magie

Le composant du groupe accepte directement un `FGridLightEmitterConfig` et un identifiant de source via `ApplyIlluminationSource()`.

La future magie pourra donc fournir le même contrat :

```text
Item équipé  -> FGridLightEmitterConfig -> PartyIllumination
Sort lumière -> FGridLightEmitterConfig -> PartyIllumination
Effet/buff   -> FGridLightEmitterConfig -> PartyIllumination
```

La politique de priorité entre équipement, magie et effets temporaires n'est pas introduite dans PARTY-LIGHT01 ; ce ticket ne traite que l'équipement.

## 7. Validation

Test dédié :

```text
Grimrock.Party.LIGHT01.EquipmentDrivenIllumination
```

Il vérifie notamment :

- une source portée par un personnage non sélectionné éclaire le groupe ;
- les paramètres intensité/rayon sont transmis ;
- les offsets physiques de l'item ne sont pas transmis ;
- le proxy ne crée aucun Niagara ;
- le proxy est sans ombres par défaut ;
- le HeldItem garde le canal Niagara mais délègue son PointLight ;
- la source la plus forte gagne ;
- le retrait d'une source sélectionne la suivante ;
- le retrait de la dernière source éteint le groupe.

Régressions recommandées :

```text
Grimrock.Items.LIGHT01
Grimrock.TechnicalDebt.TD01_2
Grimrock.Party.LIGHT01
```
