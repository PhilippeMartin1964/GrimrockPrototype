# ARM-CLEAN06.5H.11 — Ancienne initialisation d'acteur entièrement réservée à Automation

**Parent :** `d52575c0afe15daddf19c6678ba56af9f953dd36` (publication seulement après SUCCESS/SUCCESS du parent).

Le premier H.9 a montré que les fixtures historiques GI-backed (VFX Torch, identité de pickup, restore de réceptacle) ont besoin d'une initialisation temporaire depuis `UGridItemDefinitionAsset`. Le H.9 corrigé l'a conservée lorsque `GIsAutomationTesting` est vrai, tout en refusant la vieille initialisation dans une session de jeu ordinaire avec GameInstance.

**Ce lot achève la séparation par compilation :**

- Le corps de `AGridItemActor::InitializeFromItemDefinition` qui lit les propriétés d'un ancien DataAsset n'est **compilé que sous `WITH_DEV_AUTOMATION_TESTS`**. Dans un build Game/Shipping, l'API existante demeure pour compatibilité binaire/Blueprint, mais l'implémentation est inerte et ne touche aucun acteur.
- En Editor, la méthode exige `FGridLegacyWorldActorInitGate::Allows(GIsAutomationTesting)`; lancer simplement un PIE ne donne pas accès à l'ancien lecteur, avec ou sans GameInstance.
- Les fixtures Automation historiques conservent l'ancien comportement avec ou sans GameInstance ; la projection vers le registre canonique continue d'être contrôlée par les consommateurs existants. Les appels du monde à `InitializeFromCanonicalItem` ne changent pas.

Une nouvelle Automation `Grimrock.RPG.ARM.CLEAN06_5H.OldWorldActorInitializationGate` contrôle la distinction Editor normal/Automation. Les tests existants de la torche, des réceptacles et du pickup couvrent la matérialisation de l'ancien acteur en vraie Automation.

**Limites :** pas de retrait de la classe UPROPERTY, d'assets, de SaveGames, ni de changement UI. Le Cook/Shipping n'est pas prouvé par `-NullRHI` et doit faire partie de la validation de clôture.
