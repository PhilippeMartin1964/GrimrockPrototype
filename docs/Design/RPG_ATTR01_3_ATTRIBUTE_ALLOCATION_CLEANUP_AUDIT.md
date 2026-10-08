# RPG-ATTR01.3 — Attribute Allocation Cleanup Audit

Date : **8 octobre 2026**  
État : **VALIDÉ / CLOS — 10/10 Automation, PIE final validé**

## Autorité

Autorité durable unique :

```text
FGridCharacterInventoryState::Attributes
```

Autorité de mutation joueur :

```text
FRPGAttributePointService
```

Aucune monnaie de progression Attribute Point n'est persistée.

## Écritures de Attributes

Les écritures hors `FRPGAttributePointService` restantes correspondent à la
construction initiale d'un personnage :

```text
GridPartyInventoryComponent        création initiale
RPGCustomRecruitService            recrue personnalisée
RPGStoryCompanionService           compagnon scénarisé
```

Elles ne constituent pas une autorité concurrente de progression.

Le wizard possède son propre vocabulaire `AttributePoints`, mais il s'agit du
budget de redistribution **à la création**. Il n'est ni sauvegardé comme monnaie
de progression ni utilisé aux niveaux 4/8/12/16/20.

## Cleanup UI

Le second abonnement Character Sheet à `OnPartyInventoryChanged` est supprimé.

Avant :

```text
UGridInventoryWidget
    -> OnPartyInventoryChanged
    -> RefreshInventory()

UGridCharacterSheetWidget
    -> tentative d'un second binding dans NativeConstruct()
    -> HandleAttributeInventoryChanged()
```

Après :

```text
UGridInventoryWidget::RefreshInventory() [virtual]
    -> UGridCharacterSheetWidget::RefreshInventory()
       -> Super
       -> RefreshAttributeAllocationPresentation()
```

Il n'existe donc plus qu'un seul chemin de refresh de l'inventaire/feuille.

## Validation

Filtre :

```text
Grimrock.RPG.ATTR01
```

Attendu après RPG-ATTR01.3 : **10 tests**.

Le dixième test verrouille l'absence du delegate parallèle et la présence du
refresh polymorphique canonique.

Validation finale obtenue :

```text
Grimrock.RPG.ATTR01
Succeeded              : 10
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
```

PIE final validé : changement de personnage avec PERSONNAGE ouvert,
rafraîchissement immédiat des valeurs, du solde et des états −/+.

RPG-ATTR01.3 et le parent RPG-ATTR01 sont **CLOS**.
