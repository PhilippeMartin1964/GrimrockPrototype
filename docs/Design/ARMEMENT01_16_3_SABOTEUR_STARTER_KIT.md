# ARMEMENT01.16.3 — Finalisation du kit Voleur/Saboteur (niveau 1)

**État : compilation et 2/2 tests ARMEMENT01.16.3 validés localement le 01.11.2026 ; régression ARMEMENT01.16 en cours de remise en conformité, recette PIE complète à confirmer.**

## Contexte

Après TRAP01 / TRAP-SEARCH01 validés et ASSET-SYNC01 (`26ba6678`), le DataAsset
`/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_DisarmTools_01`, son icône,
son mesh et ses textures sont déjà versionnés. L'arme ARM003 N1 (stylet) et le
runtime de désamorçage fonctionnent. Le seul élément absent de
`Kit_Rogue_Saboteur` était **la trousse réellement attribuée au personnage**.

## Changement de gameplay (ciblé)

- Source unique `FRPGStartingEquipmentPresetCatalog::ApprovedKits` :
  `DA_Weapon_Stiletto_01` en `MainHand` + **un `DA_Item_DisarmTools_01` dans le sac**,
  dans le même kit `Kit_Rogue_Saboteur`.
- La transaction de départ utilise `FRPGStartingEquipmentService::TryBuildKit`
  comme les 17 autres kits ; GUID unique, propriétaire et slot corrects, et refus
  atomique si l'asset est absent ou si le sac est plein.
- Il ne s'agit ni d'une nouvelle arme, ni d'une potion, ni d'un QuickItem :
  `Item_DisarmTools_01` conserve `ItemTags={Tool.Disarm}`, ne se porte pas en main
  et **n'est pas consommé** après un désamorçage.
- Les classes `DA_Class_*`, `ST_UI`, `UI.csv`, les WBP, les cartes, les
  pièges et les 17 autres recettes ne sont pas modifiés. Les anciennes
  sauvegardes continuent d'utiliser leurs identités exactes ; aucun équipement
  n'est injecté rétroactivement dans un personnage déjà créé.
- La résolution `ResolveKnownStartingItemDefinition` parcourt déjà les
  quatre entrées de chaque kit approuvé : la trousse est désormais résolue par
  son identifiant exact pour Save/Continue, sans ajout de fallback arbitraire.

## Usage N1 : compétence et non talent

Le niveau 1 donne **4 Skill Points**, mais le kit n'accorde ni
`Skill_Traps` automatiquement ni le talent `ExpertDisarm` du niveau 2.
Un Saboteur doit dépenser au moins **un point** dans `Skill_Traps` via
l'économie `FRPGSkillPointService` pour être entraîné. Sans entraînement,
le désamorçage est refusé sans d20 ; le joueur conserve sa trousse.
Avec un rang, une cible découverte et accessible peut être désamorcée via le
moteur TRAP01.4. L'outil demeure dans le sac après le succès.

Cette distinction conserve la liberté d'allocation des 4 points N1 et
l'interdiction de donner gratuitement des talents.

## Validation automatique

Deux nouveaux tests dans
`GridARMEMENT01163SaboteurStarterTests.cpp` :

1. `Grimrock.RPG.ARMEMENT01.16.3.SaboteurKitOnly` :
   asset réel, tag, quantité, emplacement dans le sac, stylet inchangé,
   aucune trousse accordée aux deux autres orientations Rogue, aucune mutation
   du DataAsset de classe.
2. `Grimrock.RPG.ARMEMENT01.16.3.N1DisarmAndSaveContinue` :
   New Game avec vraie classe/race de production, allocation réelle d'un point
   de compétence, refus non entraîné sans RNG, réussite de désamorçage au niveau 1,
   trousse non consommée, SaveGame binaire + restauration du groupe et
   de l'état du piège.

Avec Unreal Editor fermé :

```powershell
cd D:\Development\GrimrockPrototype
git status --short
git pull --ff-only origin master
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.16.3"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.ARMEMENT01.13.ApprovedKits"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.ARMEMENT01.16"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.TRAP01"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.TRAP_SEARCH01"
```

## Recette PIE

1. `New Game` → Voleur → Saboteur : **stylet équipé + trousse dans le sac**.
2. Comparez Assassin et Ombre : pas de trousse supplémentaire.
3. Sans rang Pièges : le désamorçage est indisponible, sans consommation.
4. Ouvrez Compétences, investissez **un point N1 dans Pièges** ; découvrez un
   piège par Perception ou recherche F, puis désamorcez avec la trousse.
5. Réussite : piège désarmé, trousse conservée et événement approprié.
6. `Save → Quitter → Continue` : même GUID, même trousse, rang appris et
   état du piège persisté.
7. Vérifiez que les 4 points N1 n'ont pas été attribués implicitement et
   qu'aucun talent N2 n'a été offert.

## Contrat UI et localisation

**Aucun nouveau texte joueur, composant UMG ou asset Unreal.**
`UI.csv → ST_UI → WBP → Game/fr/en` : les quatre étapes sont **inchangées**.
Les libellés existants et les traductions Game/fr/en ne sont pas altérés.
Les contrôles globaux de traduction anglais/français restent ceux du projet.

Ne clore le jalon qu'après build, tests et recette PIE utilisateur.


## ARMEMENT01.16.3.1 — Adaptation des tests historiques après validation locale (01.11.2026)

Le journal local confirme :
- `Grimrock.RPG.ARMEMENT01.16.3` : **2/2 réussis**.
- `Grimrock.RPG.ARMEMENT01.13.ApprovedKits` : **2 réussis, 1 warning** ; zéro échec.
- `Grimrock.TRAP01` : **17/17 réussis**.
- `Grimrock.TRAP_SEARCH01` : **4/4 réussis**.
- `Grimrock.RPG.ARMEMENT01.16` : **5 réussis, 2 échecs historiques** à adapter.

Causes des deux échecs :
1. Le test Editor `NoPrematureKitGrant` affirmait que la trousse ne
   devait pas être donnée au Saboteur. Cette règle a été explicitement
   remplacée par ARMEMENT01.16.3. Le test devient `ApprovedKitGrant` :
   il exige la trousse réelle, non équipée, tout en vérifiant le stylet
   et l'absence de corruption d'ownership.
2. Le test runtime `TrapCore.SkillAndPersistentState` sélectionnait le
   Saboteur, puis postulait un inventaire sans trousse. Il sélectionne
   désormais `Kit_Rogue_Shadow`, sans trousse, pour vérifier le refus
   **sans d20**, puis ajoute le `DA_Item_DisarmTools_01` réel pour
   vérifier l'échec, la réussite et la persistance du désamorçage.

**Pas de changement du gameplay du kit ni des DataAssets.**
Le test spécifique ARMEMENT01.16.3 continue d'assurer le scénario
New Game du Saboteur entraîné au niveau 1 et le SaveGame binaire.

Réexécuter après pull et build : `Grimrock.RPG.ARMEMENT01.16`,
`Grimrock.RPG.ARMEMENT01.16.3`,
`Grimrock.RPG.ARMEMENT01.13.ApprovedKits`.
Le warning du groupe `ApprovedKits` n'a pas été modifié ; ne pas
annoncer sa disparition sans preuve de validation locale.
