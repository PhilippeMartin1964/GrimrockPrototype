# TD07.5 — Suspended Test Infrastructure / Branch Recovery

Date : 28 août 2026
Projet : GrimrockPrototype — Unreal Engine 5.5.4
Statut : VALIDÉ — CLOS

## 1. Objectif

Éliminer les reliquats de branches/tests suspendus sans perdre de couverture utile.

Règles :

- master reste l'unique branche de travail ;
- aucune vieille branche ne sera fusionnée ;
- récupérer uniquement une intention/test encore utile ;
- ne pas réintroduire de vieux assets/maps de test si une Automation C++ transiente suffit ;
- les branches historiques dont le travail est déjà matérialisé sur master sont supprimables après validation.

## 2. Audit GitHub initial

Au 28 août 2026, le dépôt contient 17 branches hors master.

### Branches derrière master, aucun commit exclusif

~~~text
codex/cc6-4-final-repair-base
codex/character-creation-cc0-tests
codex/character-creation-cc1-rpg-model
codex/character-creation-cc2-creation-api
codex/character-creation-cc3-startup-widget
codex/rpg-damage-countermeasures
codex/tmp-cc6-3-squash-base
codex/tmp-full-body-inventory-docs
feature/readables-per-instance-content
~~~

Ces 9 branches ont ahead=0 par rapport à master : aucune récupération de commit n'est nécessaire.

### Branches divergentes

~~~text
Test-Receptacle
codex/cc6-4-class-visuals-final-temp
codex/cc6-4-class-visuals-squash-temp
codex/tmp-cc6-4-inventory-class-icon-fix
codex/tmp-cc6-5-visual-polish
td06-1-party-inventory-rebaseline
td06-5-api-materialize
td06-5-materialize
~~~

Les branches CC6/TD06 sont anciennes mais leurs sorties structurantes existent aujourd'hui sur master, notamment :

~~~text
GridTD062PartyInventoryHotbarTests.cpp
TD06_1_PARTY_INVENTORY_REBASELINE.md
TD06_2_PARTY_INVENTORY_HOTBAR_CHARACTERIZATION.md
GridPartyInventoryComponentCursorTransfer.cpp
TD06_5_PARTY_INVENTORY_CURSOR_TRANSFER_EXTRACTION.md
RPGClassVisualAsset.h
GridInventoryWidgetClassIcon.cpp
CHARACTER_CREATION_CC6_5_UE5_CHECKLIST.md
~~~

Elles ne doivent pas être mergées.

Le workflow expérimental .github/workflows/td06-5-api-materialize.yml n'existe pas sur master. La validation autoritaire reste les harness locaux TD04.

## 3. Cas Test-Receptacle

Test-Receptacle :

~~~text
ahead  = 1
behind = 1021
tip    = 046d79f6fb "1st step test receptacle"
~~~

Cette branche contient uniquement des assets/maps de test historiques.

La persistance Receptacle moderne est déjà couverte par GridTD011ReceptaclePersistenceTests.cpp.

Mais l'audit a trouvé une lacune : les quatre commandes Event -> Command Receptacle n'avaient pas de test dédié :

~~~text
ReceptacleConsumeItem
ReceptacleConsumeAllItems
ReceptacleEnableRemoval
ReceptacleDisableRemoval
~~~

TD07.5 récupère donc cette intention de test sous forme d'une fixture C++ transiente, sans restaurer les vieux assets.

## 4. Recovery gate

~~~text
Grimrock.TechnicalDebt.TD07_5.Recovery.ReceptacleCommands
~~~

Le test vérifie les quatre commandes via la voie publique :

~~~text
AGridLevelRuntimeActor::ExecuteLinksFromRuntimeObject
    -> UGridActivationComponent
    -> ApplyReceptacleLinkCommand
~~~

## 5. Audit reproductible

Script read-only :

~~~text
Scripts/AuditTD075RemoteBranches.ps1
~~~

Il :

- exécute git fetch --prune origin ;
- compare toutes les branches distantes à origin/master ;
- écrit Saved/Diagnostics/TD07/TD07_5_RemoteBranchAudit.txt ;
- ne supprime ni ne modifie aucune branche.

## 6. Stop condition TD07.5

- [x] Recovery.ReceptacleCommands vert ;
- [x] audit local des branches concorde avec l'audit GitHub ;
- [x] aucune branche divergente ne contient de travail fonctionnel à récupérer ;
- [x] branches historiques supprimées de origin ;
- [x] master reste l'unique branche de travail active ;
- [x] documentation mise à jour.

Aucune fusion de branche n'est prévue.


## 7. Recovery gate validé

Validation locale du 28 août 2026 :

~~~text
Grimrock.TechnicalDebt.TD07_5.Recovery.ReceptacleCommands
Succeeded              : 1
Succeeded with warnings: 0
Failed                 : 0
Process exit code       : 0
Report                 : Saved/Automation/TD04/TD04-20260828-113434
~~~

La valeur utile de l'ancienne branche Test-Receptacle est désormais récupérée sous forme d'Automation C++ actuelle.

## 8. Nettoyage définitif préparé

Script :

~~~text
Scripts/CleanupTD075RemoteBranches.ps1
~~~

Le script :

- exige que la branche locale courante soit master ;
- exécute git fetch --prune ;
- compare les branches distantes à une liste blanche exacte de 17 branches auditées ;
- refuse d'agir si une branche inattendue apparaît ou si la liste diffère ;
- supprime les 17 branches historiques une par une via git push origin --delete ;
- exécute ensuite un nouvel audit TD07.5.

Le pseudo-ref remote HEAD raccourci en "origin" est explicitement exclu de l'audit et du cleanup.


## 9. Clôture

Nettoyage exécuté le 28 août 2026 :

```text
17 branches historiques supprimées de origin.

Post-cleanup audit:
Remote branches excluding base: 0
Behind-only: 0
Diverged: 0
Ahead-only: 0
```

Vérification GitHub finale : `master` est la seule branche du dépôt.

Les scripts `AuditTD075RemoteBranches.ps1` et `CleanupTD075RemoteBranches.ps1` étaient des outils one-shot ; ils ont été supprimés après usage.

Le test `GridTD075ReceptacleCommandRecoveryTests.cpp` reste durable pour protéger les quatre commandes Event -> Command Receptacle récupérées.
