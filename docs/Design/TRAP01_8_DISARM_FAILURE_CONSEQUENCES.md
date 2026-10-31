# TRAP01.8 — Conséquences des échecs de désamorçage

**Statut : implémentation C++ sur `master`, tests UE5.5.4 / PIE à exécuter.**

## Contrat de jeu

`FRPGSkillCheckService::TryResolveBestPartySkillCheck` reste l'autorité de la
sélection du meilleur porteur d'outils, du d20, de la DEX, des rangs Pièges,
des bonus de classe et de groupe, du DC et de `bSafeFailure`.

Une fois le contrôle d'éligibilité validé (piège détecté, portée légale,
outils personnels, compétence, pas de combat), un seul jet aboutit :

| Résultat | Effet |
|---|---|
| Total supérieur ou égal au DC | Désamorçage, événement `Disarmed`, XP une seule fois |
| Échec de 1 ou 2 points avec talent `Désamorçage expert`, première fois pour ce GUID | Échec sûr : reste détecté/armé, aucun dégât, aucun Activated |
| Autre échec, ou protection déjà dépensée | Échec dangereux : **déclenchement réel** du piège et effets normaux |

Le talent de Saboteur n'est **pas** une garantie permanente.
`SafeDisarmFailureUsedTrapIds` (dans `FGridLevelRuntimeState`,
`UPROPERTY(SaveGame)`) mémorise sa première utilisation par GUID et par
niveau, et empêche de la récupérer après Save/Continue. Si la protection
avait déjà été consommée, un nouvel échec de 1 ou 2 points n'est plus
`bSafeFailure` dans le résultat effectif.

## Activation et isolation

Une seule routine C++ `TryActivateDungeonTrap` fait autorité pour
une entrée réelle dans la case **et** un échec dangereux de désamorçage.
Elle réutilise strictement les règles existantes : contrat `OnEnterCell`,
vérification du GUID et des états `Disarmed`/`Disabled`/`Triggered`,
jet de dégâts partagé puis résistances de chaque membre, état persistant,
`Activated` et `PIT03`.

Un désamorçage devant la fosse peut ouvrir sa trappe sur la case adjacente,
mais **ne téléporte pas le groupe** vers celle-ci ; la chute ne commence
que si la troupe occupe réellement la cellule de la fosse.
Les pièges `bNoDamage` n'appliquent toujours aucun dégât direct, même
sur erreur de désamorçage.

L'échec **ne** donne **aucune XP**, ne consomme pas les outils et ne marque
pas `Disarmed`. Un piège à usage unique devenu `Triggered` ne peut plus
être désamorcé/activé une seconde fois. Un piège non unique reste activable
sur une nouvelle entrée ou un nouveau désamorçage autorisé.

## UI

Le format canonique à trois lignes de TRAP01.7.1 et les cinq termes sont
inchangés. Seule la conséquence de la ligne finale diffère :

- Réussite : `Difficulté : 14 — Réussite !`
- Échec sûr : `Difficulté : 14 — Échec sûr ! Le piège reste armé.`
- Échec déclencheur : `Difficulté : 14 — Échec ! Le piège se déclenche !`

La dernière mention n'est utilisée que si l'activation est effectivement
confirmée ; sinon le verdict reste simplement `Échec !`.
Les deux interactions (clic sur le piège détecté / outil dans le sac)
transmettent le même indicateur métier au formateur ; aucun autre jet.

## Validation

```powershell
cd D:\Development\GrimrockPrototype
git status --short
git pull --ff-only origin master
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.TRAP01.8"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.TRAP01"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.TRAP_PIT01"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.ARMEMENT01.16.TrapCore"
```

Recette PIE : tentative sûre d'un Saboteur, seconde tentative après
Save/Continue, tentative dangereuse depuis une case adjacente, dégâts
réels et mécanisme PIT03, absence de retentative d'un piège one-shot,
source d'outils valide et aucune information sur un piège caché.

**La validation locale UE/PIE n'est pas présumée.**
