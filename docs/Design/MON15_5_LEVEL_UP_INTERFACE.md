# MON15.5 — Choix de progression et interface Level Up

Statut : **implémenté en C++ — validation UE5.5.4 en attente**.

MON15.5 transforme le modèle pur MON15.4 en une transaction de jeu réelle :
une montée de niveau produit une notification, ouvre une modal de progression,
permet de préparer un ensemble de choix, puis applique cet ensemble en une
seule transaction atomique.

---

## 1. Objectifs

MON15.5 couvre :

- notification runtime après un level-up MON15.3 ;
- comparaison des statistiques avant/après ;
- affichage des points de progression de classe ;
- sélection temporaire de choix de classe ;
- validation du niveau, du budget et des prérequis ;
- confirmation atomique de tout le lot ;
- annulation sans mutation ;
- projection immédiate des choix confirmés vers le catalogue d'actions MON12 ;
- file d'attente lorsque plusieurs personnages montent de niveau ensemble.

MON15.5 ne change pas encore le format de sauvegarde. La persistance et la
migration sont explicitement réservées à MON15.6.

---

## 2. Frontière MON15.5 / MON15.6

Les choix confirmés sont autoritaires pendant la session courante, mais restent
transitoires :

```text
MON15.5
  Level / XP persistants existants
  + choix de classe runtime transitoires
  + modal et transaction

MON15.6
  choix de classe dans le contrat SaveGame
  + migration v1-v3
  + restauration exacte
  + reconciliation Level <-> Experience
```

`UGrimrockPartySaveGame::CurrentSaveVersion` reste donc `3` en MON15.5.

Cette séparation évite d'écrire une nouvelle donnée dans une sauvegarde avant
que sa migration et sa restauration soient définies.

---

## 3. Transaction autoritaire

Le nouveau service :

```cpp
FRPGClassProgressionTransactionService
```

maintient l'état runtime de progression par `CharacterId` et expose :

```cpp
RefreshCharacterProjection(...)
TryGetSelectedChoiceIds(...)
TryGetChoicePointBalance(...)
TryCommitChoices(...)
AppendRuntimeSatisfiedRequirements(...)
ResetRuntimeState(...)
```

### Transaction par lot

`TryCommitChoices()` reçoit toute la sélection à ajouter.

Avant toute mutation, il valide :

- composant et personnage ;
- définition de classe ;
- état déjà confirmé ;
- doublons dans la requête ;
- existence de chaque `ChoiceId` ;
- choix non déjà acquis ;
- niveau minimum ;
- tous les prérequis, y compris ceux présents dans le même lot ;
- budget final de points.

Une transaction invalide ne confirme aucun choix.

Une transaction valide :

1. normalise l'ordre selon `URPGClassAsset::ProgressionChoices` ;
2. reconstruit les requirements satisfaits ;
3. remplace l'état runtime en une seule étape ;
4. envoie une notification inventaire unique ;
5. diffuse `OnClassProgressionCommitted`.

---

## 4. Sélection staged dans la modal

`URPGLevelUpWidget` ne committe jamais un clic individuel.

```text
clic sur un choix
    -> PendingChoiceIds
    -> aucun état gameplay modifié

Confirmer
    -> TryCommitChoices(tout le lot)
    -> succès : commit intégral
    -> échec  : aucune mutation, modal conservée

Annuler
    -> PendingChoiceIds vidés
    -> aucune mutation
```

Un choix pending qui sert de prérequis à un autre choix pending ne peut pas être
retiré tant que la dépendance est présente.

Le joueur peut confirmer seulement une partie de ses points s'il le souhaite ;
les points non dépensés restent disponibles dans la session.

---

## 5. Comparaison avant / après

La vue C++ `FRPGLevelUpView` expose :

- nom et classe ;
- niveau précédent et nouveau niveau ;
- PV max avant/après ;
- mana max avant/après ;
- armure physique avant/après ;
- armure magique avant/après ;
- points accordés, dépensés et restants ;
- liste de choix et statut de chacun ;
- message de validation.

Les statistiques comparées sont les statistiques de base recalculées via :

```cpp
URPGCharacterRulesLibrary::CalculateDerivedStats(...)
```

Les bonus d'équipement restent extérieurs, conformément au contrat MON15.3.

---

## 6. Interface native et extensibilité Blueprint

`URPGLevelUpWidget` est `Blueprintable`.

Sans Widget Blueprint, `RebuildWidget()` construit une modal Slate native
fonctionnelle avec :

- titre `NIVEAU SUPÉRIEUR` ;
- personnage / classe / niveau ;
- ligne avant/après ;
- budget de points ;
- boutons des choix ;
- message de validation ;
- boutons `Confirmer` et `Annuler`.

Si un futur WBP fournit un `WidgetTree`, le fallback natif s'efface au profit du
Widget Blueprint. `View` et `BP_OnLevelUpViewRefreshed()` fournissent alors le
contrat de présentation.

