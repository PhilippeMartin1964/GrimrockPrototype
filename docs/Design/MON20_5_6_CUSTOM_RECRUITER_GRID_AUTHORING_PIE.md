# MON20.5.6 — Custom Recruiter Grid Authoring & PIE

Statut : **VALIDÉ UE5.5.4 — CLOS**  
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

Aucun Blueprint Graph métier parallèle n'est requis.

---

## 2. Chaîne de validation MON20.5

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

Résultat final :

```text
23 / 23 Success
0 Fail
0 Error
```

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

Le target reste data-only.

---

## 4. Connector Grid Editor

```text
Source Object = Trigger placé
Event         = Activated
Target Object = Custom Recruiter placé
Command       = Open Custom Recruit
Condition     = None
```

Le flux réutilise le pipeline Event -> Command existant ainsi que le bridge Lua générique.

---

## 5. PIE final post-hardening — VALIDÉ

### Hors combat — Annuler

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

Le warning de double `RemoveFromParent()` ne se reproduit plus.

### Hors combat — Engager

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

Le chemin reste :

```text
Wizard
-> FRPGCustomRecruitService
-> CharacterPool temporaire
-> MON20.2 TryRecruitFromPool
-> ActiveCharacters
-> spellbook runtime garanti
```

### Pendant un combat — refus propre

```text
Trigger Activated
-> OpenCustomRecruit
-> Reason=CombatActive
-> Success=false
-> aucun wizard
-> aucune nouvelle recrue
-> combat inchangé
```

---

## 6. Politique runtime finale

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

Cette politique protège l'initiative, les états de tour, les AP et le HUD combat.

---

## 7. Assets d'authoring versionnés

Le commit utilisateur :

```text
3a4a7d1cd1133a9536dbde4c964d4b81bfcbc2d4
Close MON20.5 with the correct assets
```

versionne les assets requis :

```text
DA_ObjectPalette_Default.uasset
DA_Archetype_CustomRecruiter_Service.uasset
DA_GridLevel_00.uasset
L_GrimrockEditor.umap
```

Il a été réuni avec la documentation distante dans :

```text
8ec08af4ee8e85353ab1cbd51cb67754df46ed68
```

---

## 8. Critère de clôture MON20.5

```text
[OK] 23/23 Automation tests après hardening
[OK] Authoring CustomRecruiter + palette + connector
[OK] PIE Annuler post-hardening sans warning RemoveFromParent
[OK] PIE Engager post-hardening sans régression
[OK] PIE combat : recrutement correctement refusé
[OK] Input restauré après fermeture du wizard
[OK] Aucune mutation du groupe sur Annuler / refus combat
[OK] Recrutement réel hors combat fonctionnel
[OK] Assets d'authoring versionnés dans Git
```

MON20.5.6 et son jalon parent MON20.5 sont donc **VALIDÉS UE5.5.4 — CLOS**.

La documentation de clôture synthétique se trouve dans :

```text
docs/Design/MON20_5_CLOSURE.md
```

Prochaine étape : **MON20.6 — Skills Data Model & Runtime**.
