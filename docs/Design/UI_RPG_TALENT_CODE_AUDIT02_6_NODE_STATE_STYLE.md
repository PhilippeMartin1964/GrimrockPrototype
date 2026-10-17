# UI-RPG-TALENT-CODE-AUDIT02.6 — Data-owned Talent node state styling

Date : 17 octobre 2026  
Statut : **IMPLÉMENTÉ — EN ATTENTE DE VALIDATION LOCALE**

## Constat

`GridTalentNodeWidget.cpp` contenait deux autorités visuelles codées en dur :

```text
NodeStateTextColor(EGridTalentNodeState)
NodeStateTint(EGridTalentNodeState)
```

Elles décidaient directement des RGB du texte d'état et de la luminance du
Border pour `Acquired`, `Available` et tous les états verrouillés.

La sémantique de l'état appartient bien au C++ ; les RGB/luminances n'y
appartiennent pas.

## Contrat retenu

Le read-model reste inchangé :

```text
FGridTalentNodeView::State
    EGridTalentNodeState
```

La présentation est déplacée dans les données de configuration :

```text
Config/DefaultGame.ini
    [/Script/GrimrockPrototype.RPGTalentNodeStyleSettings]
        AcquiredStyle
        AvailableStyle
        LockedLevelStyle
        LockedPrerequisiteStyle
        LockedPointsStyle
        LockedExclusiveStyle
```

Chaque entrée contient :

```text
FRPGTalentNodeStateVisualStyle
    TextColor
    NodeTint
```

## Runtime

`UGridTalentNodeWidget::ApplyNodePresentation()` :

1. conserve la résolution sémantique du libellé d'état ;
2. demande au CDO `URPGTalentNodeStyleSettings` le style de `NodeView.State` ;
3. applique les valeurs de données si le style est valide ;
4. sinon laisse le fallback Designer du WBP intact.

Le widget ne contient plus de RGB/luminance spécifiques à un état.

## Pourquoi DefaultGame.ini

Ce jalon doit être atomique et ne doit pas réécrire un `.uasset` UMG par
script. Un objet `Config=Game` permet :

- une autorité de données explicite ;
- aucune modification binaire ;
- conservation exacte du rendu runtime actuel ;
- évolution ultérieure des couleurs sans recompilation C++ ;
- maintien du fallback sombre déjà authoré dans `WBP_RPGTalentNode`.

Le WBP peut ultérieurement reprendre directement ces styles si une direction
artistique impose des bindings/animations plus riches ; le read-model n'aura
pas à changer.

## Périmètre inchangé

Aucun changement de :

- `EGridTalentNodeState` ;
- règles d'acquisition ;
- read-model Talent ;
- `DA_Class_*` ;
- `DA_ClassVisual_*` ;
- `DA_RPGTalentPresentation` ;
- SaveGame ;
- `.uasset`.

## Régressions 02.6

Le filtre `Grimrock.UI.RPG.CODEAUDIT02` ajoute :

```text
TalentNodeStateStyle.ConfiguredStyles
TalentNodeStateStyle.CppStyleBoundary
TalentNodeStateStyle.ConfigOwnership
```

Elles vérifient les six états configurés, l'absence des anciennes fonctions
C++ et la propriété des styles par `DefaultGame.ini`.
