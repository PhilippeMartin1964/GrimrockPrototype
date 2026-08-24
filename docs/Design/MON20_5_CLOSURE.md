# MON20.5 — Custom Recruit / Wizard Context Reuse — Closure

Statut : **VALIDÉ UE5.5.4 — CLOS**  
Date : **24 août 2026**

---

## 1. Résultat final

MON20.5 fournit un recrutement personnalisable complet en réutilisant le wizard de création de personnage existant et l’autorité de groupe existante.

```text
Trigger.Activated
    -> CustomRecruiter.OpenCustomRecruit
    -> AGrimrockPartyPawn::ShowCustomRecruitCharacterCreationWidget()
    -> WBP_CharacterCreationWizard Context=CustomRecruit
    -> FRPGCustomRecruitService
    -> CharacterPool temporaire
    -> FRPGPartyRecruitmentService::TryRecruitFromPool
    -> ActiveCharacters
```

Aucun second WBP métier, aucun second registre de groupe et aucune logique Blueprint parallèle n’ont été introduits.

---

## 2. Sous-tranches

```text
MON20.5.1 — Audit & Contract                              TERMINÉ
MON20.5.2 — Context + Custom Recruit Transaction          VALIDÉ — 9/9
MON20.5.3 — Wizard Context Reuse                          VALIDÉ — 16/16 cumulés
MON20.5.4 — Custom Recruit Modal Runtime Integration      VALIDÉ — 18/18 cumulés
MON20.5.5 — Event / Command Authoring Bridge              VALIDÉ — 22/22 cumulés
MON20.5.6 — Grid Authoring / PIE / Runtime Hardening      VALIDÉ — 23/23 cumulés
```

---

## 3. Validation Automation

Filtre :

```text
Grimrock.MON20.5.CustomRecruit
```

Résultat final utilisateur :

```text
23 / 23 Success
0 Fail
0 Error
```

Le test `RuntimeCombatGate` confirme le refus du recrutement pendant un combat actif.

---

## 4. Validation PIE UE5.5.4

```text
[OK] Hors combat -> Trigger -> Wizard -> Annuler
     -> retour au jeu
     -> groupe inchangé
     -> input restauré
     -> aucun warning RemoveFromParent

[OK] Hors combat -> Trigger -> Wizard -> Engager
     -> nouvelle recrue active
     -> retour au jeu
     -> héros principal intact

[OK] Pendant combat -> Trigger
     -> OpenCustomRecruit refusé
     -> aucun wizard
     -> aucune mutation du groupe
     -> combat inchangé
```

---

## 5. Authoring versionné

Les assets nécessaires ont été commités dans :

```text
3a4a7d1cd1133a9536dbde4c964d4b81bfcbc2d4
Close MON20.5 with the correct assets
```

Ce commit contient notamment :

```text
Content/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.uasset
Content/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Archetype_CustomRecruiter_Service.uasset
Content/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_00.uasset
Content/GrimrockPrototype/Maps/L_GrimrockEditor.umap
```

Le commit a ensuite été réuni avec la documentation distante dans le merge :

```text
8ec08af4ee8e85353ab1cbd51cb67754df46ed68
```

---

## 6. Architecture retenue

- même `CharacterCreationWidgetClass` pour New Game et Custom Recruit ;
- contexte transient `ERPGCharacterCreationContext` ;
- transaction custom recruit atomique ;
- identité `CharacterId` unique ;
- race, classe, portrait et attributs préservés ;
- spellbook runtime garanti ;
- `Annuler` sans mutation ;
- commande `OpenCustomRecruit` dans Event -> Command / Lua ;
- target `CustomRecruiter` data-only ;
- recrutement interdit pendant un combat actif ;
- aucune migration SaveGame introduite dans MON20.5.

---

## 7. Conclusion

MON20.5 est **VALIDÉ UE5.5.4 — CLOS**.

Le prochain jalon est :

```text
MON20.6 — Skills Data Model & Runtime
```
