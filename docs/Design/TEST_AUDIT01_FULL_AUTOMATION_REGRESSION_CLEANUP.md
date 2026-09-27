# TEST-AUDIT01 — Full automation regression cleanup

Date: **27 septembre 2026**

## Scope

Audit du run global de 962 tests après les nettoyages UI et les normalisations
de schéma récentes.

Le run fourni contient 8 échecs. L'audit n'a identifié aucune régression runtime
à corriger dans ces 8 cas : chaque échec provient d'une fixture ou d'une attente
historique qui ne représente plus le contrat courant.

## Corrections

### MON10 IdleVariationRestoreSilent

La fixture restaurait `Alert` sans connaissance de la position du groupe.
Le runtime MON14.3 normalise volontairement cet état orphelin vers `Idle`, ce
qui planifie légitimement une variation Idle.

La fixture restaure désormais une `LastKnownPartyCell` pour le cas Alert afin
de tester réellement le contrat annoncé : une restauration Alert valide ne
rejoue aucune variation Idle.

### MON13.5 RealPIEIntegration

La sauvegarde temporaire créée par le test utilisait une hotbar vide. Le schéma
courant exige au moins `FGridCombatHotbarBinding::MinimumSlotCount` et
`PrimaryAttack` au slot système.

La fixture écrit désormais une hotbar courante valide. Le runtime de chargement
n'est pas assoupli.

### TD01.3 EventCommandContract

Les tests dataient d'avant `RELOC-ACT01` et classaient encore Relocation comme
une cible StateOnly non authorable.

Le contrat courant est :

- Relocation Activate / Deactivate / Toggle = Gameplay ;
- ItemSpawn et Light restent StateOnly et ne possèdent pas de handler gameplay.

Les tests Policy, Validation et RuntimeHardening sont réalignés sur cette
distinction. RuntimeHardening vérifie aussi qu'un lien Relocation valide reste
appliqué lorsque des liens StateOnly voisins sont rejetés.

### TD07.3.3.2 RecruitmentUsesCurrentAttributes

La fixture créait des inventaires de 4 slots mais conservait la capacité globale
par défaut de 40 slots. Le service de recrutement rejetait donc correctement le
candidat.

La fixture configure explicitement la capacité de 4 slots et protège l'accès au
personnage recruté lorsque la transaction échoue.

### UI Inventory01 SelectedBagProjection

Le test injectait directement `InventoryComponent` dans le widget et contournait
ainsi `InitializeInventoryWidget()`, donc l'abonnement à
`OnPartyInventoryChanged`.

Le test utilise désormais un vrai `AGrimrockPartyPawn` dans un monde transient
et initialise le widget par son API publique. Il vérifie ainsi le chemin runtime
réel : sélection -> notification -> reprojection du sac.

### MOVINGPARTS01 RealAssetMigration

Le test supposait qu'il existerait toujours exactement 38
`UGridWorldObjectDefinitionAsset`. Le projet en contient maintenant davantage ;
ce nombre global ne fait pas partie du contrat MovingParts.

Le test conserve les comptes exacts pour les assets historiquement migrés,
vérifie qu'ils sont tous toujours présents et valide `IsDefined()` pour chaque
MovingPart de tous les assets courants. L'ajout légitime de nouveaux DataAssets
ne fait plus échouer la migration historique.

## Invariants conservés

Aucun code gameplay/runtime n'est modifié par TEST-AUDIT01.

Les tests ne sont pas supprimés : les assertions sont recentrées sur leurs
contrats fonctionnels actuels et les fixtures utilisent le schéma courant.

## Validation locale demandée

Exécuter d'abord :

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Monsters.MON10.IdleVariations"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Monsters.MON13.5"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.TechnicalDebt.TD01_3"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.TechnicalDebt.TD07_3_3_2"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.Inventory01"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.WorldObjects.MOVINGPARTS01"
```

Puis relancer le run global afin de confirmer zéro échec sur l'ensemble de la
suite.
