# EVENTHUD01.XP01 — Notifications globales de gains d'expérience

31 octobre 2026 — C++ et tests ajoutés sur master ; validation locale UE 5.5.4 à exécuter.

## Contrat de présentation validé

Tout **gain d'expérience effectivement appliqué** au groupe (monstre, piège, futures quêtes) est publié dans le flux d'événements sous une sémantique dédiée `Experience`.

- Couleur par défaut : `#72FF42` (vert vif), `FLinearColor::FromSRGBColor(FColor(0x72, 0xFF, 0x42))`, modifiable depuis la palette C++/UMG `FGridEventFeedSemanticPalette::Experience`.
- Distinct des sémantiques `Poison`, `Healing` et `Buff`.
- Catégorie : `Dungeon` / filtre **Events**. Le flux RPG ne peut donc pas dépendre de l'affichage du combat.
- Code stable : `RPG.Experience.Awarded`.
- Une seule entrée verte **par transaction de groupe**, indépendamment du nombre de personnages.
- Montant affiché : XP réellement accordée (après plafonnement), jamais montant initialement demandé.
- Partage égal : « +20 XP pour le groupe (+5 XP par personnage). »
- Partage inégal : « +10 XP pour le groupe (répartis entre 4 personnages). »
- Si 0 XP appliqué, **aucune entrée** (même si le monstre meurt ou le piège est désamorcé).
- Pas de couleur comme unique signal : montant positif explicite, « XP » et répartition.
- La publication est indépendante d'un éventuel groupe de conséquences de mécanismes ; elle ne devient pas une clause orange d'un événement `Mechanism`.

## Architecture

`FRPGExperienceRewardService::AwardToActiveParty` constitue l'unique autorité de mutation RPG.

Après avoir appliqué l'expérience aux personnages, les éventuelles montées de niveau et les notifications natives MON15.2, il transmet une **seule** notification à `GridEventFeedPublisher::PublishPartyExperienceAward`.

Cette méthode fabrique une `FGridHudEventEntry` de semantic `Experience` et appelle directement `PublishEvent`, **pas** `PublishDungeonEvent`, car cette dernière peut absorber le texte dans `UGridEventFeedSubsystem::DungeonEventGroup` et perdre la couleur verte.

Aucun branchement à faire sur chaque monstre, chaque piège ou chaque WBP. Les producteurs de récompense appellent toujours `AwardToActiveParty` et **n'émettent aucun événement XP supplémentaire**.

Le rendu des entrées dans `UGridEventFeedWidget::AppendEntryPresentation` existant lit déjà `GetSemanticColor(Entry.Semantic)` : l'ajout d'une valeur au enum et à la palette suffit, sans modifier le Blueprint.

## Tests

- `Grimrock.UI.EventHUD01.XP01.ExperiencePublication` : monde transitoire, quatre personnages dont un inconscient, partage égal puis inégal, événement unique de groupe, couleur, code d'événement, no-op 0 XP, publication indépendante d'une conséquence de mécanisme.
- `Grimrock.UI.EventHUD01.SemanticColors.Palette` et `Defaults` sont étendus à la nouvelle catégorie.
- `Grimrock.RPG.MON15.2` valide la compatibilité de toutes les anciennes sources XP.
- `Grimrock.TRAP01.4` valide le désamorçage et son gain d'expérience via le service.
- Aucune vérification de WBP ni de rendu PIE visuel n'est revendiquée avant validation locale.

## Validation locale recommandée

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.EventHUD01.XP01"

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -SkipBuild `
    -AutomationFilter "Grimrock.UI.EventHUD01.SemanticColors"

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -SkipBuild `
    -AutomationFilter "Grimrock.RPG.MON15.2"

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -SkipBuild `
    -AutomationFilter "Grimrock.TRAP01.4"
```

La validation du commit TRAP01.4.1 doit également inclure `Grimrock.TRAP01.4.1`.
