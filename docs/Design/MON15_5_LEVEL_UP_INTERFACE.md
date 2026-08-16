# MON15.5 — Choix de progression et interface Level Up

Statut : **VALIDÉ ET CLOS — UE5.5.4 — 16 août 2026**.

MON15.5 transforme le modèle pur MON15.4 en une transaction de jeu réelle : une montée de niveau produit une notification, prépare une interface modale, permet de sélectionner des choix de classe, puis applique l'ensemble en une transaction atomique.

---

## 1. Périmètre livré

MON15.5 couvre :

- notification runtime après un level-up MON15.3 ;
- comparaison des statistiques avant/après ;
- affichage des points de progression de classe ;
- sélection staged des choix de classe ;
- validation du niveau, du budget et des prérequis ;
- confirmation atomique de tout le lot ;
- annulation sans mutation ;
- projection immédiate des choix confirmés vers le catalogue d'actions MON12 ;
- isolation par `CharacterId` ;
- file d'attente des notifications de level-up ;
- sécurité combat : aucune modal ouverte au milieu d'un combat actif ;
- fusion des level-ups différés successifs d'un même personnage ;
- modal native Slate fonctionnelle sans WBP obligatoire.

MON15.5 ne change pas le format de sauvegarde. La persistance et la migration sont réservées à MON15.6.

---

## 2. Frontière MON15.5 / MON15.6

Les choix confirmés sont autoritaires pendant la session courante, mais restent transitoires.

```text
MON15.5
  Level / XP persistants existants
  + choix de classe runtime transitoires
  + transaction de sélection
  + modal Level Up

MON15.6
  choix de classe dans le contrat SaveGame
  + migration des anciennes sauvegardes
  + restauration exacte
  + reconciliation Level <-> Experience
```

`UGrimrockPartySaveGame::CurrentSaveVersion` reste `3` en MON15.5.

---

## 3. Transaction autoritaire

Le service :

```cpp
FRPGClassProgressionTransactionService
```

maintient l'état runtime de progression par `CharacterId` et expose notamment :

```cpp
RefreshCharacterProjection(...)
TryGetSelectedChoiceIds(...)
TryGetChoicePointBalance(...)
TryCommitChoices(...)
AppendRuntimeSatisfiedRequirements(...)
ResetRuntimeState(...)
```

`TryCommitChoices()` valide tout le lot avant mutation : personnage, classe, doublons, existence des choix, niveau minimum, prérequis, budget et cohérence de l'état courant.

Une transaction invalide ne confirme aucun choix. Une transaction valide remplace l'état runtime en une seule étape puis émet la notification de commit.

---

## 4. Sélection staged

`URPGLevelUpWidget` ne committe jamais un clic individuel.

```text
clic sur un choix
    -> PendingChoiceIds
    -> aucune mutation gameplay

Confirmer
    -> TryCommitChoices(tout le lot)
    -> succès : commit intégral
    -> échec  : aucune mutation, modal conservée

Annuler
    -> PendingChoiceIds vidés
    -> aucune mutation
```

Un choix pending peut satisfaire le prérequis d'un autre choix pending. Un prérequis pending ne peut pas être retiré tant qu'un autre choix pending en dépend.

---

## 5. Vue avant / après

`FRPGLevelUpView` expose :

- personnage et classe ;
- niveau précédent et nouveau niveau ;
- PV max avant/après ;
- mana max avant/après ;
- armure physique/magique avant/après ;
- points accordés, dépensés et restants ;
- liste des choix et leur statut ;
- message de validation.

Les statistiques sont recalculées via :

```cpp
URPGCharacterRulesLibrary::CalculateDerivedStats(...)
```

Les bonus d'équipement ne sont pas intégrés aux statistiques persistantes de base.

---

## 6. Interface native et extensibilité Blueprint

`URPGLevelUpWidget` est `Blueprintable`.

Sans Widget Blueprint, `RebuildWidget()` construit une modal Slate native avec :

- titre `NIVEAU SUPÉRIEUR` ;
- personnage / classe / niveau ;
- statistiques avant/après ;
- budget de progression ;
- choix de classe ;
- boutons `Confirmer` / `Annuler`.

