# MON18.9.1 — Combat Save Policy / Pre-Combat Checkpoint

## Statut

**IMPLÉMENTÉ — VALIDATION UE5.5.4 EN ATTENTE.**

Base :

```text
015cefc657740509b614a79c8074c08741386987
Remove redundant MON18.8 status note
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

Le verrou est défensif au niveau de `UGrimrockPartySaveGame::Serialize()`.
Il protège donc tous les appelants existants de `SaveCurrentGame()` :

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

MON18.9.1 ne crée pas encore automatiquement un slot `PostCombat`.

## 7. Format de sauvegarde

Aucun bump de version :

```text
UGrimrockPartySaveGame::CurrentSaveVersion = 6
```

MON18.9.1 est une politique d'écriture, pas une nouvelle donnée persistante.

## 8. Tests Automation

Namespace :

```text
Grimrock.Save.MON18.9.1
```

Tests ciblés :

```text
SaveOutsideCombatAccepted
SaveDuringCombatRejected
PreCombatCheckpointCreated
TransientCheckpointSkipped
CombatSaveDoesNotOverwriteMainSlot
DefeatPreservesPreCombatCheckpoint
```

Attendu après compilation UE5.5.4 : **6/6 Success**.

Régressions recommandées :

```text
Grimrock.Save.SAVEFIX.2
Grimrock.Magic.MON18.8
Grimrock.Monsters.MON14.1
```

Puis campagne `Grimrock` complète avant clôture de MON18.9.

## 9. Validation PIE attendue

Scénario :

```text
New Game / Continue hors combat
-> vérifier sauvegarde normale
-> approcher un monstre jusqu'au déclenchement automatique
-> log : [MON18.9.1] PreCombatCheckpoint Saved ..._AutoCombat
-> combat
-> fermer le menu/inventaire ou arrêter PIE pendant le combat
-> aucune sauvegarde de combat ne remplace le checkpoint
-> vérifier que *_AutoCombat existe et contient l'état pré-combat
```

La clôture de MON18.9.1 attend les résultats UE5.5.4 fournis par l'utilisateur.
