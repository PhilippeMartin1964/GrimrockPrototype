# RPG-SHIELD02.5 — Persistance réelle des réserves DOS2 (SaveGame disque)

**État : tests d'intégration et documentation publiés sur master ; compilation, automatisation UE5.5.4 et recette PIE locale en attente.**  
**Autorité :** [Contrat RPG-SHIELD02 v0.2](../Rules/RPG_Shields_Protection_Contract_v0_2.md) ; [politique de sauvegarde combat MON18.9.1](MON18_9_1_COMBAT_SAVE_POLICY.md).

## 1. Audit du circuit existant, sans migration de schéma

Le format actuel est `UGrimrockPartySaveGame::CurrentSaveVersion = 27` (au 26 octobre 2026). Il persiste `FGridPartyInventoryState`, notamment :

- `FGridCharacterInventoryState::Resources.CurrentPhysicalArmor` et `CurrentMagicalArmor` ;
- `FGridCharacterEquipmentState` et les `InventorySlots` de chaque personnage ;
- `FGridPartyInventoryState::CursorItem` ;
- pour chaque `FGridItemInstance`, `AbsorbedEquipmentPhysicalArmor`, `RuntimeObjectId`, `ItemLevel`, `ItemDefinitionId` et l'ownership.

`AGrimrockPartyPawn::SaveCurrentGame()` écrit ces données depuis le runtime, via `UGameplayStatics::SaveGameToSlot()`. `LoadCurrentGame()` relit le slot, puis `RestorePartyInventoryState()` et `RehydrateOwnedItemDefinitions()` rétablissent la session. **Aucune recharge automatique** ne doit être ajoutée au chargement : seules une victoire normale (RPG-SHIELD02.2) et un repos explicite réussi (RPG-SHIELD02.3) autorisent le refill.

Il n'est donc nécessaire **ni de créer une nouvelle propriété persistante, ni d'augmenter la version 27, ni de transformer l'armure en durabilité**. Les maxima restent des projections de classe/équipement, pas des maxima secondaires enregistrés sur disque.

### Politique MON18.9.1 conservée (ne pas autoriser Save en combat)

| Phase | Écriture normale | Résultat RPG-SHIELD02.5 |
| --- | --- | --- |
| Exploration | Oui | État exact des protections, y compris partiellement épuisées |
| Précombat | Checkpoint isolé `_AutoCombat` | Instantané AVANT les coups ; aucun rewrite au milieu du combat |
| PlayerPhase, EnemyPhase, EndingRound, StartingCombat | **Non** | SaveCurrentGame rejette ; le slot existant ne change pas |
| Defeat | **Non** | Checkpoint intact ; aucune régénération automatique |
| Victory stable | Oui | Réserves après refill de victoire présentes dans le snapshot |
| Repos explicite réussi, hors combat | Oui | Réserves après refill de repos présentes dans le snapshot |

**L'expression « Save/Load au milieu du combat » dans les premières intentions du contrat v0.2 est remplacée ici par le test d'interdiction MON18.9.1.** Le prototype ne sérialise pas les rounds, l'initiative ni les effets transitoires de combat.

## 2. Trois tests réels sur disque livrés

Namespace `Grimrock.RPG.SHIELD02.5` (**3 tests attendus**). Chaque test crée des slots `SHIELD025_<GUID>` isolés sous `Saved/SaveGames`, les efface en teardown, utilise un groupe finalisé avec classe et race de test, le vrai `DA_Weapon_RoundShield_01` (+5 physique), le véritable `UGrimrockPartySaveGame`, le Pawn, le niveau et le TurnManager.