Un futur WBP pourra remplacer uniquement la présentation en utilisant `View` et `BP_OnLevelUpViewRefreshed()`.

Aucun `.uasset`, `.umap` ou WBP n'a été nécessaire pour MON15.5.

---

## 7. Sécurité combat et modalité

Le comportement final validé est :

```text
level-up pendant combat
        -> Queued
        -> Deferred (CombatActive)
        -> le combat continue normalement

nouveau level-up du même personnage avant présentation
        -> Coalesced
        -> PreviousLevel conservé
        -> NewLevel étendu au niveau final

fin du combat
        -> CombatSafePoint
        -> ModalGuard Applied
        -> jeu pausé
        -> modal ouverte
```

La résolution du `UGridTurnManagerComponent` se fait depuis le `AGridLevelRuntimeActor` associé au groupe, avec fallback monde si nécessaire. Le TurnManager n'est pas un composant du PartyPawn.

Pendant la modal :

- le jeu est pausé ;
- les inputs gameplay du Pawn sont neutralisés ;
- le `PlayerController` est en mode UI ;
- hotbar et interactions monde sont bloquées ;
- la souris reste active.

À la fermeture, le garde modal restaure l'état précédent.

Le binding Slate n'utilise pas `CreateSP` sur un `UUserWidget` ; les callbacks utilisent `FOnClicked::CreateUObject`.

---

## 8. File et fusion des notifications

`URPGLevelUpNotificationSubsystem` écoute le delegate source-aware de MON15.3.

Pour plusieurs personnages, les notifications restent en file et une seule modal est présentée à la fois.

Pour un même personnage, des level-ups successifs non encore présentés sont fusionnés. Exemple validé en PIE :

```text
1 -> 2
puis 2 -> 3 pendant les combats

=> une seule notification 1 -> 3
```

Cela garantit que l'interface affiche et évalue les choix avec le véritable niveau final du personnage.

---

## 9. Raccord au catalogue MON12

Le contrat reste :

```cpp
FGridCombatActionDefinition::Requirements
```

La projection runtime ajoute :

- grants automatiques de niveau ;
- `ChoiceId` confirmés ;
- `GrantedRequirementIds` des choix confirmés.

Avant confirmation, une action exigeant un choix absent reste `MissingRequirement`. Après confirmation, elle devient disponible si les autres conditions de combat sont satisfaites.

Aucun second catalogue ni circuit d'exécution n'a été créé.

---

## 10. Validation finale

### Automation Tests

Campagne finale du 16 août 2026 :

- `Grimrock.RPG.MON15.5.*` : 8 tests uniques, tous `Success` ;
- la suite MON15.5 a été exécutée 3 fois dans le log final, soit 24 succès ;
- `Grimrock.Monsters.MON12.ActionCatalog.*` : 2 tests uniques, exécutés 2 fois, tous `Success` ;
- `Grimrock.Monsters.MON12.8.*` : 26 tests uniques, tous `Success` ;
- total du log final : **54 tests terminés, 54 Success, 0 échec** ;
- aucun assert, ensure, `CheckAddress`, exception ou `Fatal error`.

Les campagnes précédentes avaient déjà validé MON15.1–15.4 et les régressions CharacterCreation pertinentes.

### PIE

Le scénario réel Giant Rat a confirmé :

```text
Queued 1->2
Deferred CombatActive
...
Coalesced 1->3 Gained=2 Pending=1
...
CombatSafePoint Victory Pending=1
ModalGuard Applied PausedByModal=true
Opened Previous=1 New=3
GridClassProgression Committed=1
ModalGuard Restored
```

Le choix temporaire `Maîtrise martiale` a été sélectionnable puis acquis, et son état runtime a été conservé pendant la session.

---

## 11. Résultat

MON15.5 est **VALIDÉ ET CLOS**.

La prochaine étape est **MON15.6 — Save / migration**, qui devra rendre persistants les choix de progression aujourd'hui transitoires, définir la migration des sauvegardes existantes et restaurer exactement la projection de progression à la reprise d'une partie.
