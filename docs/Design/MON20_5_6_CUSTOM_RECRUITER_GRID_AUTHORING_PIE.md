# MON20.5.6 — Custom Recruiter Grid Authoring & PIE

Statut : **AUTOMATION 23/23 VALIDÉE — PIE POST-HARDENING À REVALIDER**  
Date : **24 août 2026**  
Jalon parent : **MON20.5 — Custom Recruit / Wizard Context Reuse**

---

## 1. Objectif

Valider le flux complet en conditions réelles d'authoring :

```text
Trigger.Activated
    -> CustomRecruiter.OpenCustomRecruit
    -> AGrimrockPartyPawn::ShowCustomRecruitCharacterCreationWidget()
    -> WBP_CharacterCreationWizard existant
    -> Annuler ou Engager
```

Cette tranche crée les assets d'authoring, place le target dans le `UGridLevelAsset`, crée le connector et valide le comportement PIE.

---

## 2. Préconditions validées

```text
MON20.5.2  Custom Recruit Transaction          9/9
MON20.5.3  Wizard Context Reuse               16/16 cumulés
MON20.5.4  Runtime Modal Integration           18/18 cumulés
MON20.5.5  Event / Command Authoring Bridge    22/22 cumulés
MON20.5.6  Runtime hardening                   23/23 cumulés
```

Filtre :

```text
Grimrock.MON20.5.CustomRecruit
```

Résultat après hardening MON20.5.6, validé le 24 août 2026 :

```text
23 / 23 Success
0 Fail
0 Error
```

Le test négatif `EventCommandMissingPlayerPawn` produit volontairement son warning attendu puis termine en `Success`.

Le nouveau test `RuntimeCombatGate` confirme :

```text
[GridCustomRecruitRuntime] Show Rejected ... Reason=CombatActive Phase=2 Round=3
```

et termine lui aussi en `Success`.

---

## 3. Archetype et palette

Archetype de référence :

```text
DA_Archetype_CustomRecruiter_Service
Archetype Id              = CustomRecruiter_Service
Display Name              = Custom Recruiter
Gameplay Type             = CustomRecruiter
Functional Category       = Decoration
Default Initially Enabled = true
Default Initially Active  = false
Palette Category          = Recruitment
Placement Kind            = Center
Can Share Cell            = true
Can Share Anchor          = true
Blocks Movement           = false
Runtime Interactable      = false
Runtime Readable          = false
Runtime Light Source      = false
Runtime Actor Class       = None
Item Actor Class          = None
```

Entrée de palette :

```text
Entry Id                            = CustomRecruiter_Service
Display Name Override               = Custom Recruiter
Category Override                   = Recruitment
Default Archetype                   = DA_Archetype_CustomRecruiter_Service
Default Monster Definition          = None
Default Story Companion Definition  = None
```

Le target est data-only et n'a pas besoin de RuntimeActorClass.

---

## 4. Connector validé

Le connector authoré dans le Grid Editor est :

```text
Source Object = Trigger placé
Event         = Activated
Target Object = Custom Recruiter placé
Command       = Open Custom Recruit
Condition     = None
```

Aucun Blueprint Graph n'est nécessaire.

---

## 5. PIE validé — Annuler

Le 24 août 2026, le scénario réel a été exécuté avec succès :

```text
Trigger Activated
-> OpenCustomRecruit Success=true
-> WBP_CharacterCreationWizard Context=CustomRecruit
-> Annuler
-> retour au jeu
-> groupe inchangé
-> input restauré
```

Les logs observés confirment notamment :

```text
[GridCustomRecruitRuntime] Shown ... Context=CustomRecruit
CharacterCreationWizard Cancelled ... Context=1
GridInventory UI State Open=false
Viewport MouseCaptureMode ... -> CaptureDuringMouseDown
[GridCustomRecruitRuntime] Cancelled
```

Le joueur a pu immédiatement reprendre le combat et exécuter une attaque après la fermeture du wizard.

### Anomalie détectée puis corrigée

PIE avait signalé :

```text
UWidget::RemoveFromParent() called ... which has no UMG parent
```

Cause : `CancelWizard()` notifiait synchroniquement le Pawn, le Pawn retirait le widget, puis `CancelWizard()` exécutait son propre `RemoveFromParent()`.

Hardening : le handler Pawn `HandleCustomRecruitCancelled()` libère désormais uniquement l'état modal/input ; `CancelWizard()` reste propriétaire de sa suppression visuelle. Les fermetures programmatiques continuent de passer par `FinishCustomRecruitCharacterCreationWidget()`.

Une revalidation PIE hors combat reste demandée afin de confirmer l'absence de ce warning après correctif.

---

## 6. PIE validé — Engager

