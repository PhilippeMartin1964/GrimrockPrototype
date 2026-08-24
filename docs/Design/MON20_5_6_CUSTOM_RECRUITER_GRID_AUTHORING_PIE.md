# MON20.5.6 — Custom Recruiter Grid Authoring & PIE

Statut : **À VALIDER MANUELLEMENT SOUS UE5.5.4**  
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

Cette tranche ne requiert aucun nouveau code C++ ni aucun nouveau Blueprint Graph. Elle consiste à créer les assets d'authoring, placer le target dans le `UGridLevelAsset`, créer le connector et exécuter deux scénarios PIE.

---

## 2. Préconditions validées

Les tranches précédentes sont validées :

```text
MON20.5.2  Custom Recruit Transaction          9/9
MON20.5.3  Wizard Context Reuse               16/16 cumulés
MON20.5.4  Runtime Modal Integration           18/18 cumulés
MON20.5.5  Event / Command Authoring Bridge    22/22 cumulés
```

Le filtre de référence est :

```text
Grimrock.MON20.5.CustomRecruit
```

Résultat MON20.5.5 :

```text
22 / 22 Success
0 Fail
0 Error
```

---

## 3. Créer l'archetype

Créer un Data Asset de classe :

```text
GridObjectArchetypeAsset
```

Nom recommandé :

```text
DA_Archetype_CustomRecruiter_Service
```

Le placer avec les autres archetypes Grimrock existants.

Configurer :

```text
Archetype
  Archetype Id             = CustomRecruiter_Service
  Display Name             = Custom Recruiter
  Gameplay Type            = CustomRecruiter
  Description              = Opens the custom character recruitment wizard.
  Functional Category      = Decoration

Defaults
  Default Initially Enabled = true
  Default Initially Active  = false
  Default Tag               = None

Palette
  Palette Category          = Recruitment

Placement
  Placement Kind            = Center
  Can Share Cell            = true
  Can Share Anchor          = true
  Blocks Movement           = false
  Placement Z Offset        = 12.0 (default acceptable)

Interaction
  Runtime Interactable      = false
  Runtime Readable          = false

Light
  Runtime Light Source      = false

Visual
  Main Mesh / Preview Mesh  = None
  Main Material             = None

Runtime
  Runtime Actor Class       = None
  Item Actor Class          = None
```

Le target est volontairement invisible et data-only. Un PreviewMesh pourra être ajouté plus tard uniquement si l'authoring visuel en a besoin.

Le test `CustomRecruiterArchetypeContract` confirme qu'aucun `RuntimeActorClass` n'est requis.

---

## 4. Ajouter l'entrée de palette

Ouvrir le `GridObjectPaletteAsset` utilisé par le Grid Editor (dans le projet de référence : la palette par défaut déjà utilisée pour les autres objets).

Ajouter une entrée :

```text
Entry Id                         = CustomRecruiter_Service
Display Name Override            = Custom Recruiter
Category Override                = Recruitment
Icon                             = None
Default Archetype                = DA_Archetype_CustomRecruiter_Service
Default Monster Definition       = None
Default Story Companion Definition = None
```

Aucune définition de compagnon n'est nécessaire : contrairement à `StoryCompanion`, l'identité est créée par le joueur dans le wizard.

Sauvegarder la palette.

---

## 5. Placer le target dans le niveau

Dans le Grid Editor :

1. ouvrir le niveau de test utilisé pour MON20.4/MON20.5 ;
2. dans `TOOLS / PALETTE`, ouvrir la catégorie `Recruitment` ;
3. sélectionner `Custom Recruiter` ;
4. choisir une cellule libre ou une cellule partageable proche du Trigger de test ;
5. placer l'objet ;
6. sélectionner l'objet placé et vérifier dans l'inspecteur :

```text
Gameplay Type = CustomRecruiter
ArchetypeId    = CustomRecruiter_Service
Enabled at Start = true
```

Le target n'a pas besoin d'être visible en jeu.

---

## 6. Créer le connector

Sélectionner le Trigger source puis ouvrir `CONNECTORS`.

Cliquer sur `+` et configurer exactement :

```text
Source Object = Trigger placé
Event         = Activated
Target Object = Custom Recruiter placé
Command       = Open Custom Recruit
Condition     = None
```

Puis cliquer :

```text
Create
```

Le connector doit apparaître comme connector sortant du Trigger et entrant du Custom Recruiter.

Aucun Blueprint Graph n'est nécessaire.

---

## 7. PIE — scénario A : Annuler

Précondition :

- héros principal déjà créé ;
- au moins une place libre dans le groupe actif.

Procédure :

1. lancer PIE ;
2. entrer dans la cellule du Trigger ;
3. vérifier que le **même** `WBP_CharacterCreationWizard` s'ouvre ;
4. vérifier que le wizard est utilisable après le New Game ;
5. cliquer `Annuler` ;
6. vérifier le retour immédiat au jeu.

Résultat attendu :

```text
Wizard visible
-> Context = CustomRecruit
-> Annuler
-> Wizard fermé
-> pas de retour Main Menu
-> input exploration restauré
-> nombre de personnages actifs inchangé
-> CharacterPool inchangé
```

Le Trigger peut être réutilisé plus tard : l'annulation n'est pas mémorisée comme un refus définitif.

---

## 8. PIE — scénario B : Engager

Repartir d'un état avec au moins une place libre.

Procédure :

1. entrer dans le Trigger ;
2. remplir le wizard normalement ;
3. choisir race, classe, caractéristiques, identité et portrait ;
4. atteindre la page finale ;
5. cliquer `Engager` ;
6. vérifier le retour au jeu.

Résultat attendu :

```text
Wizard CustomRecruit
-> FRPGCustomRecruitService
-> candidat temporaire dans CharacterPool
-> MON20.2 TryRecruitFromPool
-> nouveau personnage dans ActiveCharacters
-> candidat retiré du pool
-> spellbook runtime garanti
-> wizard fermé
-> input exploration restauré
-> HUD rafraîchi
```

Le héros principal existant doit rester intact.

---

## 9. PIE — réutilisation du service

Si une place reste disponible après le premier recrutement :

1. ressortir du Trigger ;
2. revenir ;
3. vérifier que le wizard peut s'ouvrir à nouveau ;
4. annuler ou recruter un autre personnage.

C'est volontaire : `CustomRecruiter` représente un service réutilisable (auberge, guilde, recruteur), pas un compagnon scénarisé unique.

Lorsque le groupe est plein, l'ouverture doit être refusée proprement sans mutation.

---

## 10. Logs utiles

En cas de problème, conserver les lignes contenant :

```text
CustomRecruit
CharacterCreationWizard
Grid link
OpenCustomRecruit
PartyInventory
Recruit
```

Les points de diagnostic prioritaires sont :

```text
le Trigger émet-il Activated ?
le connector est-il exécuté ?
le target est-il bien CustomRecruiter ?
la commande est-elle OpenCustomRecruit ?
le Pawn refuse-t-il le modal à cause d'un groupe plein ou d'un autre modal ?
le wizard est-il bien initialisé avec Context=CustomRecruit ?
```

---

## 11. Critère de clôture MON20.5

MON20.5 pourra être clos lorsque les validations suivantes auront toutes été confirmées :

```text
[OK] 22/22 Automation tests
[ ] PIE Annuler : retour au jeu sans mutation
[ ] PIE Engager : nouvelle recrue active et retour au jeu
[ ] PIE input/modal : aucune anomalie de focus ou de contrôle
[ ] Assets d'authoring versionnés dans Git
```

Les `.uasset` créés/modifiés pendant cette tranche doivent être commités localement après validation PIE.
