# CC5 - Sauvegarde minimale du groupe

## 1. Objet

CC5 permet de créer une partie, de sauvegarder Elias et de reprendre la partie dans une nouvelle session PIE.

La sauvegarde contient :

- le groupe actif et le personnage sélectionné ;
- l'identité, la race, la classe, le niveau et l'expérience ;
- les caractéristiques, PV, mana et charge ;
- les Inventaires, équipements et le curseur ;
- les identifiants runtime et l'ownership des objets ;
- la position et l'orientation du groupe ;
- l'état runtime du niveau : objets retirés, portes, mécanismes, réceptacles et objets monde.

Branche :

```text
codex/character-creation-cc3-startup-widget
```

---

## 2. Répartition des responsabilités

| Responsable | Travail CC5 |
|---|---|
| ChatGPT / Codex | Sauvegarde versionnée, sérialisation, restauration atomique, autosauvegarde, modes de démarrage et tests |
| Utilisateur dans UE5 | Compilation, configuration de `BP_GrimrockPartyPawn` et validation sur plusieurs sessions PIE |
| Hors CC5 | Écran de menu principal, sélection de plusieurs slots, migrations futures et sauvegarde en réseau |

Aucune modification des Widget Blueprints n'est requise.

---

## 3. Étape A - Récupérer et compiler

```bash
git fetch origin
git switch codex/character-creation-cc3-startup-widget
git pull
```

Fermer Unreal Editor, puis compiler :

- configuration **Development Editor** ;
- plateforme **Win64** ;
- cible `GrimrockPrototypeEditor`.

Résultat attendu : `Build succeeded`.

---

## 4. Étape B - Configurer BP_GrimrockPartyPawn

Ouvrir `BP_GrimrockPartyPawn`, puis **Class Defaults > RPG > Save**.

Valeurs recommandées :

| Propriété | Valeur |
|---|---|
| `PartyStartupMode` | `Continue` |
| `PartySaveSlotName` | `GrimrockParty` |
| `PartySaveUserIndex` | `0` |
| `bAutoSaveOnInventoryClose` | activé |

Comportement :

- `Continue` charge la sauvegarde lorsqu'elle existe ;
- `Continue` sans sauvegarde ouvre normalement la création de personnage ;
- `NewGame` supprime la sauvegarde du slot au démarrage et ouvre une nouvelle création.

Le fichier est écrit dans :

```text
Saved/SaveGames/GrimrockParty.sav
```

Les fonctions `SaveCurrentGame`, `LoadCurrentGame`, `StartNewGame` et `HasCurrentSave` sont exposées aux Blueprints pour un futur menu principal. Aucun Graph n'est nécessaire pour CC5.

---

## 5. Étape C - Tests Automation

Dans **Tools > Session Frontend > Automation**, rechercher :

```text
Grimrock.CharacterCreation
```

Exécuter les treize tests :

- quatre tests CC0 ;
- trois tests CC1 ;
- trois tests CC2 ;
- un test CC4 ;
- deux tests CC5 :
  - `Grimrock.CharacterCreation.CC5.SaveMemoryRoundTrip` ;
  - `Grimrock.CharacterCreation.CC5.RejectInvalidSnapshotAtomically`.

Les treize tests doivent être verts.

Le premier test sérialise réellement un `USaveGame` en mémoire. Il vérifie le personnage, la torche équipée, la position, l'état du niveau et l'ownership après restauration.

---

## 6. Étape D - Créer et sauvegarder une nouvelle partie

### D.1 Forcer une nouvelle partie

1. Dans `BP_GrimrockPartyPawn`, régler temporairement `PartyStartupMode = NewGame`.
2. Compiler et enregistrer le Blueprint.
3. Lancer le PIE.
4. Vérifier que l'écran de création apparaît.
5. Créer `Elias`.
6. Vérifier le log :

```text
PartySave Saved Slot=GrimrockParty Version=1
```

### D.2 Modifier l'état sauvegardé

1. Ramasser une torche placée dans le niveau.
2. Ouvrir l'Inventaire.
3. Équiper la torche en main principale.
4. Vérifier qu'elle éclaire et qu'elle disparaît de son ancien emplacement.
5. Se déplacer de plusieurs cellules.
6. Fermer l'Inventaire pour déclencher l'autosauvegarde.
7. Vérifier à nouveau le log `PartySave Saved`.
8. Arrêter le PIE.

Ne relancez pas encore le PIE avec `NewGame`, car ce mode supprimerait immédiatement la sauvegarde créée.

---

## 7. Étape E - Continuer la partie

1. Dans `BP_GrimrockPartyPawn`, régler `PartyStartupMode = Continue`.
2. Compiler et enregistrer.
3. Relancer le PIE.
4. Vérifier que l'écran de création ne s'affiche pas.
5. Vérifier le log :

```text
PartySave Continued Slot=GrimrockParty CharacterCount=1
```

6. Vérifier que le groupe reprend à la cellule sauvegardée et avec la même orientation.
7. Ouvrir l'Inventaire.
8. Vérifier :
   - nom `Elias` ;
   - race `Humain` ;
   - classe `Guerrier` ;
   - niveau `1` ;
   - caractéristiques `16 / 12 / 14 / 10 / 10 / 10` ;
   - PV `20 / 20` ;
   - mana `0 / 0` ;
   - torche toujours en main principale ;
   - charge `1.0 / 80.0`.
9. Vérifier que la torche sauvegardée n'est pas réapparue à son emplacement initial.
10. Vérifier que l'éclairage de la torche est actif.

---

## 8. Étape F - Vérifier Nouvelle partie

1. Arrêter le PIE.
2. Régler `PartyStartupMode = NewGame`.
3. Compiler et enregistrer.
4. Relancer le PIE.
5. Vérifier que l'écran de création apparaît.
6. Vérifier que l'ancien Elias n'est pas chargé.
7. Vérifier que le niveau utilise son état initial.
8. Arrêter le PIE.
9. Remettre `PartyStartupMode = Continue` pour l'utilisation normale.

---

## 9. Diagnostics

| Log | Signification |
|---|---|
| `PartySave Saved` | écriture réussie |
| `PartySave Continued` | chargement automatique réussi |
| `PartySave Loaded` | chargement demandé explicitement réussi |
| `PartySave Load Failed` | sauvegarde invalide, incompatible ou état runtime inapplicable |
| `PartySave EndPlay Failed` | sauvegarde impossible pendant la fermeture de la session |

Une sauvegarde invalide est refusée sans remplacer l'état courant du groupe.

---

## 10. Critère de validation

CC5 est validée lorsque :

- les treize tests sont verts ;
- Nouvelle partie affiche la création ;
- Continuer ne réaffiche pas la création ;
- Elias, sa position et sa torche équipée sont restaurés ;
- la torche n'est pas dupliquée dans le monde ;
- l'ownership reste valide ;
- aucune erreur de sauvegarde n'apparaît dans les logs.
