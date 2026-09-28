# MON21.6.11 — Hand-Drawn Parchment Artistic Pass

Date : **28 septembre 2026**  
Statut : **C++ IMPLÉMENTÉ — VALIDATION AUTOMATION + PIE VISUELLE UTILISATEUR REQUISES**

## 1. Objectif

MON21.6.11 applique l’identité visuelle dungeon-crawler/parchemin directement au renderer natif `UGridMapWidget`, sans créer de nouvelle autorité et sans dépendre d’un asset graphique qui n’existe pas encore dans le repository.

Le pipeline reste :

```text
FGridMapFloorView filtré
    -> caméra UI transient
    -> style artistique déterministe
    -> NativePaint()
```

## 2. Aucun asset parchemin imposé

L’audit du contenu courant ne trouve aucun asset Map/parchemin dédié.

MON21.6.11 fournit donc un fallback C++ autonome :

- teinte parchemin ;
- grain discret procédural ;
- lavis de cellule explorée ;
- hachures manuscrites ;
- feather léger aux frontières d’exploration ;
- murs/portes tracés en doubles traits irréguliers.

Une vraie texture de papier pourra être substituée plus tard sans modifier le read model.

## 3. Bruit artistique déterministe

`UGridMapWidget::ComputeDeterministicArtNoise(MapCell, Edge, Salt)` retourne une valeur stable dans `[-1,1]` à partir de :

```text
MapCell.X
MapCell.Y
Edge
Salt
```

Le même primitive produit donc exactement la même irrégularité à chaque Paint.

Interdiction respectée : **aucun random/jitter par frame**.

Le zoom et le pan déplacent/scalent le dessin mais ne changent pas sa signature de bruit.

## 4. Fond parchemin

Dans la zone définie par `MapDrawPadding`, le renderer dessine :

1. un fond `ParchmentColor` ;
2. `ParchmentGrainLineCount` fibres courtes ;
3. positions/longueurs/pentes issues du bruit déterministe.

Valeurs fonctionnelles par défaut :

```text
ParchmentGrainLineCount = 28
ParchmentColor           = brun clair / papier
ParchmentGrainColor      = brun sombre très transparent
```

Le grain reste volontairement discret pour ne pas nuire à la lecture de la grille.

## 5. Cellules explorées

Chaque cellule déjà présente dans `FGridMapFloorView.Cells` reçoit :

- un lavis léger `ExploredCellColor` ;
- jusqu’à `CellHatchLineCount` petites hachures ;
- hachures désactivées automatiquement lorsque la cellule devient trop petite à l’écran.

Les cellules inconnues restent absentes du read model et ne sont donc jamais dessinées.

## 6. Frontière d’exploration / feather

Le renderer construit un lookup temporaire des cellules visibles pendant le Paint.

Pour une arête dont la cellule voisine n’est pas visible, un trait très transparent `FogFeatherColor` adoucit la transition entre zone explorée et parchemin.

Ce feather est uniquement visuel : il ne constitue jamais la barrière de confidentialité de la géométrie.

Les murs/portes connus sont dessinés ensuite et restent prioritaires visuellement.

## 7. Traits manuscrits

Les murs, portes, portes secrètes découvertes et le marqueur du groupe utilisent `DrawHandDrawnLine()`.

Chaque primitive utilise :

- un trait principal légèrement cassé au milieu ;
- un second trait plus faible ;
- des offsets déterministes perpendiculaires à la ligne.

Paramètres :

```text
HandDrawnJitterPixels = 1.35
SecondaryStrokeAlpha  = 0.32
```

`bEnableParchmentStyle=false` permet de revenir au rendu fonctionnel simple sans modifier les données.

## 8. Portes et secrets

Une porte ordinaire reste graphiquement distincte d’un mur via son interruption centrale.

Une porte secrète **découverte** conserve son petit repère spécifique.

Un secret **non découvert** n’est jamais reçu comme `SecretDoor` par le renderer : MON21.6.6 le normalise en `Wall`, sans `ObjectId`, `DefinitionId` ni booléen secret.

Le test MON21.6.11 réaffirme explicitement cet invariant.

## 9. Couleurs par défaut

La palette fonctionnelle passe d’un style debug clair à une palette encre/parchemin :

```text
fond            -> brun papier
cellule explorée-> lavis brun léger
mur             -> encre brun très sombre
porte           -> encre brun sombre
secret découvert-> brun/orangé plus chaud
groupe          -> rouge sombre
```

Toutes ces couleurs restent éditables dans `Map|Rendering` / `Map|Art` sur `WBP_GridMap` si un ajustement visuel est nécessaire.

## 10. Validation MON21.6.10

Validation utilisateur du 28 septembre 2026 :

```text
Filter                  : Grimrock.Map.MON21_6_10
Succeeded               : 4
Succeeded with warnings : 0
Failed                  : 0
Not run                 : 0
Process exit code       : 0
Report                  : TD04-20260928-115317
```

Le smoke PIE est également confirmé **VALIDÉ** par l’utilisateur.

MON21.6.10 est **VALIDÉ**.

## 11. Automation MON21.6.11

Filtre :

```text
Grimrock.Map.MON21_6_11
```

Tests :

```text
ArtStyle.DeterministicNoise
ArtStyle.ParchmentDefaults
ArtStyle.PresentationOnlyContract
ArtStyle.HiddenSecretStillNormalWall
```

Aucun résultat MON21.6.11 n’est déclaré avant retour du harness UE5.5.4 local.

## 12. Validation PIE attendue

Aucun changement UMG n’est requis.

À vérifier visuellement en ouvrant `M` :

- fond parchemin visible uniquement dans la zone utile de la Map ;
- grain discret, pas envahissant ;
- cellules explorées en lavis/hachures ;
- murs légèrement irréguliers mais lisibles ;
- portes toujours immédiatement distinguables ;
- secret caché identique à un mur normal ;
- secret découvert toujours reconnaissable ;
- marqueur du groupe lisible ;
- zoom/pan/recentrage de MON21.6.10 toujours fonctionnels.

Si les anciens coloris sont conservés parce qu’ils ont été explicitement surchargés dans le Blueprint, utiliser `Reset to Default` sur les propriétés `Map|Rendering` concernées plutôt que recréer l’asset.

## 13. Hors périmètre

- symboles d’escaliers/téléporteurs/points d’intérêt : MON21.6.12 ;
- annotation libre joueur : future tranche distincte ;
- vraie texture papier dédiée : amélioration artistique optionnelle future ;
- regression/closure globale de la Map : MON21.6.13.

Prochaine tranche après validation : **MON21.6.12 — Map Symbols**.
