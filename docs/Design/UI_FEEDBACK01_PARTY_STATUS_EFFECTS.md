# UI-FEEDBACK01.2 — Effets de statut sur les portraits

Date : **22 septembre 2026**  
Statut : **C++ PRÊT — UMG/Automation À VALIDER**

## Objectif

Afficher sur chaque portrait du groupe une ligne compacte des effets de statut actifs sans créer de second système de statuts.

Autorité :

```text
FGridCharacterInventoryState::StatusEffects
    -> FGridStatusEffectPresentationBuilder
    -> UGridPartyMemberWidget::CachedStatusEffects
    -> HorizontalBox_StatusEffects
```

Le widget ne stocke qu'une projection transiente. Toute application, durée, stack et expiration restent autoritaires dans le système MON16.

## Présentation

Le portrait affiche jusqu'à **4 indicateurs** par défaut.

- si l'effet possède une icône : l'icône est affichée ;
- si aucune icône n'est configurée : un point `•` sert de fallback ;
- si plus de quatre effets sont actifs : la dernière case affiche `+N` ;
- les tooltips réutilisent directement `FGridStatusEffectPresentationView::ToolTipText`.

Le nombre maximal et la taille sont exposés sur `WBP_PartyMember` :

```text
MaxStatusEffectIndicators = 4
StatusEffectIndicatorSize = 24
```

## Contrat UMG

Dans :

```text
Content/GrimrockPrototype/Blueprints/UI/Inventory/WBP_PartyMember
```

ajouter sur l'Overlay du portrait :

```text
Horizontal Box
Name        = HorizontalBox_StatusEffects
Is Variable = Yes
Visibility  = Collapsed
```

Position recommandée : en bas du portrait, à proximité de `Image_WeightAlert`, sans recouvrir le visage.

Aucun enfant n'est à créer manuellement dans cette Horizontal Box : le C++ crée les indicateurs dynamiquement.

Aucun Event Graph n'est nécessaire.

## Rafraîchissement

MON16 appelle déjà `NotifyPartyInventoryChanged(CharacterIndex)` lorsque la présentation d'un statut joueur change. Les fenêtres Inventory/CharacterSheet sont donc rafraîchies par la notification existante.

Aucun nouveau delegate gameplay n'est ajouté.

## Tests

```text
Grimrock.UI.Feedback01.PartyWeightAlert
Grimrock.UI.Feedback01.PartyStatusIndicators
Grimrock.UI.Feedback01.PartyStatusProjection
```

Validation :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.Feedback01"
```

## Hors scope

- création des icônes définitives de chaque statut ;
- modification des règles MON16 ;
- affichage des statuts des monstres ;
- nouvelle fenêtre de détail des effets.
