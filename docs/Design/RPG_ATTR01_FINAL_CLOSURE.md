# RPG-ATTR01 — Clôture finale Attribute Point Economy & Allocation

Date : **8 octobre 2026**  
État : **VALIDÉ / CLOS**

## Résultat

Les points de caractéristiques sont désormais réellement jouables aux niveaux :

```text
4 / 8 / 12 / 16 / 20
```

Chaque palier accorde +1 point, soit 5 points au niveau 20.

## Autorité

Aucun compteur `AttributePoints` n'est persisté.

```text
Granted   = floor(Level / 4)
Starting  = sum(Class.BaseAttributes) + sum(Race.AttributeBonuses)
Spent     = sum(Character.Attributes) - Starting
Remaining = Granted - Spent
```

`FGridCharacterInventoryState::Attributes` reste l'unique autorité durable.

## Mutation

Autorité métier :

```text
FRPGAttributePointService
```

Le service :

- valide le budget ;
- applique +1 sur une caractéristique ;
- respecte le plafond de base 20 ;
- recalcule les DerivedStats ;
- préserve les déficits courants de PV/Mana ;
- notifie le composant d'inventaire.

## Safe Undo

Le bouton − ne constitue pas un respec libre.

```text
ouverture PERSONNAGE
    -> capture du plancher

+
    -> attribution annulable dans cette session

fermeture / réouverture
    -> nouveau plancher
    -> anciennes attributions non remboursables
```

## UMG

`WBP_CharacterSheet` matérialise :

```text
Text_AttributePoints
Button_DecreaseStrength / Button_IncreaseStrength
Button_DecreaseDexterity / Button_IncreaseDexterity
Button_DecreaseConstitution / Button_IncreaseConstitution
Button_DecreaseIntelligence / Button_IncreaseIntelligence
Button_DecreaseWisdom / Button_IncreaseWisdom
Button_DecreaseCharisma / Button_IncreaseCharisma
```

Les contrôles utilisent des icônes −/+ et aucun Event Graph métier parallèle.

## Cleanup

L'audit RPG-ATTR01.3 confirme :

```text
monnaie persistée parallèle                 aucune
mutation runtime concurrente Attributes     aucune
écritures création/recrutement              légitimes
budget AttributePoints du wizard            création uniquement
second delegate CharacterSheet              supprimé
```

Le Character Sheet étend désormais l'unique chemin canonique :

```text
OnPartyInventoryChanged
    -> UGridInventoryWidget::RefreshInventory()
    -> UGridCharacterSheetWidget::RefreshInventory()
       -> RefreshAttributeAllocationPresentation()
```

## SaveGame

Aucun champ durable ajouté.

```text
CurrentSaveVersion = 24
```

La validation du Save vérifie la cohérence du budget de caractéristiques
avec Level + Class + Race + Attributes.

## Validation

Source initiale :

```text
Grimrock.RPG.ATTR01
9 / 9
0 warning
0 fail
```

Après cleanup RPG-ATTR01.3 :

```text
Grimrock.RPG.ATTR01
10 / 10
0 warning
0 fail
0 not run
exit code 0
```

PIE validé :

- activation correcte des boutons aux paliers ;
- achat et remboursement ;
- Safe Undo après fermeture/réouverture ;
- recalcul des stats dérivées ;
- toast Level Up avec +1 point de caractéristique ;
- changement de personnage avec la feuille ouverte ;
- mise à jour immédiate du solde et des états −/+.

## Jalons

```text
RPG-ATTR01.1   CLOS — économie et transactions C++
RPG-ATTR01.2   CLOS — matérialisation WBP_CharacterSheet
RPG-ATTR01.3   CLOS — audit et refresh canonique
RPG-ATTR01     CLOS
```
