# MON20.5.3 — Wizard Context Reuse

Statut : **VALIDÉ UE5.5.4 — 16/16 AUTOMATION TESTS SUCCESS**  
Date : **24 août 2026**  
Jalon parent : **MON20.5 — Custom Recruit / Wizard Context Reuse**

---

## 1. Objectif

Rendre le wizard de création existant réellement réutilisable pour une recrue personnalisée, sans créer un second WBP et sans déplacer l’autorité du groupe hors de `UGridPartyInventoryComponent`.

Le même widget supporte désormais :

```text
ERPGCharacterCreationContext::NewGameMainHero
ERPGCharacterCreationContext::CustomRecruit
```

Le flux devient :

```text
WBP_CharacterCreationWizard
    -> URPGCharacterCreationWizardWidget
        -> NewGameMainHero
            -> UGridPartyInventoryComponent::CreateInitialCharacter
        -> CustomRecruit
            -> FRPGCustomRecruitService::TryCreateAndRecruit
                -> CharacterPool temporaire
                -> MON20.2 TryRecruitFromPool
                -> ActiveCharacters
```

---

## 2. Un seul widget

Aucun nouveau Blueprint n’est introduit.

`URPGCharacterCreationWidget` expose maintenant :

```cpp
InitializeCharacterCreationWidget(...)
InitializeCharacterCreationWidgetForContext(...)
GetCreationContext()
```

`InitializeCharacterCreationWidget(...)` conserve le contrat historique et sélectionne automatiquement :

```cpp
ERPGCharacterCreationContext::NewGameMainHero
```

Le WBP existant reste donc compatible avec le flux New Game actuel.

---

## 3. Contexte transitoire

Le widget possède :

```cpp
ERPGCharacterCreationContext CreationContext
```

Ce champ décrit uniquement le but de l’instance UI courante.

Il n’est pas ajouté à :

```text
FGridCharacterInventoryState
FGridPartyInventoryState
SaveGame
```

Aucune migration n’est nécessaire.

---

## 4. Garde de l’état du groupe

`CanSubmitCharacterCreation()` n’interprète plus systématiquement une création initiale terminée comme une interdiction absolue.

### New Game

Le submit est autorisé seulement si :

```text
bInitialCharacterCreationCompleted == false
```

### Custom Recruit

Le submit est autorisé seulement si :

```text
création initiale terminée
au moins un personnage actif
ActiveCharacters / ActiveEquipment alignés
SelectedCharacterIndex valide
MaxActiveCharacters cohérent
place active disponible
```

Cette garde UI reste une première défense. La transaction MON20.5.2 refait les validations autoritaires avant mutation.

---

## 5. Preview et édition

Le précédent early-return de `RefreshPreview()` lorsque la création initiale était terminée empêchait tout usage du wizard après le New Game.

MON20.5.3 le supprime.

Le preview continue maintenant à fonctionner en `CustomRecruit` avec :

- race ;
- classe ;
- attributs ;
- stats dérivées ;
- genre ;
- portrait ;
- résumé final.

Le bouton final reste désactivé lorsque le contexte n’est pas autorisé, notamment lorsque le groupe est plein.

---

## 6. Submit contextuel

### NewGameMainHero

Le comportement existant reste :

```text
CreateInitialCharacter
SetCharacterVisualSelection(0)
HandleInitialCharacterCreated
```

### CustomRecruit

Le wizard construit toujours la même `FRPGCharacterCreationRequest`, y compris la copie transitoire de classe contenant l’allocation d’attributs.

Puis :

```cpp
FRPGCustomRecruitService::TryCreateAndRecruit(...)
```

La classe persistée par la recrue reste l’asset canonique fourni dans :

```cpp
CombatActionSourceClassDefinition
```

Le wizard ne modifie jamais directement `ActiveCharacters`.

---

## 7. Commit / Cancel natifs

Le widget expose deux delegates C++ natifs :

```text
OnCustomRecruitCommitted
OnCustomRecruitCancelled
```

Ils constituent le seam prévu pour MON20.5.4.

Après un recrutement réussi :

```text
OnCustomRecruitCommitted(Widget, CharacterIndex)
```

Lors d’un `CancelWizard()` en contexte `CustomRecruit` :

```text
OnCustomRecruitCancelled(Widget)
RemoveFromParent()
```

Important : contrairement au New Game, l’annulation d’une recrue personnalisée **ne demande pas un retour au Main Menu**.

Le flux New Game conserve son comportement historique `RequestReturnToMainMenu()`.

---

## 8. Limite volontaire de MON20.5.3

Cette tranche ne branche pas encore le wizard au Pawn runtime.

Elle n’ajoute donc pas encore :

- `ShowCustomRecruitCharacterCreationWidget()` sur `AGrimrockPartyPawn` ;
- gestion du focus/input du modal CustomRecruit ;
- restauration du gameplay après Cancel/Commit ;
- Event -> Command du recruteur ;
- objet Grid Editor de recrutement ;
- coût en or ;
- dialogue d’auberge/guilde ;
- nouveau WBP.

Ces responsabilités appartiennent à MON20.5.4 et suivantes.

---

## 9. Automation Tests

Le filtre reste :

```text
Grimrock.MON20.5.CustomRecruit
```

MON20.5.3 ajoute :

```text
ContextGateCompletedParty
ContextGateIncompleteParty
ContextGatePartyFull
WizardContextDefault
WizardCustomRecruitSubmit
WizardCustomCancelNoMutation
WizardNewGameSubmitRegression
```

Les 9 tests MON20.5.2 restent présents.

Validation réalisée dans UE5.5.4 le 24 août 2026 :

```text
16 / 16 Success
0 Fail
```

Le journal utilisateur confirme notamment :

```text
WizardCustomCancelNoMutation  -> Success
WizardCustomRecruitSubmit     -> Success
WizardNewGameSubmitRegression -> Success
```

Le submit CustomRecruit produit bien :

```text
CharacterCreationWizard RecruitedCustomCharacter Name=Ariane Race=Human Class=Warrior CharacterIndex=1
```

et la non-régression New Game produit toujours :

```text
CharacterCreationWizard CreatedCharacter Name=Héros Race=Human Class=Warrior
```

MON20.5.3 est donc validé sous l’éditeur cible.

---

## 10. Suite

```text
MON20.5.4 — Custom Recruit Modal Runtime Integration
```

Cette tranche branche les delegates de MON20.5.3 sur `AGrimrockPartyPawn`, réutilise la garde d’input existante et permet d’ouvrir/fermer le même WBP pendant l’aventure.
