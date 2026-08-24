# MON20.5.6 — Custom Recruiter Grid Authoring & PIE

Statut : **VALIDÉ UE5.5.4 — CLÔTURE EN ATTENTE DU VERSIONING DES ASSETS**  
Date : **24 août 2026**  
Jalon parent : **MON20.5 — Custom Recruit / Wizard Context Reuse**

---

## 1. Objectif

Valider le flux complet de recrutement personnalisé en conditions réelles d'authoring :

```text
Trigger.Activated
    -> CustomRecruiter.OpenCustomRecruit
    -> AGrimrockPartyPawn::ShowCustomRecruitCharacterCreationWidget()
    -> WBP_CharacterCreationWizard existant
    -> Annuler ou Engager
```

MON20.5.6 couvre :

- le target data-only `CustomRecruiter` ;
- son entrée dans la palette du Grid Editor ;
- le connector Event -> Command ;
- l'ouverture du wizard existant en contexte `CustomRecruit` ;
- la fermeture propre par `Annuler` ;
- le recrutement effectif par `Engager` ;
- la protection contre un recrutement pendant un combat actif ;
- la validation automatisée et PIE sous UE5.5.4.

Aucun Blueprint Graph métier parallèle n'est requis.

---

## 2. Chaîne de validation MON20.5

Les sous-tranches précédentes sont validées :

```text
MON20.5.2  Custom Recruit Transaction          9/9
MON20.5.3  Wizard Context Reuse               16/16 cumulés
MON20.5.4  Runtime Modal Integration           18/18 cumulés
MON20.5.5  Event / Command Authoring Bridge    22/22 cumulés
MON20.5.6  Runtime hardening                   23/23 cumulés
```

Filtre de référence :

```text
Grimrock.MON20.5.CustomRecruit
```

Validation utilisateur du 24 août 2026 :

```text
23 / 23 Success
0 Fail
0 Error
```

Le test négatif `EventCommandMissingPlayerPawn` produit volontairement son warning attendu puis termine en `Success`.

Le test `RuntimeCombatGate` confirme également :

```text
[GridCustomRecruitRuntime] Show Rejected ... Reason=CombatActive Phase=2 Round=3
```

Toutes les régressions MON20.5.2 à MON20.5.5 restent vertes.

---

## 3. Archetype et palette validés

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

`CustomRecruiter` reste volontairement data-only. Aucun `RuntimeActorClass` n'est nécessaire.

---

## 4. Connector Grid Editor validé

Le connector authoré et testé est :

```text
Source Object = Trigger placé
Event         = Activated
Target Object = Custom Recruiter placé
Command       = Open Custom Recruit
Condition     = None
```

Le flux utilise le pipeline Event -> Command existant et reste compatible avec le bridge Lua générique.

---

## 5. PIE final post-hardening — VALIDÉ

Le 24 août 2026, les trois scénarios finaux ont été revalidés sous UE5.5.4.

### 5.1 Hors combat — Annuler

Résultat validé :

```text
Trigger Activated
-> OpenCustomRecruit Success=true
-> WBP_CharacterCreationWizard Context=CustomRecruit
-> Annuler
-> wizard fermé
-> retour au jeu
-> groupe inchangé
-> input restauré
```

Le warning précédemment observé :

```text
UWidget::RemoveFromParent() called ... which has no UMG parent
```

ne se reproduit plus après hardening.

La cause était une double suppression synchrone : `CancelWizard()` notifiait le Pawn, le Pawn retirait le widget, puis `CancelWizard()` tentait de le retirer une seconde fois. La responsabilité est désormais claire :

- `CancelWizard()` reste propriétaire de la suppression visuelle après annulation utilisateur ;
- le handler Pawn libère l'état modal et restaure l'input ;
- les fermetures programmatiques passent toujours par `FinishCustomRecruitCharacterCreationWidget()`.

### 5.2 Hors combat — Engager

Résultat validé après hardening :

```text
Trigger Activated
-> OpenCustomRecruit Success=true
-> WBP_CharacterCreationWizard Context=CustomRecruit
-> Engager
-> nouvelle recrue active
-> wizard fermé
-> retour au jeu
-> input restauré
```

Le chemin de recrutement reste celui défini par les étapes précédentes :

```text
Wizard
-> FRPGCustomRecruitService
-> candidat temporaire CharacterPool
-> MON20.2 TryRecruitFromPool
-> ActiveCharacters
-> candidat retiré du pool
-> spellbook runtime garanti
```

Le test précédent avait notamment créé avec succès :

```text
Nom             = Elarion
Race            = Elf
Classe          = Mage
CharacterIndex  = 1
Attributs       = 8 / 12 / 10 / 15 / 12 / 9
```

Le héros principal reste intact.

### 5.3 Pendant un combat — refus propre

Résultat validé :

```text
Trigger Activated
-> OpenCustomRecruit
-> Reason=CombatActive
-> Success=false
-> aucun wizard
-> aucune nouvelle recrue
-> combat inchangé
```

Le joueur conserve donc le contrôle normal du combat et le groupe n'est jamais modifié au milieu d'un snapshot d'initiative.

---

## 6. Politique runtime finale

La règle MON20.5 est désormais explicite :

```text
Exploration + héros principal présent + place disponible
    -> CustomRecruit autorisé

Combat actif
    -> CustomRecruit refusé

Groupe plein
    -> CustomRecruit refusé

Autre modal de recrutement / création actif
    -> CustomRecruit refusé
```

L'interdiction pendant un combat protège la cohérence entre :

- `ActiveCharacters` ;
- `InitiativeOrder` ;
- `PlayerCharacterTurnStates` ;
- AP du personnage actif ;
- état de manche / phase ;
- HUD combat.

Aucune reconstruction opportuniste de l'initiative au milieu d'un combat n'est introduite.

---

## 7. Validation automatisée finale

Tests validés dans le filtre `Grimrock.MON20.5.CustomRecruit` :

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

Résultat :

```text
23 / 23 Success
```

---

## 8. Critère de clôture MON20.5

État final fonctionnel :

```text
[OK] 23/23 Automation tests après hardening
[OK] Authoring CustomRecruiter + palette + connector
[OK] PIE Annuler post-hardening sans warning RemoveFromParent
[OK] PIE Engager post-hardening sans régression
[OK] PIE combat : recrutement correctement refusé
[OK] Input restauré après fermeture du wizard
[OK] Aucune mutation du groupe sur Annuler / refus combat
[OK] Recrutement réel hors combat fonctionnel
[ ] Assets d'authoring versionnés dans Git
```

La logique C++, les tests et les scénarios PIE sont donc **validés sous UE5.5.4**.

Le seul élément restant avant de marquer MON20.5 `CLOS` est le versioning des `.uasset` créés/modifiés pendant l'authoring, notamment l'archetype Custom Recruiter, la palette et le `GridLevelAsset` contenant le target et son connector.

---

## 9. Prochaine action

Avant MON20.6 :

1. identifier précisément les `.uasset` locaux modifiés avec `git status --short` ;
2. versionner uniquement les assets MON20.5.6 nécessaires ;
3. pousser le commit sur `origin/master` ;
4. vérifier leur présence dans Git ;
5. passer ce document à `VALIDÉ UE5.5.4 — CLOS` ;
6. ouvrir **MON20.6 — Skills Data Model & Runtime**.
