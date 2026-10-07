# UI-RPG05 — Notifications et feedback de progression

Date : **7 octobre 2026**  
État : **SOURCE PRÊTE — validation locale UE5.5.4 requise**

## Objectif

Unifier les messages de progression RPG et fournir une notification visuelle réutilisable.

UI-RPG05 ne crée aucune nouvelle règle métier.

Les autorités existantes restent :

```text
FRPGClassProgressionService
FRPGClassProgressionTransactionService
FGridSkillsPageService
```

## Architecture

```text
TryCommitChoices()
    ↓
FRPGClassProgressionCommitResult
    ↓
FRPGProgressionFeedbackService
    ↓
FRPGProgressionNotificationView
    ↓
UGridSkillsWidget
    ├── LastProgressionNotification
    ├── OnProgressionNotification
    └── Notification_Progression (optionnel)
            ↓
       UGridRPGNotificationWidget
```

## FRPGProgressionFeedbackService

Ce service contient uniquement la traduction de présentation des résultats.

Il centralise notamment les rejets :

```text
personnage invalide
classe invalide
état incohérent
requête vide
doublon
talent inconnu
déjà acquis
niveau insuffisant
prérequis manquant
points insuffisants
variante exclusive déjà acquise
```

`URPGLevelUpWidget` utilise désormais la même traduction que Skills/Talents.

Il n’existe donc plus deux tables de messages concurrentes.

## Notifications de talent

Succès :

```text
Titre   : Talent acquis
Message : « <nom> » a été acquis. Points de talent restants : N.
Sévérité: Success
```

Échec :

```text
Titre   : Acquisition refusée
Message : raison précise fournie par le résultat de transaction
Sévérité: Error
```

## Notification de niveau

Le service sait également produire :

```text
Niveau supérieur
Elias passe du niveau 1 au niveau 2 et gagne 1 point(s) de talent.
```

Cette vue est réutilisable par les futures surfaces globales de progression.

## UGridRPGNotificationWidget

Classe C++ de présentation.

API :

```text
ShowNotification(View)
DismissNotification()
HasNotification()
```

Le widget :

- affiche titre et message ;
- applique une couleur d’accent selon la sévérité ;
- se masque automatiquement après `DurationSeconds` ;
- reste sans autorité gameplay.

## Matérialisation UMG prévue

Créer :

```text
/Game/GrimrockPrototype/Blueprints/UI/InGameMenu/RPG/WBP_RPGNotification
```

Parent C++ :

```text
UGridRPGNotificationWidget
```

Hiérarchie minimale :

```text
Border_NotificationRoot
└── HB_Notification
    ├── Border_NotificationAccent       VARIABLE
    └── VB_NotificationText
        ├── Text_NotificationTitle      VARIABLE
        └── Text_NotificationMessage    VARIABLE
```

Puis ajouter dans `WBP_GridSkills` une instance nommée exactement :

```text
Notification_Progression
```

Le binding est temporairement optionnel pour permettre la validation source avant modification des `.uasset`.

## Tests

```text
Grimrock.UI.RPG05.Feedback.RejectMessages
Grimrock.UI.RPG05.Feedback.TalentSuccess
Grimrock.UI.RPG05.Feedback.TalentFailure
Grimrock.UI.RPG05.Feedback.LevelUp
Grimrock.UI.RPG05.Notification.WidgetState
```

Validation :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.RPG05"
```

Attendu :

```text
Succeeded              : 5
Succeeded with warnings: 0
Failed                 : 0
```

## Invariants

- aucun calcul de coût dans UMG ;
- aucun calcul de prérequis dans UMG ;
- aucun calcul d’exclusivité dans UMG ;
- aucune seconde autorité de progression ;
- aucune duplication de traduction des rejets entre Level Up et Skills/Talents.
