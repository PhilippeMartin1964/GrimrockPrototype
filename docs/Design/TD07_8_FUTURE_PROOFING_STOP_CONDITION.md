# TD07.8 — Future-Proofing Re-Audit / Stop Condition

Date : 28 août 2026
Projet : GrimrockPrototype — Unreal Engine 5.5.4
Parent : TD07 — Future-Proofing
Statut : VALIDÉ — STOP CONDITION ATTEINTE

## 1. Objet

TD07.8 est la tranche de clôture de la campagne technique TD07.

Elle ne doit pas ouvrir un nouveau refactor. Elle vérifie que les décisions TD07.1 à TD07.7 restent cohérentes ensemble et qu'aucune dette résiduelle ne justifie de prolonger la campagne avant la reprise de MON21.4.

## 2. Re-audit transversal

### TD07.1 — Build / Dependency Reproducibility

Statut : **VALIDÉ / CLOS**

Contrat courant :

- Meshy reste optionnel, désactivé et non requis pour un clone propre ;
- `CheckProjectDependencies.ps1` reste le gate de dépendances ;
- la toolchain MSVC non préférée reste classée `TD-BUILD-002`, P2 surveillée, car Editor et Shipping sont verts.

### TD07.2 — UE deprecation / warning audit

Statut : **VALIDÉ / CLOS**

Contrats :

- API Skeleton dépréciée supprimée du périmètre first-party concerné ;
- collision Python ItemTransfer résolue via `ScriptName` ;
- warnings Engine/plugin/toolchain externes restent classés, sans modification opportuniste.

### TD07.3 — Prototype Data Model Reset

Statut : **VALIDÉ / CLOS**

Contrats :

- SaveGame v22 exact-match ;
- aucune migration arrière prototype ;
- autorités Character/Authoring/Combat normalisées ;
- assets courants réparés ;
- gate `TD07_3_8.StrictCurrentSchema` = 5 tests.

### TD07.4 — ActivationComponent characterization

Statut : **VALIDÉ / CLOS SANS EXTRACTION**

Décision maintenue :

- un seul bus Event -> Command ;
- une seule autorité Activation ;
- aucune extraction sans nouveau signal concret ;
- TD07.7 a remplacé les 38 `LogTemp` du composant par `LogGridActivation`.

### TD07.5 — Suspended test infrastructure / branch recovery

Statut : **VALIDÉ / CLOS**

État courant GitHub :

- `master` est la seule branche ;
- aucune branche historique n'a été mergée ;
- le test Receptacle récupéré reste durable ;
- les scripts one-shot TD07.5 ont été supprimés.

### TD07.6 — Legacy asset/API cleanup audit

Statut : **ABSORBÉ PAR TD07.3**

Aucune tranche supplémentaire nécessaire.

### TD07.7 — Targeted log / formatting hygiene

Statut : **VALIDÉ / CLOS**

Résultat :

- Activation ne dépend plus de `LogTemp` ;
- le nettoyage global des 44 autres fichiers `LogTemp` n'est volontairement pas ouvert ;
- baseline clang-format 19.1.5 : **568/568 conforme** ;
- scripts one-shot TD07.7 supprimés.

## 3. Dette restante après re-audit

Le registre courant ne contient plus aucune dette P0/P1 active.

Restent :

```text
P0 : 0
P1 : 0
P2 : 9 surveillées / différées
P3 : 2 opportunistes / fonctionnelles
```

Les P2 restantes sont des risques surveillés ou différés avec stop condition explicite :

- toolchain MSVC non préférée ;
- RuntimeActor, PartyInventory, PartyPawn déjà sous stop condition ;
- PlayerController sans signal de split ;
- ActivationComponent caractérisé ;
- complexité Editor surveillée ;
- taxonomie LogTemp opportuniste par domaine touché ;
- vraie CI UE différée jusqu'à disponibilité d'un runner réel.

Aucune ne bloque une nouvelle tranche fonctionnelle.

Les P3 sont opportunistes et ne justifient pas une prolongation de TD07.

## 4. Incohérences documentaires corrigées

TD07.8 corrige les autorités courantes qui étaient devenues obsolètes pendant la campagne :

