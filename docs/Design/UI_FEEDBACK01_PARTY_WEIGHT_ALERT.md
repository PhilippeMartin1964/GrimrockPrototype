# UI-FEEDBACK01.1 — Indicateur de surcharge sur les portraits

Date : **22 septembre 2026**  
Statut : **C++ PRÊT — UMG/Automation À VALIDER**

## Objectif

Afficher un avertissement discret directement sur le portrait du personnage concerné lorsqu'il dépasse sa capacité de charge.

Le feedback ne duplique aucune règle de gameplay.

Autorité existante :

```text
FGridInventoryCharacterSummary::WeightState
    Normal
    Heavy
    Overloaded
```

UI-FEEDBACK01.1 utilise uniquement `Overloaded`.

## Contrat UMG

`WBP_PartyMember` peut maintenant exposer :

```text
Image_WeightAlert
```

Type exact :

```text
Image
Is Variable = Yes
```

Le C++ gère entièrement sa visibilité :

```text
Normal      -> Collapsed
Heavy       -> Collapsed
Overloaded  -> HitTestInvisible
```

Aucun Event Graph, aucun Switch Blueprint et aucun recalcul de poids ne sont nécessaires.

## Asset visuel

Réutiliser l'icône existante :

```text
Content/GrimrockPrototype/Blueprints/UI/Icons/T_Weight
```

Le choix de taille/position appartient au WBP. L'icône doit être un overlay discret du portrait et ne doit pas bloquer les clics ou le drag/drop.

## Test

```text
Grimrock.UI.Feedback01.PartyWeightAlert
```

Le test vérifie les transitions Normal -> Heavy -> Overloaded -> Normal.

## Hors scope

Le handicap gameplay de déplacement du groupe en surcharge reste un développement runtime séparé. UI-FEEDBACK01.1 n'ajoute aucune règle de mouvement.