Le second passage sur le même Trigger a aussi été validé avant hardening :

```text
Trigger Activated
-> OpenCustomRecruit Success=true
-> WBP_CharacterCreationWizard Context=CustomRecruit
-> Engager
-> Elarion créé
-> Race=Elf
-> Class=Mage
-> CharacterIndex=1
-> retour au jeu
```

Les attributs observés étaient :

```text
8 / 12 / 10 / 15 / 12 / 9
```

Le log runtime confirme :

```text
[GridCustomRecruitRuntime] Committed ... CharacterIndex=1
```

Le héros principal est resté actif et le contrôle du jeu a été restauré.

Une revalidation PIE hors combat reste demandée après le hardening afin de confirmer que ce chemin n'a subi aucune régression.

---

## 7. Politique finale : pas de recrutement pendant un combat

Le premier test manuel avait volontairement été effectué pendant un combat actif. Techniquement le flux fonctionnait, mais cette situation n'est pas retenue comme comportement supporté.

Raison : l'initiative, les `PlayerCharacterTurnStates`, les AP et la séquence courante sont établis au début / pendant le combat. Ajouter un nouveau membre à `ActiveCharacters` au milieu d'un round créerait une divergence entre l'état du groupe et le snapshot combat en cours.

Politique MON20.5.6 :

```text
Exploration + place disponible
    -> CustomRecruit autorisé

Combat actif
    -> OpenCustomRecruit refusé
    -> aucun modal
    -> aucune mutation du groupe
```

`ShowCustomRecruitCharacterCreationWidget()` vérifie le `UGridTurnManagerComponent` existant et refuse si `bCombatActive == true`.

Log attendu :

```text
[GridCustomRecruitRuntime] Show Rejected ... Reason=CombatActive Phase=... Round=...
```

Le connector retourne alors `Success=false` sans ouvrir le wizard.

---

## 8. Automation hardening — VALIDÉE

Test supplémentaire :

```text
Grimrock.MON20.5.CustomRecruit.RuntimeCombatGate
```

Il vérifie qu'avec un TurnManager actif en combat :

- `ShowCustomRecruitCharacterCreationWidget()` retourne false ;
- aucun modal n'est marqué actif ;
- aucune instance de Character Creation n'est créée.

Validation utilisateur du 24 août 2026 :

```text
23 / 23 Success
0 Fail
0 Error
```

Toutes les régressions MON20.5.2 à MON20.5.5 restent vertes, notamment :

```text
AllocatedAttributesPreserved
ContextContract
ContextGateCompletedParty
ContextGateIncompleteParty
ContextGatePartyFull
CustomRecruiterArchetypeContract
EditorLinkPolicy
EventCommandContract
EventCommandMissingPlayerPawn
InitialHeroStatePreserved
InvalidRequestAtomicReject
PartyFullAtomicReject
RecruitmentRollbackLeavesNoPoolCandidate
RuntimeCombatGate
RuntimeContract
RuntimeDefaultState
UniqueCharacterIdentity
ValidCreateAndRecruit
VisualSelectionPreserved
WizardContextDefault
WizardCustomCancelNoMutation
WizardCustomRecruitSubmit
WizardNewGameSubmitRegression
```

---

## 9. Revalidation finale demandée

### A — Automation

```text
[OK] Grimrock.MON20.5.CustomRecruit
[OK] 23 / 23 Success
```

### B — PIE exploration

Hors combat :

```text
Trigger -> wizard -> Annuler
```

Attendu :

```text
wizard fermé
input restauré
aucune mutation du groupe
aucune ligne RemoveFromParent() ... has no UMG parent
```

Puis :

```text
Trigger -> wizard -> Engager
```

Attendu : nouvelle recrue active et retour au jeu.

### C — PIE combat

Pendant un combat actif, traverser le même Trigger.

Attendu :

```text
Reason=CombatActive
pas de wizard
pas de nouvelle recrue
combat inchangé
```

---

## 10. Critère de clôture MON20.5

```text
[OK] 23/23 Automation tests après hardening
[OK] Authoring CustomRecruiter + palette + connector
[OK] PIE Annuler fonctionnel avant hardening
[OK] PIE Engager fonctionnel avant hardening
[OK] Input restauré après les deux chemins avant hardening
[OK] Recrue réelle Elarion créée CharacterIndex=1
[ ] PIE Annuler post-hardening sans warning RemoveFromParent
[ ] PIE Engager post-hardening sans régression
[ ] PIE combat post-hardening : recrutement correctement refusé
[ ] Assets d'authoring versionnés dans Git
```

Les `.uasset` créés/modifiés pendant cette tranche doivent être commités localement après la revalidation finale.
