# PARTY-LIGHT01 — Equipment-Driven Party Illumination

Statut : implémenté en C++, validation UE locale réussie le 16.09.2026.

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

Depuis `LIGHT-CONFIG02`, les bases du PointLight sont uniques :

```text
LightIntensity
LightRadius
LightColor
```

Le flicker module ces valeurs via ses amplitudes ; il ne possède plus ses propres champs de base.

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

Le composant doit être ajouté explicitement à `BP_GrimrockPartyPawn`, idéalement comme enfant de `Camera`, afin de régler son transform et ses multiplicateurs. Le C++ ne crée plus aucun `PartyIlluminationRuntime` automatique : si le Blueprint ne contient pas de `UGridPartyIlluminationComponent`, le groupe ne diffuse aucune lumière proxy.

### Transform du proxy

Le `Transform` de `GridPartyIllumination` contrôle uniquement l'origine du **PointLight ergonomique du groupe**. Le PointLight runtime est attaché au composant ; `BuildPartyConfig()` remet volontairement `PointLightRelativeLocation` et `PointLightRelativeRotation` de l'item à zéro afin que l'origine du proxy soit exactement celle du composant `GridPartyIllumination`.

Ainsi :

```text
GridPartyIllumination.Location / Rotation
  -> déplace/oriente le PointLight invisible du groupe

HeldItemRoot + HeldItemRelativeLocation / Rotation / Scale
  -> place le mesh de l'item tenu

DA_Item_Torch.LightEmitter.NiagaraRelativeLocation / Rotation
  -> place la flamme Niagara relativement à l'item tenu
```

Déplacer `GridPartyIllumination` ne doit donc **jamais déplacer la torche visible ni sa flamme**. Pour déplacer la torche tenue, régler `HeldItemRoot` et/ou `HeldItemRelativeLocation`. Pour recaler uniquement la flamme sur le mesh, régler les transforms Niagara de `DA_Item_Torch`.

Avec un rayon de plusieurs mètres, un déplacement du proxy de quelques dizaines de centimètres peut être visuellement discret. Pour vérifier le transform du proxy en PIE, utiliser temporairement un offset nettement plus grand, puis remettre la valeur d'authoring voulue.

### Ombres runtime

Le PointLight créé par `UGridLightEmitterComponent` est explicitement `Movable`. Les chemins d'ombres pour géométrie statique et dynamique restent activés ; `bCastShadows` sert de commutateur maître.

```text
bCastShadows = false
  -> aucune ombre portée par la lumière du groupe

bCastShadows = true
  -> les murs, grilles, portes et autres objets pouvant caster une ombre
     occultent la lumière du groupe en temps réel
```

`bCastShadows` ne contrôle pas l'ombre du mesh de la torche tenue lui-même ; il contrôle les ombres produites par le PointLight du groupe sur le décor.

## 4. Sélection de la source d'équipement

L'éclairage est une propriété du groupe et non du personnage sélectionné.

Le runtime inspecte les `MainHand` et `OffHand` de tous les personnages actifs. Une source est admissible si :

```text
ItemInstance.bLightsEnabled == true
&& ItemDefinition.LightEmitter.bUsePointLight == true
```

Si plusieurs sources sont présentes, la source possédant la plus forte `LightIntensity` est retenue. En cas d'égalité, l'ordre est déterministe : index de personnage croissant, puis `MainHand`, puis `OffHand`.

Cette règle évite d'additionner brutalement plusieurs torches et ne dépend plus d'un fallback `BaseLightIntensity`.

Le recalcul de l'éclairage du groupe est volontairement séparé du recalcul du visuel tenu du personnage sélectionné :

```text
notification personnage sélectionné / INDEX_NONE
  -> refresh PartyIllumination
  -> refresh HeldItem du personnage sélectionné

notification d'un autre personnage
  -> refresh PartyIllumination uniquement
  -> HeldItem sélectionné inchangé
```

Ainsi, équiper ou retirer une torche sur un compagnon modifie bien l'éclairage global sans recréer ni supprimer le visuel first-person du personnage sélectionné.

## 5. Item tenu

Le `HeldItemActor` reste logiquement allumé, mais son exécuteur lumineux reçoit un masque de présentation :

```text
Niagara    = ON
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

Tests dédiés :

```text
Grimrock.Party.LIGHT01.EquipmentDrivenIllumination
Grimrock.Items.LIGHT_CONFIG02.SinglePointLightAuthority
```

Ils vérifient notamment :

- qu'un Pawn C++ nu ne reçoit plus automatiquement de `PartyIlluminationRuntime` ;
- qu'un composant d'illumination ajouté explicitement est utilisé ;
- une source portée par un personnage non sélectionné éclaire le groupe ;
- les paramètres intensité/rayon sont transmis ;
- les offsets physiques de l'item ne sont pas transmis ;
- le proxy ne crée aucun Niagara ;
- le PointLight runtime est `Movable` ;
- les ombres de géométrie statique et dynamique sont autorisées quand `Cast Shadows` est actif ;
- le commutateur d'ombres peut être désactivé/réactivé à l'exécution ;
- le HeldItem garde le canal Niagara mais délègue son PointLight ;
- la source la plus forte gagne selon `LightIntensity` ;
- le retrait d'une source sélectionne la suivante ;
- le retrait de la dernière source éteint le groupe ;
- les trois anciennes bases alternatives ont disparu de la réflexion.

Le code garantit en outre que le PointLight runtime est enfant de `GridPartyIllumination`, avec un offset local de proxy nul. Le test automatisé courant ne mesure pas encore explicitement le transform monde issu du composant Blueprint ; ce point se vérifie en PIE si un doute subsiste sur un asset particulier.

Régressions recommandées :

```text
Grimrock.Items.LIGHT01
Grimrock.Items.LIGHT_CONFIG02
Grimrock.TechnicalDebt.TD01_2
Grimrock.Party.LIGHT01
```
