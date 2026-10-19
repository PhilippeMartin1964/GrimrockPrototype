# UI-RPG-CLEAN02 — Nettoyage des notifications de niveau devenues obsoletes

Date : 2026-10-19
Statut : **code publie, validation locale UE5.5.4 attendue**.

## Decision

**WBP_RPGNotification doit etre conserve.** Il est reference par
`WBP_GridPersistentHud` et `WBP_GridSkills` (UI-WBP-AUDIT02 valide), et
reste utilise pour afficher les resultats et refus de transactions :

- Achat / remboursement de points de caracteristique ;
- Achat / remboursement de rang de competence ;
- Acquisition (ou rejet) de talent.

Le retrait des toasts **Level Up** et leur remplacement par des badges
persistants de portrait (PARTYHUD02) puis par un rappel dans le Flux
(EVENTHUD01) ne suppriment pas ces besoins.

## Code mort supprime

`FRPGProgressionFeedbackService::MakeLevelUpNotification()` etait
appele uniquement par les tests historiques ; aucun appel dans les
sources de production. Son declaration et sa definition sont retirees,
avec les chaines LOCTEXT exclusivement associees a ce toast.

Les anciens tests de **contenu et duree du toast de niveau** sont
remplaces, en conservant le nombre de tests :

- `Grimrock.UI.RPG05.Feedback.LevelUp` ->
  `Grimrock.UI.RPG05.Feedback.SkillAndAttribute` :
  confirmation de competence et rejet d'attribut ;
- `Grimrock.RPG.LEVELUX01.Feedback.SkillOnlyLevel` ->
  `Grimrock.RPG.LEVELUX01.Progression.OddLevelBalances` :
  soldes canoniques au niveau 5, plafond de rang et badge de portrait ;
- `Grimrock.RPG.LEVELUX01.Feedback.SkillAndTalentLevel` ->
  `Grimrock.RPG.LEVELUX01.Progression.EvenLevelBalances` :
  soldes canoniques niveau 6 et badge ;
- `Grimrock.RPG.ATTR01.Feedback.LevelFour` ->
  `Grimrock.RPG.ATTR01.Feedback.Allocation` :
  confirmation d'attribution de caracteristique.

On ne supprime **aucune** classe `UGridRPGNotificationWidget`,
`FRPGProgressionNotificationView` ni ses consommateurs encore actifs.
Pas de modification de Blueprint ni de schema SaveGame.

## Validation locale requise

Fermer Unreal Editor, puis :

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.RPG05"

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.RPG.LEVELUX01"

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.RPG.ATTR01"

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.PartyHud01"

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.EventHUD01.ProgressionReminder"
```

La validation PIE doit conserver le feedback non modal pour les
attributions/rejets dans Skills/CharacterSheet, les badges sur les
portraits, et le message de rappel dans le Flux.
