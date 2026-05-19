# GrimrockPrototype — Decisions Log

Ce fichier conserve les décisions validées afin de ne pas perdre la connaissance acquise entre les sessions ChatGPT, Codex et les commits Git.

---

## 2026-05-19 — Organisation du chantier objets / mécanismes / connexions

### Décisions validées

- Les objets placés dans le donjon doivent être simplifiés et rapprochés de l’esprit *Legend of Grimrock*.
- La logique doit reposer sur une séparation entre :
  - événements émis ;
  - commandes reçues ;
  - liens logiques.
- Le modèle cible est :

```text
SourceObjectId + SourceEvent -> TargetObjectId + TargetCommand + Delay + bOneShot
```

- Les objets ne doivent pas se connaître directement.
- Un bouton ne doit pas ouvrir une porte directement.
- Un bouton émet un événement.
- Le système de liens exécute une commande sur la porte.
- Les portes doivent être principalement des cibles de commandes.
- Le runtime doit évoluer vers un dispatcher central, probablement `UGridActivationComponent`.

---

## 2026-05-19 — Objets concrets vs comportements factorisés

### Décisions validées

- Les objets doivent rester distincts dans la palette et dans les DataAssets lorsqu’ils sont visuellement différents.
- `Button_Normal`, `Button_Secret` et `Button_Wall` sont trois objets distincts.
- Ces trois boutons peuvent partager la même classe C++ :

```cpp
AGridButtonActor
```

- Ils sont différents visuellement, mais pas nécessairement conceptuellement.
- Les différences doivent être portées par les archétypes, les meshes, les matériaux et les paramètres.

---

## 2026-05-19 — Catégories d’objets

### Décisions validées

- `EGridObjectCategory` doit rester conservateur.
- Il ne faut pas supprimer inutilement les catégories existantes.
- `Spawn` doit être conservé.
- `Readable` doit être conservé.
- Les catégories recommandées sont :

```text
None
Mechanism
Receptacle
Passage
Item
Decoration
Readable
Spawn
Trigger
Light
Hazard
```

- `Light` et `Hazard` peuvent être ajoutés plus tard si nécessaire.
- `Category` sert d’abord à classer l’objet dans l’éditeur et la palette, pas à contenir toute la logique runtime.

---

## 2026-05-19 — WallInscription

### Décisions validées

- `WallInscription` est l’objet existant.
- Il ne doit pas être renommé en `Inscription`.
- Il appartient à la catégorie `Readable`.
- Il peut émettre `OnUse`.
- Il doit conserver son rôle narratif actuel.

---

## 2026-05-19 — Porte secrète

### Décisions validées

- La porte secrète doit être un objet explicite.
- Elle appartient à la catégorie `Passage`.
- Elle doit probablement reposer sur :

```cpp
AGridSecretDoorActor
```

- `Door_Secret` doit être un archétype distinct de `Door_Stone`.
- Le principe de mesh fixe + mesh mobile doit être conservé.
- La partie fixe doit être visible en édition et en runtime.

---

## 2026-05-19 — Réceptacles

### Décisions validées

- Les réceptacles concrets suivants doivent exister :
  - `Alcove`
  - `TorchHolder`
  - `Altar`
  - `OfferingBowl`
  - `CoinSlot`
  - `Lock/Keyhole`

- Ils peuvent partager une classe C++ :

```cpp
AGridReceptacleActor
```

- Le support de torche ne doit pas coder la torche en dur.
- Le support de torche est un réceptacle paramétré.
- La torche est un item.
- La lumière est un effet lié à la présence d’un item valide dans le réceptacle.

---

## 2026-05-19 — Événements

### Décisions validées

Événements recommandés :

```text
OnActivate
OnDeactivate
OnToggle
OnEnter
OnExit
OnInsertItem
OnRemoveItem
OnUse
OnUnlock
OnTimer
OnSpawn
```

---

## 2026-05-19 — Commandes

### Décisions validées

Commandes recommandées :

```text
Activate
Deactivate
Toggle
Open
Close
ToggleOpen
Lock
Unlock
Enable
Disable
StartTimer
StopTimer
ResetTimer
Teleport
Spawn
Destroy
ShowText
PlayAnimation
PlaySound
```

---

## 2026-05-19 — Organisation ChatGPT / Codex / Git

### Décisions validées

- ChatGPT ne doit pas être la mémoire principale du projet.
- Codex ne doit pas être la mémoire principale du projet.
- La mémoire stable doit être dans le dépôt Git, dans `Docs/Design`.
- Les décisions importantes doivent être ajoutées dans ce fichier.
- Codex doit recevoir des tâches courtes, ciblées, avec fichiers autorisés.
- Il ne faut pas demander à Codex de refactoriser tout le système d’un coup.
- Chaque tâche doit produire :
  - un diff minimal ;
  - une compilation ;
  - un test UE5 ;
  - un commit Git ;
  - une mise à jour éventuelle des documents.

---

## 2026-05-19 — Feuille de route validée

### Étapes validées

1. Créer la documentation de référence.
2. Ajouter les types C++ `EGridObjectEvent`, `EGridObjectCommand`, `FGridObjectLink`.
3. Adapter progressivement `UGridObjectArchetypeAsset`.
4. Créer ou corriger les archétypes concrets.
5. Créer ou consolider un dispatcher runtime.
6. Brancher d’abord Button -> Door.
7. Brancher ensuite Lever, PressurePlate, Receptacle.
8. Adapter l’inspecteur éditeur.
9. Créer une carte de test.
10. Nettoyer l’ancien code après validation.

---

## Prochaine étape

Créer les fichiers Markdown dans :

```text
Docs/Design/
```

Puis commencer par une tâche Codex courte :

```text
Tâche Codex 01 — Ajouter EGridObjectEvent et EGridObjectCommand
```