1. **`ExplorationDiskRoundTripNoFreeRefill`** : état physique de classe 1/2, magique 1/3, bouclier N1 équipé 2/5 disponible (trois points absorbés), second shield de sac et troisième shield sur curseur épuisés différemment. `SaveCurrentGame` -> fichier disque -> modification volontaire de l'état RAM -> `LoadCurrentGame` -> vérification de tous les nombres, de l'autorité du DataAsset, du read-model 3/7 physique, des trois GUID, des PV/mana et du CharacterId. **Aucun refill parasite au chargement.**
2. **`CheckpointAndCombatSavePolicy`** : sauvegarde d'exploration puis `FGridCombatSavePolicy::PreparePreCombatCheckpoint` ; l'état du bouclier et de la magie du checkpoint est lu sur disque. Simulation de consommation d'armure pendant un combat actif ; `SaveCurrentGame` est **refusé** et laisse inchangés les slots principal et `_AutoCombat`.
3. **`PostRecoveryDiskRoundTrip`** : `TryRestParty()` autorisé, refill physique/magique, sauvegarde et rechargement des pools complets sans modification des PV/mana ; puis consommation, appel du service de récupération de victoire déjà testé en SHIELD02.2 sous phase `Victory`, sauvegarde après victoire, rechargement et vérification des valeurs pleines. Le test caractérise la persistance du résultat après une victoire, **pas le déclenchement du vrai événement de fin de rencontre**, couvert par SHIELD02.2 et sa recette PIE utilisateur.

Les cas des équipés/sacs/curseur et de leurs identités sont volontairement testés ensemble : une simple vérification du pool du personnage ne prouverait pas la durabilité de `AbsorbedEquipmentPhysicalArmor`.

## 3. Validation sur l'installation Windows UE5.5.4

Fermer Unreal Editor, préserver les quatre `.uasset` non committés. Exécuter :

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.SHIELD02.5"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.Save.MON18.9.1"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.Save.SAVEFIX.2"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.ARMEMENT01.9.SaveRehydrate"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.SHIELD02.2"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.SHIELD02.3"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.SHIELD02.4A"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.SHIELD02.4B"
git status --short
```

Attendus : **3/3 nouveaux tests**, puis aucune régression dans les suites précédemment validées. Relever les chiffres réels de chaque campagne ; ne jamais déclarer l'ensemble validé avant réception des logs locaux.

## 4. Recette PIE manuelle complémentaire

En partie réelle, vérifier l'exploration avec un bouclier N1 connu, une victoire rechargeant à 5/5, puis sauver et charger : 5/5 doit subsister sans bonus cumulatif. Dépenser l'armure en combat, constater le refus d'une sauvegarde manuelle tant que le combat est actif ; vérifier que le checkpoint `_AutoCombat` a été capturé avant l'engagement. Hors combat, déclencher explicitement `GrimrockRest` et vérifier la persistance de 5/5 après reprise.

Une sauvegarde contenant une armure **partiellement** épuisée hors combat est couverte par l'automatisation mais peut nécessiter un montage de test/objet récupéré : ce n'est pas le chemin normal après victoire, car celle-ci recharge déjà l'armure.

## 5. Limites et périmètre fermé

- **Aucun nouveau SaveGame schema/version**, aucune migration des sauvegardes historiques, aucun autosave supplémentaire.
- **Aucune sauvegarde en plein combat**, pas de round/initiative/cooldowns persistés ; MON18.9.1 reste autoritaire.
- Pas de modification de comportement `SaveCurrentGame`, `LoadCurrentGame`, `FinishCombat`, `TryRestParty`, `RestorePhysicalArmor` : leur contrat en production paraît correct à l'audit ; ce jalon ajoute des **preuves automatisées sur disque** et la documentation.
- Pas de mutation d'assets de production, d'UMG, de `ST_UI`, de bibliothèque ARMEMENT ou des classes de personnages.
- La revue des références physiques/magiques sous buffs temporaires ou profils d'objets N2–N20 relève d'un jalon de caractérisation séparé, si des données autoritaires sont disponibles.

**Livraison : un commit master, tests et documentation seulement. Réussite et clôture après logs UE5.5.4 utilisateur.**
