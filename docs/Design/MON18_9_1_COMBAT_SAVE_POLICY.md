# MON18.9.1 — Combat Save Policy / Pre-Combat Checkpoint

## Statut

**VALIDÉ ET CLOS SOUS UE5.5.4 — 22 AOÛT 2026.**

Implémentation initiale :

```text
36f7a85f2f98baf99fd2177c48b0ee6b8b54c5ed
Implement MON18.9.1 combat save policy
```

Correctif de validation :

```text
f2c09e8af6b2547c2d200755cb142d0fe85c5b7b
Fix MON18.9.1 combat save rejection
```

## 1. Décision

GrimrockPrototype ne persiste pas un combat en cours.

Le SaveGame reste un snapshot d'état durable : groupe, inventaire, équipement,
hotbar, progression, Spellbook, Status Effects persistants, donjon, objets,
monstres et position du groupe.

Ne sont volontairement pas ajoutés au format v6 :

- round courant ;
- phase de combat ;
- ordre d'initiative ;
- combattant actif ;
- PA/PAM du tour ;
- cooldowns runtime ;
- actions en attente ;
- ciblage en attente ;
- projectiles, animations ou VFX en cours.

## 2. Politique de sauvegarde

```text
Exploration     -> sauvegarde normale autorisée
Victory         -> sauvegarde normale autorisée
StartingCombat  -> sauvegarde normale refusée
PlayerPhase     -> sauvegarde normale refusée
EnemyPhase      -> sauvegarde normale refusée
EndingRound     -> sauvegarde normale refusée
Defeat          -> sauvegarde normale refusée
```

`bCombatActive=true` bloque également toujours l'écriture, indépendamment de
la phase affichée.

Le verrou principal est appliqué dans `AGrimrockPartyPawn::SaveCurrentGame()`,
avant toute capture runtime et avant `UGameplayStatics::SaveGameToSlot()`.
`UGrimrockPartySaveGame::Serialize()` conserve en plus un garde-fou défensif pour
les écritures directes qui contourneraient le Pawn.

Cette double barrière protège donc tous les appelants existants de `SaveCurrentGame()` :

- sauvegarde manuelle future ;
- autosave à la fermeture du menu/inventaire ;
- autosave `EndPlay` ;
- tout autre appel C++ utilisant le même SaveGame.

La correspondance entre SaveGame et runtime se fait par `CharacterId`. Un monde
de test ou un second monde de jeu sans personnage commun ne bloque pas la
sauvegarde d'un autre groupe.

## 3. Slot de checkpoint pré-combat

Le checkpoint est dérivé du slot courant :

```text
GrimrockParty     -> GrimrockParty_AutoCombat
GrimrockParty_2   -> GrimrockParty_2_AutoCombat
```

La règle est :

```text
<PartySaveSlotName> + "_AutoCombat"
```

Il ne remplace jamais le slot principal.

## 4. Moment exact du checkpoint

Le chemin d'engagement automatique MON14 effectue désormais :

```text
évaluation différée MON14
-> runtime stable / groupe sur une cellule
-> perception visuelle directe confirmée
-> capture exploration vers *_AutoCombat
-> si checkpoint réussi : StartCombatFromPerception()
-> StartCombatInternal()
```

La pré-évaluation utilise le même contrat `FScopedVisualSourceRequirement` que
MON14.1. Les évaluations ordinaires sans contact restent donc sans écriture
disque.

Un échec du checkpoint empêche le combat automatique de démarrer.

Si la seconde passe synchrone de perception démarrait exceptionnellement un
combat alors que la pré-évaluation n'avait trouvé aucune source visuelle, le
combat est immédiatement aborté et une erreur de contrat est journalisée. Le
jeu ne doit jamais continuer silencieusement dans un combat automatique sans
checkpoint.

## 5. Fixtures/transients

Les anciens tests de combat créent parfois des Pawns sans vraie partie
persistante.

Le checkpoint est alors explicitement `Skipped` lorsque :

- le slot de sauvegarde est vide ; ou
- le groupe n'a pas encore terminé la création initiale du personnage.

Ce cas ne concerne pas une partie joueur persistante et ne doit pas forcer la
réécriture de MON5 à MON17.

## 6. Défaite et victoire

### Défaite

`Defeat` reste non sauvegardable.

Ainsi :

```text
checkpoint pré-combat
-> combat
-> défaite
-> EndPlay / fermeture menu / autre autosave éventuel
-> écriture refusée
-> checkpoint pré-combat conservé
```

Une future UI `Recommencer le combat` pourra charger le slot `_AutoCombat`.
Le bouton et l'API de reprise ne font pas partie de MON18.9.1.

### Victoire

`Victory` est un état stable et sauvegardable. Un autosave post-combat pourra
donc être ajouté sans étendre le format SaveGame.

MON18.9.1 ne crée pas automatiquement un slot `PostCombat`.

## 7. Format de sauvegarde

Aucun bump de version :

```text
UGrimrockPartySaveGame::CurrentSaveVersion = 6
```

MON18.9.1 est une politique d'écriture, pas une nouvelle donnée persistante.

## 8. Validation Automation UE5.5.4

Namespace ciblé :

```text
Grimrock.Save.MON18.9.1
```

Résultat fourni le 22 août 2026 : **6/6 Success**.

```text
CombatSaveDoesNotOverwriteMainSlot      Success
DefeatPreservesPreCombatCheckpoint      Success
PreCombatCheckpointCreated              Success
SaveDuringCombatRejected                Success
SaveOutsideCombatAccepted               Success
TransientCheckpointSkipped              Success
```

Les logs confirment le contrat attendu :

```text
PartySave SaveRejected ... Reason=CombatStateNotSaveable
```

apparaît pendant le combat ou en défaite sans écriture normale ultérieure, tandis que le
checkpoint `_AutoCombat` est sauvegardé, relu et conserve la cellule pré-combat.

## 9. Régressions validées

Les campagnes recommandées ont également été exécutées sous UE5.5.4 :

```text
Grimrock.Save.SAVEFIX.2                1/1 Success
Grimrock.Magic.MON18.8                12/12 Success
Grimrock.Monsters.MON14.1              7/7 Success
```

Points couverts :

- un échec de `Continue` reste non destructif ;
- la persistance/migration Spellbook v6 reste intacte ;
- les bindings Spell de hotbar restent cohérents ;
- l'engagement automatique MON14.1 fonctionne toujours ;
- les fixtures transientes MON14.1 utilisent explicitement `Checkpoint=SkippedTransient` ;
- aucun état de combat partiel n'écrase le slot principal ou le checkpoint pré-combat.

## 10. Conclusion

MON18.9.1 est **VALIDÉ ET CLOS**.

Le correctif de validation a confirmé que `Serialize()` seul ne pouvait pas être le
verrou autoritaire, car `UGameplayStatics::SaveGameToSlot()` pouvait encore retourner
un succès après une erreur d'archive. Le refus est donc correctement appliqué en amont
dans `AGrimrockPartyPawn::SaveCurrentGame()`.

La campagne `Grimrock` complète reste à exécuter dans la phase finale de MON18.9 avant
clôture du jalon majeur MON18.
