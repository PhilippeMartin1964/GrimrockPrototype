# MAP-UI02 — Map Title Bar & Close Button

Date : 29 septembre 2026  
Statut : **C++ IMPLÉMENTÉ — CONFIGURATION UMG + VALIDATION LOCALE/PIE REQUISES**

## Prérequis

MAP-UI01 est validé par l'utilisateur :

- `Grimrock.UI.MapUI01` : 2/2, 0 warning, 0 échec — `TD04-20260929-085148` ;
- `Grimrock.UI.Navigation01` : 2/2, 0 warning, 0 échec — `TD04-20260929-085832` ;
- `Grimrock.UI.GlobalHud01` : 2/2, 0 warning, 0 échec — `TD04-20260929-085851` ;
- smoke PIE confirmé.

## Objectif

Ajouter à la fenêtre autonome `WBP_GridMap` un bandeau supérieur cohérent avec les surfaces d'inventaire, avec un bouton `X` qui ferme la carte proprement.

Le ticket ne déplace pas encore les contrôles d'étage et ne modifie pas le rendu des symboles.

## Contrat C++

`UGridMapWidget` expose maintenant :

```text
Button_CloseMap : UButton (BindWidgetOptional)
```

`AGrimrockPartyPawn` :

- branche le bouton à `HandleMapWindowCloseClicked()` lors de la création de la carte ;
- ferme `M`, `ESC` et le bouton `X` par le même chemin Map ;
- restaure l'input gameplay via le chemin Major UI existant ;
- ne déclenche plus l'auto-save "InventoryClose" lorsqu'on ferme uniquement la carte.

Aucune autorité Map, aucune donnée durable et aucune version de SaveGame ne changent.

## Modification UMG manuelle requise

Dans `WBP_GridMap`, créer un bandeau dans la marge haute déjà réservée par :

```text
MapDrawPadding.Top = 72
```

Hiérarchie recommandée, sans imposer la structure exacte du root existant :

```text
Overlay / Canvas existant
└── Border_TitleBar
    └── HorizontalBox_TitleBar
        ├── Spacer gauche
        ├── Text_MapTitle ("Carte")
        ├── Spacer extensible
        └── Button_CloseMap
            └── Text / Image "X"
```

Réglages recommandés :

- hauteur du bandeau : 48 à 56 px ;
- marge gauche/droite : environ 16 px ;
- `Text_MapTitle` centré verticalement ;
- `Button_CloseMap` aligné à droite ;
- réutiliser le style du bouton de fermeture de l'inventaire ;
- nom exact obligatoire pour le bouton : **`Button_CloseMap`** ;
- aucun Blueprint Event Graph n'est nécessaire pour le clic : le binding est natif.

Le bandeau doit rester au-dessus du rendu de la carte et dans la zone de padding, afin de ne pas réduire artificiellement la zone de dessin.

## Automation ciblée

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.MapUI02"
```

Attendu : **2 tests**.

Régression recommandée :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.MapUI01"
```

puis :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Map.MON21_6"
```

## Smoke PIE

- `M` ouvre la carte ;
- bandeau visible et cohérent avec l'inventaire ;
- clic `X` ferme la carte ;
- `M` ferme toujours la carte ;
- `ESC` ferme toujours la carte avant le menu principal ;
- après fermeture, curseur/input reviennent au gameplay ;
- réouverture par `M` fonctionne ;
- zoom, pan, recentrage et navigation d'étage inchangés.

## Stop condition

MAP-UI02 est validé après :

1. build UE 5.5.4 vert ;
2. `Grimrock.UI.MapUI02` vert ;
3. UMG configuré avec `Button_CloseMap` ;
4. smoke PIE confirmé.
