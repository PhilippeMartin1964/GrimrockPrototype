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
