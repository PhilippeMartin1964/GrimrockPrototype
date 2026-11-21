# ARM-CLEAN06.5H.7 — Garde sur la matérialisation physique dans les mondes configurés

**Parent :** `42f48e54fcd3c668c0396661398f2c70d1b31727` (publication uniquement après double statut Windows SUCCESS).

Le point de création `AGridLevelRuntimeActor::SpawnItemActorForDefinition` acceptait un pointeur `UGridItemDefinitionAsset` dans n'importe quel `UWorld`, puis la définition historique pouvait imposer son `ItemDefinitionId`. Or les appels de production actuels de la serrure, du curseur tenu, des réceptacles et du restore persistent passent déjà un `ItemId` canonique **sans pointeur historique**. Le chemin historique existe pour des fixtures qui ne possèdent pas de `GameInstance`.

**Modification :** dès que le `UWorld` possède un `GameInstance`, l'acteur ne peut être créé que si le `UGridItemSubsystem` est configuré, que l'identité demandée existe dans le registre et qu'aucun pointeur historique n'est fourni. Si l'un des critères manque, le code refuse le spawn **avant création de l'acteur**. Dans les worlds de fixtures sans `GameInstance`, le comportement historique déjà documenté reste possible.

Le scénario Automation existant `Grimrock.RPG.ARM.CLEAN06_3.CanonicalWorldPickupIdentity.RegisteredLevelGuards` couvre désormais trois cas supplémentaires : ancien asset rejeté malgré un ItemId valide, ItemId inconnu rejeté, et ItemId canonique accepté sans old UObject.

**Exclusions :** aucune modification d'asset, niveau, puzzle, sauvegarde, action ou gameplay physique ; pas de réécriture des GUID. La suppression des assets historiques exige un autre chantier natif Unreal, un audit privé complet et des garanties Cook/Save/Continue.

**Tests :** suite `Grimrock.RPG.ARM.CLEAN06_3.CanonicalWorldPickupIdentity` et validation automatique Windows du SHA exact.