- compteur P1 du registre ;
- statut global TD07 ;
- baseline formatting désormais réellement verte ;
- prochain travail recommandé ;
- roadmap qui indiquait encore TD07.4 comme prochaine tranche ;
- synthèse dont la dernière tranche validée restait TD07.3.

Les documents de jalon historiques restent des snapshots datés et ne sont pas réécrits rétroactivement.

## 5. Gate final TD07.8

Script one-shot :

```text
Scripts/ValidateTD078StopCondition.ps1
```

Il exige :

1. branche locale `master`, tracked working tree propre et synchronisé avec `origin/master` ;
2. aucune autre branche distante active ;
3. absence des scripts one-shot TD07.7 supprimés ;
4. `CheckProjectDependencies.ps1` vert ;
5. `CheckCppFormat.ps1` vert ;
6. Development Editor build vert ;
7. `Grimrock.TechnicalDebt.TD07_3_8.StrictCurrentSchema` vert ;
8. `Grimrock.TechnicalDebt.TD07_2` vert ;
9. `Grimrock.TechnicalDebt.TD07_4.Characterization` vert ;
10. `Grimrock.TechnicalDebt.TD07_5.Recovery.ReceptacleCommands` vert ;
11. `Grimrock.TechnicalDebt.TD01_3.EventCommandContract.RuntimeHardening` vert ;
12. `Grimrock.Quests.MON21_4.Characterization` vert ;
13. Win64 Shipping package vert.

## 6. Stop condition TD07

TD07 sera clos lorsque le gate final ci-dessus est entièrement vert.

Après validation :

- supprimer `Scripts/ValidateTD078StopCondition.ps1` ;
- marquer TD07.8 **VALIDÉ — STOP CONDITION ATTEINTE** ;
- marquer TD07 global **VALIDÉ — CLOS** ;
- lever la suspension de MON21.4 ;
- reprendre MON21.4 à partir de sa caractérisation existante, sans nouvelle phase d'audit redondante.


## 7. Validation finale

Gate exécuté localement le 28 août 2026 :

```text
Git branch / cleanup contract                    OK
TD07.1 project dependency contract              OK
TD07.7 global C++ format baseline               568/568 conforme
Development Editor build                        OK

Grimrock.TechnicalDebt.TD07_3_8.StrictCurrentSchema
    Succeeded              : 5
    Succeeded with warnings: 0
    Failed                 : 0

Grimrock.TechnicalDebt.TD07_2
    Succeeded              : 1
    Succeeded with warnings: 0
    Failed                 : 0

Grimrock.TechnicalDebt.TD07_4.Characterization
    Succeeded              : 4
    Succeeded with warnings: 0
    Failed                 : 0

Grimrock.TechnicalDebt.TD07_5.Recovery.ReceptacleCommands
    Succeeded              : 1
    Succeeded with warnings: 0
    Failed                 : 0

Grimrock.TechnicalDebt.TD01_3.EventCommandContract.RuntimeHardening
    Succeeded              : 1
    Succeeded with warnings: 0
    Failed                 : 0

Grimrock.Quests.MON21_4.Characterization
    Succeeded              : 4
    Succeeded with warnings: 0
    Failed                 : 0

Win64 Shipping
    BUILD SUCCESSFUL
    Cook                    : Success - 0 error(s), 0 warning(s)
    Pak files               : 1
    Archive files           : 41
    Archive bytes           : 905990059
    Archive                 : Saved/Packaging/TD04/TD04-Shipping-20260828-125341
```

Résumé du harness :

```text
Branch             : master only
Dependencies       : validated
C++ format         : validated
Strict schema      : validated
UE compatibility   : validated
Activation         : validated
Receptacle recovery: validated
Event->Command     : validated
MON21.4 assumptions: validated
Shipping           : validated
```

**TD07.8 a atteint sa stop condition.**

Le script one-shot `Scripts/ValidateTD078StopCondition.ps1` est supprimé après usage.

## 8. Clôture TD07

La campagne TD07 est **VALIDÉE — CLOSE**.

Aucune dette P0/P1 active ne subsiste. Les dettes restantes sont surveillées, différées ou opportunistes et ne justifient pas de prolonger la campagne technique.

Conséquence autoritaire :

**MON21.4 — Quest Persistence est techniquement débloqué, mais reste en attente du feu vert explicite de l'utilisateur avant toute reprise.**