Aucun `.uasset` ou WBP n'est nécessaire pour MON15.5.

---

## 7. Modalité et input

Pendant la modal :

- les inputs du Pawn sont désactivés ;
- le `PlayerController` passe en `UIOnly` ;
- le ciblage combat est annulé via l'état UI existant ;
- la souris reste disponible pour la modal.

À la fermeture :

- les inputs du Pawn sont réactivés ;
- le mode `GameAndUI` est restauré ;
- l'état précédent de l'UI inventaire est restauré.

Le comportement attendu est que déplacement, raccourcis et interactions monde
restent bloqués pendant la confirmation ; cette modalité fait partie de la
validation PIE de MON15.5.

---

## 8. Notification automatique

`FRPGLevelUpService` conserve son delegate MON15.3 historique et ajoute un
delegate source-aware qui transporte aussi :

```text
UGridPartyInventoryComponent*
CharacterIndex
PreviousLevel
NewLevel
LevelsGained
```

Avant les delegates, le service rafraîchit la projection MON15.5 du personnage.

`URPGLevelUpNotificationSubsystem` (`UGameInstanceSubsystem`) écoute cet événement :

```text
Level-up appliqué
   -> notification mise en file
   -> modal URPGLevelUpWidget
   -> fermeture
   -> notification suivante
```

Les level-ups simultanés de plusieurs membres ne se superposent donc pas.

---

## 9. Raccord au catalogue MON12

Le catalogue reste fondé sur l'unique contrat :

```cpp
FGridCombatActionDefinition::Requirements
```

`FRPGLevelUpService` et la transaction de confirmation maintiennent une
projection runtime par `CharacterId`. `FGridCombatActionCatalog::Build()` copie
le contexte reçu puis y fusionne cette projection avant d'évaluer les
requirements :

- requirements automatiques accordés par le niveau ;
- `ChoiceId` confirmés ;
- `GrantedRequirementIds` des choix confirmés.

Le `ClassId` et les tags d'équipement continuent d'être fournis par le contexte
MON12 existant.

Ainsi :

```text
Action Requirements=[Choice_A]

avant confirmation Choice_A
    -> MissingRequirement

après confirmation Choice_A
    -> Enabled (si les autres conditions combat sont satisfaites)
```

Le catalogue MON12 ne possède aucun état et n'effectue aucune mutation : il
consomme seulement la projection transient détenue par le service MON15.5.

---

## 10. Atomicité et erreurs

`ERPGClassProgressionCommitRejectReason` distingue notamment :

```text
InvalidInventory
InvalidCharacter
InvalidClassDefinition
InvalidCurrentSelection
EmptyRequest
DuplicateRequest
UnknownChoice
AlreadySelected
LevelTooLow
MissingPrerequisite
InsufficientChoicePoints
```

Une erreur de confirmation conserve la modal ouverte et affiche une raison
lisible. Les choix déjà confirmés avant la transaction restent intacts ; aucun
élément du lot refusé n'est ajouté.

Un état runtime déjà confirmé devenu incohérent n'est jamais silencieusement
supprimé : le rafraîchissement échoue et laisse la reconciliation à une étape
explicite, notamment MON15.6.

---

## 11. Tests automatisés

Suite dédiée :

```text
Grimrock.RPG.MON15.5.AtomicBatchCommit
Grimrock.RPG.MON15.5.AtomicFailure
Grimrock.RPG.MON15.5.WidgetCancelIsNonMutating
Grimrock.RPG.MON15.5.WidgetConfirmTransaction
Grimrock.RPG.MON15.5.CombatCatalogUnlock
Grimrock.RPG.MON15.5.LevelUpNotificationSource
Grimrock.RPG.MON15.5.CharacterIsolation
Grimrock.RPG.MON15.5.TransientPersistenceBoundary
```

Elle couvre :

- batch avec chaîne de prérequis ;
- échec sans mutation partielle ;
- annulation de la modal ;
- confirmation staged ;
- comparaison avant/après ;
- requirement de choix vers le catalogue combat ;
- delegate source-aware de level-up ;
- grants automatiques post-level-up ;
- isolation par personnage ;
- frontière volontaire de persistance MON15.6 ;
- SaveVersion inchangée.

---

## 12. Porte de sortie

MON15.5 pourra être marqué **VALIDÉ ET CLOS** lorsque :

- le projet compile sous UE5.5.4 ;
- les huit tests `Grimrock.RPG.MON15.5.*` réussissent ;
- MON15.1 à MON15.4 restent verts ;
- les tests de création de personnage pertinents restent verts ;
- les régressions `ActionCatalog` et `MON12.8` restent vertes ;
- un PIE réel confirme l'ouverture de la modal après franchissement de niveau ;
- Annuler ne change aucun choix ;
- Confirmer applique le lot et débloque le requirement attendu ;
- plusieurs notifications ne se superposent pas ;
- `CurrentSaveVersion` reste `3` ;
- aucun `.uasset`, `.umap` ou WBP n'est modifié.