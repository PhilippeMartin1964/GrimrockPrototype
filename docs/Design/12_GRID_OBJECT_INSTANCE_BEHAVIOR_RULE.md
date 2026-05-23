# 12 — Règle des comportements d'instances d'objets

Statut : document actif de référence  
Date : 2026-05-23  
Projet : GrimrockPrototype — Grid Object Behavior

## 1. Objectif

Ce document fixe la règle officielle concernant les paramètres de comportement des objets placés dans un niveau.

Il complète notamment :

- `01_GRID_OBJECT_SYSTEM.md` ;
- `02_OBJECT_ARCHETYPES.md` ;
- `06_GRID_EDITOR_UX_SPEC.md` ;
- `11_GRID_OBJECT_ARCHETYPE_PARAMETERS_REFERENCE.md`.

La décision importante est la suivante :

```text
L'archétype donne les valeurs initiales.
L'objet placé possède sa propre copie.
L'éditeur modifie cette copie.
Le runtime lit cette copie.
```

---

## 2. Règle source de vérité

### 2.1 Archétype

`UGridObjectArchetypeAsset::DefaultBehavior` sert uniquement de modèle au moment où un objet est créé ou replacé depuis la palette.

Il définit les valeurs par défaut d'un type concret d'objet, par exemple :

```text
Button_Normal
Door_Stone
Lever
PressurePlate
Receptacle_TorchHolder
Teleporter_Rune
```

L'archétype peut donc être compris comme un **template de création**.

Il n'est pas la source de vérité permanente des objets déjà placés.

### 2.2 Objet placé

Chaque objet placé dans le niveau stocke sa propre copie locale dans :

```cpp
FGridLevelObjectData::Behavior
```

Cette copie est la source de vérité de l'objet placé.

Lorsqu'un designer modifie un paramètre dans le Grimrock Grid Editor Mode, l'éditeur doit modifier directement cette structure :

```cpp
SelectedObject.Behavior
```

### 2.3 Runtime

Les acteurs runtime doivent toujours lire les paramètres depuis :

```cpp
ObjectData.Behavior
```

Ils ne doivent pas relire ni réinjecter automatiquement :

```cpp
Archetype->DefaultBehavior
```

au moment du runtime.

---

## 3. Cycle de vie attendu

### 3.1 Choix dans la palette

Quand l'utilisateur choisit un objet dans la palette :

```cpp
ObjectBehavior = SelectedArchetype->DefaultBehavior;
```

L'éditeur prépare donc les valeurs initiales qui seront copiées au placement.

### 3.2 Placement dans le niveau

Quand l'objet est placé :

```cpp
NewObject.Behavior = ObjectBehavior;
```

À partir de ce moment, l'objet placé devient autonome.

### 3.3 Édition d'une instance

Quand l'utilisateur sélectionne un objet déjà placé et modifie un champ contextuel :

```cpp
FGridObjectBehaviorParams NewBehavior = Obj.Behavior;
NewBehavior.ButtonAnimation.ButtonPressDistance = NewValue;
EditorActor->ApplyBehaviorToSelectedObject(NewBehavior);
```

La modification concerne l'instance sélectionnée, pas l'archétype.

### 3.4 Runtime

Au runtime :

```cpp
FGridLevelObjectData RuntimeObjectData = ObjectData;
Actor->InitializeGridObject(RuntimeObjectData, ...);
```

L'acteur utilise `RuntimeObjectData.Behavior`, qui vient de l'objet placé.

---

## 4. Suppression de `bOverrideBehavior`

La logique `bOverrideBehavior` a été supprimée volontairement.

Elle ne doit pas être réintroduite.

Ancien modèle à éviter :

```text
Si bOverrideBehavior = false
    utiliser Archetype->DefaultBehavior
Sinon
    utiliser ObjectData.Behavior
```

Nouveau modèle :

```text
Toujours utiliser ObjectData.Behavior.
```

Raison : l'ancien modèle introduisait une ambiguïté permanente dans l'éditeur.

Un champ pouvait être visible et modifiable, mais ne pas être réellement utilisé selon l'état d'un booléen caché ou mal compris.

La règle UX validée est désormais :

```text
Ce que je vois et modifie dans l'éditeur est ce que l'objet utilisera en jeu.
```

---

## 5. Groupes `Behavior` supportés

`FGridObjectBehaviorParams` regroupe les paramètres d'instance spécifiques aux objets.

Les groupes actuellement prévus sont :

```text
Teleporter
Receptacle
ButtonAnimation
LeverAnimation
PressurePlateAnimation
DoorAnimation
```

### 5.1 Teleporter

```cpp
Behavior.Teleporter.TargetCellX
Behavior.Teleporter.TargetCellY
```

Utilisé par les téléporteurs pour définir la cellule de destination.

### 5.2 Receptacle

