# Journal de validation terrain — Donjon de référence du Grid Editor

**Début :** 29 août 2026  
**Branche :** `master`  
**Référence initiale :** `8b63c86cd22a5ceaad39cdc853ce7922784753a3`  
**Contexte :** construction d'un niveau de donjon complet avec le Grimrock Grid Editor après clôture de GEUI01–GEUI10.

Ce document consigne uniquement les problèmes réellement rencontrés pendant l'authoring du donjon de référence et les corrections motivées par ces problèmes.

---

## Incident 001 — MON13.5 dépendait des monstres historiques du niveau de travail

### Observation

Après nettoyage du niveau de test afin de repartir d'un niveau presque vierge, le test :

`Grimrock.Monsters.MON13.5.RealPIEIntegration`

échouait avant même le lancement PIE avec :

- `The real encounter anchor exists` ;
- `The second wave-zero Rat exists` ;
- `The wave-one Rat exists`.

### Cause

`GridEditorMON135RealPIETests.cpp` utilisait directement trois `MonsterSpawn` historiques enregistrés dans le `UGridLevelAsset` courant de `L_GrimrockEditor`.

Le test imposait notamment :

- trois GUID de placement précis ;
- les cellules `(29,25)`, `(28,26)` et `(27,25)` ;
- le groupe `Encounter_Rats_01` ;
- un trigger en `(27,24)` ;
- un lien `Trigger.Activated -> StartEncounter`.

Ces données n'étaient donc pas seulement des données de démonstration : elles constituaient implicitement le fixture du test automatisé.

Le nettoyage légitime du niveau d'auteur supprimait ce fixture et cassait le test sans qu'aucune régression runtime ne soit présente.

### Classification

- **Type :** dette de test / couplage Editor-Content ;
- **Impact authoring :** élevé, car le niveau de travail ne pouvait pas être nettoyé librement ;
- **Régression runtime :** aucune identifiée ;
- **Correction du contenu utilisateur :** aucune nécessaire.

### Décision

Ne pas remettre les Rats historiques dans le niveau.

`FGridEditorMON135RealPIEIntegrationTest` doit rester un vrai test PIE, mais son scénario MON13.5 est désormais construit dans un `UGridLevelAsset` transient appartenant au test.

Le fixture :

- utilise toujours le vrai `DA_MON_RatGiant` ;
- utilise donc toujours le vrai `BP_MON_RatGiant` référencé par cette définition ;
- conserve les trois vagues/placements historiques du scénario MON13.5 ;
- conserve le vrai chemin Trigger -> `StartEncounter` ;
- conserve les deux sessions PIE Fresh puis Continue ;
- conserve le vrai chemin SaveGame ;
- ne dépend plus des objets actuellement placés dans le niveau en cours d'authoring.

Si un `UGridDungeonAsset` est assigné, le test en crée également une copie transient et rebinde uniquement le niveau courant vers le fixture transient. Cela maintient la vérification de cohérence effectuée par `PreparePIETestFromStartInternal` sans modifier le DungeonAsset persistant.

À la fin du test, les références originales de l'EditorActor et du PreviewRuntimeActor sont restaurées.

### Garanties

La correction ne modifie aucun :

- `.uasset` ;
- `.umap` ;
- placement du niveau de travail ;
- DungeonAsset persistant.

Le niveau de référence reste donc libre d'évoluer et d'être nettoyé sans casser MON13.5.

### Validation UE5.5.4 demandée

Validation ciblée :

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Monsters.MON13.5.RealPIEIntegration"
~~~

Puis validation du jalon MON13.5 :

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Monsters.MON13.5"
~~~

Après succès, poursuivre l'authoring du niveau presque vierge sans réintroduire les Rats historiques.

---

## Incident 002 — Feedback visuel lors d'un déplacement bloqué

### Observation

En PIE, une tentative de déplacement contre un mur, une porte fermée ou un obstacle était simplement refusée. La position restait correcte, mais le joueur ne recevait aucun retour spatial permettant de distinguer clairement « commande ignorée » de « passage bloqué ».

### Classification

- **Type :** UX / Runtime ;
- **Sous-système :** déplacement du groupe ;
- **Impact gameplay :** aucun changement de règle de déplacement ;
- **Impact sauvegarde / triggers / occupancy :** aucun.

### Décision

Ajouter dans `AGrimrockPartyPawn` un feedback de déplacement bloqué purement visuel :

1. la translation logique reste refusée ;
2. `CurrentCellX` / `CurrentCellY` ne changent pas ;
3. le Pawn avance brièvement de `BlockedMoveDistance` dans la direction demandée ;
4. il revient exactement au centre de sa cellule logique ;
5. aucun `HandlePartyCellChanged`, trigger de cellule, transition ou coût de déplacement supplémentaire n'est produit.

Valeurs par défaut :

- `bEnableBlockedMoveFeedback = true` ;
- `BlockedMoveDistance = 15 cm` ;
- `BlockedMoveForwardDuration = 0.08 s` ;
- `BlockedMoveReturnDuration = 0.10 s`.

Le feedback participe à `IsBusy()`. Les nouvelles commandes reçues pendant ces ~0,18 s ne sont pas bufferisées afin d'éviter l'empilement de chocs visuels.

En combat, le feedback n'est lancé que pour les refus spatiaux :

- `TargetCellUnavailable` ;
- `PassageBlocked` ;
- `TargetCellOccupied`.

Les refus liés au tour actif, aux PA/PAM ou à l'état du groupe restent sans déplacement visuel.

### Test automatisé

Nouveau test :

`Grimrock.Runtime.PartyMovement.BlockedFeedback`

Il vérifie notamment :

- qu'un mur refuse toujours la translation logique ;
- que la cellule logique ne change ni à l'aller ni au retour ;
- que le Pawn atteint la distance visuelle configurée ;
- qu'il revient exactement au centre de sa cellule ;
- qu'aucune entrée supplémentaire n'est bufferisée pendant le choc ;
- que désactiver le feedback ne change pas la règle de collision ;
- qu'un passage ouvert utilise toujours le mouvement de grille normal.

### Validation UE5.5.4 demandée

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Runtime.PartyMovement.BlockedFeedback"
~~~

Puis, pour vérifier les contrats de déplacement déjà existants :

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.TechnicalDebt.TD02_9.PartyMovementFacade"
~~~

Enfin, valider en PIE le ressenti des valeurs par défaut et ajuster uniquement les quatre paramètres `Movement|Blocked Feedback` si nécessaire.
