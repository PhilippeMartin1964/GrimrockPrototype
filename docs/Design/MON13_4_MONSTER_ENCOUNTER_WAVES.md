# MON13.4 — Rencontres et vagues de monstres

## Objectif

MON13.4 regroupe des placements `MonsterSpawn` existants en rencontres
persistantes, sans créer un nouveau type d'objet de grille :

```text
Trigger.Activated
    → Rat_Wave0_A.StartEncounter
    → vague 0 atomique
    → morts réelles de tous ses membres
    → vague suivante
    → EncounterCompleted
```

Le `MonsterSpawn` ciblé sert d'ancre logique. Son `EncounterGroupId` identifie
la rencontre et ses liens émettent les événements de vague et de fin.

## Données d'édition

Chaque `MonsterSpawn` possède :

- `EncounterGroupId` : `None` conserve le comportement individuel MON13.3 ;
- `EncounterWaveIndex` : indice entier à partir de zéro.

Tous les membres ayant le même groupe et le même indice appartiennent à la
même vague. Les indices peuvent comporter des trous : le runtime sélectionne
toujours le plus petit indice restant.

Règles de validation :

- l'indice ne peut pas être négatif ;
- un indice supérieur à zéro exige un groupe ;
- une vague future doit être désactivée au démarrage ;
- deux membres d'une même vague ne peuvent pas partager leur cellule de spawn.

Le panneau Selected Object expose directement `EncounterWaveIndex`.

## Commande et événements

La commande `StartEncounter` est disponible sur une cible `MonsterSpawn`
possédant un groupe. Elle est idempotente : la répéter pendant une vague active
ne crée ni doublon, ni second événement de démarrage.

Deux événements sont émis par l'ancre :

- `EncounterWaveStarted`, après création complète d'une vague ;
- `EncounterCompleted`, une seule fois après la dernière vague.

Les événements individuels MON13.3 restent inchangés. Chaque nouveau membre
émet notamment `MonsterSpawned` après validation de toute la vague.

## Progression et atomicité

Une vague avance uniquement après un `CommitDeath` réel de chacun de ses
membres. `Despawn` retire un Actor mais ne compte jamais comme une victoire et
n'ajoute pas son `SpawnId` aux identifiants vaincus.

La création d'une vague est transactionnelle : tous les placements sont
validés et créés avant l'émission des événements. Si un membre ne peut pas
apparaître, les Actors déjà créés par cette tentative sont détruits et les
maps `Monsters`/`MonsterPlacements` ainsi que l'état de rencontre sont remis à
leur valeur précédente. La commande peut alors être retentée.

Une vague réussie interrompt le combat courant, comme les mutations de
population MON13.3, afin que l'initiative soit reconstruite depuis la
population réelle.

`StartEncounter` crée la population de la vague, mais ne force pas le début du
combat. Dans la configuration actuelle, `NumPad 1` demande un combat avec les
monstres qui perçoivent le groupe et `NumPad 5` constitue la commande de debug
avec tous les monstres vivants. Le déclenchement automatique du combat lors de
la création d'une vague serait un contrat gameplay distinct.

## Persistance

`FGridLevelRuntimeState::MonsterEncounters` conserve par groupe :

- l'identifiant de l'ancre ;
- la vague active ;
- les états commencé/terminé ;
- les `SpawnId` réellement vaincus.

Cet état est sérialisé avec `SaveGame`. Un `Continue` normal restaure donc la
progression, tandis qu'un playtest frais préparé par le Grid Editor continue
d'ignorer l'état de donjon sauvegardé conformément à MON13.3.

## Automation Tests

MON13.4 ajoute :

- `Grimrock.Monsters.MON13.4.EncounterWaves` ;
- `Grimrock.Monsters.MON13.4.AtomicWaveFailure` ;
- `Grimrock.Monsters.MON13.4.Validation` ;
- `Grimrock.Monsters.MON13.4.EditorLinkPolicy`.

Commande UE 5.5.4 :

```bat
D:\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe D:\Development\GrimrockPrototype\GrimrockPrototype.uproject -unattended -nop4 -nosplash -NullRHI -ExecCmds="Automation RunTests Grimrock.Monsters.MON13.4" -TestExit="Automation Test Queue Empty" -ReportOutputPath="D:\Development\GrimrockPrototype\Saved\TestReports\MON13_4"
```

## Checklist UE 5.5.4

1. Placer deux rats avec le même `EncounterGroupId`, vague `0`, et un troisième
   rat du même groupe en vague `1`.
2. Décocher `Enabled at Start` sur les trois placements.
3. Créer `Trigger.Activated → Rat_Wave0_A.StartEncounter`.
4. Facultativement relier `EncounterWaveStarted` et `EncounterCompleted` à des
   leviers ou lumières témoins.

- [ ] aucun rat n'apparaît avant le trigger ;
- [ ] les deux membres de la vague 0 apparaissent ensemble ;
- [ ] repasser sur le trigger ne crée aucun doublon ;
- [ ] despawner un membre n'ouvre pas la vague suivante ;
- [ ] la mort réelle des deux membres ouvre la vague 1 ;
- [ ] la mort du dernier membre émet `EncounterCompleted` ;
- [ ] un rebuild/Continue conserve la progression ;
- [ ] aucun `PresentationWarning` ni Actor partiel n'est produit.

### Validation PIE du 13 août 2026

- les deux membres de la vague 0 apparaissent ensemble ;
- `StartCombatFromPerception=true` avec les deux Rats ;
- la première mort conserve la vague 0 ;
- la seconde mort ouvre la vague 1 ;
- le dernier `CommitDeath` émet `EncounterCompleted` avec trois membres vaincus ;
- chaque Actor est un `BP_MON_RatGiant_C` apte au combat ;
- la victoire finale est atteinte sans warning de composant monstre manquant.

Les warnings de butin `Item_RatTooth` et `Item_RatMeat` observés pendant cette
validation relèvent de leurs DataAssets encore absents et non de MON13.4.

## Hors périmètre

- téléportation inter-niveaux d'un monstre ;
- embranchements de vagues ou conditions autres que la mort de tous les
  membres ;
- réinitialisation explicite d'une rencontre terminée ;
- résolution Asset Manager depuis `MonsterDefinitionId` seul.
