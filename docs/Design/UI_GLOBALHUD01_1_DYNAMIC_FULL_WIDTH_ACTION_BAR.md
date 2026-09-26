# UI-GLOBALHUD01.1 — Dynamic Full-Width Action Bar

Date : **26 septembre 2026**  
Statut : **C++ prêt pour validation UE5.5.4**

## Décision

La barre générale d'actions n'a plus de nombre visuel fixe de slots.

Les douze premiers slots conservent les raccourcis du clavier suisse :

```text
1 2 3 4 5 6 7 8 9 0 ' ^
```

Tous les slots suivants restent disponibles à la souris.

## Règle de largeur

Les slots gardent leur largeur UMG réelle et restent jointifs.

```text
VisibleSlotCount =
    floor((ViewportWidth - NavigationWidth) / ActionSlotWidth)
```

avec un minimum de 12 slots.

Les enfants du `HorizontalBox` utilisent :

```text
Size rule = Auto
Padding   = 0
```

Aucun `Fill` n'est appliqué aux slots.

Le reliquat inférieur à la largeur d'un slot reste volontairement vide à droite de la barre. Il n'est ni redistribué entre les slots ni converti en espacement.

## Mesures runtime

`UGridPersistentHudWidget` utilise :

- la largeur logique du viewport ;
- la largeur désirée de `Panel_GlobalNavigation` ;
- la largeur désirée du premier `UGridCombatHudActionWidget`.

`FallbackActionSlotWidth` n'est utilisé qu'avant que le widget d'action puisse fournir sa largeur désirée.

Le recalcul n'effectue une reconstruction de la rangée que lorsqu'une mesure significative change : viewport, navigation, largeur de slot, personnage actif ou dernier slot réellement utilisé.

## Stockage

`FGridCombatHotbarBinding::MinimumSlotCount = 12`.

Le tableau `CombatHotbarSlots` devient extensible :

- une création de personnage initialise 12 slots ;
- une sauvegarde plus large conserve tous ses slots ;
- `EnsureCharacterCombatHotbarCapacity()` agrandit le tableau si l'écran peut afficher davantage de slots ;
- aucune réduction implicite n'est effectuée.

Le nombre visible peut donc diminuer lorsqu'une fenêtre devient plus petite sans détruire les bindings persistants.

Si un binding existant se trouve au-delà de la capacité normalement calculée pour la largeur actuelle, le Persistent HUD conserve suffisamment de slots visibles pour que ce binding reste accessible.

## Compatibilité

Le symbole historique `FGridCombatHotbarBinding::SlotCount` reste temporairement un alias de `MinimumSlotCount` pour limiter le bruit de migration. Il ne représente plus la largeur runtime de la barre.

Les méthodes de binding utilisent désormais la longueur réelle du tableau du personnage.

## UMG

Aucun nouveau réglage destructif n'est demandé dans `WBP_GridPersistentHud`.

En particulier :

- conserver le `Size To Content` qui permet au HUD actuel d'être visible ;
- ne pas forcer `Panel_ActionBar` ou ses slots en `Fill` ;
- garder `Panel_ActionBar` comme `HorizontalBox`.

Le C++ ajoute les slots de gauche à droite, avec largeur automatique et padding nul.

## Validation

Filtres principaux :

```text
Grimrock.UI.GlobalHud01
Grimrock.UI.Navigation01
Grimrock.Monsters.MON12.CombatHUD
Grimrock.Monsters.MON12.8.3
```

PIE :

1. aucun espace entre deux slots ;
2. le nombre de slots augmente jusqu'au dernier slot entier qui tient dans le viewport ;
3. un reliquat éventuel reste vide à droite ;
4. les slots 1..12 affichent `1..0, ', ^` ;
5. les suivants n'affichent aucun raccourci clavier ;
6. un redimensionnement de fenêtre adapte le nombre visible sans perdre les bindings ;
7. le changement de personnage adapte sa capacité sans tronquer celle des autres.
