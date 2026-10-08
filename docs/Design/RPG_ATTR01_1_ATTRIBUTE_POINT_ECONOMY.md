# RPG-ATTR01.1 — Attribute Point Economy & Allocation — source

Date : **8 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
État : **RPG-ATTR01.1 VALIDÉ 9/9 ; RPG-ATTR01.2 matérialisé/PIE validé ; RPG-ATTR01.3 cleanup source à revalider**

## Objectif

Matérialiser la progression universelle des caractéristiques prévue aux niveaux :

```text
4 / 8 / 12 / 16 / 20
```

Chaque palier accorde **+1 point de caractéristique**, soit **5 points** au niveau 20.

La valeur de base normale d'une caractéristique reste plafonnée à **20**.

## Autorité durable

Aucun compteur `AttributePoints` n'est ajouté au SaveGame.

`FGridCharacterInventoryState::Attributes` reste l'unique autorité durable.

Le solde est entièrement dérivé :

```text
Granted   = floor(Level / 4)

Starting  = somme(Class.BaseAttributes)
          + somme(Race.AttributeBonuses)

Spent     = somme(Character.Attributes) - Starting

Remaining = Granted - Spent
```

Les bonus d'équipement ne participent jamais à ce calcul : ils restent projetés
séparément dans `FGridInventoryCharacterSummary::EquipmentStatBonus`.

## Pourquoi la dérivation est possible

Le wizard de création peut redistribuer les caractéristiques de classe, mais il
redistribue un **budget total fixe**. La distribution initiale peut donc varier
sans changer `Starting`.

Exemple :

```text
profil canonique : 12 / 12 / 13 / 12 / 12 / 12
redistribution    : 13 / 11 / 13 / 12 / 12 / 12

même somme
-> Spent progression = 0
```

Une augmentation gagnée plus tard augmente réellement la somme et devient donc
un point dépensé reconstructible.

## Service C++

Nouvelle autorité métier :

```text
FRPGAttributePointService
```

API :

```text
GetTotalPointsGranted(Level)
TryGetBalance(Character)
GetAttributeValue(Character, Target)
GetIncreaseAvailability(Character, Target)
TryPurchasePoint(...)
TryRefundPurchasedPoint(... SessionFloorValue ...)
```

Cibles :

```text
Strength
Dexterity
Constitution
Intelligence
Wisdom
Charisma
```

## Transaction

`TryPurchasePoint()` :

1. valide Inventory / Character / Level ;
2. résout les définitions canoniques Class + Race ;
3. reconstruit le solde ;
4. refuse si aucun point ne reste ;
5. refuse une valeur de base >= 20 ;
6. augmente exactement une caractéristique de +1 ;
7. recalcule `DerivedStats` ;
8. préserve les déficits actuels de PV/Mana ;
9. notifie `OnPartyInventoryChanged`.

Aucun UMG n'écrit directement dans `Character.Attributes`.

## Safe Undo

Comme RPG-SKILL01, le bouton `−` n'est pas un respec permanent.

```text
ouverture PERSONNAGE
    -> capture du plancher de session

+
    -> valeur +1
    -> point consommé
    -> cette augmentation devient annulable

-
    -> possible uniquement jusqu'au plancher de session

fermer / rouvrir PERSONNAGE
    -> nouveau plancher
    -> les anciennes attributions ne sont plus annulables
```

Les planchers sont transitoires dans `UGridCharacterSheetWidget` et ne sont
jamais persistés.

## Character Sheet

`UGridCharacterSheetWidget` expose en option :

```text
Text_AttributePoints

Button_DecreaseStrength
Button_IncreaseStrength

Button_DecreaseDexterity
Button_IncreaseDexterity

Button_DecreaseConstitution
Button_IncreaseConstitution

Button_DecreaseIntelligence
Button_IncreaseIntelligence

Button_DecreaseWisdom
Button_IncreaseWisdom

Button_DecreaseCharisma
Button_IncreaseCharisma
```

Le C++ possède les bindings de clics. Aucun Event Graph Blueprint n'est requis.

`AGrimrockPartyPawn::ShowInventoryWorkspace()` appelle
`BeginAttributeAllocationSession()` à chaque réouverture du workspace.

## Level Up toast

Le toast non modal RPG-LEVELUX01 annonce désormais également les points de
caractéristiques gagnés.

Exemple niveau 3 -> 4 :

```text
Niveau 4 atteint

Elias passe du niveau 3 au niveau 4.
+1 point de compétence.
+1 point de talent.
+1 point de caractéristique.
```

## SaveGame

Aucun nouveau champ durable n'est ajouté.

Le schéma reste :

```text
CurrentSaveVersion = 24
```

La validation du Save vérifie désormais que le total des caractéristiques est
compatible avec le niveau et avec le total initial Class + Race.

## RPG-ATTR01.2 — matérialisation UMG

Après validation de la source, `WBP_CharacterSheet` recevra :

- une ligne de solde `Text_AttributePoints` ;
- un bouton `−` et un bouton `+` pour chacune des six caractéristiques ;
- aucun graphe Blueprint ;
- aucune monnaie stockée dans le WBP.

Les réglages UMG précis seront donnés après validation du filtre source.

## Automation

Filtre :

```text
Grimrock.RPG.ATTR01
```

Attendu : **9 tests**.

Couverture :

```text
Economy.GrantSchedule
Economy.DerivedBalance
Mutation.Purchase
Mutation.Cap20
Undo.RefundToSessionFloor
Undo.RejectBelowSessionFloor
UI.CharacterSheetContract
Feedback.LevelFour
Authority.NoPersistedCurrency
```

Commande :

```powershell
cd D:\Development\GrimrockPrototype

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.RPG.ATTR01"
```

RPG-ATTR01 ne sera clos qu'après matérialisation UMG et validation PIE.


## RPG-ATTR01.3 — audit / cleanup

Audit de clôture effectué après matérialisation UMG.

Constats :

```text
monnaie AttributePoints persistée              aucune
SpentAttributePoints persistant                aucun
AttributePointBalance persistant               aucun
mutation runtime concurrente Character.Attributes aucune
écritures initiales création/recrutement       légitimes
budget AttributePoints du wizard               création initiale uniquement
```

Le seul résidu concret trouvé était un second abonnement
`OnPartyInventoryChanged` dans `UGridCharacterSheetWidget`. Il était tenté
dans `NativeConstruct()`, avant l'injection de `InventoryComponent` par
`InitializeInventoryWidget()`.

RPG-ATTR01.3 le supprime et fait de `RefreshInventory()` l'unique route de
refresh :

```text
UGridPartyInventoryComponent::OnPartyInventoryChanged
    -> UGridInventoryWidget::HandlePartyInventoryChanged
    -> virtual RefreshInventory()
    -> UGridCharacterSheetWidget::RefreshInventory()
       -> Super::RefreshInventory()
       -> RefreshAttributeAllocationPresentation()
```

Cela couvre aussi le changement de personnage pendant que PERSONNAGE reste
ouvert, sans ajouter de delegate parallèle.
