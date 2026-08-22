# MON18.9.3 — Final Diagnostics / Global Regression / Closure

## Statut

**VALIDÉ ET CLOS sous UE5.5.4 — 22 août 2026.**

Base d'implémentation :

```text
07e655707bca61007de847bc6504fdff9022b589
Implement MON18.9.3 final diagnostics
```

## 1. Objectif

MON18.9.3 constitue la dernière passe de stabilisation de `MON18 — Magic & Spellbook`.

Aucun nouveau gameplay n'est introduit. Le sous-jalon :

- rend les diagnostics de slots SaveGame résiduels attribuables à un slot précis ;
- garantit que le checkpoint pré-combat `_AutoCombat` ne devient pas un slot manuel normal ;
- valide la non-régression globale du projet ;
- confirme en PIE le flux `Continue -> engagement automatique -> checkpoint -> refus de sauvegarde en combat`.

## 2. Diagnostic des anciens slots

Le menu principal sonde :

```text
GrimrockParty
GrimrockParty_2
GrimrockParty_3
```

Lorsqu'un slot existant n'est pas chargeable, `UGrimrockGameInstance::HasPartySaveGame()` ajoute désormais un diagnostic stable :

```text
[MON18.9.3] SlotProbe Slot=<nom> UserIndex=<index> Result=Rejected Reason=<raison>
```

Raisons distinguées :

```text
LoadFailedOrWrongClass
IncompatibleSave
PartyInventoryStateNotLoadable
```

Aucun ancien slot n'est supprimé ou réécrit automatiquement.

### Validation PIE finale

Le log fourni le 22 août 2026 identifie sans ambiguïté le slot auxiliaire incompatible :

```text
[MON18.9.3] SlotProbe Slot=GrimrockParty_2 UserIndex=0 Result=Rejected Reason=IncompatibleSave Version=6 Detail=Le snapshot contient 0 états de progression pour 1 personnages actifs.
```

Le bruit de validation observé depuis MON18.8 est donc expliqué : `GrimrockParty_2` est un ancien snapshot incompatible ; la sauvegarde principale n'est pas en cause.

## 3. Isolation du checkpoint pré-combat

Le checkpoint MON18.9.1 reste :

```text
<slot courant>_AutoCombat
```

Il n'est pas ajouté à `ConfiguredPartySaveSlotNames` et n'apparaît donc pas comme une sauvegarde manuelle normale dans le menu de chargement.

## 4. Automation ciblée

Namespace :

```text
Grimrock.Magic.MON18.9.3
```

Résultats fournis après reconstruction complète de `GrimrockPrototypeEditor` :

```text
Grimrock.Magic.MON18.9.3.CheckpointIsolation   Success
Grimrock.Magic.MON18.9.3.SaveSlotDiagnostics   Success
```

Bilan : **2/2 Success**.

## 5. Campagne globale

Commande :

```text
Automation RunTests Grimrock
```

Le log complet fourni contient :

```text
221 tests terminés
221 Success
0 Fail
```

Aucune occurrence d'`Ensure condition failed`, `Assertion failed` ou erreur fatale n'a été relevée.

Les warnings rencontrés appartiennent aux fixtures de tests volontairement dégradées : absence de composant de mouvement, RuntimeActor manquant, définition visuelle incomplète, classe invalide testée, etc. Les tests correspondants terminent en `Success` et ne constituent pas une régression production.

La campagne globale avait été exécutée avant le rebuild qui a enregistré le nouveau fichier MON18.9.3 ; les deux tests MON18.9.3 ont donc été exécutés séparément ensuite. L'ensemble de la preuve finale est :

```text
Campagne globale Grimrock     221/221 Success
MON18.9.3 ciblé                 2/2 Success
```

## 6. Validation PIE finale

Le log final fourni confirme :

### Continue

```text
GrimrockGameInstance PendingLoadSlot Set Slot=GrimrockParty UserIndex=0
PartySave Continued Slot=GrimrockParty CharacterCount=1
```

Le slot principal reste chargeable malgré l'ancien `GrimrockParty_2` incompatible.

### Checkpoint pré-combat

Juste avant l'engagement :

```text
PartySave Saved Slot=GrimrockParty_AutoCombat Version=6 Characters=1 Spellbooks=0 Cell=(29,22) Facing=1
[MON18.9.1] PreCombatCheckpoint Saved Slot=GrimrockParty_AutoCombat SourceSlot=GrimrockParty Cell=(29,22)
```

Puis :

```text
[MON14.1] Automatic combat started ... Reason=PatrolVision ... Checkpoint=Saved
```

Le checkpoint est donc créé avant l'entrée effective en combat.

### Refus de sauvegarde pendant le combat

À la fermeture PIE en combat :

```text
PartySave SaveRejected Slot=GrimrockParty Reason=CombatStateNotSaveable
PartySave EndPlay Failed Slot=GrimrockParty Reason=La sauvegarde est interdite pendant un combat ou après une défaite.
```

Le slot principal n'est pas écrasé par un état transitoire de combat.

## 7. Portée de la preuve Spellbook

Le dernier log PIE utilise une sauvegarde où `SpellbookCharacters=0` et ne rejoue pas les casts de production. Il n'est donc pas utilisé comme preuve nouvelle de `Arcane Bolt`, `Lesser Heal`, `Haste`, `Cure Poison` ou du round-trip Spellbook.

Cette preuve existe déjà et reste valide dans les étapes précédentes :

- UI01.4.3e.2 : **6/6 Success** avec exécution `ArcaneBolt` et `LesserHeal` ;
- MON18.8 : **12/12 Success** ;
- PIE MON18.8 : `SeedProduction Added=4`, Save v6 avec `Spellbooks=1`, puis Stop PIE -> Continue -> `Added=0 AlreadyKnown=4` ;
- MON18.9.2 : **5/5 Success** plus **51/51** régressions cross-system.

La clôture finale repose donc sur l'ensemble cumulatif de ces validations et non sur une interprétation abusive du dernier log.

## 8. Conclusion

Les critères de clôture sont satisfaits :

```text
Grimrock.Magic.MON18.9.3     2/2 Success
Automation RunTests Grimrock 221/221 Success
PIE diagnostics/Continue     VALIDÉ
PIE checkpoint/combat-save   VALIDÉ
Spellbook/casts/persistence  déjà VALIDÉS dans MON18.7 / MON18.8 / MON18.9.2
```

**MON18.9.3 est VALIDÉ ET CLOS sous UE5.5.4.**

Cette validation autorise la clôture du jalon majeur **MON18 — Magic & Spellbook**.
