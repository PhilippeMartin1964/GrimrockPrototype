# UGrimrockDesignSurfaceWidget

## Role

`UGrimrockDesignSurfaceWidget` est le standard commun pour les menus plein ecran et semi-plein ecran.

Il centralise le scaling lie au viewport, au DPI et aux marges physiques disponibles. Les widgets enfants travaillent dans une resolution logique stable et ne compensent jamais eux-memes le DPI ou la resolution.

La resolution logique de design reste portee par `SizeBox_DesignSurface`. Par defaut, elle est de `1920x1080`.

Les menus plein ecran doivent heriter directement ou indirectement de `UGrimrockDesignSurfaceWidget`.

Les popups internes doivent de preference etre placees dans un layer deja contenu dans `SizeBox_DesignSurface`, plutot que de recalculer leur propre scaling.

Les anciens noms `ScaleBox_MenuRoot` et `SizeBox_MenuDesign` sont temporairement supportes uniquement pour compatibilite. Les nouveaux widgets doivent utiliser `ScaleBox_DesignRoot` et `SizeBox_DesignSurface`.

## Hierarchie standard

```text
CanvasPanel_Root
-> ScaleBox_DesignRoot
   -> SizeBox_DesignSurface
      -> Overlay_UILayers ou contenu reel
```

La regle generale est :

```text
CanvasPanel_Root
-> ScaleBox_DesignRoot
   -> SizeBox_DesignSurface
      -> contenu reel du widget
```

`ScaleBox_DesignRoot` doit rester enfant direct de `CanvasPanel_Root`.
`SizeBox_DesignSurface` doit rester enfant direct de `ScaleBox_DesignRoot`.
Le contenu reel du widget doit etre place sous `SizeBox_DesignSurface`.

## Reglages standards

`ScaleBox_DesignRoot` :

```text
Stretch = Scale To Fit
Stretch Direction = Down Only
```

`SizeBox_DesignSurface` :

```text
Width Override = 1920
Height Override = 1080
```

`CanvasPanelSlot` de `ScaleBox_DesignRoot` :

```text
Anchors = Center
Alignment = 0.5 / 0.5
Position = 0 / 0
```

## A ne pas faire

- Ne pas mettre `ScaleBox_DesignRoot` sous une `SafeZone`.
- Ne pas mettre `ScaleBox_DesignRoot` sous un `Border`.
- Ne pas modifier `SizeBox_DesignSurface` selon le DPI.
- Ne pas creer de parametre local `InventorySlotSize`.
- Ne pas ajouter un second `ScaleBox` dans un enfant pour compenser la resolution.
