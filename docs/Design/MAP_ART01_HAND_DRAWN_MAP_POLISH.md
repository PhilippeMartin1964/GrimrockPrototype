# MAP-ART01 — Hand-Drawn Map Symbols & Geometry Polish

Date : 29 septembre 2026  
Statut : **C++ IMPLÉMENTÉ — VALIDATION LOCALE/PIE REQUISE**

## Prérequis validé

MAP-UI03 a été validé par l'utilisateur :

- `Grimrock.UI.MapUI03` : 2/2, 0 warning, 0 échec — `TD04-20260929-093744` ;
- `Grimrock.Map.MON21_6_9` : 5/5, 0 warning, 0 échec — `TD04-20260929-093803` ;
- comportement PIE confirmé.

## Objectif

Renforcer le caractère de carte de donjon dessinée à la main sans changer l'architecture MON21.6.

Le pass reste entièrement dans `UGridMapWidget::NativePaint()` :

```text
FGridMapFloorView
 -> primitives procédurales de présentation
 -> Slate NativePaint
```

Il n'ajoute :

- aucun widget par cellule ;
- aucune texture obligatoire ;
- aucun acteur Map ;
- aucune donnée SaveGame ;
- aucun identifiant d'objet dans le read model.

## Murs

Les murs conservent le double trait irrégulier MON21.6.11 et gagnent de petites marques transversales déterministes suggérant des joints de pierre.

Réglage :

```text
WallStoneMarkCount = 3
```

Les positions sont légèrement irrégulières mais stables pour une même cellule/arête.

## Portes

Les jambages existants restent la silhouette principale.

### Porte fermée

Ajout de :

- traits secondaires de panneau ;
- contreventement diagonal ;
- irrégularité d'encrage conservée.

Réglage :

```text
DoorPanelLineCount = 2
```

### Porte ouverte

La rupture centrale reste visible et un trait de vantail incliné suggère l'ouverture.

### Porte secrète découverte

Le petit signe transversal existant devient un glyphe plus travaillé.

Une porte secrète **non découverte** reste projetée comme `Wall` par le read model : le renderer ne possède toujours aucune information lui permettant de la distinguer d'un mur normal.

## Escaliers

Les glyphes Up/Down deviennent de petits escaliers dessinés :

- marches en perspective/taper ;
- deux lignes latérales légères ;
- flèche directionnelle ;
- légère asymétrie selon la variante.

Réglage :

```text
StairStepCount = 4
```

## Pits

Le pit devient une trappe/fosse plus lisible :

- contour extérieur irrégulier ;
- second contour intérieur ;
- quatre traits de profondeur ;
- hachures internes.

Réglage :

```text
PitDepthLineCount = 3
```

Le pit conserve `HazardSymbolColor`.

## Relocation / téléporteur

Le symbole devient plus runique :

- losange extérieur ;
- losange intérieur décalé ;
- quatre rayons ;
- trait central irrégulier.

## Points d'intérêt

Le POI devient un petit signe de cartographe/rose manuscrite :

- quatre grands axes ;
- diagonales secondaires ;
- losange intérieur ;
- marque centrale.

## Variantes procédurales

Les symboles ont plusieurs variantes visuelles sans créer plusieurs assets.

```text
SymbolVariantCount = 3
```

La variante est calculée par :

```text
ComputeDeterministicSymbolVariant(MapCell, SymbolKind, VariantCount)
```

Elle dépend uniquement :

- de la coordonnée Map ;
- du type de symbole ;
- du nombre de variantes.

Conséquences :

- deux symboles du même type peuvent avoir de petites différences ;
- le rendu ne change pas aléatoirement à chaque ouverture ;
- aucune nouvelle donnée persistante n'est nécessaire.

## UMG

**Aucune modification de `WBP_GridMap` n'est requise pour MAP-ART01.**

Les nouvelles propriétés peuvent être ajustées dans les Class Defaults si nécessaire, mais les valeurs C++ sont les valeurs de référence :

```text
WallStoneMarkCount = 3
DoorPanelLineCount = 2
StairStepCount = 4
PitDepthLineCount = 3
SymbolVariantCount = 3
```

Si le Blueprint possède ultérieurement un override explicite de ces propriétés, utiliser **Reset to Default** pour retrouver les valeurs du pass MAP-ART01.

## Automation ciblée

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.MapART01"
```

Attendu : **3 tests**.

Régression art/read-model :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Map.MON21_6_11"
```

Attendu : **4 tests**.

Régression Map complète recommandée ensuite :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Map.MON21_6"
```

## Smoke PIE

Vérifier sur une zone comportant autant que possible murs, portes, escaliers, pit et relocation :

- murs plus organiques mais toujours lisibles ;
- marques de pierre discrètes, pas de surcharge ;
- porte fermée immédiatement identifiable ;
- porte ouverte immédiatement identifiable ;
- porte secrète découverte distincte ;
- porte secrète non découverte strictement identique à un mur ;
- Stairs Up / Down clairement différents ;
- pit visuellement plus profond et plus dangereux ;
- relocation/teleporter reconnaissable ;
- POI reconnaissable ;
- plusieurs symboles identiques présentent de légères variations stables ;
- zoom arrière reste lisible ;
- zoom avant ne révèle aucune géométrie hors clipping.

## Stop condition

MAP-ART01 est validé après :

1. build UE 5.5.4 vert ;
2. `Grimrock.UI.MapART01` vert ;
3. `Grimrock.Map.MON21_6_11` vert ;
4. idéalement la suite complète `Grimrock.Map.MON21_6` verte ;
5. validation visuelle PIE par l'utilisateur.
