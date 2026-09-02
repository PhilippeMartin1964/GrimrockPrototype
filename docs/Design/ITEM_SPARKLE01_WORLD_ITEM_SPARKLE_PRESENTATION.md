# ITEM-SPARKLE01 — World Item Sparkle Presentation

Date : 02.09.2026

## Objectif

Permettre à certains items importants de produire un scintillement discret lorsqu'ils sont visibles dans le monde.

Cas de référence :

- clé en cuivre : éclat cuivré ;
- clé en fer : éclat argenté/froid ;
- gemme, artefact ou objet de quête : réutilisation du même système avec une autre couleur.

Le système n'est pas lié à `EGridItemType::Key`. Chaque `UGridItemDefinitionAsset` décide explicitement si l'effet est actif.

## Contrat de données

Dans :

`UGridItemDefinitionAsset > Visual > World Sparkle`

```text
Enable World Sparkle
World Sparkle Material
World Sparkle Color
World Sparkle Intensity
World Sparkle Speed
World Sparkle Variation
```

Valeurs par défaut :

```text
Enable World Sparkle = false
Color                = (1.0, 0.78, 0.42, 1.0)
Intensity            = 2.0
Speed                = 1.0
Variation            = 0.35
```

Une définition avec `Enable World Sparkle=true` et sans matériau de scintillement est invalide.

## Runtime

`AGridItemActor` contient désormais :

```text
SceneRoot
└─ MeshComponent
   └─ WorldSparkleMesh
```

`WorldSparkleMesh` :

- reprend exactement le même Static Mesh que `MeshComponent` ;
- n'a aucune collision ;
- ne simule jamais la physique ;
- n'émet aucune ombre ;
- n'altère jamais le matériau normal de l'item ;
- est caché lorsque le sparkle est inactif.

Le matériau de scintillement est instancié avec un `UMaterialInstanceDynamic`.

## Cycle de vie

```text
InitializeFromItemDefinition
→ Sparkle OFF

ConfigureAsWorldPickup
→ Sparkle ON si l'asset le demande

ConfigureAsAttachedItem
→ Sparkle OFF

OnRemovedFromWorld
→ Sparkle OFF

Retour dans le monde
→ Sparkle ON à nouveau
```

Le scintillement n'est donc pas automatiquement visible :

- dans la main ;
- comme objet attaché ;
- après retrait du monde.

Un item physiquement posé comme pickup peut scintiller.

### Cas `PhysicalAtHit`

Le code réel de `AGridReceptacleActor::ApplyVisualPlacement()` détache l'acteur et appelle explicitement `ConfigureAsWorldPickup()` en mode `PhysicalAtHit`. Ce mode est donc traité comme une présentation physique dans le monde : le sparkle est ON si l'asset le demande. Les modes de réceptacle réellement attachés passent par `ConfigureAsAttachedItem()` et gardent le sparkle OFF.

## Contrat du matériau

Le matériau recommandé est :

`M_Item_WorldSparkle`

Il doit exposer exactement ces paramètres :

```text
Vector  SparkleColor
Scalar  SparkleIntensity
Scalar  SparkleSpeed
Scalar  SparkleVariation
Scalar  SparklePhase
```

Le runtime les alimente automatiquement.

`SparklePhase` est déterminé à partir du RuntimeObjectId afin que plusieurs items identiques ne pulsent pas tous exactement en même temps.

## Matériau recommandé

Configuration UE5 suggérée :

```text
Material Domain : Surface
Blend Mode      : Additive ou Translucent
Shading Model   : Unlit
Two Sided       : selon le mesh
```

Le matériau doit produire un éclat bref, pas une émission permanente.

Principe :

```text
Time
 × SparkleSpeed
 + SparklePhase
        ↓
fonction périodique / noise
        ↓
Variation
        ↓
pulse étroit
        ↓
SparkleColor × SparkleIntensity × pulse
        ↓
Emissive Color
```

Pour éviter le chevauchement exact avec le mesh source, un très léger World Position Offset selon `VertexNormalWS` peut être utilisé dans le matériau.

Ne pas désactiver le Depth Test : l'éclat ne doit pas être visible à travers les murs.

## Réglages de départ

### Clé cuivre

```text
Enable World Sparkle = true
World Sparkle Material = M_Item_WorldSparkle
World Sparkle Color ≈ cuivre chaud
  R = 0.78
  G = 0.38
  B = 0.12
Intensity = 2.0
Speed = 1.2
Variation = 0.35
```

### Clé fer / argent

```text
Enable World Sparkle = true
World Sparkle Material = M_Item_WorldSparkle
World Sparkle Color ≈ argent froid
  R = 0.75
  G = 0.82
  B = 1.00
Intensity = 1.8
Speed = 1.0
Variation = 0.30
```

## Performance

Aucun Tick C++ n'est nécessaire pour l'animation.

Le runtime crée un MID par item scintillant et le matériau utilise `Time` côté GPU.

Le second mesh est strictement une couche de présentation, sans physique ni collision.

## Automation

Filtre :

`Grimrock.Items.ITEM_SPARKLE01`

Le test vérifie :

- création du composant de scintillement ;
- réplication du Static Mesh ;
- état OFF après initialisation ;
- activation en World Pickup ;
- création du MID ;
- absence de collision et physique ;
- désactivation comme Attached Item ;
- réactivation après retour dans le monde ;
- désactivation après retrait ;
- refus propre d'un sparkle activé sans matériau.