```cpp
Behavior.Receptacle.bAcceptAnyItem
Behavior.Receptacle.AcceptedItemTags
Behavior.Receptacle.AcceptedArchetypeIds
Behavior.Receptacle.RejectedItemArchetypeIds
Behavior.Receptacle.InitialContainedItemArchetypeId
```

Utilisé par les réceptacles : alcôve, support de torche, autel, bol d'offrande, serrure ou assimilés.

### 5.3 ButtonAnimation

```cpp
Behavior.ButtonAnimation.ButtonPressDistance
Behavior.ButtonAnimation.ButtonPressDuration
Behavior.ButtonAnimation.ButtonReleaseDuration
Behavior.ButtonAnimation.ButtonHoldTime
```

Utilisé par les boutons normaux, secrets et muraux.

### 5.4 LeverAnimation

```cpp
Behavior.LeverAnimation.LeverOffPitch
Behavior.LeverAnimation.LeverOnPitch
Behavior.LeverAnimation.ToggleDuration
```

Utilisé par les leviers.

### 5.5 PressurePlateAnimation

```cpp
Behavior.PressurePlateAnimation.ReleasedHeightAboveFloor
Behavior.PressurePlateAnimation.PressedHeightAboveFloor
Behavior.PressurePlateAnimation.MoveDuration
```

Utilisé par les plaques de pression.

### 5.6 DoorAnimation

```cpp
Behavior.DoorAnimation.OpenHeight
Behavior.DoorAnimation.MoveDuration
```

Utilisé par les portes et portes secrètes.

---

## 6. Règles pour le Grimrock Grid Editor Mode

L'inspecteur d'objet doit appliquer automatiquement les changements d'instance.

À faire :

```text
Modifier un champ contextuel
-> copier Obj.Behavior
-> modifier la valeur concernée
-> ApplyBehaviorToSelectedObject(NewBehavior)
-> RequestRefresh()
```

À éviter :

```text
Apply Behavior
Apply Selected Object
Override Behavior
Reset automatique depuis l'archétype au runtime
```

Les sections contextuelles doivent exposer uniquement les champs utiles au type sélectionné :

```text
Button          -> ButtonAnimation
Lever           -> LeverAnimation
PressurePlate   -> PressurePlateAnimation
Door            -> DoorAnimation
Teleporter      -> Teleporter
Receptacle      -> Receptacle
```

Les champs avancés ou bruts restent dans `Advanced / Debug` si nécessaire.

---

## 7. Règles pour le runtime

Les acteurs runtime doivent lire leurs paramètres dans `ObjectData.Behavior` durant leur initialisation.

Exemples :

```cpp
PressDistance = ObjectData.Behavior.ButtonAnimation.ButtonPressDistance;
```

```cpp
LeverOffPitch = ObjectData.Behavior.LeverAnimation.LeverOffPitch;
LeverOnPitch = ObjectData.Behavior.LeverAnimation.LeverOnPitch;
ToggleDuration = ObjectData.Behavior.LeverAnimation.ToggleDuration;
```

```cpp
OpenHeight = ObjectData.Behavior.DoorAnimation.OpenHeight;
MoveDuration = ObjectData.Behavior.DoorAnimation.MoveDuration;
```

Le runtime ne doit pas dépendre d'un état d'override.

---

## 8. Règles pour les archétypes

Modifier `DefaultBehavior` dans un archétype affecte uniquement :

```text
les futurs objets placés depuis cet archétype
```

Cela ne doit pas modifier automatiquement les objets déjà placés dans un niveau.

Si un jour une fonction de synchronisation est souhaitée, elle doit être explicite et nommée clairement, par exemple :

```text
Reset Selected Object Behavior From Archetype
Apply Archetype Defaults To Selected Objects
```

Ces actions doivent rester des actions volontairement déclenchées par l'utilisateur, idéalement dans `Advanced / Debug` ou dans un menu contextuel.

---

## 9. Checklist de validation

Après tout changement lié aux comportements d'objets :

```text
[ ] Recherche globale `bOverrideBehavior` : aucune occurrence C++
[ ] Le placement copie `DefaultBehavior` vers `FGridLevelObjectData.Behavior`
[ ] L'inspecteur modifie `FGridLevelObjectData.Behavior`
[ ] Le runtime lit `ObjectData.Behavior`
[ ] Aucun runtime actor ne réécrit `Behavior` depuis `Archetype->DefaultBehavior`
[ ] Les champs édités dans l'éditeur persistent dans le LevelAsset
[ ] Les changements sont visibles en PIE/runtime
```

---

## 10. Décision officielle

La règle officielle du projet est :

```text
GridObjectArchetypeAsset.DefaultBehavior
= valeurs par défaut utilisées au placement

FGridLevelObjectData.Behavior
= source de vérité de l'objet placé

Grimrock Grid Editor Mode
= modifie directement FGridLevelObjectData.Behavior

Runtime actors
= lisent toujours ObjectData.Behavior

bOverrideBehavior
= supprimé, ne doit pas revenir
```
