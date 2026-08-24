# MON20.5.4 — Custom Recruit Modal Runtime Integration

Statut : **IMPLÉMENTÉ — VALIDATION UE5.5.4 EN ATTENTE**  
Date : **24 août 2026**  
Jalon parent : **MON20.5 — Custom Recruit / Wizard Context Reuse**

---

## 1. Objectif

Permettre d’ouvrir le **même wizard de création de personnage existant** pendant l’exploration en contexte :

```text
ERPGCharacterCreationContext::CustomRecruit
```

sans créer :

- un second WBP ;
- une seconde classe de widget ;
- un second état modal ;
- une seconde transaction de recrutement.

Le flux runtime devient :

```text
AGrimrockPartyPawn
    -> ShowCustomRecruitCharacterCreationWidget()
        -> CharacterCreationWidgetClass existante
        -> WBP_CharacterCreationWizard existant
        -> InitializeCharacterCreationWidgetForContext(CustomRecruit)
        -> FRPGCustomRecruitService
        -> MON20.2 TryRecruitFromPool
        -> delegate Commit / Cancel
        -> restauration input gameplay
```

---

## 2. Réutilisation stricte de l’existant

MON20.5.4 réutilise les propriétés déjà présentes sur `AGrimrockPartyPawn` :

```text
CharacterCreationWidgetClass
CharacterCreationWidgetInstance
bCharacterCreationModalActive
```

Aucun champ parallèle :

```text
CustomRecruitWidgetClass
CustomRecruitWidgetInstance
```

n’est ajouté.

Le Blueprint `BP_GrimrockPartyPawn` continue donc à fournir une seule classe configurée :

```text
WBP_CharacterCreationWizard
```

---

## 3. API runtime Pawn

`AGrimrockPartyPawn` expose désormais :

```cpp
bool ShowCustomRecruitCharacterCreationWidget();
void CloseCustomRecruitCharacterCreationWidget();
bool IsCustomRecruitCharacterCreationModalActive() const;
URPGCharacterCreationWidget* GetCustomRecruitCharacterCreationWidget() const;
```

Ces fonctions sont `BlueprintCallable` / `BlueprintPure` uniquement pour conserver un seam runtime exploitable. Les règles de création et de recrutement restent en C++.

---

## 4. Garde d’ouverture

L’ouverture est rejetée avant création du widget si :

```text
PartyInventoryComponent absent
héros principal non créé
groupe actif vide
groupe actif complet
un Character Creation modal est déjà actif
un Story Companion Recruitment modal est déjà actif
PlayerController absent
CharacterCreationWidgetClass non configurée
```

La validation transactionnelle complète reste effectuée plus tard par MON20.5.2 au submit.

---

## 5. Ouverture du modal

Une instance fraîche du même widget est créée :

```cpp
CreateWidget<URPGCharacterCreationWidget>(
    PlayerController,
    CharacterCreationWidgetClass)
```

puis initialisée :

```cpp
InitializeCharacterCreationWidgetForContext(
    this,
    ERPGCharacterCreationContext::CustomRecruit)
```

Les delegates MON20.5.3 sont branchés sur le Pawn :

```text
OnCustomRecruitCommitted
    -> HandleCustomRecruitCommitted

OnCustomRecruitCancelled
    -> HandleCustomRecruitCancelled
```

Le Pawn réutilise ensuite :

```text
bCharacterCreationModalActive = true
ClearBufferedCommand()
ApplyCharacterCreationInputMode(true)
```

Le menu d’inventaire est fermé s’il était visible.

---

## 6. Commit

Après un submit CustomRecruit réussi, MON20.5.3 émet :

```text
OnCustomRecruitCommitted(Widget, CharacterIndex)
```

Le Pawn :

1. vérifie que le callback provient bien de l’instance courante en contexte `CustomRecruit` ;
2. récupère le `CharacterId` de la nouvelle recrue ;
3. garantit l’existence de son Spellbook runtime ;
4. ferme le wizard ;
5. remet `bCharacterCreationModalActive` à `false` ;
6. vide le buffer d’input ;
7. restaure `GameAndUI` via `ApplyCharacterCreationInputMode(false)` ;
8. resynchronise l’objet tenu ;
9. rafraîchit le HUD de combat.

Le recrutement lui-même reste déjà committé par :

```text
FRPGCustomRecruitService
    -> FRPGPartyRecruitmentService
```

Le Pawn ne modifie jamais directement `ActiveCharacters`.

---

## 7. Annulation

En contexte `CustomRecruit`, `CancelWizard()` émet le delegate MON20.5.3.

Le Pawn ferme alors le modal et restaure l’input sans mutation de groupe.

Contrairement au contexte New Game :

```text
aucun retour au Main Menu
aucune création de personnage
aucun candidat laissé dans CharacterPool
```

---

## 8. Exclusion avec les autres UI modales

Le système conserve une seule garde de Character Creation :

```text
bCharacterCreationModalActive
```

Conséquences :

- l’inventaire reste bloqué pendant le wizard ;
- les raccourcis de combat restent bloqués ;
- le Story Companion Recruitment refuse de s’ouvrir pendant ce wizard ;
- le Custom Recruit refuse de s’ouvrir pendant un Story Companion Recruitment ;
- aucune deuxième instance du wizard ne peut se superposer.

---

## 9. Hors scope

MON20.5.4 n’ajoute pas encore :

- objet `Recruiter` dans `GridLevelAsset` ;
- Event -> Command pour ouvrir le Custom Recruit ;
- entrée palette Grid Editor ;
- coût en or ;
- dialogue d’auberge / guilde ;
- niveau de recrue dynamique ;
- équipement de départ ;
- réserve lorsque le groupe est plein ;
- nouveau WBP.

Le déclenchement data-driven appartient à la tranche suivante.

---

## 10. Automation Tests

Le filtre reste :

```text
Grimrock.MON20.5.CustomRecruit
```

MON20.5.4 ajoute :

```text
RuntimeContract
RuntimeDefaultState
```

Les 16 tests MON20.5.2 + MON20.5.3 restent présents.

Total attendu :

```text
18 tests
```

La validation UE5.5.4 doit être fournie par l’utilisateur. Aucun résultat n’est présumé.

---

## 11. Suite après validation

Après compilation + :

```text
18 / 18 Success
```

la suite proposée est :

```text
MON20.5.5 — Custom Recruit Event / Command Authoring Bridge
```

Objectif : permettre à un objet data-driven du niveau (recruteur, bouton, trigger ou logique de quête) d’exécuter l’ouverture du wizard CustomRecruit sans Blueprint Graph.
